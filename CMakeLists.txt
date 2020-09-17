set(COMPONENT_ADD_INCLUDEDIRS
    joylink_sdk/extern
    joylink_sdk/joylink/inc
    joylink_sdk/joylink/inc/json
    joylink_sdk/joylink/inc/softap
    joylink_sdk/pal/inc
    port/include
    port/include/net)

# Edit following two lines to set component requirements (see docs)

set(COMPONENT_PRIV_REQUIRES nvs_flash app_update esp-tls  tcpip_adapter)

set(COMPONENT_SRCS
    joylink_sdk/extern/joylink_extern.c
    joylink_sdk/extern/joylink_extern_json.c
    joylink_sdk/extern/joylink_extern_ota.c
    joylink_sdk/extern/joylink_extern_sub_dev.c
    joylink_sdk/extern/joylink_extern_user.c
    joylink_sdk/pal/src/joylink_memory.c
    joylink_sdk/pal/src/joylink_socket.c
    joylink_sdk/pal/src/joylink_softap.c
    joylink_sdk/pal/src/joylink_stdio.c
    joylink_sdk/pal/src/joylink_string.c
    joylink_sdk/pal/src/joylink_thread.c
    joylink_sdk/pal/src/joylink_time.c)

set(COMPONENT_REQUIRES )
register_component()

component_compile_options(-D__LINUX_UB2__ -D__LINUX__ -DJLdevice_aes_decrypt=device_aes_decrypt -D_GET_HOST_BY_NAME_ -Wno-error=unused-label -Wno-error=maybe-uninitialized -Wno-error=implicit-function-declaration -Wno-error=pointer-sign -Wno-error=char-subscripts -Wno-error=sizeof-pointer-memaccess -Wno-error=format -Wno-error=return-type -lpthread)

if(CONFIG_IDF_TARGET_ESP8266)
set(LIBS joylink) 
add_library(${LIBS} STATIC IMPORTED)
set_property(TARGET ${LIBS} PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/joylink_sdk/joylink/lib/xtensa_esp8266/lib${LIBS}.a)

target_link_libraries(${COMPONENT_LIB} INTERFACE ${LIBS})
set_property(TARGET ${LIBS} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${COMPONENT_LIB})
target_compile_definitions(${COMPONENT_LIB} PUBLIC -D __ESP_PAL__=1)
endif()
