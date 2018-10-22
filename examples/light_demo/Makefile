#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp-joylink

ESP_JOYLINK_VER := $(shell git rev-parse --short HEAD)

CFLAGS += -DESP_JOYLINK_COMMIT_ID=\"$(ESP_JOYLINK_VER)\"

export PROJECT_DIR ?= $(PWD)/../..

EXTRA_COMPONENT_DIRS += $(PROJECT_DIR)/components/port
EXTRA_COMPONENT_DIRS += $(PROJECT_DIR)/components/joylink_cloud
EXTRA_COMPONENT_DIRS += $(PWD)/button

include $(IDF_PATH)/make/project.mk
