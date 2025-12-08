set(IDF_VER_DIR "idf_v${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}")

# Fallback to v5.4 if the current version directory doesn't exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${IDF_VER_DIR}/include")
    message(WARNING "Directory ${IDF_VER_DIR}/include not found, falling back to idf_v5.4")
    set(IDF_VER_DIR "idf_v5.4")
endif()


if(NOT CONFIG_ESP_WIFI_ENABLED AND NOT CONFIG_ESP_HOST_WIFI_ENABLED)
    set(src_wifi_is_remote esp_wifi_remote.c ${IDF_VER_DIR}/esp_wifi_with_remote.c esp_wifi_remote_net.c)
endif()

if(CONFIG_ESP_WIFI_REMOTE_LIBRARY_EPPP)
    set(src_wifi_remote_eppp eppp/wifi_remote_rpc_client.cpp eppp/wifi_remote_rpc_server.cpp eppp/eppp_init.c)
else()
    set(src_wifi_remote_weak ${IDF_VER_DIR}/esp_wifi_remote_weak.c)
endif()

idf_component_register(INCLUDE_DIRS include ${IDF_VER_DIR}/include
                    SRCS ${src_wifi_remote_weak}
                         ${src_wifi_remote_eppp}
                         ${src_wifi_is_remote}
                    PRIV_INCLUDE_DIRS eppp
                    REQUIRES esp_event esp_netif
                    PRIV_REQUIRES esp_wifi esp-tls vfs)

idf_component_get_property(wifi esp_wifi COMPONENT_LIB)
target_link_libraries(${wifi} PUBLIC ${COMPONENT_LIB})
