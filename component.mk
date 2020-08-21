#
# component Makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

JOYLINK_EXTERN ?= joylink_sdk/extern
JOYLINK_LIB ?= joylink_sdk/joylink
JOYLINK_PAL ?= joylink_sdk/pal
JOYLINK_PORT ?= port

COMPONENT_ADD_INCLUDEDIRS += $(JOYLINK_EXTERN)
COMPONENT_SRCDIRS += $(JOYLINK_EXTERN)

JOYLINK_LIB_INCLUDEDIRS = inc inc/json inc/softap
COMPONENT_ADD_INCLUDEDIRS += $(addprefix $(JOYLINK_LIB)/,$(JOYLINK_LIB_INCLUDEDIRS))

JOYLINK_PAL_INCLUDEDIRS = inc
JOYLINK_PAL_SRCDIRS = src

COMPONENT_ADD_INCLUDEDIRS += $(addprefix $(JOYLINK_PAL)/,$(JOYLINK_PAL_INCLUDEDIRS))
COMPONENT_SRCDIRS += $(addprefix $(JOYLINK_PAL)/,$(JOYLINK_PAL_SRCDIRS))

JOYLINK_PORT_INCLUDEDIRS = include
COMPONENT_ADD_INCLUDEDIRS += $(addprefix $(JOYLINK_PORT)/,$(JOYLINK_PORT_INCLUDEDIRS))

ifdef CONFIG_IDF_TARGET_ESP8266
LIBS += joylink

COMPONENT_ADD_LDFLAGS += -L $(COMPONENT_PATH)/joylink_sdk/joylink/lib/xtensa_esp8266 \
                           $(addprefix -l,$(LIBS))
endif

CFLAGS += -D__ESP_PAL__