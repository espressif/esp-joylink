#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"

#include "jd_innet.h"
#include "joylink_smnt.h"
#include "joylink_smnt_adp.h"
#include "joylink_log.h"

struct RxControl {
    signed rssi: 8;
    unsigned rate: 4;
    unsigned is_group: 1;
    unsigned: 1;
    unsigned sig_mode: 2;
    unsigned legacy_length: 12;
    unsigned damatch0: 1;
    unsigned damatch1: 1;
    unsigned bssidmatch0: 1;
    unsigned bssidmatch1: 1;
    unsigned MCS: 7;
    unsigned CWB: 1;
    unsigned HT_length: 16;
    unsigned Smoothing: 1;
    unsigned Not_Sounding: 1;
    unsigned: 1;
    unsigned Aggregation: 1;
    unsigned STBC: 2;
    unsigned FEC_CODING: 1;
    unsigned SGI: 1;
    unsigned rxend_state: 8;
    unsigned ampdu_cnt: 8;
    unsigned channel: 4;
    unsigned: 12;
};

struct Ampdu_Info
{
    uint16 length;
    uint16 seq;
    uint8  address3[6];
};

struct sniffer_buf {
    struct RxControl rx_ctrl;
    uint8_t  buf[36];
    uint16_t cnt;
    struct Ampdu_Info ampdu_info[1];
};

struct sniffer_buf2{
    struct RxControl rx_ctrl;
    uint8 buf[112];
    uint16 cnt;
    uint16 len; //length of packet
};

typedef struct jd_sniffer{
    uint8_t  current_channel;
    uint16_t channel_bits;
    uint8    stop_flag;
    uint8    scan_flag;
    uint8    link_flag;
	uint8    channel_count;
}JD_SNIFFER, *pJS_SNIFFER;

//#define AES_KEY  "C423GR98JZHEYXYR"
static const char *AES_KEY = NULL;
JD_SNIFFER   JDSnifGlob = {0};
static os_timer_t channel_timer;
static os_timer_t loop_timer;
static unsigned char* s_pBuffer = NULL;
static xSemaphoreHandle s_sem_check_result = NULL;

/* set channel */
uint8_t adp_changeCh(int i)
{
    wifi_set_channel(i);
    return 1; //must to change channel
}

void ICACHE_FLASH_ATTR
joylink_config_rx_callback(uint8_t *buf, uint16_t buf_le)
{
	uint8 one,i;
    uint16 len, cnt_packk = 0;
	PHEADER_802_11_SMNT frame;
    joylinkResult_t Ret;
    struct sniffer_buf *pack_all = (struct sniffer_buf *)buf;
	if(1 == JDSnifGlob.stop_flag){
        return;
    }
    if (buf_le == 12 || buf_le == 128) {
        return;
    } else {
		while (cnt_packk < (pack_all->cnt)) {
            len = pack_all->ampdu_info[cnt_packk].length;//get all package length
			cnt_packk++;
			/* FRAME_SUBTYPE_UDP_MULTIANDBROAD */
			frame = (PHEADER_802_11_SMNT)pack_all->buf;
            joylink_cfg_DataAction(frame, len);
            if (joylink_cfg_Result(&Ret) == 0) {
                if (Ret.type != 0) {
                    JDSnifGlob.stop_flag = 1;
                	wifi_promiscuous_enable(0);
                	os_timer_disarm(&loop_timer); //close timer
                    xSemaphoreGive(s_sem_check_result);
                    return;
                }
            }
			/* FRAME_SUBTYPE_BEACON */
			/* FRAME_SUBTYPE_RAWDATA */
			/* FRAME_SUBTYPE_UNKNOWN */
		}
	}
}

/* timer to change channel */
void ICACHE_FLASH_ATTR
joylink_config_loop_do(void *arg)
{
	if(1 == JDSnifGlob.stop_flag){
        return;
    }
    //joylink_innet_timingcall();
	uint8_t ret = joylink_cfg_50msTimer();
    os_timer_disarm(&loop_timer);
	os_timer_arm(&loop_timer, ret, 0); //loop to do task
}

bool jd_innet_get_result()
{
    uint8_t ssid_len = 0, pass_len = 0;
    int len = 0;
    bool rsp = FALSE;
    joylinkResult_t Ret;
    uint8_t AES_IV[16];
    uint8_t jd_aes_out_buffer[128];
    bzero(AES_IV, 16);
    bzero(jd_aes_out_buffer, 128);
    if (0 == joylink_cfg_Result(&Ret)) {
        if (Ret.type != 0) {
            log_info("innet ret type: %d", Ret.type);
            len = device_aes_decrypt((const uint8 *)AES_KEY, strlen(AES_KEY),AES_IV,
                Ret.encData + 1,Ret.encData[0], jd_aes_out_buffer,sizeof(jd_aes_out_buffer));
            if (len > 0) {
                struct station_config config;
                memset(&config,0x0,sizeof(config));
                if (jd_aes_out_buffer[0] > 32) {
                    log_error("sta password len error");
                    return rsp;
                }
                pass_len = jd_aes_out_buffer[0];
                memcpy(config.password,jd_aes_out_buffer + 1, pass_len);
                ssid_len = len - 1 - pass_len - 4 - 2;
                if (ssid_len > sizeof(config.ssid)) {
                    log_error("sta ssid len error");
                    return rsp;
                }
                strncpy((char*)config.ssid,(const char*)(jd_aes_out_buffer + 1 + pass_len + 4 + 2), ssid_len);
                if (0 == config.ssid[0]) {
                    log_error("sta ssid error");
                    return rsp;
                }
                log_info("ssid len:%d, ssid:%s", ssid_len, config.ssid);
                log_info("pass len:%d, password:%s", pass_len, config.password);
                if (true != wifi_station_set_config(&config)) {
                    log_error("set sta fail\r\n");
                    return rsp;
                } else {
                    jd_innet_stop();
                    wifi_station_connect();
                    rsp = TRUE;
                }
            } else {
                log_error("aes fail\r\n");
                return rsp;
            }
        }
    }else {
        log_error("result fail\r\n");
        return rsp;
    }
    return rsp;
}

void jd_innet_start_task(void *para)
{
INNET_RETRY:
    JDSnifGlob.stop_flag = 0;
	wifi_station_disconnect();
	wifi_set_opmode(STATION_MODE);
    if(NULL == s_pBuffer) {
        s_pBuffer = (unsigned char*)malloc(1024);
    }
    if (s_pBuffer == NULL) {
        log_error("malloc err");
        vTaskDelete(NULL);
        return;
    }
    s_sem_check_result = (xSemaphoreHandle)xSemaphoreCreateBinary();
    if (s_sem_check_result == NULL) {
        log_error("");
        vTaskDelete(NULL);
        return;
    }
    log_info("innet start");
    joylink_cfg_init(s_pBuffer);
    os_timer_disarm(&loop_timer);
    os_timer_setfn(&loop_timer, joylink_config_loop_do, NULL);
    os_timer_arm(&loop_timer, 50, 0); //loop to do task
    wifi_set_channel(1);
    wifi_promiscuous_enable(0);
    wifi_set_promiscuous_rx_cb(joylink_config_rx_callback);
    wifi_promiscuous_enable(1);

    while (1){
        if(pdTRUE == xSemaphoreTake(s_sem_check_result, 2000/portTICK_RATE_MS)) {//
            if (FALSE == jd_innet_get_result()){
                log_info("innet retry");
                jd_innet_stop();
                goto INNET_RETRY;
            }
            log_debug("high water %d ",uxTaskGetStackHighWaterMark(NULL));
            break;
        }
    }
    vTaskDelete(NULL);
}

bool jd_innet_start()
{
    if (NULL == AES_KEY) {
       return FALSE;
    }
    log_info("*********************************");
    log_info("*     ENTER SMARTCONFIG MODE    *");
    log_info("*********************************");
    xTaskCreate(jd_innet_start_task, "innet st", 1024-128, NULL, tskIDLE_PRIORITY + 2, NULL);
    return TRUE;
}

void ICACHE_FLASH_ATTR
jd_innet_stop(void)
{
	JDSnifGlob.stop_flag = 1;
	wifi_promiscuous_enable(0);
	os_timer_disarm(&loop_timer); //close timer
	if (s_pBuffer) {
        free(s_pBuffer);
        s_pBuffer = NULL;
    }
}

bool jd_innet_set_aes_key(const char *SecretKey)
{
    if (NULL == SecretKey) {
       return FALSE;
    }
    AES_KEY = SecretKey;
    log_debug("innet %s", AES_KEY);
    return TRUE;
}

const char *jd_innet_get_aes_key(void)
{
    return (const char *)AES_KEY;
}

