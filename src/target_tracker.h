// Target Tracker Module
// Handles target detection, tracking, and person presence logic

#ifndef TARGET_TRACKER_H
#define TARGET_TRACKER_H

#include <stdint.h>
#include <stdbool.h>
#include "hlk_ld6002.h"

#ifdef __cplusplus
extern "C" {
#endif

// ========== CONFIGURATION ==========

#define TRACKER_MOVEMENT_THRESHOLD_M    0.05f   // 5cm movement threshold
#define TRACKER_UPDATE_INTERVAL_MS      5000    // Log updates every 5 seconds
#define TRACKER_PRESENCE_LOG_INTERVAL_MS 30000  // Log presence every 30 seconds

// ========== DATA STRUCTURES ==========

// Target tracking statistics
typedef struct {
    uint32_t first_detection_time;  // When person was first detected
    uint32_t last_update_time;      // Last time we logged target info
    uint32_t stationary_count;      // Consecutive stationary detections
    int32_t last_target_count;      // Previous target count
    float last_x, last_y, last_z;   // Last target position
    bool person_detected;           // Person currently present
} target_stats_t;

// Zone tracking statistics
typedef struct {
    uint32_t zone_presence[4];      // Current zone occupancy
    uint32_t last_update_time;      // Last time we logged presence
    bool changed;                   // Did zones change this update
} zone_stats_t;

// ========== API FUNCTIONS ==========

/**
 * Initialize target tracker
 */
void target_tracker_init(void);

/**
 * Process target detection data
 * @param targets Array of detected targets
 * @param count Number of targets detected
 * @return true if person state changed (detected/left)
 */
bool target_tracker_update(const hlk_target_t* targets, int32_t count);

/**
 * Process zone presence data
 * @param zone0 Zone 0 occupancy (0=empty, 1=occupied)
 * @param zone1 Zone 1 occupancy
 * @param zone2 Zone 2 occupancy
 * @param zone3 Zone 3 occupancy
 * @return true if zone state changed
 */
bool zone_tracker_update(uint32_t zone0, uint32_t zone1, uint32_t zone2, uint32_t zone3);

/**
 * Get current target statistics
 * @return Pointer to target stats structure
 */
const target_stats_t* target_tracker_get_stats(void);

/**
 * Get current zone statistics
 * @return Pointer to zone stats structure
 */
const zone_stats_t* zone_tracker_get_stats(void);

/**
 * Check if person is currently detected
 * @return true if person present
 */
bool target_tracker_person_present(void);

/**
 * Get detection duration in seconds
 * @return Seconds since first detection, or 0 if no person
 */
uint32_t target_tracker_get_duration(void);

#ifdef __cplusplus
}
#endif

#endif // TARGET_TRACKER_H
