#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp-joylink

ESP_JOYLINK_VER := $(shell git rev-parse --short HEAD)

CFLAGS += -DESP_JOYLINK_COMMIT_ID=\"$(ESP_JOYLINK_VER)\"
CFLAGS += -DJOYLINK_CLOUD_AUTH
CFLAGS += -D_IS_DEV_REQUEST_ACTIVE_SUPPORTED_

EXTRA_COMPONENT_DIRS += $(realpath ../..)
EXTRA_COMPONENT_DIRS += ./button

include $(IDF_PATH)/make/project.mk
