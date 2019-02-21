

#ifndef _MTK_CUSTOM_WLAN_PROBE_H_
#define _MTK_CUSTOM_WLAN_PROBE_H_

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <stdio.h>
#include <errno.h>
#include "stdint.h"

/******************************************************************************
*                              C O N S T A N T S
*******************************************************************************
*/
#define MTK_CUSTOM_SAVE_PCAP_FILE               0
#define MTK_CUSTOM_PRINT_FRAME                  0

/* Address 4 excluded */
#define WLAN_MAC_MGMT_HEADER_LEN                24
/* 7.3.1.10 Timestamp field */
#define TIMESTAMP_FIELD_LEN                         8
/* 7.3.1.3 Beacon Interval field */
#define BEACON_INTERVAL_FIELD_LEN                   2
/* 7.3.1.4 Capability Information field */
#define CAP_INFO_FIELD_LEN                          2
/* 7.3.2 Element IDs of information elements */
#define ELEM_HDR_LEN                                2

/* Information Element IDs */
#define WLAN_EID_SSID 0
#define WLAN_EID_SUPP_RATES 1
#define WLAN_EID_FH_PARAMS 2
#define WLAN_EID_DS_PARAMS 3
#define WLAN_EID_CF_PARAMS 4
#define WLAN_EID_TIM 5
#define WLAN_EID_IBSS_PARAMS 6
#define WLAN_EID_COUNTRY 7
#define WLAN_EID_BSS_LOAD 11
#define WLAN_EID_CHALLENGE 16
#define WLAN_EID_ERP_INFO 42
#define WLAN_EID_EXT_SUPP_RATES 50
#define WLAN_EID_VENDOR_SPECIFIC 221

#define GL_CUSTOM_PROBE_RESP_SSID ("MTK_Probe_SSID")

#define WLAN_CAPABILITY_ESS ((unsigned int) 1U << 0)

#define WLAN_FC_TYPE_MGMT		0
#define WLAN_FC_TYPE_CTRL		1
#define WLAN_FC_TYPE_DATA		2

/* management */
#define WLAN_FC_STYPE_ASSOC_REQ		0
#define WLAN_FC_STYPE_ASSOC_RESP	1
#define WLAN_FC_STYPE_REASSOC_REQ	2
#define WLAN_FC_STYPE_REASSOC_RESP	3
#define WLAN_FC_STYPE_PROBE_REQ		4
#define WLAN_FC_STYPE_PROBE_RESP	5
#define WLAN_FC_STYPE_BEACON		8
#define WLAN_FC_STYPE_ATIM			9
#define WLAN_FC_STYPE_DISASSOC		10
#define WLAN_FC_STYPE_AUTH			11
#define WLAN_FC_STYPE_DEAUTH		12
#define WLAN_FC_STYPE_ACTION		13

#define IEEE80211_FC(type, stype) ((type << 2) | (stype << 4))

#define MAX_PROBERESP_LEN 1024

#define MAX_CUSTOM_VENDOR_LEN 256

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#ifdef __GNUC__
#define STRUCT_PACKED __attribute__ ((packed))
#else
#define STRUCT_PACKED
#endif

struct ieee80211_hdr {
	uint16_t frame_control;
	uint16_t duration_id;
	uint8_t addr1[6];
	uint8_t addr2[6];
	uint8_t addr3[6];
	uint16_t seq_ctrl;
	/* followed by 'u8 addr4[6];' if ToDS and FromDS is set in data frame
	 */
} STRUCT_PACKED;

#define IEEE80211_HDRLEN (sizeof(struct ieee80211_hdr))

struct ieee80211_mgmt {
	uint16_t frame_control;
	uint16_t duration;
	uint8_t da[6];
	uint8_t sa[6];
	uint8_t bssid[6];
	uint16_t seq_ctrl;
	union {
		struct {
			/* only variable items: SSID, Supported rates */
			uint8_t variable[0];
		} STRUCT_PACKED probe_req;
		struct {
			uint8_t timestamp[8];
			uint16_t beacon_int;
			uint16_t capab_info;
			/* followed by some of SSID, Supported rates,
			 * FH Params, DS Params, CF Params, IBSS Params */
			uint8_t variable[0];
		} STRUCT_PACKED probe_resp;
		struct {
			uint8_t timestamp[8];
			uint16_t beacon_int;
			uint16_t capab_info;
			/* followed by some of SSID, Supported rates,
			 * FH Params, DS Params, CF Params, IBSS Params, TIM */
			uint8_t variable[0];
		} STRUCT_PACKED beacon;
	} u;
} STRUCT_PACKED;

struct pcap_pkthdr
{
	int tv_sec;
	int tv_usec;
	int caplen;
	int len;
};

struct pcap_file_header
{
	int   magic;
	short version_major;
	short version_minor;
	int   thiszone;   /* gmt to local correction */
	int   sigfigs;    /* accuracy of timestamps */
	int   snaplen;    /* max length saved portion of each pkt */
	int   linktype;   /* data link type (LINKTYPE_*) */
};

#endif
