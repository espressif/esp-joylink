#
# component Makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

JOYLINK_SDK_DIR ?= joylink_dev_sdk

JOYLINK_LIB = $(JOYLINK_SDK_DIR)/joylink
JOYLINK_PAL = $(JOYLINK_SDK_DIR)/pal
JOYLINK_PORT ?= port
JOYLINK_BLE ?= $(JOYLINK_SDK_DIR)/ble_sdk

JOYLINK_LIB_INCLUDEDIRS = inc inc/json inc/softap
COMPONENT_ADD_INCLUDEDIRS += $(addprefix $(JOYLINK_LIB)/,$(JOYLINK_LIB_INCLUDEDIRS))

JOYLINK_PAL_INCLUDEDIRS = inc
JOYLINK_PAL_SRCDIRS = src

COMPONENT_ADD_INCLUDEDIRS += $(addprefix $(JOYLINK_PAL)/,$(JOYLINK_PAL_INCLUDEDIRS))
COMPONENT_SRCDIRS += $(addprefix $(JOYLINK_PAL)/,$(JOYLINK_PAL_SRCDIRS))

JOYLINK_PORT_INCLUDEDIRS = include
COMPONENT_ADD_INCLUDEDIRS += $(addprefix $(JOYLINK_PORT)/,$(JOYLINK_PORT_INCLUDEDIRS))

COMPONENT_DEPENDS = joylink_extern

ifeq ($(strip $(CONFIG_JOYLINK_BLE_ENABLE)), y)
JOYLINK_BLE_INCLUDEDIRS = include
JOYLINK_BLE_SRCDIRS = adapter
COMPONENT_ADD_INCLUDEDIRS += $(addprefix $(JOYLINK_BLE)/,$(JOYLINK_BLE_INCLUDEDIRS))
COMPONENT_SRCDIRS += $(addprefix $(JOYLINK_BLE)/,$(JOYLINK_BLE_SRCDIRS))
endif

COMPONENT_SRCDIRS += ${JOYLINK_PORT}

LIBS = joylink

ifeq ($(strip $(CONFIG_IDF_TARGET_ESP8266)), y)
CONFIG_IDF_TARGET ?= esp8266
endif

COMPONENT_ADD_LDFLAGS += -L $(COMPONENT_PATH)/$(JOYLINK_SDK_DIR)/joylink/lib/${CONFIG_IDF_TARGET} \
                           $(addprefix -l,$(LIBS))

ifeq ($(strip $(CONFIG_JOYLINK_BLE_ENABLE)), y)
LIBS += joylinkblesdk
COMPONENT_ADD_LDFLAGS += -L $(COMPONENT_PATH)/$(JOYLINK_SDK_DIR)/ble_sdk/target/lib/${CONFIG_IDF_TARGET} \
                           $(addprefix -l,$(LIBS))
endif

CFLAGS += -D__LINUX_PAL__ -Wno-error=maybe-uninitialized -Wno-error=return-type