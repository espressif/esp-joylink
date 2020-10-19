# (Automatically converted from project Makefile by convert_to_cmake.py.)

# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

execute_process(COMMAND git rev-parse --short HEAD
                OUTPUT_VARIABLE ESP_JOYLINK_VER
                OUTPUT_STRIP_TRAILING_WHITESPACE)
add_compile_options("-DESP_JOYLINK_COMMIT_ID=\"${ESP_JOYLINK_VER}\"")

if (NOT DEFINED ESP_JOYLINK_DIR)
if (NOT DEFINED ENV{ESP_JOYLINK_DIR})
set (ESP_JOYLINK_DIR "../..")
else()
set (ESP_JOYLINK_DIR $ENV{ESP_JOYLINK_DIR})
endif()
endif()

set (EXTRA_COMPONENT_DIRS ${ESP_JOYLINK_DIR})

if(EXISTS "$ENV{IDF_PATH}/components/esp8266")
    set(IDF_TARGET "esp8266")
else()
if(NOT DEFINED IDF_TARGET)
    set(IDF_TARGET "esp32")
endif()
endif()

if(NOT DEFINED SDKCONFIG_DEFAULTS)
set(SDKCONFIG_DEFAULTS ${CMAKE_CURRENT_LIST_DIR}/default_configs/${IDF_TARGET}/sdkconfig.defaults)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-joylink)
