BIN_ROOT_PATH:=${PROJECT_ROOT_PATH}/../../../../../tools/gcc/gcc-arm-none-eabi/bin

CC=$(BIN_ROOT_PATH)/arm-none-eabi-gcc
AR=$(BIN_ROOT_PATH)/arm-none-eabi-ar
RANLIB=${BIN_ROOT_PATH}/arm-none-eabi-gcc-ranlib

#ALLFLAGS = -mlittle-endian -mthumb -mcpu=cortex-m4
#FPUFLAGS = -fsingle-precision-constant -Wdouble-promotion -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fno-common
#CFLAGS += $(ALLFLAGS) $(FPUFLAGS) -ffunction-sections -fdata-sections -fno-builtin -Wimplicit-function-declaration

CFLAGS += -D__MTK_7687__
ALLFLAGS = -mlittle-endian -mthumb -mcpu=cortex-m4
FPUFLAGS = -fsingle-precision-constant -Wdouble-promotion -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS += $(ALLFLAGS) $(FPUFLAGS) -ffunction-sections -fdata-sections -fno-builtin -Wimplicit-function-declaration
CFLAGS += -Os -Wall -fno-strict-aliasing -fno-common -O2
CFLAGS += -Wall -Wimplicit-function-declaration -Werror=uninitialized -Wno-error=maybe-uninitialized -Werror=return-type
CFLAGS += -D free=vPortFree -D malloc=pvPortMalloc -D calloc=pvPortCalloc -D realloc=pvPortRealloc -D strdup=mtk_strdup
CFLAGS += -D LWIP_IPV6=1 -D LWIP_IGMP=1 -D SO_REUSE=1

INCLUDES+= -I${PROJECT_ROOT_PATH}/../../iot_sdk/inc
INCLUDES+= -I${PROJECT_ROOT_PATH}/../../../../../kernel/service/inc
INCLUDES+= -I${PROJECT_ROOT_PATH}/../../../../../middleware/third_party/lwip/src/include
INCLUDES+= -I${PROJECT_ROOT_PATH}/../../../../../middleware/third_party/lwip/ports/include
INCLUDES+= -I${PROJECT_ROOT_PATH}/../../../../../driver/board/mt76x7_hdk/wifi/inc
INCLUDES+= -I${PROJECT_ROOT_PATH}/../../../../../driver/chip/inc
INCLUDES+= -I${PROJECT_ROOT_PATH}/../../../../../driver/chip/mt7687/inc
INCLUDES+= -I${PROJECT_ROOT_PATH}/../../../../../driver/CMSIS/Device/MTK/mt7687/Include
INCLUDES+= -I${PROJECT_ROOT_PATH}/../../../../../driver/CMSIS/Include
