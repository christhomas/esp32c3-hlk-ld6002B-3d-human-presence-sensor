// Web Server for HLK-LD6002B-3D Radar Sensor
// Provides HTTP server and SSE (Server-Sent Events) streaming

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

// Server configuration
#define WEB_SERVER_PORT 80
#define WEB_SERVER_MAX_CONNECTIONS 4
#define MAX_TARGETS 10  // Maximum targets to track simultaneously

// Target data structure
typedef struct {
    float x;            // X coordinate (meters)
    float y;            // Y coordinate (meters)
    float z;            // Z coordinate (meters)
    int32_t velocity;   // Doppler velocity index
    int32_t cluster_id; // Cluster ID
} radar_target_t;

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
void web_server_send_targets(const radar_target_t* targets, int32_t target_count);

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

#endif // WEB_SERVER_H
