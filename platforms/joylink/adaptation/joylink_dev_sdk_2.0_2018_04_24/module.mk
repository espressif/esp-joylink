
JOYLINK_SRC = middleware/third_party/joylink_v2/joylink_dev_sdk_2.0

C_FILES  += 		 		$(JOYLINK_SRC)/auth/crc.c \
		 					$(JOYLINK_SRC)/auth/joylinkAES.c \
		 					$(JOYLINK_SRC)/auth/joylink_crypt.c \
		 					$(JOYLINK_SRC)/auth/md5.c \
		 					$(JOYLINK_SRC)/auth/uECC.c \
		 					$(JOYLINK_SRC)/example/joylink_extern.c \
		 					$(JOYLINK_SRC)/example/joylink_extern_json.c \
		 					$(JOYLINK_SRC)/example/joylink_extern_sub_dev.c \
							$(JOYLINK_SRC)/example/mtk_ctrl.c \
							$(JOYLINK_SRC)/example/joylink_porting_layer.c \
		 					$(JOYLINK_SRC)/joylink/joylink_dev_lan.c \
		 					$(JOYLINK_SRC)/joylink/joylink_dev_sdk.c \
		 					$(JOYLINK_SRC)/joylink/joylink_dev_server.c \
		 					$(JOYLINK_SRC)/joylink/joylink_dev_timer.c \
		 					$(JOYLINK_SRC)/joylink/joylink_join_packet.c \
		 					$(JOYLINK_SRC)/joylink/joylink_packets.c \
		 					$(JOYLINK_SRC)/joylink/joylink_security.c \
							$(JOYLINK_SRC)/joylink/joylink_sub_dev.c \
		 					$(JOYLINK_SRC)/joylink/joylink_utils.c \
		 					$(JOYLINK_SRC)/json/cJSON.c \
		 					$(JOYLINK_SRC)/json/joylink_json.c \
		 					$(JOYLINK_SRC)/json/joylink_json_sub_dev.c \
			 				$(JOYLINK_SRC)/list/joylink_list.c

#C_FILES  += $(JOYLINK_SRC)/src/auth/aes.c \
#C_FILES  += $(JOYLINK_SRC)/src/extern/joylink_extern.c \
#C_FILES  += $(JOYLINK_SRC)/src/extern/joylink_extern_sub_dev.c \
			 				 	 					
#################################################################################
#include path
CFLAGS  += -I$(SOURCE_DIR)/$(JOYLINK_SRC)
CFLAGS  += -I$(SOURCE_DIR)/$(JOYLINK_SRC)/auth
CFLAGS  += -I$(SOURCE_DIR)/$(JOYLINK_SRC)/example
CFLAGS  += -I$(SOURCE_DIR)/$(JOYLINK_SRC)/extern
CFLAGS  += -I$(SOURCE_DIR)/$(JOYLINK_SRC)/joylink
CFLAGS  += -I$(SOURCE_DIR)/$(JOYLINK_SRC)/json
CFLAGS  += -I$(SOURCE_DIR)/$(JOYLINK_SRC)/list
CFLAGS	+= -I$(SOURCE_DIR)/middleware/third_party/lwip/src/include
CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/lwip/src/include/lwip
CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/lwip/ports/include
CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source/include 
CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source/portable/GCC/ARM_CM4F
CFLAGS  += -I$(SOURCE_DIR)/driver/chip/mt7687/inc
CFLAGS  += -I$(SOURCE_DIR)/driver/chip/inc
CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/httpclient/inc
CFLAGS 	+= -I$(SOURCE_DIR)/middleware/MTK/fota/inc
CFLAGS 	+= -I$(SOURCE_DIR)/middleware/MTK/fota/inc/76x7

CFLAGS += -D__MTK_7687__ -D_GET_HOST_BY_NAME_
CFLAGS += -D free=vPortFree -D malloc=pvPortMalloc -D calloc=pvPortCalloc -D realloc=pvPortRealloc -D strdup=mtk_strdup
CFLAGS += -D LWIP_IPV6=1 -D LWIP_IGMP=1 -D SO_REUSE=1
