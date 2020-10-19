# xtensa-esp32s2-elf-gcc (crosstool-NG esp-2020r2) 8.2.0
CROSS_COMPILE := /work/disk1/share/toolchain/xtensa-esp32s2-elf/bin/xtensa-esp32s2-elf-

# x86 arch 
CFLAGS := -DuECC_PLATFORM=uECC_arch_other -std=gnu99 -Og -ggdb -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -mlongcalls -nostdlib -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-old-style-declaration
# other arch 
# CFLAGS := -DuECC_PLATFORM=uECC_arch_other -D__LINUX_PAL__ -DJOYLINK_SDK_EXAMPLE_TEST -D_SAVE_FILE_

LDFLAGS = -lpthread -lm

USE_JOYLINK_JSON=yes

#----------------------------------------------以下固定参数
CFLAGS += -D_IS_DEV_REQUEST_ACTIVE_SUPPORTED_ -D_GET_HOST_BY_NAME_

TARGET_DIR = ${TOP_DIR}/target
TARGET_LIB = ${TARGET_DIR}/lib
TARGET_BIN = ${TARGET_DIR}/bin

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
RANLIB=$(CROSS_COMPILE)ranlib
STRIP=$(CROSS_COMPILE)strip

RM = rm -rf
CP = cp -rf
MV = mv -f


