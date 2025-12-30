// API Layer - Bridge between Web Interface and Sensor/Tracker Modules
// Handles data transformation, command queue processing, and state management

#ifndef API_H
#define API_H

#include <stdint.h>
#include <stdbool.h>
#include "hlk_ld6002.h"
#include "target_tracker.h"
#include "web_server.h"

#ifdef __cplusplus
extern "C" {
#endif

// ========== API INITIALIZATION ==========

/**
 * Initialize API layer
 * Sets up command queue and registers all necessary callbacks
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t api_init(void);

// ========== SENSOR CALLBACKS ==========
// These are called by the sensor module when data arrives

/**
 * Handle target detection data from sensor
 * Called by sensor when target position data arrives
 * @param targets Array of detected targets
 * @param count Number of targets
 */
void api_on_target_detected(const hlk_target_t* targets, int32_t count);

/**
 * Handle presence detection data from sensor
 * Called by sensor when zone presence data arrives
 * @param zone0 Zone 0 occupancy
 * @param zone1 Zone 1 occupancy
 * @param zone2 Zone 2 occupancy
 * @param zone3 Zone 3 occupancy
 */
void api_on_presence_detected(uint32_t zone0, uint32_t zone1, uint32_t zone2, uint32_t zone3);

/**
 * Handle zone configuration data from sensor
 * Called by sensor when zone boundary data arrives
 * @param zones Array of 4 zone boundaries
 * @param is_interference true for interference zones, false for detection zones
 */
void api_on_zones_received(const hlk_zone_t* zones, bool is_interference);

/**
 * Handle configuration data from sensor
 * Called by sensor for sensitivity, trigger speed, etc.
 * @param msg_type Message type from sensor
 * @param data Raw data bytes
 * @param len Data length
 */
void api_on_config_received(uint16_t msg_type, const uint8_t* data, uint16_t len);

// ========== COMMAND PROCESSING ==========

/**
 * Process web commands from command queue
 * Should be called regularly from sensor task
 * Converts web commands to sensor commands
 */
void api_process_web_commands(void);

// ========== STATISTICS ==========

/**
 * Log periodic statistics
 * Should be called from main loop
 */
void api_log_stats(void);

#ifdef __cplusplus
}
#endif

#endif // API_H
