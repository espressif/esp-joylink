set(JOYLINK_SDK_DIR joylink_dev_sdk)
set(includes
    ${JOYLINK_SDK_DIR}/joylink/inc
    ${JOYLINK_SDK_DIR}/joylink/inc/softap
    ${JOYLINK_SDK_DIR}/pal/inc
    port/include)


# Edit following two lines to set component requirements (see docs)
set(components ${IDF_TARGET} nvs_flash app_update esp-tls tcpip_adapter joylink_extern)

set(srcs
    ${JOYLINK_SDK_DIR}/pal/src/joylink_flash.c
    ${JOYLINK_SDK_DIR}/pal/src/joylink_memory.c
    ${JOYLINK_SDK_DIR}/pal/src/joylink_socket.c
    ${JOYLINK_SDK_DIR}/pal/src/joylink_softap.c
    ${JOYLINK_SDK_DIR}/pal/src/joylink_stdio.c
    ${JOYLINK_SDK_DIR}/pal/src/joylink_string.c
    ${JOYLINK_SDK_DIR}/pal/src/joylink_thread.c
    ${JOYLINK_SDK_DIR}/pal/src/joylink_time.c)

if ("${IDF_TARGET}" STREQUAL "esp32")
list(APPEND components "bt")
endif()

if(CONFIG_JOYLINK_BLE_ENABLE)
list(APPEND includes ${JOYLINK_SDK_DIR}/ble_sdk/include)
list(APPEND srcs ${JOYLINK_SDK_DIR}/ble_sdk/adapter/joylink_adapter.c port/joylink_upgrade.c port/joylink_ble.c)
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${includes}"
                       REQUIRES "${components}")

if(CONFIG_JOYLINK_BLE_ENABLE)
message(STATUS "CHIP is ${IDF_TARGET},linking ble libary")
set(LIBBLE joylinkblesdk) 
add_library(${LIBBLE} STATIC IMPORTED)
set_property(TARGET ${LIBBLE} PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/${JOYLINK_SDK_DIR}/ble_sdk/target/lib/${IDF_TARGET}/lib${LIBBLE}.a)
target_link_libraries(${COMPONENT_LIB} INTERFACE ${LIBBLE})
set_property(TARGET ${LIBBLE} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${COMPONENT_LIB})
endif()

set(LIBS joylink)
add_library(${LIBS} STATIC IMPORTED)
set_property(TARGET ${LIBS} PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/${JOYLINK_SDK_DIR}/joylink/lib/${IDF_TARGET}/lib${LIBS}.a)
target_link_libraries(${COMPONENT_LIB} INTERFACE ${LIBS})
set_property(TARGET ${LIBS} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${COMPONENT_LIB})
target_compile_definitions(${COMPONENT_LIB} PUBLIC -D __LINUX_PAL__=1)

component_compile_options(-D__LINUX_UB2__ -D__LINUX__ -DJLdevice_aes_decrypt=device_aes_decrypt -D_GET_HOST_BY_NAME_ -Wno-error=unused-label -Wno-error=maybe-uninitialized -Wno-error=implicit-function-declaration -Wno-error=pointer-sign -Wno-error=char-subscripts -Wno-error=sizeof-pointer-memaccess -Wno-error=format -Wno-error=return-type -lpthread)