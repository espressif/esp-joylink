#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp-joylink

ESP_JOYLINK_VER := $(shell git rev-parse --short HEAD)

CFLAGS += -DESP_JOYLINK_COMMIT_ID=\"$(ESP_JOYLINK_VER)\"


ifeq ($(ESP_JOYLINK_DIR), )
ESP_JOYLINK_DIR := $(realpath ../..)
endif

EXTRA_COMPONENT_DIRS += $(ESP_JOYLINK_DIR)

ifneq (,$(wildcard $(IDF_PATH)/components/esp8266/*))
export IDF_TARGET = esp8266
else
ifndef IDF_TARGET
export IDF_TARGET := esp32
endif
endif

SDKCONFIG_DEFAULTS ?= $(realpath .)/default_configs/$(IDF_TARGET)/sdkconfig.defaults

include $(IDF_PATH)/make/project.mk
