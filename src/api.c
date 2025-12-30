// API Layer Implementation
// Bridge between Web Interface and Sensor/Tracker Modules

#include "api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "API";

// ========== INITIALIZATION ==========

esp_err_t api_init(void) {
    ESP_LOGI(TAG, "API layer initialized");
    return ESP_OK;
}

// ========== SENSOR CALLBACKS ==========

void api_on_target_detected(const hlk_target_t* targets, int32_t count) {
    // Update target tracker (handles logging and state management)
    target_tracker_update(targets, count);
    
    // Broadcast to web clients
    web_server_send_targets(targets, count);
}

void api_on_presence_detected(uint32_t zone0, uint32_t zone1, uint32_t zone2, uint32_t zone3) {
    // Update zone tracker (handles logging and state management)
    zone_tracker_update(zone0, zone1, zone2, zone3);
    
    // Broadcast to web clients
    web_server_send_presence(zone0, zone1, zone2, zone3);
}

void api_on_zones_received(const hlk_zone_t* zones, bool is_interference) {
    // Convert sensor format to web format
    zone_bounds_t web_zones[4];
    for (int i = 0; i < 4; i++) {
        web_zones[i].x_min = zones[i].x_min;
        web_zones[i].x_max = zones[i].x_max;
        web_zones[i].y_min = zones[i].y_min;
        web_zones[i].y_max = zones[i].y_max;
        web_zones[i].z_min = zones[i].z_min;
        web_zones[i].z_max = zones[i].z_max;
    }
    
    // Broadcast to web clients
    web_server_send_zones(web_zones, is_interference);
}

void api_on_config_received(uint16_t msg_type, const uint8_t* data, uint16_t len) {
    if (len < 1) return;
    
    switch (msg_type) {
        case MSG_IND_HUMAN_DETECTION_3D_PWM_DELAY:
            if (len >= 4) {
                uint32_t delay = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
                ESP_LOGI(TAG, "⏱️  Hold Delay: %lu seconds", delay);
            }
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_DETECT_SENSITIVITY:
            ESP_LOGI(TAG, "🎚️  Sensitivity: %s (%d)",
                     hlk_sensitivity_to_string(data[0]), data[0]);
            web_server_send_config(data[0], 255, 255);  // 255 = unchanged
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_DETECT_TRIGGER:
            ESP_LOGI(TAG, "⚡ Trigger Speed: %s (%d)",
                     hlk_trigger_speed_to_string(data[0]), data[0]);
            web_server_send_config(255, data[0], 255);  // 255 = unchanged
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_Z_RANGE:
            if (len >= 8) {
                // Read floats from little-endian
                union { uint8_t b[4]; float f; } z_min, z_max;
                z_min.b[0] = data[0]; z_min.b[1] = data[1]; z_min.b[2] = data[2]; z_min.b[3] = data[3];
                z_max.b[0] = data[4]; z_max.b[1] = data[5]; z_max.b[2] = data[6]; z_max.b[3] = data[7];
                ESP_LOGI(TAG, "📐 Z-Axis Range: [%.2f to %.2f] meters", z_min.f, z_max.f);
            }
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_INSTALL_SITE:
            ESP_LOGI(TAG, "🔧 Installation: %s (%d)",
                     hlk_install_method_to_string(data[0]), data[0]);
            web_server_send_config(255, 255, data[0]);  // 255 = unchanged
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_LOW_POWER_MODE:
            ESP_LOGI(TAG, "💤 Low Power Mode: %s", data[0] ? "Enabled" : "Disabled");
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_LOW_POWER_TIME:
            if (len >= 4) {
                uint32_t time = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
                ESP_LOGI(TAG, "💤 Low Power Sleep Time: %lu ms", time);
            }
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_MODE:
            {
                const char *modes[] = {"Low Power", "Normal"};
                uint8_t mode = data[0];
                ESP_LOGI(TAG, "⚙️  Working Mode: %s (%d)", 
                         mode < 2 ? modes[mode] : "Unknown", mode);
            }
            break;
            
        default:
            ESP_LOGD(TAG, "Config message type: 0x%04X (len=%d)", msg_type, len);
            break;
    }
}

// ========== COMMAND PROCESSING ==========

void api_process_web_commands(void) {
    QueueHandle_t cmd_queue = web_server_get_cmd_queue();
    if (!cmd_queue) return;
    
    radar_cmd_t cmd;
    if (xQueueReceive(cmd_queue, &cmd, 0) == pdTRUE) {
        ESP_LOGI(TAG, "Processing web command: type=%d param=%d", cmd.type, cmd.param);
        
        switch (cmd.type) {
            case RADAR_CMD_SET_SENSITIVITY:
                if (cmd.param == 0) {
                    hlk_ld6002_send_command(CMD_SET_SENSITIVITY_LOW);
                } else if (cmd.param == 1) {
                    hlk_ld6002_send_command(CMD_SET_SENSITIVITY_MEDIUM);
                } else if (cmd.param == 2) {
                    hlk_ld6002_send_command(CMD_SET_SENSITIVITY_HIGH);
                }
                vTaskDelay(pdMS_TO_TICKS(200));
                hlk_ld6002_send_command(CMD_GET_SENSITIVITY);
                break;
                
            case RADAR_CMD_SET_TRIGGER_SPEED:
                if (cmd.param == 0) {
                    hlk_ld6002_send_command(CMD_SET_TRIGGER_SPEED_SLOW);
                } else if (cmd.param == 1) {
                    hlk_ld6002_send_command(CMD_SET_TRIGGER_SPEED_MEDIUM);
                } else if (cmd.param == 2) {
                    hlk_ld6002_send_command(CMD_SET_TRIGGER_SPEED_FAST);
                }
                vTaskDelay(pdMS_TO_TICKS(200));
                hlk_ld6002_send_command(CMD_GET_TRIGGER_SPEED);
                break;
                
            case RADAR_CMD_CLEAR_INTERFERENCE_ZONE:
                hlk_ld6002_send_command(CMD_CLEAR_INTERFERENCE_ZONE);
                vTaskDelay(pdMS_TO_TICKS(200));
                hlk_ld6002_send_command(CMD_GET_ZONES);
                break;
                
            case RADAR_CMD_RESET_DETECTION_ZONE:
                hlk_ld6002_send_command(CMD_RESET_DETECTION_ZONE);
                vTaskDelay(pdMS_TO_TICKS(200));
                hlk_ld6002_send_command(CMD_GET_ZONES);
                break;
                
            case RADAR_CMD_AUTO_GEN_INTERFERENCE_ZONE:
                hlk_ld6002_send_command(CMD_AUTO_GEN_INTERFERENCE_ZONE);
                ESP_LOGI(TAG, "Auto-generating interference zones (30-60s)...");
                break;
                
            case RADAR_CMD_GET_ZONES:
                hlk_ld6002_send_command(CMD_GET_ZONES);
                break;
                
            default:
                ESP_LOGW(TAG, "Unknown command type: %d", cmd.type);
                break;
        }
    }
}

// ========== STATISTICS ==========

void api_log_stats(void) {
    static uint32_t last_stats_time = 0;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (now - last_stats_time > 60000) {  // Every 60 seconds
        // Sensor statistics
        uint32_t total, target, presence;
        hlk_ld6002_get_stats(&total, &target, &presence);
        ESP_LOGI(TAG, "📊 Sensor: %lu frames (%lu target, %lu presence)",
                 total, target, presence);
        
        // Tracker statistics
        if (target_tracker_person_present()) {
            uint32_t duration = target_tracker_get_duration();
            ESP_LOGI(TAG, "📊 Person present for %lu seconds", duration);
        }
        
        last_stats_time = now;
    }
}
