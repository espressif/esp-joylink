# (Automatically converted from project Makefile by convert_to_cmake.py.)

# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

execute_process(COMMAND git rev-parse --short HEAD
                OUTPUT_VARIABLE ESP_JOYLINK_VER
                OUTPUT_STRIP_TRAILING_WHITESPACE)
add_compile_options("-DESP_JOYLINK_COMMIT_ID=\"${ESP_JOYLINK_VER}\"")

set (EXTRA_COMPONENT_DIRS "..")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-joylink)
