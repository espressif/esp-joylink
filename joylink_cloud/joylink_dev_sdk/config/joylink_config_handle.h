/*
 * joylink_thunder_test.h
 *
 *  Created on: 2018Äê9ÔÂ5ÈÕ
 *      Author: lizhiwei5
 */

#ifndef _JOYLINK_SMARTCONFIG_JOYLINK_THUNDER_TEST_H_
#define _JOYLINK_SMARTCONFIG_JOYLINK_THUNDER_TEST_H_

#include "joylink_thunder_slave_sdk.h"
#include "joylink_smart_config.h"

int joylink_thunder_slave_init(void);
int joylink_thunder_slave_finish(tc_slave_result_t *result);

int joylink_smart_config_init(void);
void joylinke_smart_config_finish(joylink_smnt_result_t* presult);

int joylink_config_start(uint32_t time_out);
int joylink_config_stop(void);


#endif /* LIB_JOYLINK_SMARTCONFIG_JOYLINK_THUNDER_TEST_H_ */
