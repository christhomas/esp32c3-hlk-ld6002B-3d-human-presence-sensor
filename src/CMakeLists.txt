# HLK-LD6002B-3D Radar Sensor Application

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# Required ESP-IDF components:
# - driver: GPIO, UART
# - esp_http_server: HTTP server and WebSocket
# - esp_wifi: WiFi station mode
# - nvs_flash: Non-volatile storage for WiFi
# - mdns: mDNS for easy network discovery
# - lwip: TCP/IP stack
# - json: cJSON library for WebSocket messages
idf_component_register(
    SRCS ${app_sources}
    REQUIRES
        driver
        esp_http_server
        esp_wifi
        nvs_flash
        mdns
        lwip
        json
)
