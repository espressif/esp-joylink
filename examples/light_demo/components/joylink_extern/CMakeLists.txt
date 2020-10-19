set(COMPONENT_ADD_INCLUDEDIRS ".")

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES esp-joylink nvs_flash app_update esp-tls  tcpip_adapter)

file(GLOB_RECURSE srcs *.c)

set(COMPONENT_SRCS ${srcs})

register_component()
