// WiFi Manager for HLK-LD6002B-3D Radar Sensor
// Handles WiFi station mode connection

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

// WiFi credentials are stored in a separate file (not in git)
// Copy wifi_credentials.h.example to wifi_credentials.h and update with your credentials
#include "wifi_credentials.h"

// WiFi connection timeout
#define WIFI_CONNECT_TIMEOUT_MS 10000

/**
 * Initialize and connect to WiFi
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_init(void);

/**
 * Check if WiFi is connected
 * @return true if connected, false otherwise
 */
bool wifi_manager_is_connected(void);

/**
 * Get the local IP address
 * @return IP address string (static buffer, do not free)
 */
const char* wifi_manager_get_ip(void);

/**
 * Disconnect and deinitialize WiFi
 */
void wifi_manager_deinit(void);

#endif // WIFI_MANAGER_H
