#ifndef _JOYLINK_CLOUD_LOG_H_
#define _JOYLINK_CLOUD_LOG_H_
#define TAGID_LOG_REV_SSID				"D1"
#define TAGID_LOG_AP_CONNECTED			"D2"
#define TAGID_LOG_AP_GET_FEEDID_RES		"D3"
#define TAGID_LOG_AP_WRITE_FEEDID_RES	"D4"
#define TAGID_LOG_AP_FULL_RES			"D5"

#define RES_LOG_SUCCES					"1"
#define RES_LOG_FAIL					"-1"

int joylink_cloud_log_param_set(char *urlstr,char *tokenstr);
int joylink_cloud_log_post(char *tagId, char *result,char *time,char *payload,char *duration,char *feedid);

#endif
