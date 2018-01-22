/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __JOYLINK_CONFIG_H__
#define __JOYLINK_CONFIG_H__

#ifdef  __cplusplus
extern "C" {
#endif

#define JOYLINK_SNTP_SERVER1_NAME       "cn.pool.ntp.org"       /*!< first sntp server addr, sync internet time */
#define JOYLINK_SNTP_SERVER2_NAME       "edu.ntp.org.cn"        /*!< second sntp time */

/* joylink device info */
#ifdef JOYLINK_PASSTHROUGH
#define JOYLINK_AES_KEY      "EZN9V4XDHHA7CFS2"     
#define JOYLINK_VERSION      1
#define JOYLINK_ACCESSKEY    ""
#define JOYLINK_LOCAL_KEY    ""
#define JOYLINK_FEEDID       ""
#define JOYLINK_DEVTYPE      E_JLDEV_TYPE_NORMAL
#define JOYLINK_SERVER       ""
#define JOYLINK_SERVER_PORT  2002
#define JOYLINK_BRAND        "38C4"
#define JOYLINK_CID          "103008"
#define JOYLINK_FW_VERSION   "001"
#define JOYLINK_MODEL_CODE   "iK2A"
#define JOYLINK_UUID         "K47XQH"
#define JOYLINK_LAN_CTRL     E_LAN_CTRL_DISABLE
#define JOYLINK_CMD_TYPE     E_CMD_TYPE_JSON

#else

#define JOYLINK_AES_KEY      "K8AG43A6M4BN6PAG"
#define JOYLINK_LOCAL_PORT   4320
#define JOYLINK_VERSION      1
#define JOYLINK_ACCESSKEY    ""
#define JOYLINK_LOCAL_KEY    ""
#define JOYLINK_FEEDID       ""
#define JOYLINK_DEVTYPE      E_JLDEV_TYPE_NORMAL
#define JOYLINK_SERVER       "live.smart.jd.com"
#define JOYLINK_SERVER_PORT  2002
#define JOYLINK_BRAND        "38C4"
#define JOYLINK_CID          "011c022b"
#define JOYLINK_FW_VERSION   "0.1.1"
#define JOYLINK_MODEL_CODE   "A3"
#define JOYLINK_UUID         "CF1484"
#define JOYLINK_LAN_CTRL     E_LAN_CTRL_DISABLE
#define JOYLINK_CMD_TYPE     E_CMD_TYPE_JSON
#define CLOUD_PUB_KEY        "03D5A54ACF235E77FC1240754DB26BC0E20949E5A3C68338A635CA646EC336D1D9"


#endif

#ifdef __cplusplus
}
#endif

#endif
