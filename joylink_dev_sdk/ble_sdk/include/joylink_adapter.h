/**************************************************************************************************
  Filename:       joylink_adapter.h
  Revised:        $Date: 2015-10-14
  Revision:       $Revision: 1.0, Zhang Hua

  Description:    This file contains the Joylink profile definitions and
                  prototypes.

  Copyright 2010 - 2013 JD.COM. All rights reserved.

**************************************************************************************************/


#ifndef __JOYLINK_ADAPTER_H__
#define __JOYLINK_ADAPTER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "joylink_sdk.h"

typedef struct {
	uint32_t second;
	uint32_t usecond;
}jl_timestamp_t;

int32_t jl_adapter_send_data(uint8_t *buf, uint32_t size);
int32_t jl_adapter_set_response(uint16_t data_tag, int32_t send_state);
int32_t jl_adapter_set_config_data(jl_net_config_data_t *data);
int32_t jl_adapter_set_system_time(jl_timestamp_t *time);

#ifdef __cplusplus
}
#endif

#endif /* __JOYLINK_ADAPTER_H__ */
