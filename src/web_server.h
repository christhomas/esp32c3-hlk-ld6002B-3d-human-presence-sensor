// Web Server for HLK-LD6002B-3D Radar Sensor
// Provides HTTP server and SSE (Server-Sent Events) streaming

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "hlk_ld6002.h"  // For hlk_target_t

// Server configuration
#define WEB_SERVER_PORT 80
#define WEB_SERVER_MAX_CONNECTIONS 4
#define MAX_TARGETS 10  // Maximum targets to track simultaneously

// Command queue configuration
#define CMD_QUEUE_SIZE 10

// Command types for radar control
typedef enum {
    RADAR_CMD_SET_SENSITIVITY,
    RADAR_CMD_SET_TRIGGER_SPEED,
    RADAR_CMD_CLEAR_INTERFERENCE_ZONE,
    RADAR_CMD_RESET_DETECTION_ZONE,
    RADAR_CMD_AUTO_GEN_INTERFERENCE_ZONE,
    RADAR_CMD_GET_ZONES
} radar_cmd_type_t;

// Command structure
typedef struct {
    radar_cmd_type_t type;
    uint8_t param;  // Parameter value (e.g., sensitivity level)
} radar_cmd_t;

// Zone data structure
typedef struct {
    float x_min;
    float x_max;
    float y_min;
    float y_max;
    float z_min;
    float z_max;
} zone_bounds_t;

/**
 * Initialize and start the web server
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t web_server_init(void);

/**
 * Stop and deinitialize the web server
 */
void web_server_deinit(void);

/**
 * Check if web server is running
 * @return true if running, false otherwise
 */
bool web_server_is_running(void);

/**
 * Broadcast target position data to all connected SSE clients
 * @param targets Array of target data
 * @param target_count Number of targets in array
 */
void web_server_send_targets(const hlk_target_t* targets, int32_t target_count);

/**
 * Broadcast point cloud data to all connected SSE clients
 * @param point_count Number of points
 * @param points Array of point data (x, y, z, speed, cluster)
 * @param max_points Maximum number of points to send
 */
void web_server_send_point_cloud(int32_t point_count, const float* points, int max_points);

/**
 * Broadcast presence status to all connected SSE clients
 * @param zone0 Zone 0 occupancy (0=empty, 1=occupied)
 * @param zone1 Zone 1 occupancy
 * @param zone2 Zone 2 occupancy
 * @param zone3 Zone 3 occupancy
 */
void web_server_send_presence(uint32_t zone0, uint32_t zone1,
                              uint32_t zone2, uint32_t zone3);

/**
 * Broadcast sensor configuration to all connected SSE clients
 * @param sensitivity Detection sensitivity (0=Low, 1=Medium, 2=High)
 * @param trigger_speed Trigger speed (0=Slow, 1=Medium, 2=Fast)
 * @param install_method Installation method (0=Top, 1=Side)
 */
void web_server_send_config(uint8_t sensitivity, uint8_t trigger_speed,
                            uint8_t install_method);

/**
 * Get number of connected SSE clients
 * @return Number of active SSE connections
 */
int web_server_get_client_count(void);

/**
 * Get command queue handle for radar control
 * @return Queue handle for sending commands to radar task
 */
QueueHandle_t web_server_get_cmd_queue(void);

/**
 * Broadcast zone data to all connected SSE clients
 * @param zones Array of 4 detection zones
 * @param is_interference true for interference zones, false for detection zones
 */
void web_server_send_zones(const zone_bounds_t* zones, bool is_interference);

#endif // WEB_SERVER_H
