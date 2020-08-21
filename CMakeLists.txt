set(COMPONENT_ADD_INCLUDEDIRS
    joylink_sdk/extern
    joylink_sdk/joylink/inc
    joylink_sdk/joylink/inc/json
    /home/xiewenxiang/esp/esp-joylink-refactor/joylink_sdk/joylink/inc/softap/joylink_softap_start.h
    joylink_sdk/pal/inc
    port/include
    port/include/net)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES nvs_flash app_update)
set(COMPONENT_PRIV_REQUIRES )

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

register_component()
component_compile_options(-D__LINUX_UB2__ -D__LINUX__ -DJLdevice_aes_decrypt=device_aes_decrypt -D_GET_HOST_BY_NAME_ -Wno-error=unused-label -Wno-error=maybe-uninitialized -Wno-error=implicit-function-declaration -Wno-error=pointer-sign -Wno-error=char-subscripts -Wno-error=sizeof-pointer-memaccess -Wno-error=format)

if(CONFIG_IDF_TARGET_ESP8266)
target_link_libraries(esp-joylink "-L${CMAKE_CURRENT_LIST_DIR}/joylink_sdk/joylink/lib/xtensa_esp8266")
target_link_libraries(esp-joylink joylink)
endif()