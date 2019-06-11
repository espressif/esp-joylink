#ifndef _JOYLINK_PROBE_H_
#define _JOYLINK_PROBE_H_

#include "joylink_wlan_probe.h"
//#include "joylink_syshdr.h"
#include "stdint.h"

struct ieee802_11_elems {
	const uint8_t *ssid;
	const uint8_t *supp_rates;
	const uint8_t *ds_params;
	const uint8_t *challenge;
	const uint8_t *erp_info;
	const uint8_t *ext_supp_rates;
	const uint8_t *wpa_ie;
	const uint8_t *rsn_ie;
	const uint8_t *wmm; /* WMM Information or Parameter Element */
	const uint8_t *wmm_tspec;
	const uint8_t *wps_ie;
	const uint8_t *supp_channels;
	const uint8_t *mdie;
	const uint8_t *ftie;
	const uint8_t *timeout_int;
	const uint8_t *ht_capabilities;
	const uint8_t *ht_operation;
	const uint8_t *vht_capabilities;
	const uint8_t *vht_operation;
	const uint8_t *vht_opmode_notif;
	const uint8_t *vendor_ht_cap;
	const uint8_t *p2p;
	const uint8_t *wfd;
	const uint8_t *link_id;
	const uint8_t *interworking;
	const uint8_t *qos_map_set;
	const uint8_t *hs20;
	const uint8_t *ext_capab;
	const uint8_t *bss_max_idle_period;
	const uint8_t *ssid_list;
	const uint8_t *osen;

	const uint8_t *vendor_custom;

	uint8_t ssid_len;
	uint8_t supp_rates_len;
	uint8_t ds_params_len;
	uint8_t challenge_len;
	uint8_t erp_info_len;
	uint8_t ext_supp_rates_len;
	uint8_t wpa_ie_len;
	uint8_t rsn_ie_len;
	uint8_t wmm_len; /* 7 = WMM Information; 24 = WMM Parameter */
	uint8_t wmm_tspec_len;
	uint8_t wps_ie_len;
	uint8_t supp_channels_len;
	uint8_t mdie_len;
	uint8_t ftie_len;
	uint8_t timeout_int_len;
	uint8_t ht_capabilities_len;
	uint8_t ht_operation_len;
	uint8_t vht_capabilities_len;
	uint8_t vht_operation_len;
	uint8_t vendor_ht_cap_len;
	uint8_t p2p_len;
	uint8_t wfd_len;
	uint8_t interworking_len;
	uint8_t qos_map_set_len;
	uint8_t hs20_len;
	uint8_t ext_capab_len;
	uint8_t ssid_list_len;
	uint8_t osen_len;

	uint8_t vendor_custom_len;
};


int ieee802_11_parse_elems(const uint8_t *start, size_t len,struct ieee802_11_elems *elems);

int joylink_gen_vendor_specific(uint8_t *vendor_ie, uint8_t *content, int content_len);
/*
* Genarate probe request frame
*/
uint8_t * joylink_gen_probe_req(void *vendor_ie, int vendor_len, int *req_len);

uint8_t * joylink_gen_probe_resp(void *vendor_ie, int vendor_len, const struct ieee80211_mgmt *probe_req, int *resp_len);
#endif


