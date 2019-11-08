
#include "joylink_log.h"
#include "joylink_probe.h"
#include "joylink_utils.h"

/* Microsoft (also used in Wi-Fi specs) 00:50:F2 */
#define OUI_MICROSOFT 0x0050f2
#define OUI_BROADCOM 0x00904c /* Broadcom (Epigram) */
#define VENDOR_HT_CAPAB_OUI_TYPE 0x33 /* 00-90-4c:0x33 */

#define WMM_OUI_TYPE 2
#define WMM_OUI_SUBTYPE_INFORMATION_ELEMENT 0
#define WMM_OUI_SUBTYPE_PARAMETER_ELEMENT 1
#define WMM_OUI_SUBTYPE_TSPEC_ELEMENT 2
#define WMM_VERSION 1

#define OUI_WFA 0x506f9a
#define P2P_OUI_TYPE 9
#define WFD_OUI_TYPE 10
#define HS20_INDICATION_OUI_TYPE 16
#define HS20_OSEN_OUI_TYPE 18

#define OUI_MEDIATEK 0x000ce7 /* MediaTek Information Element */

uint8_t g_vendor_request[] = "Tell me AP info";
uint8_t g_vendor_reply[] = "SSID:Test PSK:12345678";

static inline uint32_t WPA_GET_BE24(const uint8_t *a)
{
	return (a[0] << 16) | (a[1] << 8) | a[2];
}

static inline void WPA_PUT_BE24(uint8_t *a, uint32_t val)
{
	a[0] = (val >> 16) & 0xff;
	a[1] = (val >> 8) & 0xff;
	a[2] = val & 0xff;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to parse vendor specific element
 *
 * @param pos        Pointer to the start of vendor specific information element
 * @param len        Length of vendor specific IE in octets
 * @param elems      Data structure for parsed elements
 *
 * @retval 0      Success to parse elems
 * @retval -1     Fail to parse elems
 */
/*----------------------------------------------------------------------------*/
int ieee802_11_parse_vendor_specific(const uint8_t *pos, size_t elen,
	struct ieee802_11_elems *elems)
{
	unsigned int oui;

	/* first 3 bytes in vendor specific information element are the IEEE
	* OUI of the vendor. The following byte is used a vendor specific
	* sub-type.
	*/
	if (elen < 4) {
		log_error("short vendor specific information element ignored (len=%lu)\n",
			(unsigned long) elen);
		return -1;
	}

	oui = WPA_GET_BE24(pos);
	oui = OUI_MEDIATEK;				//changed by meng
	switch (oui) {
	//case OUI_MEDIATEK:
	//	elems->vendor_custom = pos;
	//	elems->vendor_custom_len = elen;
	//	break;
	case OUI_MICROSOFT:
		/* Microsoft/Wi-Fi information elements are further typed and subtyped */
		switch (pos[3]) {
		case 1:
			/* Microsoft OUI (00:50:F2) with OUI Type 1:
			* real WPA information element
			*/
			elems->wpa_ie = pos;
			elems->wpa_ie_len = elen;
			break;
		case WMM_OUI_TYPE:
			/* WMM information element */
			if (elen < 5) {
				log_error("short WMM information element ignored (len=%lu)\n",
					(unsigned long) elen);
				return -1;
			}
			switch (pos[4]) {
			case WMM_OUI_SUBTYPE_INFORMATION_ELEMENT:
			case WMM_OUI_SUBTYPE_PARAMETER_ELEMENT:
				/*
				* Share same pointer since only one of these
				* is used and they start with same data.
				* Length field can be used to distinguish the
				* IEs.
				*/
				elems->wmm = pos;
				elems->wmm_len = elen;
				break;
			case WMM_OUI_SUBTYPE_TSPEC_ELEMENT:
				elems->wmm_tspec = pos;
				elems->wmm_tspec_len = elen;
				break;
			default:
				log_error("unknown WMM information element ignored (subtype=%d len=%lu)\n",
					pos[4], (unsigned long) elen);
				return -1;
			}
			break;
		case 4:
			/* Wi-Fi Protected Setup (WPS) IE */
			elems->wps_ie = pos;
			elems->wps_ie_len = elen;
			break;
		default:
			log_error("Unknown Microsoft information element ignored (type=%d len=%lu)\n",
				pos[3], (unsigned long) elen);
			return -1;
		}
		break;

	case OUI_WFA:
		switch (pos[3]) {
		case P2P_OUI_TYPE:
			/* Wi-Fi Alliance - P2P IE */
			elems->p2p = pos;
			elems->p2p_len = elen;
			break;
		case WFD_OUI_TYPE:
			/* Wi-Fi Alliance - WFD IE */
			elems->wfd = pos;
			elems->wfd_len = elen;
			break;
		case HS20_INDICATION_OUI_TYPE:
			/* Hotspot 2.0 */
			elems->hs20 = pos;
			elems->hs20_len = elen;
			break;
		case HS20_OSEN_OUI_TYPE:
			/* Hotspot 2.0 OSEN */
			elems->osen = pos;
			elems->osen_len = elen;
			break;
		default:
			log_error("Unknown WFA information element ignored (type=%d len=%lu)\n",
				pos[3], (unsigned long) elen);
			return -1;
		}
		break;

	case OUI_BROADCOM:
		switch (pos[3]) {
		case VENDOR_HT_CAPAB_OUI_TYPE:
			elems->vendor_ht_cap = pos;
			elems->vendor_ht_cap_len = elen;
			break;
		default:
			log_error("Unknown Broadcom information element ignored (type=%d len=%lu)\n",
				pos[3], (unsigned long) elen);
			return -1;
		}
		break;

	default:
		elems->vendor_custom = pos;
		elems->vendor_custom_len = elen;
		return 0;
		log_error("unknown vendor specific information element ignored (vendor OUI "
			"%02x:%02x:%02x len=%lu)\n",
			pos[0], pos[1], pos[2], (unsigned long) elen);
		return -1;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to parse information elements in management frames
 *
 * @param start      Pointer to the start of IEs
 * @param len        Length of IE buffer in octets
 * @param elems      Data structure for parsed elements
 *
 * @retval 0      Success to parse elems
 * @retval -1     Fail to parse elems
 */
/*----------------------------------------------------------------------------*/
int ieee802_11_parse_elems(const uint8_t *start, size_t len,
	struct ieee802_11_elems *elems)
{
	size_t left = len;
	const uint8_t *pos = start;
	int unknown = 0;

	memset(elems, 0, sizeof(*elems));

	while (left >= 2) {
		uint8_t id, elen;

		id = *pos++;
		elen = *pos++;
		left -= 2;
/*
		if(elen == 0 || elen > left)
		{
		    return 0;
		}
		else if (elen > left) {
			joylink_log_error("IEEE 802.11 element parse failed (id=%d elen=%d "
				"left=%lu)", id, elen, (unsigned long) left);
			return -1 ;
		}
*/
		//printf("id: %02x\n", id);

		switch (id) {
		case WLAN_EID_SSID:
			elems->ssid = pos;
			elems->ssid_len = elen;
			break;
		case WLAN_EID_SUPP_RATES:
			elems->supp_rates = pos;
			elems->supp_rates_len = elen;
			break;
		case WLAN_EID_DS_PARAMS:
			elems->ds_params = pos;
			elems->ds_params_len = elen;
			break;
		case WLAN_EID_CF_PARAMS:
		case WLAN_EID_TIM:
			break;
		case WLAN_EID_CHALLENGE:
			elems->challenge = pos;
			elems->challenge_len = elen;
			break;
		case WLAN_EID_ERP_INFO:
			elems->erp_info = pos;
			elems->erp_info_len = elen;
			break;
		case WLAN_EID_EXT_SUPP_RATES:
			elems->ext_supp_rates = pos;
			elems->ext_supp_rates_len = elen;
			break;
		case WLAN_EID_VENDOR_SPECIFIC:
			elems->vendor_custom = pos;
			elems->vendor_custom_len = elen;
		    return 0;
			//if (ieee802_11_parse_vendor_specific(pos, elen, elems))
			//	unknown++;
			//break;
		default:
			unknown++;
			break;
		}
		left -= elen;
		pos += elen;
	}

	if (left)
	{
		log_error("elem_len=, left=\n");
		return -1;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to genarate vendor specific element
 *
 * @param vendor_ie      Pointer to the element buffer
 *
 * @retval element length
 */
/*----------------------------------------------------------------------------*/
int joylink_gen_vendor_specific(uint8_t *vendor_ie, uint8_t *content, int content_len)
{
	uint8_t *pos, *len;

	if(vendor_ie == NULL)
		return 0;

	pos = vendor_ie;
	len = vendor_ie;
	*pos++ = WLAN_EID_VENDOR_SPECIFIC;
	len = pos++; /* to be filled */
	WPA_PUT_BE24(pos, OUI_MEDIATEK);
	pos += 3;

	memcpy(pos, content, content_len);
	pos += content_len;

	*len = pos -len -1; /* fill vendor len */

	return pos - vendor_ie;
}

uint8_t g_own_mac[ETH_ALEN] = {0};

/*
* Genarate probe request frame
*/
uint8_t * joylink_gen_probe_req(void *vendor_ie, int vendor_len, int *req_len)
{
    struct ieee80211_mgmt *resp = NULL;
    uint8_t *pos = NULL;
    int buflen = MAX_PROBERESP_LEN;
    char *ssid = NULL;
    int ssid_len = 0;

    uint8_t supp_rate[4] = {0x02, 0x04, 0x0b, 0x16};
    uint8_t da_mac[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (vendor_ie == NULL || vendor_len == 0 || req_len == 0)
    {
        log_error("mtk_custom_gen_probe_req() param error\n");
        return NULL;
    }

    /* alloc buffer for probe response */
    resp = joylink_util_malloc(buflen);
    if (resp == NULL)
    {
        log_error("malloc buffer fail\n");
        return NULL;
    }

    memset(resp, 0, buflen);

    resp->frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
        WLAN_FC_STYPE_PROBE_REQ);

    memcpy(resp->da, da_mac, ETH_ALEN);
    memcpy(resp->sa, g_own_mac, ETH_ALEN);
    memcpy(resp->bssid, da_mac, ETH_ALEN);
    resp->seq_ctrl = 0;

    pos = resp->u.probe_req.variable;

    /* SSID */
    *pos++ = WLAN_EID_SSID;
    *pos++ = ssid_len;
    memcpy(pos, ssid, ssid_len);
    pos += ssid_len;

    /* Supported rates */
    *pos++ = WLAN_EID_SUPP_RATES;
    *pos++ = 4;
    memcpy(pos, supp_rate, 4);
    pos += 4;

    /* Vendor specific */
    if (vendor_ie != NULL && vendor_len > 0) {
        memcpy(pos, vendor_ie, vendor_len);
        pos += vendor_len;
    }

    *req_len = pos - (uint8_t *)resp;

    return (uint8_t *) resp;
}

/*
* Genarate probe response frame
*/
uint8_t * joylink_gen_probe_resp(void *vendor_ie, int vendor_len,
    const struct ieee80211_mgmt *probe_req, int *resp_len)
{
    struct ieee80211_mgmt *resp = NULL;
    uint8_t *pos = NULL;
    int buflen = MAX_PROBERESP_LEN;
    char *ssid = GL_CUSTOM_PROBE_RESP_SSID;
    int ssid_len = strlen(GL_CUSTOM_PROBE_RESP_SSID);
    uint8_t supp_rate[4] = {0x02, 0x04, 0x0b, 0x16};

    if (vendor_ie == NULL || vendor_len == 0 || probe_req == NULL || resp_len == 0)
    {
        log_error("mtk_custom_gen_probe_resp() param error\n");
        return NULL;
    }

    /* alloc buffer for probe response */
    resp = joylink_util_malloc(buflen);
    if (resp == NULL)
    {
        log_error("malloc buffer fail\n");
        return NULL;
    }

    memset(resp, 0, buflen);

    resp->frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
        WLAN_FC_STYPE_PROBE_RESP);

    memcpy(resp->da, probe_req->sa, ETH_ALEN);
    memcpy(resp->sa, g_own_mac, ETH_ALEN);
    memcpy(resp->bssid, g_own_mac, ETH_ALEN);
    resp->seq_ctrl = 0;
    resp->u.probe_resp.timestamp[0] = 0;
    resp->u.probe_resp.beacon_int = 100;
    resp->u.probe_resp.capab_info = WLAN_CAPABILITY_ESS;

    pos = resp->u.probe_resp.variable;

    /* SSID */
    *pos++ = WLAN_EID_SSID;
    *pos++ = ssid_len;
    memcpy(pos, ssid, ssid_len);
    pos += ssid_len;

    /* Supported rates */
    *pos++ = WLAN_EID_SUPP_RATES;
    *pos++ = 4;
    memcpy(pos, supp_rate, 4);
    pos += 4;

    /* Vendor specific */
    if (vendor_ie != NULL && vendor_len > 0) {
        memcpy(pos, vendor_ie, vendor_len);
        pos += vendor_len;
    }

    *resp_len = pos - (uint8_t *)resp;

    return (uint8_t *) resp;
}


