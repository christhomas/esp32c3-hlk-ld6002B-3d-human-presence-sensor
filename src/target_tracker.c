// Target Tracker Module Implementation
// Handles target detection, tracking, and person presence logic

#include "target_tracker.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

static const char *TAG = "TargetTracker";

// ========== GLOBAL STATE ==========

static target_stats_t g_target_stats = {0};
static zone_stats_t g_zone_stats = {0};

// ========== TARGET TRACKING ==========

void target_tracker_init(void) {
    memset(&g_target_stats, 0, sizeof(g_target_stats));
    memset(&g_zone_stats, 0, sizeof(g_zone_stats));
    ESP_LOGI(TAG, "Target tracker initialized");
}

bool target_tracker_update(const hlk_target_t* targets, int32_t count) {
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    bool state_changed = false;
    
    // Detect state changes
    bool count_changed = (count != g_target_stats.last_target_count);
    
    if (count > 0) {
        // Get first target for movement tracking
        float x = targets[0].x;
        float y = targets[0].y;
        float z = targets[0].z;
        int32_t dop_idx = targets[0].velocity;
        
        // Calculate distance and movement
        float distance = hlk_calc_distance_3d(x, y, z);
        float dx = x - g_target_stats.last_x;
        float dy = y - g_target_stats.last_y;
        float dz = z - g_target_stats.last_z;
        float movement = sqrtf(dx * dx + dy * dy + dz * dz);
        
        // Check if target moved significantly
        bool moved = (movement > TRACKER_MOVEMENT_THRESHOLD_M) || count_changed;
        
        // Track new person detection
        if (!g_target_stats.person_detected) {
            g_target_stats.person_detected = true;
            g_target_stats.first_detection_time = now;
            state_changed = true;
            
            ESP_LOGI(TAG, "═══════════════════════════════════════");
            ESP_LOGI(TAG, "👋 PERSON DETECTED");
            ESP_LOGI(TAG, "═══════════════════════════════════════");
        }
        
        // Log target info on significant events
        if (moved || (now - g_target_stats.last_update_time > TRACKER_UPDATE_INTERVAL_MS)) {
            if (moved) {
                g_target_stats.stationary_count = 0;
            } else {
                g_target_stats.stationary_count++;
            }
            
            const char *motion_status = (dop_idx != 0) ? "🏃 Moving" :
                                       (movement > TRACKER_MOVEMENT_THRESHOLD_M) ? "🚶 Slow" : "🧍 Still";
            
            if (count == 1) {
                ESP_LOGI(TAG, "🎯 Target: pos=(%.2f, %.2f, %.2f)m dist=%.2fm %s",
                         x, y, z, distance, motion_status);
            } else {
                ESP_LOGI(TAG, "🎯 %ld Targets detected:", count);
                for (int i = 0; i < count && i < 3; i++) {
                    float t_dist = hlk_calc_distance_3d(targets[i].x, targets[i].y, targets[i].z);
                    const char *t_motion = (targets[i].velocity != 0) ? "🏃 Moving" : "🧍 Still";
                    ESP_LOGI(TAG, "   #%d: pos=(%.2f, %.2f, %.2f)m dist=%.2fm %s",
                             i + 1, targets[i].x, targets[i].y, targets[i].z, t_dist, t_motion);
                }
                if (count > 3) {
                    ESP_LOGI(TAG, "   (+%ld more targets)", count - 3);
                }
            }
            
            g_target_stats.last_update_time = now;
        }
        
        // Update position history
        g_target_stats.last_x = x;
        g_target_stats.last_y = y;
        g_target_stats.last_z = z;
        
    } else if (count_changed && g_target_stats.person_detected) {
        // Person left
        uint32_t detection_duration = (now - g_target_stats.first_detection_time) / 1000;
        state_changed = true;
        
        ESP_LOGI(TAG, "═══════════════════════════════════════");
        ESP_LOGI(TAG, "👋 PERSON LEFT (detected for %lu seconds)", detection_duration);
        ESP_LOGI(TAG, "═══════════════════════════════════════");
        
        g_target_stats.person_detected = false;
    }
    
    g_target_stats.last_target_count = count;
    return state_changed;
}

// ========== ZONE TRACKING ==========

bool zone_tracker_update(uint32_t zone0, uint32_t zone1, uint32_t zone2, uint32_t zone3) {
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Check if any zone changed
    bool changed = false;
    if (zone0 != g_zone_stats.zone_presence[0] ||
        zone1 != g_zone_stats.zone_presence[1] ||
        zone2 != g_zone_stats.zone_presence[2] ||
        zone3 != g_zone_stats.zone_presence[3]) {
        changed = true;
    }
    
    // Only log on change or periodically
    if (changed || (now - g_zone_stats.last_update_time > TRACKER_PRESENCE_LOG_INTERVAL_MS)) {
        int occupied_count = (zone0 ? 1 : 0) + (zone1 ? 1 : 0) +
                            (zone2 ? 1 : 0) + (zone3 ? 1 : 0);
        
        if (occupied_count > 0) {
            ESP_LOGI(TAG, "📍 Zones occupied: %d/4 [%s%s%s%s]",
                     occupied_count,
                     zone0 ? "0" : "-",
                     zone1 ? "1" : "-",
                     zone2 ? "2" : "-",
                     zone3 ? "3" : "-");
        }
        
        g_zone_stats.last_update_time = now;
    }
    
    // Update state
    g_zone_stats.zone_presence[0] = zone0;
    g_zone_stats.zone_presence[1] = zone1;
    g_zone_stats.zone_presence[2] = zone2;
    g_zone_stats.zone_presence[3] = zone3;
    g_zone_stats.changed = changed;
    
    return changed;
}

// ========== QUERY FUNCTIONS ==========

const target_stats_t* target_tracker_get_stats(void) {
    return &g_target_stats;
}

const zone_stats_t* zone_tracker_get_stats(void) {
    return &g_zone_stats;
}

bool target_tracker_person_present(void) {
    return g_target_stats.person_detected;
}

uint32_t target_tracker_get_duration(void) {
    if (!g_target_stats.person_detected) {
        return 0;
    }
    
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return (now - g_target_stats.first_detection_time) / 1000;
}
