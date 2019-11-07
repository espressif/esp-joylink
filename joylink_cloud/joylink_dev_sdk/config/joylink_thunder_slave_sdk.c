#include "joylink_thunder_slave_sdk.h"

#include "joylink_utils.h"
#include "joylink_auth_crc.h"
#include "joylink3_auth_uECC.h"

#include "joylink_probe.h"
#include "joylink_aes.h"
#include "joylink_log.h"
#include "joylink_ret_code.h"

static int joySlaveTxbufSetClaType(uint8_t *buf,tc_cla_type_t clatype);
static int joySlaveAESDecrypt(uint8_t *pEncIn, int encLength, uint8_t *pPlainOut, int maxOutLen); 
static int joySlaveAESEncrypt(uint8_t *pPlanin,int plainlength,uint8_t *pencout,int maxoutlen);
static int joySlaveDevSigGen(char *sig_buf,tc_vl_t *cloud_random);
static int joyCloudSigVerify(tc_vl_t *sig);
static int joyThunderProbeReqSend(uint8_t *cmddata,uint8_t len,void *prob_req);
static int joyDevRandomReSend(void);
static int joy80211PacketResend(void);



const char version_thunder[] = "JOYL_V1.0";

tc_slave_ctl_t 				tc_slave_ctl;
tc_slave_result_t			tc_slave_result;

switch_channel_cb_t 		switch_channel = NULL;
get_random_cb_t				get_random		= NULL;
thunder_finish_cb_t			result_notify_cb = NULL;
packet_80211_send_cb_t		packet_80211_send_cb = NULL;

static int joySlaveRamReset(void)
{
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	tc_slave_result_t *tthunder_result = &tc_slave_result;
/*
	if(tthunder_ctl->deviceid.value != NULL && tthunder_ctl->deviceid.length != 0){
		joylink_util_free(tthunder_ctl->deviceid.value);
	}

	if(tthunder_ctl->deviceinfo.value != NULL && tthunder_ctl->deviceinfo.length != 0){
		joylink_util_free(tthunder_ctl->deviceinfo.value);
	}
*/
	memset(&tthunder_ctl->thunder_state, 0, sizeof(tc_slave_state_t));
	tthunder_ctl->tcount = 0;
	tthunder_ctl->randSendTimes = 0;
	memset(tthunder_ctl->vendor_tx, 0, MAX_CUSTOM_VENDOR_LEN);
	tthunder_ctl->vendor_tx_len = 0;
	tthunder_ctl->current_channel = 1;

    memset(tthunder_ctl->mac_master, 0, JOY_MAC_ADDRESS_LEN);
    memset(tthunder_ctl->dev_random, 0, JOY_RANDOM_LEN);

	//memset(tthunder_ctl,0,sizeof(tc_slave_ctl_t));

	if(tthunder_result->ap_password.value != NULL && tthunder_result->ap_password.length != 0){
		joylink_util_free(tthunder_result->ap_password.value);
	}
	if(tthunder_result->ap_ssid.value != NULL && tthunder_result->ap_ssid.length != 0){
		joylink_util_free(tthunder_result->ap_ssid.value );
	}
	if(tthunder_result->cloud_feedid.value != NULL && tthunder_result->cloud_feedid.length != 0){
		joylink_util_free(tthunder_result->cloud_feedid.value );
	}
	if(tthunder_result->cloud_ackey.value != NULL && tthunder_result->cloud_ackey.length != 0){
		joylink_util_free(tthunder_result->cloud_ackey.value );
	}
	memset(tthunder_result,0,sizeof(tc_slave_result_t));
	
	return E_RET_OK;
}

static void joySlaveTimeReset(void)
{
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	tthunder_ctl->tcount = 0;
}


/**
 * brief: joylink thunderconfig state get.
 * 
 * @Param: N
 *
 * @Returns:N 
 */
tc_slave_state_t joySlaveStateGet(void)
{
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	return tthunder_ctl->thunder_state;
}

/**
 * brief: joylink thunderconfig state set.
 * 
 * @Param: tstate,the state to set.
 *
 * @Returns:N 
 */
static void joySlaveStateSet(tc_slave_state_t tstate)
{
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	tthunder_ctl->thunder_state = tstate;
	joySlaveTimeReset();
}

static int joySlaveCommCheck(tc_ins_t cmd,void *probe_req)
{
	int ret = E_RET_ERROR;

	tc_slave_state_t tstate;
	tstate = joySlaveStateGet();
	if(cmd == INS_LOCK_CHANNEL){
		if(tstate > sReqChannel){
			log_error("cmd INS_LOCK_CHANNEL not right,now state is %d",tstate);
			goto RET;
		}
	}else if(cmd == INS_AUTH_ALLOWED){
		if(tstate != sDevInfoUp){
			log_error("cmd INS_AUTH_ALLOWED not sDevInfoUp,now state is %d",tstate);
			goto RET;
		}
	}else if(cmd == INS_CHALLENGE_CLOUD_ACK){
		if(tstate != sDevChallengeUp){
			log_error("cmd INS_CHALLENGE_CLOUD_ACK not sDevChallengeUp,now state is %d",tstate);
			goto RET;
		}
	}else if(cmd == INS_AUTH_INFO_B){
		if(tstate != sDevSigUp){
			log_error("cmd INS_AUTH_INFO_B not sDevSigUp,now state is %d",tstate);
			goto RET;
		}
	}else{
		log_error("cmd not support");
		goto RET;
	}

	ret = E_RET_OK;
RET:	
	return ret;
}


static uint8_t joyGenChannelReqData(uint8_t *txbuf)
{
	uint8_t *tbuf,lc,tlen;
	tc_package_t *tc_packet;
	uint16_t tcrc16;
	
	if(txbuf == NULL){
		log_error("buff NULL");
		return 0;
	}

	
	tbuf = txbuf;
	tc_packet = (tc_package_t *)tbuf;
	tc_packet->oui_type = JOY_OUI_TYPE;
	memcpy(tc_packet->magic,JOY_MEGIC_HEADER,strlen(JOY_MEGIC_HEADER));
	tc_packet->cla 		= JOY_CLA_DECRYPT | JOY_CLA_UPLINK | JOY_CLA_FIRSTPACKET;
	tc_packet->ins		= INS_CONFIG_REQ;
	tc_packet->p1		= 0;
	tc_packet->p2		= 0;
	lc = joyTLVDataAdd(&(tc_packet->lc) + 1,TLV_TAG_THUNDERCONFIG_VERSION,strlen(version_thunder)+1,(uint8_t *)version_thunder);
	tc_packet->lc		= lc;
	
	tcrc16 = CRC16(txbuf+1,tc_packet->lc + JOY_PACKET_HEADER_LEN);
	tlen = JOY_PACKET_HEADER_LEN+tc_packet->lc + 1;
	
	memcpy(txbuf+tlen,&tcrc16,2);
	tlen += 2;				//crc size

	return tlen;
}

static uint8_t joyGenLockChlAckData(uint8_t *txbuf,tc_msg_value_t msg)
{
	uint8_t *tbuf,lc,tlen,offset_value = 0,*value;
	tc_package_t *tc_packet;
	uint16_t tcrc16;
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	
	if(txbuf == NULL){
		log_error("buff NULL");
		return 0;
	}
	
	tbuf = txbuf;
	tc_packet = (tc_package_t *)tbuf;
	tc_packet->oui_type = JOY_OUI_TYPE;
	memcpy(tc_packet->magic,JOY_MEGIC_HEADER,strlen(JOY_MEGIC_HEADER));
	tc_packet->cla 		= JOY_CLA_DECRYPT | JOY_CLA_UPLINK | JOY_CLA_FIRSTPACKET;
	tc_packet->ins		= INS_LOCK_CHANNEL_ACK;
	tc_packet->p1		= 0;
	tc_packet->p2		= 0;

	offset_value = 0;
	value = &(tc_packet->lc) + 1;
	
	
	if(msg != MSG_OK){
		lc = joyTLVDataAdd(value+offset_value,TLV_TAG_MSG,sizeof(msg),(uint8_t *)&msg);
		tc_packet->lc		= lc;
	}else{
		//uuid
		lc = joyTLVDataAdd(value+offset_value,TLV_TAG_UUID,JOY_UUID_LEN,tthunder_ctl->uuid);
		offset_value += lc;
		tc_packet->lc += lc;
		//device id
		if((tthunder_ctl->deviceid.length > 0) && (tthunder_ctl->deviceid.value != NULL)){
			lc = joyTLVDataAdd(value+offset_value,TLV_TAG_DEVICE_ID,\
								tthunder_ctl->deviceid.length,tthunder_ctl->deviceid.value);
			offset_value += lc;
			tc_packet->lc += lc;
		}
		//device info
		if((tthunder_ctl->deviceinfo.length > 0) && (tthunder_ctl->deviceinfo.value != NULL)){
			lc = joyTLVDataAdd(value+offset_value,TLV_TAG_DEVICE_INFO,\
								tthunder_ctl->deviceinfo.length,tthunder_ctl->deviceinfo.value );
			offset_value += lc;
			tc_packet->lc += lc;
		}
		//cid
		//brand

	}

	tcrc16 = CRC16(txbuf+1,tc_packet->lc + JOY_PACKET_HEADER_LEN);
	tlen = JOY_PACKET_HEADER_LEN+tc_packet->lc + 1;
	
	memcpy(txbuf+tlen,&tcrc16,2);
	tlen += 2;				//crc size

	return tlen;
}


static uint8_t joyGenDevRandomData(uint8_t *txbuf,tc_msg_value_t msg)
{
	uint8_t *tbuf,lc,tlen,offset_value = 0,*value;
	tc_package_t *tc_packet;
	uint16_t tcrc16;
	int i = 0;
	
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	
	if(txbuf == NULL){
		log_error("buff NULL");
		return 0;
	}
	
	tbuf = txbuf;
	tc_packet = (tc_package_t *)tbuf;
	tc_packet->oui_type = JOY_OUI_TYPE;
	memcpy(tc_packet->magic,JOY_MEGIC_HEADER,strlen(JOY_MEGIC_HEADER));
	tc_packet->cla 		= JOY_CLA_DECRYPT | JOY_CLA_UPLINK | JOY_CLA_FIRSTPACKET;
	tc_packet->ins		= INS_CHALLENGE_CLOUD;
	tc_packet->p1		= 0;
	tc_packet->p2		= 0;

	offset_value = 0;
	value = &(tc_packet->lc) + 1;
	
	
	if(msg != MSG_OK){
		lc = joyTLVDataAdd(value+offset_value,TLV_TAG_MSG,sizeof(msg),(uint8_t *)&msg);
		tc_packet->lc		= lc;
	}else{
		//uuid must param
		lc = joyTLVDataAdd(value+offset_value,TLV_TAG_UUID,JOY_UUID_LEN,tthunder_ctl->uuid);
		offset_value += lc;
		tc_packet->lc += lc;

		//mac must param
		lc = joyTLVDataAdd(value+offset_value,TLV_TAG_MAC,JOY_MAC_ADDRESS_LEN,tthunder_ctl->mac_dev);
		offset_value += lc;
		tc_packet->lc += lc;

		//device id optional param
		if((tthunder_ctl->deviceid.length > 0) && (tthunder_ctl->deviceid.value != NULL)){
			lc = joyTLVDataAdd(value+offset_value,TLV_TAG_DEVICE_ID,\
								tthunder_ctl->deviceid.length,tthunder_ctl->deviceid.value);
			offset_value += lc;
			tc_packet->lc += lc;
		}
		//device info optional pram
		if((tthunder_ctl->deviceinfo.length > 0) && (tthunder_ctl->deviceinfo.value != NULL)){
			lc = joyTLVDataAdd(value+offset_value,TLV_TAG_DEVICE_INFO,\
								tthunder_ctl->deviceinfo.length,tthunder_ctl->deviceinfo.value );
			offset_value += lc;
			tc_packet->lc += lc;
		}

		//random must param
		if(get_random == NULL){
			log_error("get random function NULL");
			return 0;
		}
		for(i=0;i<JOY_RANDOM_LEN;i++){
			tthunder_ctl->dev_random[i] = (uint8_t)(get_random());
		}
		lc = joyTLVDataAdd(value+offset_value,TLV_TAG_RANDOM,JOY_RANDOM_LEN,tthunder_ctl->dev_random);
		offset_value += lc;
		tc_packet->lc += lc;

		//cid
		//brand

	}

	tcrc16 = CRC16(txbuf+1,tc_packet->lc + JOY_PACKET_HEADER_LEN);
	tlen = JOY_PACKET_HEADER_LEN+tc_packet->lc + 1;
	
	memcpy(txbuf+tlen,&tcrc16,2);
	tlen += 2;				//crc size

	return tlen;
}


static uint8_t joyGenDevSigData(uint8_t *txbuf,tc_msg_value_t msg,tc_vl_t *cloud_random)
{
	uint8_t *tbuf,lc,tlen,offset_value = 0,*value;
	tc_package_t *tc_packet;
	uint16_t tcrc16;
	char sig_cloud_random[JOY_ECC_SIG_LEN];
	
	if(txbuf == NULL){
		log_error("buff NULL");
		return 0;
	}
	
	tbuf = txbuf;
	tc_packet = (tc_package_t *)tbuf;
	tc_packet->oui_type = JOY_OUI_TYPE;
	memcpy(tc_packet->magic,JOY_MEGIC_HEADER,strlen(JOY_MEGIC_HEADER));
	tc_packet->cla 		= JOY_CLA_DECRYPT | JOY_CLA_UPLINK | JOY_CLA_FIRSTPACKET;
	tc_packet->ins		= INS_CHALLENGE_DEVICE_ACK;
	tc_packet->p1		= 0;
	tc_packet->p2		= 0;

	offset_value = 0;
	value = &(tc_packet->lc) + 1;
	

	if((msg == MSG_OK) && (joySlaveDevSigGen(sig_cloud_random,cloud_random) == E_RET_ERROR)){
		msg = MSG_ERROR_UNKNOWN;
	}
	
	if(msg != MSG_OK){
		lc = joyTLVDataAdd(value+offset_value,TLV_TAG_MSG,sizeof(msg),(uint8_t *)&msg);
		tc_packet->lc		= lc;
	}else{
		//uuid must param
		lc = joyTLVDataAdd(value+offset_value,TLV_TAG_SIGNATURE,JOY_ECC_SIG_LEN,(uint8_t *)sig_cloud_random);
		offset_value += lc;
		tc_packet->lc += lc;

	}

	tcrc16 = CRC16(txbuf+1,tc_packet->lc + JOY_PACKET_HEADER_LEN);
	tlen = JOY_PACKET_HEADER_LEN+tc_packet->lc + 1;
	
	memcpy(txbuf+tlen,&tcrc16,2);
	tlen += 2;				//crc size

	return tlen;
}


static uint8_t joyGenAuthAckData(uint8_t *txbuf,tc_msg_value_t msg)
{
	uint8_t *tbuf,lc,tlen,offset_value = 0,*value;
	tc_package_t *tc_packet;
	uint16_t tcrc16;
	
	uint8_t encrypt_buf[256] = {0};
	uint8_t lc_en;
	
	if(txbuf == NULL){
		log_error("buff NULL");
		return 0;
	}
	
	tbuf = txbuf;
	tc_packet = (tc_package_t *)tbuf;
	tc_packet->oui_type = JOY_OUI_TYPE;
	memcpy(tc_packet->magic,JOY_MEGIC_HEADER,strlen(JOY_MEGIC_HEADER));
	tc_packet->cla 		= JOY_CLA_DECRYPT | JOY_CLA_UPLINK | JOY_CLA_FIRSTPACKET;
	tc_packet->ins		= INS_CONNECT_AP_ACK;
	tc_packet->p1		= 0;
	tc_packet->p2		= 0;

	offset_value = 0;
	value = &(tc_packet->lc) + 1;

	lc = joyTLVDataAdd(value+offset_value,TLV_TAG_MSG,sizeof(msg),(uint8_t *)&msg);
	tc_packet->lc		= lc;

	//decyptdata,
	log_info("decrypt data:lc = 0x%02X",tc_packet->lc);
	joylink_util_fmt_p("decrypt data:",value,tc_packet->lc);
	
	lc_en = joySlaveAESEncrypt(value,tc_packet->lc,encrypt_buf,sizeof(encrypt_buf));
	if(lc_en == 0){
		log_error("encrypt data error");
	}
	memcpy(value,encrypt_buf,lc_en);
	tc_packet->lc = lc_en;
		
	log_info("encrypt data:lc = 0x%02X",tc_packet->lc);
	joylink_util_fmt_p("decrypt data:",value,tc_packet->lc);
		
	tcrc16 = CRC16(txbuf+1,tc_packet->lc + JOY_PACKET_HEADER_LEN);
	tlen = JOY_PACKET_HEADER_LEN+tc_packet->lc + 1;
	
	memcpy(txbuf+tlen,&tcrc16,2);
	tlen += 2;				//crc size

	return tlen;
}



static int joyLockChlCommH(uint8_t *tlvsdata,uint8_t lc,tc_packet_type_t ttype,void *probe_req)
{
	int ret = E_RET_OK;
	uint8_t txlen,fixchl;
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	tc_msg_value_t msg_r = MSG_OK;
	
	struct ieee80211_mgmt *header_802 = NULL;
	uint8_t tag,offset= 0;

//	uint8_t len;

	if(tlvsdata == NULL || probe_req == NULL){
		log_error("BUFF NULL");
		ret = E_RET_ERROR;
		goto RET;
	}
	
	header_802 = (struct ieee80211_mgmt *)probe_req;
	log_info("joyLockChlCommH In ,addr: %02x:%02x:%02x:%02x:%02x:%02x",MAC2STR(header_802->sa));

	tag = *tlvsdata;
	offset++;//len
//	len = *(tlvsdata + offset);
	offset++;//value
	
	if(tag == TLV_TAG_WIFI_CHANNEL){
		fixchl = *(tlvsdata + offset);
		if(fixchl > 14){
			log_error("rev channel error");
		}
		
		log_info("fix channel:%d,currunt channel:%d",fixchl,tthunder_ctl->current_channel);
		if((fixchl != tthunder_ctl->current_channel) && (switch_channel != NULL)){
			switch_channel(fixchl);
			tthunder_ctl->current_channel = fixchl;
			log_info("verify channel to %d",fixchl);
		}
	}

    joySlaveStateSet(sDevInfoUp);

#if 0
	//must check param set
	if((tthunder_ctl->deviceid.length == 0) || (tthunder_ctl->deviceid.value == NULL)){
		log_error("deviceid NULL");
		msg_r = MSG_ERROR_UNKNOWN;
	}else{
		msg_r = MSG_OK;
	}
#endif	
	memset(tthunder_ctl->vendor_tx,0,sizeof(tthunder_ctl->vendor_tx));

	txlen = joyGenLockChlAckData(tthunder_ctl->vendor_tx,msg_r);
	if(txlen == 0){
		ret = E_RET_ERROR;
		goto RET;
	}

	tthunder_ctl->vendor_tx_len = txlen;
	log_info("joyGenLockChlAckData tx");
	joylink_util_fmt_p("vendor data:",tthunder_ctl->vendor_tx,tthunder_ctl->vendor_tx_len);
	
	ret = joyThunderProbeReqSend(tthunder_ctl->vendor_tx,tthunder_ctl->vendor_tx_len,probe_req);
	if(ret != E_RET_OK){
		log_error("pobe request send error");
		goto RET;
	}

	joySlaveStateSet(sDevInfoUp);
	//joySlaveTxbufSetClaType(tthunder_ctl->vendor_tx,joy_cla_resendpacket);

	//set mac address
	memcpy(tthunder_ctl->mac_master,header_802->sa,JOY_MAC_ADDRESS_LEN);
	//memcpy(tthunder_ctl->dev_info.mac,tthunder_ctl->dev_mac,JOY_MAC_ADDRESS_LEN);
	
	log_info("joyLockChlCommH Finsh,addr: %02x:%02x:%02x:%02x:%02x:%02x",MAC2STR(header_802->sa));
RET:
	
	return ret;
}

static int joy80211PacketResend(void)
{
	int ret = E_RET_OK;
	uint8_t txlens;
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	ret = joyThunderProbeReqSend(tthunder_ctl->vendor_tx,tthunder_ctl->vendor_tx_len,NULL);
	if(ret != E_RET_OK){
		log_error("pobe request send error");
	}

	return ret;
}


static int joyAuthAllowCommH(uint8_t *tlvsdata,uint8_t lc,tc_packet_type_t ttype,void *probe_req)
{
	int ret = E_RET_OK;
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	uint8_t toffset = 0,tag,length;
	struct ieee80211_mgmt *header_802 = NULL;
	uint8_t txlen;
	
	tc_msg_value_t rmsg = MSG_OK;
	tc_slave_state_t state;
	state = joySlaveStateGet();
	if(state != sDevInfoUp){
		log_error("now state is not right:%d",state);
	}
	
	if(tlvsdata == NULL || probe_req == NULL){
		log_error("NULL");
		ret = E_RET_ERROR;
		goto RET;
	}
	
	header_802 = (struct ieee80211_mgmt *)probe_req;
	log_info("joyAuthAllowCommH In,addr: %02x:%02x:%02x:%02x:%02x:%02x",MAC2STR(header_802->sa));

	while(toffset < lc){
		tag = *(tlvsdata + toffset);
		toffset++;
		length	= *(tlvsdata + toffset);
		toffset++;
		switch(tag){
			case TLV_TAG_MSG:
				if(length != 1){
					log_error("length of msg not right");
					break;
				}
				rmsg = *(tlvsdata + toffset);
				break;
				
			default:
				log_error("not support tag get:0x%02X",tag);
				break;
		}
		toffset += length;
	}
	if(rmsg != MSG_OK){
		joySlaveStateSet(sReject);
		ret = E_RET_ERROR;
		goto RET;
	}
	memset(tthunder_ctl->vendor_tx,0,sizeof(tthunder_ctl->vendor_tx));
	txlen = joyGenDevRandomData(tthunder_ctl->vendor_tx,rmsg);
	if(txlen == 0){
		ret = E_RET_ERROR;
		goto RET;
	}
	
	tthunder_ctl->vendor_tx_len = txlen;
	log_info("joyGenDevRandomData tx");
	joylink_util_fmt_p("vendor data:",tthunder_ctl->vendor_tx,tthunder_ctl->vendor_tx_len);
		
	ret = joyThunderProbeReqSend(tthunder_ctl->vendor_tx,tthunder_ctl->vendor_tx_len,probe_req);
	if(ret != E_RET_OK){
		log_error("pobe request send error");
		goto RET;
	}

	joySlaveStateSet(sDevChallengeUp);
	//joySlaveTxbufSetClaType(tthunder_ctl->vendor_tx,joy_cla_resendpacket);
	log_info("joyAuthAllowCommH Finish");
RET:
	return ret;
}


static int joyDevRandomReSend(void)
{
	int ret = E_RET_OK;
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	uint8_t txlen;

	memset(tthunder_ctl->vendor_tx,0,sizeof(tthunder_ctl->vendor_tx));
	txlen = joyGenDevRandomData(tthunder_ctl->vendor_tx,MSG_OK);
	if(txlen == 0){
		ret = E_RET_ERROR;
		goto RET;
	}
	
	tthunder_ctl->vendor_tx_len = txlen;
	log_info("joyDevRandomReSend tx");
	joylink_util_fmt_p("vendor data:",tthunder_ctl->vendor_tx,tthunder_ctl->vendor_tx_len);
		
	ret = joyThunderProbeReqSend(tthunder_ctl->vendor_tx,tthunder_ctl->vendor_tx_len,NULL);
	if(ret != E_RET_OK){
		log_error("pobe request send error");
		goto RET;
	}

	joySlaveStateSet(sDevChallengeUp);
	//joySlaveTxbufSetClaType(tthunder_ctl->vendor_tx,joy_cla_resendpacket);
	log_info("joyAuthAllowCommH Finish");
RET:
	return ret;
}




static int joyCloudChanngeCommH(uint8_t *tlvsdata,uint8_t lc,tc_packet_type_t ttype,void *probe_req)
{
	int ret = E_RET_OK;
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	uint8_t toffset = 0,tag,length;
	struct ieee80211_mgmt *header_802 = NULL;
	uint8_t txlen;
	tc_vl_t cloud_sig,cloud_random;
	
	tc_msg_value_t rmsg = MSG_OK;

	tc_slave_state_t state;
	state = joySlaveStateGet();
	if(state != sDevChallengeUp){
		log_error("now state is not right:%d",state);
	}
	
	if(tlvsdata == NULL || probe_req == NULL){
		log_error("NULL");
		ret = E_RET_ERROR;
		goto RET;
	}
	
	header_802 = (struct ieee80211_mgmt *)probe_req;
	log_info("joyAuthAllowCommH In,addr: %02x:%02x:%02x:%02x:%02x:%02x",MAC2STR(header_802->sa));

	memset(&cloud_sig,0,sizeof(cloud_sig));
	memset(&cloud_random,0,sizeof(cloud_random));
	
	while(toffset < lc){
		tag = *(tlvsdata + toffset);
		toffset++;
		length	= *(tlvsdata + toffset);
		toffset++;
		switch(tag){
			case TLV_TAG_MSG:
				if(length != 1){
					log_error("length of msg not right");
					break;
				}
				rmsg = *(tlvsdata + toffset);
				if(rmsg != MSG_OK){
					joySlaveStateSet(sReject);
					ret = E_RET_ERROR;
					goto RET;
				}
				break;
			case TLV_TAG_SIGNATURE:
				cloud_sig.length = length;
				cloud_sig.value = joylink_util_malloc(length);

				if(cloud_sig.value == NULL){
					rmsg = MSG_ERROR_UNKNOWN;
					log_error("malloc error");
					goto RET;
				}

				memcpy(cloud_sig.value,tlvsdata + toffset,length);
				
				break;
			case TLV_TAG_RANDOM:
				cloud_random.length = length;
				cloud_random.value = joylink_util_malloc(length);

				if(cloud_random.value == NULL){
					rmsg = MSG_ERROR_UNKNOWN;
					log_error("malloc error");
					goto RET;
				}
				memcpy(cloud_random.value,tlvsdata+toffset,length);
				break;
			default:
				log_error("not support tag get:0x%02X",tag);
				break;
		}
		toffset += length;
	}

	if(cloud_sig.length == 0 || cloud_random.length == 0){
		rmsg = MSG_ERROR_UNKNOWN;
	}
	
	//cloud signature verify
	if(E_RET_ERROR== joyCloudSigVerify(&cloud_sig)){
		rmsg = MSG_ERROR_VERIFY_FAILED;
	}
	memset(tthunder_ctl->vendor_tx,0,sizeof(tthunder_ctl->vendor_tx));
	txlen = joyGenDevSigData(tthunder_ctl->vendor_tx,rmsg,&cloud_random);
	if(txlen == 0){
		ret = E_RET_ERROR;
		goto RET;
	}
	
	tthunder_ctl->vendor_tx_len = txlen;
	log_info("joyGenDevSigData tx");
	joylink_util_fmt_p("vendor data:",tthunder_ctl->vendor_tx,tthunder_ctl->vendor_tx_len);
		
	ret = joyThunderProbeReqSend(tthunder_ctl->vendor_tx,tthunder_ctl->vendor_tx_len,probe_req);
	if(ret != E_RET_OK){
		log_error("pobe request send error");
		goto RET;
	}

	joySlaveStateSet(sDevSigUp);
	tthunder_ctl->randSendTimes = 0;
	//joySlaveTxbufSetClaType(tthunder_ctl->vendor_tx,joy_cla_resendpacket);
	log_info("joyAuthAllowCommH Finish");
RET:
	if(cloud_sig.value != NULL){
		joylink_util_free(cloud_sig.value);
	}
	if(cloud_random.value != NULL){
		joylink_util_free(cloud_random.value);
	}
	
	return ret;
}

static int joyCloudAuthInfoCommH(uint8_t *tlvsdata,uint8_t lc,tc_packet_type_t ttype,void *probe_req)
{
	int ret = E_RET_OK;
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	tc_slave_result_t *thunderresult = &tc_slave_result;
	uint8_t toffset = 0,tag,length;
	struct ieee80211_mgmt *header_802 = NULL;
	uint8_t txlen;
	
	tc_msg_value_t rmsg = MSG_OK;

	tc_slave_state_t state;
	state = joySlaveStateGet();
	if(state != sDevSigUp){
		log_error("now state is not right:%d",state);
	}
	
	if(tlvsdata == NULL || probe_req == NULL){
		log_error("NULL");
		ret = E_RET_ERROR;
		goto RET;
	}
	
	header_802 = (struct ieee80211_mgmt *)probe_req;
	log_info("joyCloudAuthInfoCommH In,addr: %02x:%02x:%02x:%02x:%02x:%02x",MAC2STR(header_802->sa));
	
	while(toffset < lc){
		tag = *(tlvsdata + toffset);
		toffset++;
		length	= *(tlvsdata + toffset);
		toffset++;
		switch(tag){			
			case TLV_TAG_MSG:
				if(length != 1){
					log_error("length of msg not right");
					break;
				}
				rmsg = *(tlvsdata + toffset);
				if(rmsg != MSG_OK){
					joySlaveStateSet(sReject);
					ret = E_RET_ERROR;
					goto RET;
				}
				break;
			case TLV_TAG_SSID:
				if(length > 32){
					log_error("ssid length error");
					rmsg = MSG_ERROR_UNKNOWN;
					break;
				}
				thunderresult->ap_ssid.length = length;
				thunderresult->ap_ssid.value = joylink_util_malloc(length+1);
				if(thunderresult->ap_ssid.value == NULL){
					log_error("malloc error");
					rmsg = MSG_ERROR_UNKNOWN;
					break;
				}
				memset(thunderresult->ap_ssid.value,0,length+1);
				memcpy(thunderresult->ap_ssid.value,tlvsdata + toffset, length);
				break;

			case TLV_TAG_PASSWD:
				if(length > 64){
					log_error("pass word length error");
					rmsg = MSG_ERROR_UNKNOWN;
					break;
				}
				if(length == 0)	break;
				thunderresult->ap_password.length = length;
				thunderresult->ap_password.value = joylink_util_malloc(length + 1);
				if(thunderresult->ap_password.value == NULL){
					log_error("malloc error");
					rmsg = MSG_ERROR_UNKNOWN;
					break;
				}
				memset(thunderresult->ap_password.value,0,length);
				memcpy(thunderresult->ap_password.value,tlvsdata + toffset,length);
				break;
			case TLV_TAG_FEEDID:
				if(length == 0)	break;
				thunderresult->cloud_feedid.length = length;
				thunderresult->cloud_feedid.value = joylink_util_malloc(length + 1);
				if(thunderresult->cloud_feedid.value == NULL){
					log_error("malloc error");
					rmsg = MSG_ERROR_UNKNOWN;
					break;
				}
				memset(thunderresult->cloud_feedid.value,0,length);
				memcpy(thunderresult->cloud_feedid.value,tlvsdata + toffset,length);
				break;
			case TLV_TAG_ACKEY:
				if(length == 0)	break;
				thunderresult->cloud_ackey.length = length;
				thunderresult->cloud_ackey.value = joylink_util_malloc(length + 1);
				if(thunderresult->cloud_ackey.value == NULL){
					log_error("malloc error");
					rmsg = MSG_ERROR_UNKNOWN;
					break;
				}
				memset(thunderresult->cloud_ackey.value,0,length);
				memcpy(thunderresult->cloud_ackey.value,tlvsdata + toffset,length);
				break;
			case TLV_TAG_CLOUD_SERVER:
				if(length == 0)	break;
				thunderresult->cloud_server.length = length;
				thunderresult->cloud_server.value = joylink_util_malloc(length + 1);
				if(thunderresult->cloud_server.value == NULL){
					log_error("malloc error");
					rmsg = MSG_ERROR_UNKNOWN;
					break;
				}

				memset(thunderresult->cloud_server.value,0,length);
				memcpy(thunderresult->cloud_server.value,tlvsdata + toffset,length);
				//log_info("server: %s\r\n", thunderresult->cloud_server.value);
				break;
				
			default:
				log_error("not support tag get:0x%02X",tag);
				break;
		}
		if(rmsg != MSG_OK)	break;
		toffset += length;
		
	}
	memset(tthunder_ctl->vendor_tx,0,sizeof(tthunder_ctl->vendor_tx));
	txlen = joyGenAuthAckData(tthunder_ctl->vendor_tx,rmsg);
	if(txlen == 0){
		ret = E_RET_ERROR;
		goto RET;
	}
	
	tthunder_ctl->vendor_tx_len = txlen;
	log_info("joyGenAuthAckData tx");
	joylink_util_fmt_p("vendor data:",tthunder_ctl->vendor_tx,tthunder_ctl->vendor_tx_len);
		
	ret = joyThunderProbeReqSend(tthunder_ctl->vendor_tx,tthunder_ctl->vendor_tx_len,probe_req);
	if(ret != E_RET_OK){
		log_error("pobe request send error");
		goto RET;
	}

	joySlaveStateSet(sFinish);
	//joySlaveTxbufSetClaType(tthunder_ctl->vendor_tx,joy_cla_resendpacket);
	log_info("joyCloudAuthInfoCommH Finish");
RET:

	return ret;
}


static int joySlaveCommHandle(uint8_t *venderspecific,uint8_t len,void *probe_req)
{
	int ret = E_RET_OK;
	uint8_t *vdata = venderspecific;
	uint16_t crc16_src,crc16_ds;
	tc_package_t *tcmmand;
	tc_packet_type_t tpacket_type;

	
	uint8_t dec_buf[256] = {0},dec_len;
//	struct ieee80211_mgmt *mgmt;
//	mgmt = (struct ieee80211_mgmt *)probe_req;
	
	tcmmand = (tc_package_t *)vdata;
	
	if(memcmp(tcmmand->magic,JOY_MEGIC_HEADER,strlen(JOY_MEGIC_HEADER)) != 0){
		ret = E_RET_ERROR;
		log_error("magic error");
		goto RET;
	}
	
//	log_info("joy packet from address %02x:%02x:%02x:%02x:%02x:%02x len=%d\n",MAC2STR(mgmt->sa), len);
		
	crc16_src = *(uint16_t *)(vdata+len-2);
tcmmand->cla &= 0xDF;//clear resend tag
	crc16_ds  = CRC16((uint8_t *)&(tcmmand->magic),(unsigned int)(tcmmand->lc + JOY_PACKET_HEADER_LEN));//+1
	if(crc16_src != crc16_ds){
		log_error("crc error:calc=0x%04X,src=0x%04X\n",crc16_ds,crc16_src);
		ret = E_RET_ERROR;
		goto RET;
	}
	
	if((tcmmand->p1 != 0) || (tcmmand->p2 != 0)){
		log_error("not support group packet now");
		ret = E_RET_ERROR;
		goto RET;
	}

	ret = joySlaveCommCheck(tcmmand->ins,probe_req);
	if(ret != E_RET_OK){
		log_error("command is not right operated level");
		goto RET;
	}
	
	if(tcmmand->cla & 0x80){
		//decrypt data
		dec_len = joySlaveAESDecrypt(&(tcmmand->lc) + 1,tcmmand->lc,dec_buf,sizeof(dec_buf));
		if((dec_len == 0) || (dec_len > tcmmand->lc)){
			log_error("decrypt error:enlen=0x%02X,dec_len=0x%02X",tcmmand->lc,dec_len);
			ret = E_RET_ERROR;
			goto RET;
		}
		memcpy(&(tcmmand->lc) + 1,dec_buf,dec_len);

		log_info("dec buffer->,len=%d",dec_len);
		joylink_util_fmt_p("dec buffer->",&(tcmmand->lc) + 1,dec_len);

		
		tcmmand->lc = dec_len;
	}

	tpacket_type = ((tcmmand->cla & 0x20) == 0)?packet_init:packet_resend;

	switch(tcmmand->ins){							//command handler
		case INS_V1_BASE:
			break;
			
		case INS_LOCK_CHANNEL:
			joyLockChlCommH(&(tcmmand->lc) + 1,tcmmand->lc,tpacket_type,probe_req);
			break;
			
		case INS_AUTH_ALLOWED:
			joyAuthAllowCommH(&(tcmmand->lc) + 1,tcmmand->lc,tpacket_type,probe_req);
			break;
			
		case INS_CHALLENGE_CLOUD_ACK:
			joyCloudChanngeCommH(&(tcmmand->lc) + 1,tcmmand->lc,tpacket_type,probe_req);
			break;
			
		case INS_AUTH_INFO_B:
			joyCloudAuthInfoCommH(&(tcmmand->lc) + 1,tcmmand->lc,tpacket_type,probe_req);
			break;
			
		default:
			log_error("command not support now");
			break;
	};

RET:
	return ret;
}

int joylink_send_probe_request(void *vendor_ie, int vendor_len)
{
    uint8_t *req_frame = NULL;
    int req_len = 0;

    if(vendor_len > MAX_CUSTOM_VENDOR_LEN || vendor_ie == NULL)
    {
        log_error("Input vendor ie invalid!\n");
        return -1;
    }

    req_frame = joylink_gen_probe_req(vendor_ie, vendor_len, &req_len);
    if(req_frame == NULL)
    {
        log_error("Genarate probe response frame error\n");
        return -1;
    }

    packet_80211_send_cb(req_frame, req_len);
    //mtk_custom_send_cmd(g_nl_sock, g_nl_family_id, getpid(), GL_CUSTOM_COMMAND_SEND,
    //    GL_CUSTOM_ATTR_MSG, req_frame, req_len);

    joylink_util_free(req_frame);
    return 0;
}

//probe response data cmddata:OUT-Type+payload
static int joyThunderProbeReqSend(uint8_t *cmddata,uint8_t len,void *prob_req)
{
	int ret = E_RET_OK;
	int vendor_len = 0;
	uint8_t vendor_ie[MAX_CUSTOM_VENDOR_LEN];
	vendor_len = joylink_gen_vendor_specific(vendor_ie,cmddata,len);

	ret = joylink_send_probe_request(vendor_ie, vendor_len);

	if(ret < 0){
		log_error("send probe response error");
		ret = E_RET_ERROR;
		goto RET;
	}

	//usleep(50*1000);
	//mico_rtos_thread_msleep(50);
	//system("wpa_cli -iwlan0 -p/tmp/wpa_supplicant scan");

	ret = E_RET_OK;
RET:
	return ret;
}

static int joySlaveChanelReq(void)
{
	int ret = E_RET_OK;
	tc_slave_state_t tstate;
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	uint8_t txlen;
//	struct ieee80211_mgmt header_802;
	
	//log_info("joySlaveChanelReq In");
	//check state
	tstate = joySlaveStateGet();
	if(tstate != sReqChannel){
		log_error("current state = 0x%02x is not thunder sReqChannel state",tstate);
		ret = E_RET_ERROR;
		goto RET;
	}

	//send probe response to fix channel
	memset(tthunder_ctl->vendor_tx,0,sizeof(tthunder_ctl->vendor_tx));

	txlen = joyGenChannelReqData(tthunder_ctl->vendor_tx);
	if(txlen == 0){
		ret = E_RET_ERROR;
		goto RET;
	}

	tthunder_ctl->vendor_tx_len = txlen;

//	memcpy(header_802.sa,tthunder_ctl->dev_mac,JOY_MAC_ADDRESS_LEN);
	
	ret = joyThunderProbeReqSend(tthunder_ctl->vendor_tx,tthunder_ctl->vendor_tx_len,NULL);
	if(ret != E_RET_OK){
		log_error("pobe request send error");
		goto RET;
	}
	
//	joyTxbufSetClaType(tthunder_ctl->vendor_tx,joy_cla_resendpacket);
	//log_info("joySlaveChanelReq Finish");

RET:
	return ret;
}

/**
 * brief:  aes decrypt 
 * @Param: pPlanin,
 * @Param: plainlength,length	
 * @Param: pencout,
 * @Param: maxoutlen,
 * @Returns:0-failed,<0-length
*/ 
static int joySlaveAESDecrypt(uint8_t *pEncIn, int encLength, uint8_t *pPlainOut, int maxOutLen)
{

	uint8_t sharekey[32],iv[16];
	
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	uint8_t dc_pubkey[64] = {0};
	jl3_uECC_decompress(tthunder_ctl->pubkey_c, dc_pubkey, uECC_secp256r1());
		
	if(0 == jl3_uECC_shared_secret(dc_pubkey,tthunder_ctl->prikey_d,sharekey,uECC_secp256r1())){
		log_error("gen share key error");
		return 0;
	}

	memcpy(iv,sharekey+16,sizeof(iv));

	return device_aes_decrypt(sharekey,16,iv,pEncIn,encLength,pPlainOut,maxOutLen);
	//return JLdevice_aes_decrypt(sharekey,16,iv,pEncIn,encLength,pPlainOut,maxOutLen);
	
}


/**
 * brief:  aes encrypt 
 * @Param: pPlanin,
 * @Param: plainlength,length	
 * @Param: pencout,
 * @Param: maxoutlen,
 * @Returns:0-failed,<0-length
*/ 
static int joySlaveAESEncrypt(uint8_t *pPlanin,int plainlength,uint8_t *pencout,int maxoutlen)
{
	uint8_t sharekey[32],iv[16];
		
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	
    uint8_t dc_pubkey[64] = {0};
    jl3_uECC_decompress(tthunder_ctl->pubkey_c, dc_pubkey, uECC_secp256r1());
	
	if(0 == jl3_uECC_shared_secret(jl3_uECC_decompress,tthunder_ctl->prikey_d,sharekey,uECC_secp256r1())){
		log_error("gen share key error");
		return 0;
	}

	joylink_util_fmt_p("sharekey->",sharekey,sizeof(sharekey));
	
	return device_aes_decrypt(sharekey,16,iv,pPlanin,plainlength,pencout,maxoutlen);
	//return JLdevice_aes_encrypt(sharekey,16,iv,pPlanin,plainlength,pencout,maxoutlen);
}


/**
 * brief:  generate signature used prikey_d to cloud random
 * @Param: sig_buf,buffer used to store device signature
 * @Param: cloud_random,cloud random in	
 * @Returns:E_RET_OK,E_RET_ERROR;
*/ 
static int joySlaveDevSigGen(char *sig_buf,tc_vl_t *cloud_random)
{
	int ret = E_RET_OK;
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	int sign_res;

	if(sig_buf == NULL || cloud_random == NULL){
		log_error("param NULL");
		return E_RET_ERROR;
	}

	if(cloud_random->value == NULL || cloud_random->length == 0){
		log_error("random NULL");
		return E_RET_ERROR;
	}
char rand_buf[65] = {0};
	int i = 0;

    joylink_util_byte2hexstr(cloud_random->value, cloud_random->length, rand_buf, 64);
    for(i = 0; i < 64; i++)
    {
        if(rand_buf[i] == 'a') rand_buf[i] = 'A';
        if(rand_buf[i] == 'b') rand_buf[i] = 'B';
        if(rand_buf[i] == 'c') rand_buf[i] = 'C';
        if(rand_buf[i] == 'd') rand_buf[i] = 'D';
        if(rand_buf[i] == 'e') rand_buf[i] = 'E';
        if(rand_buf[i] == 'f') rand_buf[i] = 'F';
    }
    printf("cloud random: %s\r\n", rand_buf);

	sign_res = jl3_uECC_sign((const uint8_t *)(tthunder_ctl->prikey_d),(const uint8_t *)rand_buf,\
	                            strlen(rand_buf),(uint8_t *)sig_buf,uECC_secp256r1());
	if(sign_res == 1){
		log_info("gen devsigature to cloud random success");
		joylink_util_fmt_p("cloud random:",cloud_random->value,cloud_random->length);
		joylink_util_fmt_p("prikey_d:",tthunder_ctl->prikey_d,JOY_ECC_PRIKEY_LEN);
		joylink_util_fmt_p("signature:",(uint8_t *)sig_buf,JOY_ECC_SIG_LEN);
		ret = E_RET_OK;
	}else{
		log_error("gen devsigature to cloud random error");
		ret = E_RET_ERROR;
	}
	return ret;
	
}

/**
 * brief:  generate signature used prikey_d to cloud random
 * @Param: sig_buf,buffer used to store device signature
 * @Param: cloud_random,cloud random in	
 * @Returns:E_RET_OK,E_RET_ERROR;
*/ 
static int joyCloudSigVerify(tc_vl_t *sig)
{
	int ret = E_RET_OK;
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	int verify_res;
	char rand_buf[65] = {0};
	int i;

	if(sig->value == NULL || sig == NULL){
		log_error("pram NULL");
		return E_RET_ERROR;
	}
	if(sig->length!= JOY_ECC_SIG_LEN){
		log_error("signature lengthe error:0x%02x",sig->length);
		return E_RET_ERROR;
	}

	log_info("verify cloud signature:");
	joylink_util_fmt_p("dev random:",tthunder_ctl->dev_random,JOY_RANDOM_LEN);
	joylink_util_fmt_p("cloud sig:",sig->value,sig->length);

joylink_util_byte2hexstr(tthunder_ctl->dev_random, JOY_RANDOM_LEN, rand_buf, 64);

    for(i = 0; i < 64; i++)
    {
        if(rand_buf[i] == 'a') rand_buf[i] = 'A';
        if(rand_buf[i] == 'b') rand_buf[i] = 'B';
        if(rand_buf[i] == 'c') rand_buf[i] = 'C';
        if(rand_buf[i] == 'd') rand_buf[i] = 'D';
        if(rand_buf[i] == 'e') rand_buf[i] = 'E';
        if(rand_buf[i] == 'f') rand_buf[i] = 'F';
    }
    log_info("dev_random: %s, len: %d\r\n", rand_buf, (int)strlen(rand_buf));

    uint8_t dc_pubkey[64] = {0};
    jl3_uECC_decompress(tthunder_ctl->pubkey_c, dc_pubkey, uECC_secp256r1());

    joylink_util_fmt_p("pubkey:", tthunder_ctl->pubkey_c, 64);
    joylink_util_fmt_p("pubkey_d:",dc_pubkey, 64);

	verify_res = jl3_uECC_verify(dc_pubkey, rand_buf,\
	                                strlen(rand_buf),sig->value,uECC_secp256r1());
	if(verify_res == 1){
		log_info("cloud sig verify success");
		ret = E_RET_OK;
	}else{
		log_info("cloud sig verify failed");
		ret =  E_RET_ERROR;
	}
	ret = E_RET_OK; /*!!!*/
	return ret;
}


static int joySlaveTxbufSetClaType(uint8_t *buf,tc_cla_type_t clatype)
{
	int ret = E_RET_OK;
	if(buf == NULL){
		log_error("buf NULL");
		ret = E_RET_ERROR;
		goto RET;
	}
	
	tc_package_t *tc_packet;
	tc_packet = (tc_package_t *)buf;

	if(clatype == joy_cla_firstpacket){
		tc_packet->cla = tc_packet->cla & 0xDF;
	}else{
		tc_packet->cla |= JOY_CLA_RESENDPACKET;
	}

	
RET:
	return ret;
}


static int joyThunderSlaveFinish(tc_msg_value_t errorcode)
{
	tc_slave_result_t	*tthunderresult = &tc_slave_result;
	if(errorcode == MSG_ERROR_TIMEOUT){
		log_info("TIMEOUT ,restart now");
		joyThunderSlaveStart();
		return E_RET_ERROR;
	}
	
	if(result_notify_cb == NULL){
		log_error("result_notify_cb NULL");
	}
	tthunderresult->errorcode = errorcode;
	result_notify_cb(tthunderresult);
	
	joySlaveRamReset();
	joySlaveStateSet(sInit);
	return E_RET_OK;
}


/**
 * brief:  thunder config slave init function,
 * @Param: tc_slave_func_param_t
 			typedef struct{
					uint8_t 		uuid[JOY_UUID_LEN];				//UUID,MUST Param
					uint8_t			prikey_d[JOY_ECC_PRIKEY_LEN];	//prikey_d,MUST Param
					uint8_t			pubkey_c[JOY_ECC_PUBKEY_LEN];	//pubkey_c,MUST Param
	
					uint8_t			mac_dev[JOY_MAC_ADDRESS_LEN];	//device mac address,MUSG param
					deviceid_t 		deviceid;						//device id,MUST Param,value--value address,length--value length
					deviceinfo_t	deviceinfo;						//device id,opentional param,if no set len = 0 or value = NULL
					void 			*switch_channel(unsigned char);	//swithc channel call back function
					int  			*get_random(void);					//get random call back function
					int				*result_notify_cb(tc_slave_result_t *);	//result notify call back function
					int 			*packet_80211_send_cb(unsigned char *buf,int buflen);//802.11 layer packet send call back function
			}tc_slave_func_param_t;
			
 * @Returns:E_RET_OK,E_RET_ERROR;
*/ 
extern uint8_t g_own_mac[ETH_ALEN];

int joyThunderSlaveInit(tc_slave_func_param_t *para)
{
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;
	
	log_info("thunder slave init,VERSION:%s",version_thunder);

	joySlaveRamReset();

	if(para->switch_channel == NULL){
		log_error("switch_channel NULL");
	}

	switch_channel = para->switch_channel;

	if(para->get_random == NULL){
		log_error("get_random NULL");
	}
	get_random = para->get_random;
	
	//jl3_uECC_set_rng((uECC_RNG_Function)para->get_random);

	if(para->result_notify_cb == NULL){
		log_error("result_notify_cb NULL");
	}
	result_notify_cb = para->result_notify_cb;

	if(para->packet_80211_send_cb == NULL){
		log_error("packet_80211_send_cb NULL");
	}
	packet_80211_send_cb = para->packet_80211_send_cb;
	
	//uuid
	memcpy(tthunder_ctl->uuid,para->uuid,JOY_UUID_LEN);
	joylink_util_fmt_p("set UUID:",tthunder_ctl->uuid,JOY_UUID_LEN);

	//pubkey_c
	memcpy(tthunder_ctl->pubkey_c,para->pubkey_c,JOY_ECC_PUBKEY_LEN);
	joylink_util_fmt_p("set pubkey_c:",tthunder_ctl->pubkey_c,JOY_ECC_PUBKEY_LEN);

	//prikey_d
	memcpy(tthunder_ctl->prikey_d,para->prikey_d,JOY_ECC_PRIKEY_LEN);
	joylink_util_fmt_p("set prikey_d:",tthunder_ctl->prikey_d,JOY_ECC_PRIKEY_LEN);

	//mac adress
	memcpy(tthunder_ctl->mac_dev,para->mac_dev,JOY_MAC_ADDRESS_LEN);
	memcpy(g_own_mac,para->mac_dev,JOY_MAC_ADDRESS_LEN);
	joylink_util_fmt_p("set MAC:",tthunder_ctl->mac_dev,JOY_MAC_ADDRESS_LEN);
	
	if((para->deviceid.value != NULL) && (para->deviceid.length > 0)){
	    if(tthunder_ctl->deviceid.value != NULL){
	        joylink_util_free(tthunder_ctl->deviceid.value);
	    }
	    tthunder_ctl->deviceid.value = joylink_util_malloc(para->deviceid.length);
		tthunder_ctl->deviceid.length = para->deviceid.length;
		memcpy(tthunder_ctl->deviceid.value ,para->deviceid.value,tthunder_ctl->deviceid.length);
		//log_info("set device id:len=%d",tthunder_ctl->deviceid.length);
		//joylink_util_fmt_p("device id:",tthunder_ctl->deviceid.value,tthunder_ctl->deviceid.length);
	}else{
		//device id is the must set param
		log_error("device id NULL");
	}

	if((para->deviceinfo.value != NULL) && (para->deviceinfo.length > 0)){
		tthunder_ctl->deviceinfo.length = para->deviceinfo.length;
		memcpy(tthunder_ctl->deviceinfo.value,para->deviceinfo.value,tthunder_ctl->deviceinfo.length);
		log_info("set device info:len=%d",tthunder_ctl->deviceinfo.length);
		joylink_util_fmt_p("device info:",tthunder_ctl->deviceinfo.value,tthunder_ctl->deviceinfo.length);
	}
	return E_RET_OK;
}

/**
 * brief:  thunderconfig slave start function
 * @Param: NULL
 * @Returns:E_RET_OK,E_RET_ERROR;
*/ 
int joyThunderSlaveStart(void)
{
	joySlaveRamReset();
	joySlaveChanelReq();
	joySlaveStateSet(sReqChannel);
	return E_RET_OK;
}

/**
 * brief:  thunderconfig slave stop function
 * @Param: NULL
 * @Returns:E_RET_OK,E_RET_ERROR;
*/ 
int joyThunderSlaveStop(void)
{
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;

	joySlaveRamReset();

	if(tthunder_ctl->deviceid.value != NULL && tthunder_ctl->deviceid.length != 0){
		joylink_util_free(tthunder_ctl->deviceid.value);
	}

	if(tthunder_ctl->deviceinfo.value != NULL && tthunder_ctl->deviceinfo.length != 0){
		joylink_util_free(tthunder_ctl->deviceinfo.value);
	}

	memset(tthunder_ctl,0,sizeof(tc_slave_ctl_t));
	
	return E_RET_OK;
}
/**
 * brief:  802.11 packet handler function,the device capture the 802.11 layer packet,
 		transfer it to this function to handle it.
 * @Param: probe_req,packet buffer address
 * @Param: req_len,packet len
 * @Returns:NULL
*/ 
void joyThunderSlaveProbeH(void *probe_req, int req_len)
{
	struct ieee80211_mgmt *mgmt;
	const uint8_t *ie;
	size_t ie_len;
	struct ieee802_11_elems elems;
	uint8_t content[MAX_CUSTOM_VENDOR_LEN] = {0};
	static uint16_t seq = 0xFFFF;
	mgmt = (struct ieee80211_mgmt *)probe_req;
		
	if((mgmt->frame_control & IEEE80211_FC(WLAN_FC_TYPE_MGMT, WLAN_FC_STYPE_PROBE_RESP))  != IEEE80211_FC(WLAN_FC_TYPE_MGMT, WLAN_FC_STYPE_PROBE_RESP) )
	{
		return;
	}
	ie = mgmt->u.probe_resp.variable;

	if (req_len < IEEE80211_HDRLEN + sizeof(mgmt->u.probe_req))
		return;

	ie_len = req_len - (IEEE80211_HDRLEN + sizeof(mgmt->u.probe_req));

//	if(memcmp(mgmt->sa,addr,6) == 0){
//		log_info("get request packet:seq num = 0x%04X",mgmt->seq_ctrl);
//	}
//	log_info("[Demo]Got ProbeReq from addr %02x:%02x:%02x:%02x:%02x:%02x len=%d\n",MAC2STR(mgmt->sa), req_len);

	/* 1 parse probe request ie fields */
	if (ieee802_11_parse_elems(ie, ie_len, &elems) != 0) {
		log_error("Could not parse ProbeReq from \n");
		return;
	}

	/* 2 Decide if it is wanted probe request frame from elems.vendor_custom */
	/* elems.vendor_custom is alibaba vendor now */
	memset(content, 0, MAX_CUSTOM_VENDOR_LEN);
	if(seq == mgmt->seq_ctrl){
		log_error("seq number same");
		return ;
	}

	seq = mgmt->seq_ctrl;
	
	if(elems.vendor_custom != NULL && elems.vendor_custom_len > 3)
	{
		memcpy(content, elems.vendor_custom + 3, elems.vendor_custom_len - 3);
		joySlaveCommHandle(content,elems.vendor_custom_len - 3,probe_req);
	}else{
		if(elems.vendor_custom == NULL){
			log_error("vendor_custom NULL");
		}
		log_error("vendor_custom_len=%d",elems.vendor_custom_len);
	}
}

/**
 * brief:  cycle call function ,the function is expected call every 50ms
 * @Param: NULL
 * @Param: NULL
 * @Returns:NULL
*/ 
void joyThunderSlave50mCycle(void)
{
	tc_slave_state_t tstate;
	tc_slave_ctl_t *tthunder_ctl = &tc_slave_ctl;

	tstate = joySlaveStateGet();
	//log_info("state: %d\n", tstate);
	switch(tstate){
		case sInit:					//init
			break;	
		case sReqChannel:				//requeset channel of thunderconfig master
			tthunder_ctl->tcount++;
			if(tthunder_ctl->tcount == JOY_REQCHANNEL_STAY_COUNT){
				if(tthunder_ctl->current_channel >= 13) 
					tthunder_ctl->current_channel = 1;
				else 
					tthunder_ctl->current_channel++;
				
				//tthunder_ctl->current_channel = 13;
				if(switch_channel != NULL){
					log_info("-->switch channel to:%d",tthunder_ctl->current_channel);
					switch_channel(tthunder_ctl->current_channel);
					joySlaveTimeReset();
				}
			}
			
			//send channel requeset packet every 3*50ms
			if(tthunder_ctl->tcount % 3 == 0){
				joySlaveChanelReq();
			}
			
			break;
		case sDevInfoUp: 			//devinfo transfer		//user confirm
			tthunder_ctl->tcount++;
			if(((tthunder_ctl->tcount) % 4) == 0){
				joy80211PacketResend();
			}
			
			if(tthunder_ctl->tcount > JOY_DEVINFO_UP_TIMEOUT){
				joyThunderSlaveFinish(MSG_ERROR_TIMEOUT);
			}

			break;
		case sDevChallengeUp:		//device challenge up
		
			tthunder_ctl->tcount++;
			if(tthunder_ctl->tcount > JOY_DEVRAND_UP_TIMEOUT){
				
				if(tthunder_ctl->randSendTimes > JOY_THUNDER_RANDOM_SEND_TIME){
					joyThunderSlaveFinish(MSG_ERROR_TIMEOUT);
					tthunder_ctl->randSendTimes++;
				}else{
					tthunder_ctl->randSendTimes++;
					joyDevRandomReSend();
				}
			}
			break;
		case sDevSigUp:				//device signature up
			tthunder_ctl->tcount++;
			if(tthunder_ctl->tcount > JOY_DEVSIG_UP_TIMEOUT){
				
				if(tthunder_ctl->randSendTimes > JOY_THUNDER_RANDOM_SEND_TIME){
					joyThunderSlaveFinish(MSG_ERROR_TIMEOUT);
					tthunder_ctl->randSendTimes++;
				}else{
					tthunder_ctl->randSendTimes++;
					joyDevRandomReSend();
				}
			}
			break;
		case sFinish:
			tthunder_ctl->tcount++;
			if(tthunder_ctl->tcount > JOY_THUNDER_TIMEOUT_FINISH){
				joyThunderSlaveFinish(MSG_OK);
			}
			break;
		case sReject:
			tthunder_ctl->tcount++;
			if(tthunder_ctl->tcount > JOY_THUNDER_TIMEOUT_REJECT){
				joyThunderSlaveFinish(MSG_ERROR_REQ_DENIED);
			}
			break;
		default:
			break;
	}
}


