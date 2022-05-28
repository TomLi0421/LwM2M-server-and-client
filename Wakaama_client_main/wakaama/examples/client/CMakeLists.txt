cmake_minimum_required(VERSION 3.13)

project(lwm2mclient C)

include(../../wakaama.cmake)

set(SOURCES
    lwm2mclient.c
    lwm2mclient.h
    object_access_control.c
    object_connectivity_moni.c
    object_connectivity_stat.c
    object_device.c
    object_firmware.c
    object_location.c
    object_security.c
    object_server.c
    object_device_extension.c
    object_water_meter.c
    system_api.c
)

# Client without DTLS support
add_executable(lwm2mclient ${SOURCES})
target_compile_definitions(lwm2mclient PRIVATE LWM2M_CLIENT_MODE LWM2M_BOOTSTRAP LWM2M_SUPPORT_SENML_JSON LWM2M_SUPPORT_TLV LWM2M_SUPPORT_JSON LWM2M_WITH_LOGS)
target_sources_wakaama(lwm2mclient)
target_sources_shared(lwm2mclient)

if(FALSE) # not using DTLS for now

# Client with DTLS support provided by tinydtls
add_executable(lwm2mclient_tinydtls ${SOURCES})
set_target_properties(lwm2mclient_tinydtls PROPERTIES DTLS "tinydtls")
target_compile_definitions(lwm2mclient_tinydtls PRIVATE LWM2M_CLIENT_MODE LWM2M_BOOTSTRAP LWM2M_SUPPORT_SENML_JSON)
target_sources_wakaama(lwm2mclient_tinydtls)
target_sources_shared(lwm2mclient_tinydtls)

endif()