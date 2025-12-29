// ESP-IDF example for HLK-LD6002B-3D presence detection radar sensor
//
// Based on TinyFrame protocol specification V1.2
// UART Configuration: 115200 baud, 8N1, no flow control
//
// IMPORTANT WIRING:
// HLK-LD6002 Pin 7 (TX0) → ESP32 GPIO20 (D7/RX)
// HLK-LD6002 Pin 8 (RX0) → ESP32 GPIO21 (D6/TX)
// HLK-LD6002 Pin 3 (P19) → GND (BOOT1 must be LOW!)
// HLK-LD6002 Pin 1 (3V3) → 3.3V (requires ≥1A supply!)
// HLK-LD6002 Pin 2 (GND) → GND
//

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mdns.h"
#include <string.h>
#include <math.h>

// Web interface modules
#include "wifi_manager.h"
#include "web_server.h"

// Feature flags
#define ENABLE_WEB_INTERFACE 1  // Set to 0 to disable WiFi/web for debugging

// UART Configuration
#ifndef HLK_LD6002_UART_PORT
#define HLK_LD6002_UART_PORT UART_NUM_1
#endif

#ifndef HLK_LD6002_TX_PIN
#define HLK_LD6002_TX_PIN GPIO_NUM_21  // D6 on XIAO → Pin 8 (RX0) on sensor
#endif

#ifndef HLK_LD6002_RX_PIN
#define HLK_LD6002_RX_PIN GPIO_NUM_20  // D7 on XIAO → Pin 7 (TX0) on sensor
#endif

// HLK-LD6002B-3D default baud rate: 115200
#ifndef HLK_LD6002_BAUDRATE
#define HLK_LD6002_BAUDRATE 115200
#endif

#define UART_BUF_SIZE 2048
#define FRAME_BUF_SIZE 1152  // Max: 1 + 2 + 2 + 2 + 1 + 1024 + 1 = 1033 bytes

static const char *TAG = "LD6002-3D";

// TinyFrame Protocol Constants
#define TF_SOF 0x01  // Start of Frame

// Command Message Types (Host → Radar)
#define MSG_CFG_HUMAN_DETECTION_3D                      0x0201
#define MSG_CFG_HUMAN_DETECTION_3D_AREA                 0x0202
#define MSG_CFG_HUMAN_DETECTION_3D_PWM_DELAY            0x0203
#define MSG_CFG_HUMAN_DETECTION_3D_Z                    0x0204
#define MSG_CFG_HUMAN_DETECTION_3D_LOW_POWER_MODE_TIME  0x0205

// Report Message Types (Radar → Host)
#define MSG_IND_HUMAN_DETECTION_3D_TGT_RES              0x0A04  // Target Position
#define MSG_IND_3D_CLOUD_RES                            0x0A08  // Point Cloud
#define MSG_IND_HUMAN_DETECTION_3D_RES                  0x0A0A  // Presence Status
#define MSG_IND_HUMAN_DETECTION_3D_INTERFERENCE_ZONES   0x0A0B  // Interference Zones
#define MSG_IND_HUMAN_DETECTION_3D_DETECTION_ZONES      0x0A0C  // Detection Zones
#define MSG_IND_HUMAN_DETECTION_3D_PWM_DELAY            0x0A0D  // Hold Delay Time
#define MSG_IND_HUMAN_DETECTION_3D_DETECT_SENSITIVITY   0x0A0E  // Detection Sensitivity
#define MSG_IND_HUMAN_DETECTION_3D_DETECT_TRIGGER       0x0A0F  // Trigger Speed
#define MSG_IND_HUMAN_DETECTION_3D_Z_RANGE              0x0A10  // Z-Axis Range
#define MSG_IND_HUMAN_DETECTION_3D_INSTALL_SITE         0x0A11  // Installation Method
#define MSG_IND_HUMAN_DETECTION_3D_LOW_POWER_MODE       0x0A12  // Low Power Mode Status
#define MSG_IND_HUMAN_DETECTION_3D_LOW_POWER_TIME       0x0A13  // Low Power Sleep Time
#define MSG_IND_HUMAN_DETECTION_3D_MODE                 0x0A14  // Working Mode

// Control Commands (for MSG_CFG_HUMAN_DETECTION_3D)
#define CMD_AUTO_GEN_INTERFERENCE_ZONE      0x01
#define CMD_GET_ZONES                       0x02
#define CMD_CLEAR_INTERFERENCE_ZONE         0x03
#define CMD_RESET_DETECTION_ZONE            0x04
#define CMD_GET_HOLD_DELAY                  0x05
#define CMD_ENABLE_POINT_CLOUD              0x06
#define CMD_DISABLE_POINT_CLOUD             0x07
#define CMD_ENABLE_TARGET_DISPLAY           0x08
#define CMD_DISABLE_TARGET_DISPLAY          0x09
#define CMD_SET_SENSITIVITY_LOW             0x0A
#define CMD_SET_SENSITIVITY_MEDIUM          0x0B
#define CMD_SET_SENSITIVITY_HIGH            0x0C
#define CMD_GET_SENSITIVITY                 0x0D
#define CMD_SET_TRIGGER_SPEED_SLOW          0x0E
#define CMD_SET_TRIGGER_SPEED_MEDIUM        0x0F
#define CMD_SET_TRIGGER_SPEED_FAST          0x10
#define CMD_GET_TRIGGER_SPEED               0x11
#define CMD_GET_Z_AXIS_RANGE                0x12
#define CMD_SET_INSTALL_TOP_MOUNTED         0x13
#define CMD_SET_INSTALL_SIDE_MOUNTED        0x14
#define CMD_GET_INSTALL_METHOD              0x15
#define CMD_ENABLE_LOW_POWER_MODE           0x16
#define CMD_DISABLE_LOW_POWER_MODE          0x17
#define CMD_GET_LOW_POWER_MODE              0x18
#define CMD_GET_LOW_POWER_SLEEP_TIME        0x19
#define CMD_RESET_NO_PERSON_STATE           0x1A

// State tracking for smart logging
typedef struct {
    // Presence state
    uint32_t zone_presence[4];
    bool zone_changed;
    
    // Target tracking
    int32_t last_target_count;
    float last_x, last_y, last_z;
    uint32_t last_target_time;
    uint32_t stationary_count;
    
    // Statistics
    uint32_t total_frames;
    uint32_t presence_frames;
    uint32_t target_frames;
    uint32_t first_detection_time;
    bool person_detected;
} radar_state_t;

static radar_state_t g_radar_state = {0};

// TinyFrame structure
typedef struct {
    uint8_t sof;           // Start of Frame (0x01)
    uint16_t id;           // Frame ID (big-endian)
    uint16_t len;          // Data length (big-endian)
    uint16_t type;         // Message type (big-endian)
    uint8_t head_cksum;    // Header checksum
    uint8_t *data;         // Pointer to data (little-endian)
    uint8_t data_cksum;    // Data checksum
} tinyframe_t;

// Calculate checksum using TF_CKSUM_XOR (XOR all bytes, then invert)
static uint8_t calc_checksum(const uint8_t *data, uint16_t len) {
    uint8_t result = 0;
    for (uint16_t i = 0; i < len; i++) {
        result ^= data[i];
    }
    return ~result;
}

// Read uint16 from big-endian bytes
static uint16_t read_uint16_be(const uint8_t *bytes) {
    return ((uint16_t)bytes[0] << 8) | bytes[1];
}

// Read int32 from little-endian bytes
static int32_t read_int32_le(const uint8_t *bytes) {
    return (int32_t)bytes[0] | ((int32_t)bytes[1] << 8) |
           ((int32_t)bytes[2] << 16) | ((int32_t)bytes[3] << 24);
}

// Read uint32 from little-endian bytes
static uint32_t read_uint32_le(const uint8_t *bytes) {
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

// Read float from little-endian bytes
static float read_float_le(const uint8_t *bytes) {
    union {
        uint8_t b[4];
        float f;
    } convert;
    convert.b[0] = bytes[0];
    convert.b[1] = bytes[1];
    convert.b[2] = bytes[2];
    convert.b[3] = bytes[3];
    return convert.f;
}

// Write uint16 to big-endian bytes
static void write_uint16_be(uint8_t *bytes, uint16_t value) {
    bytes[0] = (value >> 8) & 0xFF;
    bytes[1] = value & 0xFF;
}

// Write int32 to little-endian bytes
static void write_int32_le(uint8_t *bytes, int32_t value) {
    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = (value >> 16) & 0xFF;
    bytes[3] = (value >> 24) & 0xFF;
}

// Calculate 3D distance
static float calc_distance_3d(float x, float y, float z) {
    return sqrtf(x * x + y * y + z * z);
}

// Send TinyFrame command
static void send_command(uint16_t msg_type, const uint8_t *data, uint16_t data_len) {
    uint8_t frame[256];
    uint8_t pos = 0;
    
    // Build frame header
    frame[pos++] = TF_SOF;
    write_uint16_be(&frame[pos], 0x0000);  // Frame ID = 0
    pos += 2;
    write_uint16_be(&frame[pos], data_len);
    pos += 2;
    write_uint16_be(&frame[pos], msg_type);
    pos += 2;
    
    // Calculate and add header checksum
    frame[pos] = calc_checksum(frame, 7);
    pos++;
    
    // Add data
    if (data_len > 0 && data != NULL) {
        memcpy(&frame[pos], data, data_len);
        pos += data_len;
        
        // Calculate and add data checksum
        frame[pos] = calc_checksum(data, data_len);
        pos++;
    }
    
    // Send frame
    uart_write_bytes(HLK_LD6002_UART_PORT, frame, pos);
    
    ESP_LOGI(TAG, "📤 Sent command 0x%04X, len=%d", msg_type, data_len);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, frame, pos, ESP_LOG_DEBUG);
}

// Send control command with int32 parameter
static void send_control_command(uint32_t cmd) {
    uint8_t data[4];
    write_int32_le(data, cmd);
    send_command(MSG_CFG_HUMAN_DETECTION_3D, data, 4);
}

// Parse target position message (0x0A04)
static void parse_target_position(const uint8_t *data, uint16_t len) {
    if (len < 4) return;
    
    int32_t target_num = read_int32_le(&data[0]);
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    g_radar_state.target_frames++;
    
    // Detect state changes
    bool target_count_changed = (target_num != g_radar_state.last_target_count);
    
    if (target_num > 0) {
        // Validate data length: 4 bytes header + (20 bytes per target)
        uint16_t expected_len = 4 + (target_num * 20);
        if (len < expected_len) {
            ESP_LOGW(TAG, "Incomplete target data: got %d bytes, expected %d", len, expected_len);
            return;
        }
        
        // Limit to MAX_TARGETS
        int32_t num_to_process = target_num > MAX_TARGETS ? MAX_TARGETS : target_num;
        
        // Allocate target array
        radar_target_t targets[MAX_TARGETS];
        
        // Process all targets
        for (int i = 0; i < num_to_process; i++) {
            uint16_t offset = 4 + (i * 20);
            targets[i].x = read_float_le(&data[offset]);
            targets[i].y = read_float_le(&data[offset + 4]);
            targets[i].z = read_float_le(&data[offset + 8]);
            targets[i].velocity = read_int32_le(&data[offset + 12]);
            targets[i].cluster_id = read_int32_le(&data[offset + 16]);
        }
        
        // Track first target for movement detection
        float x = targets[0].x;
        float y = targets[0].y;
        float z = targets[0].z;
        int32_t dop_idx = targets[0].velocity;
        
        // Calculate distance and movement
        float distance = calc_distance_3d(x, y, z);
        float dx = x - g_radar_state.last_x;
        float dy = y - g_radar_state.last_y;
        float dz = z - g_radar_state.last_z;
        float movement = sqrtf(dx * dx + dy * dy + dz * dz);
        
        // Check if target moved significantly (> 5cm)
        bool moved = (movement > 0.05f) || target_count_changed;
        
        // Track new person detection
        if (!g_radar_state.person_detected) {
            g_radar_state.person_detected = true;
            g_radar_state.first_detection_time = now;
            ESP_LOGI(TAG, "═══════════════════════════════════════");
            ESP_LOGI(TAG, "👋 PERSON DETECTED");
            ESP_LOGI(TAG, "═══════════════════════════════════════");
        }
        
        // Log target info on significant events
        if (moved || (now - g_radar_state.last_target_time > 5000)) {
            if (moved) {
                g_radar_state.stationary_count = 0;
            } else {
                g_radar_state.stationary_count++;
            }
            
            const char *motion_status = (dop_idx != 0) ? "🏃 Moving" :
                                       (movement > 0.05f) ? "🚶 Slow" : "🧍 Still";
            
            if (num_to_process == 1) {
                ESP_LOGI(TAG, "🎯 Target: pos=(%.2f, %.2f, %.2f)m dist=%.2fm %s",
                         x, y, z, distance, motion_status);
            } else {
                ESP_LOGI(TAG, "🎯 %ld Targets detected:", target_num);
                for (int i = 0; i < num_to_process && i < 3; i++) {
                    float t_dist = calc_distance_3d(targets[i].x, targets[i].y, targets[i].z);
                    const char *t_motion = (targets[i].velocity != 0) ? "🏃 Moving" : "🧍 Still";
                    ESP_LOGI(TAG, "   #%d: pos=(%.2f, %.2f, %.2f)m dist=%.2fm %s",
                             i + 1, targets[i].x, targets[i].y, targets[i].z, t_dist, t_motion);
                }
                if (num_to_process > 3) {
                    ESP_LOGI(TAG, "   (+%ld more targets)", num_to_process - 3);
                }
            }
            
            g_radar_state.last_target_time = now;
        }
        
        // Update state
        g_radar_state.last_x = x;
        g_radar_state.last_y = y;
        g_radar_state.last_z = z;
        
        // Stream to web clients - send all targets
        #if ENABLE_WEB_INTERFACE
        web_server_send_targets(targets, num_to_process);
        #endif
        
    } else if (target_count_changed && g_radar_state.person_detected) {
        // Person left
        uint32_t detection_duration = (now - g_radar_state.first_detection_time) / 1000;
        ESP_LOGI(TAG, "═══════════════════════════════════════");
        ESP_LOGI(TAG, "👋 PERSON LEFT (detected for %lu seconds)", detection_duration);
        ESP_LOGI(TAG, "═══════════════════════════════════════");
        g_radar_state.person_detected = false;
        
        // Clear targets on web
        #if ENABLE_WEB_INTERFACE
        web_server_send_targets(NULL, 0);
        #endif
    }
    
    g_radar_state.last_target_count = target_num;
}

// Parse point cloud message (0x0A08)
static void parse_point_cloud(const uint8_t *data, uint16_t len) {
    if (len < 4) return;
    
    int32_t point_num = read_int32_le(&data[0]);
    
    // Only log occasionally to avoid flooding
    static uint32_t last_cloud_log = 0;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (point_num > 0 && (now - last_cloud_log > 10000)) {  // Every 10 seconds
        ESP_LOGI(TAG, "☁️  Point Cloud: %ld points", point_num);
        
        // Show first few points
        int max_display = 3;
        if (len >= 4 + (point_num * 20)) {
            for (int i = 0; i < point_num && i < max_display; i++) {
                uint16_t offset = 4 + (i * 20);
                int32_t cluster_idx = read_int32_le(&data[offset]);
                float x = read_float_le(&data[offset + 4]);
                float y = read_float_le(&data[offset + 8]);
                float z = read_float_le(&data[offset + 12]);
                float speed = read_float_le(&data[offset + 16]);
                
                ESP_LOGI(TAG, "  Point %d: (%.2f, %.2f, %.2f)m speed=%.2f m/s",
                         i + 1, x, y, z, speed);
            }
            if (point_num > max_display) {
                ESP_LOGI(TAG, "  ... +%ld more points", point_num - max_display);
            }
        }
        last_cloud_log = now;
    }
}

// Parse presence status message (0x0A0A)
static void parse_presence_status(const uint8_t *data, uint16_t len) {
    if (len < 16) return;
    
    uint32_t zone0 = read_uint32_le(&data[0]);
    uint32_t zone1 = read_uint32_le(&data[4]);
    uint32_t zone2 = read_uint32_le(&data[8]);
    uint32_t zone3 = read_uint32_le(&data[12]);
    
    g_radar_state.presence_frames++;
    
    // Check if any zone changed
    bool changed = false;
    if (zone0 != g_radar_state.zone_presence[0] ||
        zone1 != g_radar_state.zone_presence[1] ||
        zone2 != g_radar_state.zone_presence[2] ||
        zone3 != g_radar_state.zone_presence[3]) {
        changed = true;
    }
    
    // Only log on change or periodically
    static uint32_t last_presence_log = 0;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (changed || (now - last_presence_log > 30000)) {  // Every 30 seconds
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
        
        last_presence_log = now;
    }
    
    // Update state
    g_radar_state.zone_presence[0] = zone0;
    g_radar_state.zone_presence[1] = zone1;
    g_radar_state.zone_presence[2] = zone2;
    g_radar_state.zone_presence[3] = zone3;
    g_radar_state.zone_changed = changed;
    
    // Stream to web clients (send on every update)
    #if ENABLE_WEB_INTERFACE
    web_server_send_presence(zone0, zone1, zone2, zone3);
    #endif
}

// Parse zone coordinates (0x0A0B or 0x0A0C)
static void parse_zones(const uint8_t *data, uint16_t len, const char *zone_type) {
    if (len < 96) return;  // 4 zones * 6 floats * 4 bytes = 96 bytes
    
    ESP_LOGI(TAG, "📍 %s Zones:", zone_type);
    for (int i = 0; i < 4; i++) {
        uint16_t offset = i * 24;
        float x_min = read_float_le(&data[offset]);
        float x_max = read_float_le(&data[offset + 4]);
        float y_min = read_float_le(&data[offset + 8]);
        float y_max = read_float_le(&data[offset + 12]);
        float z_min = read_float_le(&data[offset + 16]);
        float z_max = read_float_le(&data[offset + 20]);
        
        ESP_LOGI(TAG, "  Zone %d: X[%.1f to %.1f] Y[%.1f to %.1f] Z[%.1f to %.1f]m",
                 i, x_min, x_max, y_min, y_max, z_min, z_max);
    }
}

// Parse and handle a complete TinyFrame
static void parse_tinyframe(const uint8_t *frame, uint16_t frame_len) {
    if (frame_len < 9) {  // Minimum frame: 1+2+2+2+1+0+1
        ESP_LOGW(TAG, "Frame too short: %d bytes", frame_len);
        return;
    }
    
    // Parse header (big-endian)
    uint8_t sof = frame[0];
    uint16_t frame_id = read_uint16_be(&frame[1]);
    uint16_t data_len = read_uint16_be(&frame[3]);
    uint16_t msg_type = read_uint16_be(&frame[5]);
    uint8_t head_cksum_rx = frame[7];
    
    // Verify SOF
    if (sof != TF_SOF) {
        ESP_LOGW(TAG, "Invalid SOF: 0x%02X", sof);
        return;
    }
    
    // Verify header checksum (XOR bytes 0-6, then invert)
    uint8_t head_cksum_calc = calc_checksum(frame, 7);
    if (head_cksum_calc != head_cksum_rx) {
        ESP_LOGW(TAG, "Header checksum failed: calc=0x%02X rx=0x%02X", head_cksum_calc, head_cksum_rx);
        return;
    }
    
    // Verify frame length
    uint16_t expected_len = 8 + data_len + 1;
    if (frame_len != expected_len) {
        ESP_LOGW(TAG, "Frame length mismatch: got=%d expected=%d", frame_len, expected_len);
        return;
    }
    
    // Get data pointer
    const uint8_t *data = &frame[8];
    uint8_t data_cksum_rx = frame[8 + data_len];
    
    // Verify data checksum
    if (data_len > 0) {
        uint8_t data_cksum_calc = calc_checksum(data, data_len);
        if (data_cksum_calc != data_cksum_rx) {
            ESP_LOGW(TAG, "Data checksum failed: calc=0x%02X rx=0x%02X", data_cksum_calc, data_cksum_rx);
            return;
        }
    }
    
    g_radar_state.total_frames++;
    ESP_LOGD(TAG, "Frame #%lu: ID=0x%04X Type=0x%04X Len=%d", 
             g_radar_state.total_frames, frame_id, msg_type, data_len);
    
    // Process message based on type
    switch (msg_type) {
        case MSG_IND_HUMAN_DETECTION_3D_TGT_RES:
            parse_target_position(data, data_len);
            break;
            
        case MSG_IND_3D_CLOUD_RES:
            parse_point_cloud(data, data_len);
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_RES:
            parse_presence_status(data, data_len);
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_INTERFERENCE_ZONES:
            parse_zones(data, data_len, "Interference");
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_DETECTION_ZONES:
            parse_zones(data, data_len, "Detection");
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_PWM_DELAY:
            if (data_len >= 4) {
                uint32_t delay = read_uint32_le(data);
                ESP_LOGI(TAG, "⏱️  Hold Delay: %lu seconds", delay);
            }
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_DETECT_SENSITIVITY:
            if (data_len >= 1) {
                const char *levels[] = {"Low", "Medium", "High"};
                uint8_t level = data[0];
                ESP_LOGI(TAG, "🎚️  Sensitivity: %s (%d)", 
                         level < 3 ? levels[level] : "Unknown", level);
            }
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_DETECT_TRIGGER:
            if (data_len >= 1) {
                const char *speeds[] = {"Slow", "Medium", "Fast"};
                uint8_t speed = data[0];
                ESP_LOGI(TAG, "⚡ Trigger Speed: %s (%d)", 
                         speed < 3 ? speeds[speed] : "Unknown", speed);
            }
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_Z_RANGE:
            if (data_len >= 8) {
                float z_min = read_float_le(&data[0]);
                float z_max = read_float_le(&data[4]);
                ESP_LOGI(TAG, "📐 Z-Axis Range: [%.2f to %.2f] meters", z_min, z_max);
            }
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_INSTALL_SITE:
            if (data_len >= 1) {
                const char *methods[] = {"Top-mounted", "Side-mounted"};
                uint8_t method = data[0];
                ESP_LOGI(TAG, "🔧 Installation: %s (%d)", 
                         method < 2 ? methods[method] : "Unknown", method);
            }
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_LOW_POWER_MODE:
            if (data_len >= 1) {
                ESP_LOGI(TAG, "💤 Low Power Mode: %s", data[0] ? "Enabled" : "Disabled");
            }
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_LOW_POWER_TIME:
            if (data_len >= 4) {
                uint32_t time = read_uint32_le(data);
                ESP_LOGI(TAG, "💤 Low Power Sleep Time: %lu ms", time);
            }
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_MODE:
            if (data_len >= 1) {
                const char *modes[] = {"Low Power", "Normal"};
                uint8_t mode = data[0];
                ESP_LOGI(TAG, "⚙️  Working Mode: %s (%d)", 
                         mode < 2 ? modes[mode] : "Unknown", mode);
            }
            break;
            
        default:
            ESP_LOGD(TAG, "Unknown message type: 0x%04X (len=%d)", msg_type, data_len);
            break;
    }
}

// Main reader task with state machine for frame parsing
static void ld6002_reader_task(void *arg) {
    static uint8_t frame_buf[FRAME_BUF_SIZE];
    uint16_t pos = 0;
    bool syncing = false;
    uint16_t expected_frame_len = 0;
    
    uint32_t total_bytes = 0;
    
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "HLK-LD6002B-3D TinyFrame Protocol");
    ESP_LOGI(TAG, "Baud: %d | TX:%d RX:%d", 
             HLK_LD6002_BAUDRATE, HLK_LD6002_TX_PIN, HLK_LD6002_RX_PIN);
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    
    // Wait for sensor to stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Enable target display
    ESP_LOGI(TAG, "📡 Initializing sensor...");
    send_control_command(CMD_ENABLE_TARGET_DISPLAY);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Request configuration
    send_control_command(CMD_GET_SENSITIVITY);
    vTaskDelay(pdMS_TO_TICKS(100));
    send_control_command(CMD_GET_TRIGGER_SPEED);
    vTaskDelay(pdMS_TO_TICKS(100));
    send_control_command(CMD_GET_INSTALL_METHOD);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "✅ Ready - waiting for detections...");
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    
    uint32_t last_stats_time = 0;
    
    while (true) {
        uint8_t byte;
        int len = uart_read_bytes(HLK_LD6002_UART_PORT, &byte, 1, pdMS_TO_TICKS(100));
        
        if (len == 1) {
            total_bytes++;
            
            if (!syncing) {
                // Looking for SOF
                if (byte == TF_SOF) {
                    frame_buf[0] = byte;
                    pos = 1;
                    syncing = true;
                    expected_frame_len = 0;
                    ESP_LOGD(TAG, "SOF detected at byte %lu", total_bytes);
                }
            } else {
                // Building frame
                if (pos < FRAME_BUF_SIZE) {
                    frame_buf[pos++] = byte;
                    
                    // After receiving first 7 bytes, calculate expected frame length
                    if (pos == 7) {
                        uint16_t data_len = read_uint16_be(&frame_buf[3]);
                        expected_frame_len = 8 + data_len + 1;
                        
                        if (expected_frame_len > FRAME_BUF_SIZE) {
                            ESP_LOGW(TAG, "Frame too large: %d bytes (max %d)", 
                                     expected_frame_len, FRAME_BUF_SIZE);
                            syncing = false;
                            pos = 0;
                        } else if (expected_frame_len < 9) {
                            ESP_LOGW(TAG, "Invalid frame length: %d", expected_frame_len);
                            syncing = false;
                            pos = 0;
                        }
                    }
                    
                    // Check if we have a complete frame
                    if (expected_frame_len > 0 && pos >= expected_frame_len) {
                        parse_tinyframe(frame_buf, expected_frame_len);
                        syncing = false;
                        pos = 0;
                        expected_frame_len = 0;
                    }
                } else {
                    // Buffer overflow
                    ESP_LOGW(TAG, "Frame buffer overflow");
                    syncing = false;
                    pos = 0;
                    expected_frame_len = 0;
                }
            }
        }
        
        // Periodic statistics (every 60 seconds)
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now - last_stats_time > 60000) {
            ESP_LOGI(TAG, "📊 Stats: %lu frames (%lu target, %lu presence) | %lu bytes",
                     g_radar_state.total_frames,
                     g_radar_state.target_frames,
                     g_radar_state.presence_frames,
                     total_bytes);
            last_stats_time = now;
        }
    }
}

static void ld6002_init_uart(void) {
    const uart_config_t uart_config = {
        .baud_rate = HLK_LD6002_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(HLK_LD6002_UART_PORT, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(HLK_LD6002_UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(HLK_LD6002_UART_PORT, HLK_LD6002_TX_PIN, HLK_LD6002_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void app_main(void) {
    ESP_LOGI(TAG, "╔═══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  HLK-LD6002B-3D Radar Sensor         ║");
    ESP_LOGI(TAG, "║  TinyFrame Protocol + Web Interface   ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════╝");
    
    // Initialize UART for sensor communication
    ESP_LOGI(TAG, "Initializing UART%d (TX:%d RX:%d @ %d baud)...",
             HLK_LD6002_UART_PORT, HLK_LD6002_TX_PIN, HLK_LD6002_RX_PIN, HLK_LD6002_BAUDRATE);
    ld6002_init_uart();
    ESP_LOGI(TAG, "✅ UART initialized");

#if ENABLE_WEB_INTERFACE
    // Initialize WiFi
    ESP_LOGI(TAG, "Connecting to WiFi...");
    if (wifi_manager_init() == ESP_OK) {
        ESP_LOGI(TAG, "✅ WiFi connected: %s", wifi_manager_get_ip());
        
        // Initialize mDNS for easy access
        esp_err_t err = mdns_init();
        if (err == ESP_OK) {
            mdns_hostname_set("radar");
            mdns_instance_name_set("HLK-LD6002B-3D Radar");
            mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
            ESP_LOGI(TAG, "✅ mDNS started: http://radar.local");
        }
        
        // Start web server
        if (web_server_init() == ESP_OK) {
            ESP_LOGI(TAG, "✅ Web server started");
            ESP_LOGI(TAG, "╔═══════════════════════════════════════╗");
            ESP_LOGI(TAG, "║  🌐 Open: http://%s              ║", wifi_manager_get_ip());
            ESP_LOGI(TAG, "║  🌐 Or:   http://radar.local          ║");
            ESP_LOGI(TAG, "╚═══════════════════════════════════════╝");
        } else {
            ESP_LOGE(TAG, "Failed to start web server");
        }
    } else {
        ESP_LOGW(TAG, "WiFi connection failed - continuing without web interface");
        ESP_LOGW(TAG, "Check WiFi credentials in wifi_manager.h");
    }
#else
    ESP_LOGI(TAG, "Web interface disabled (ENABLE_WEB_INTERFACE=0)");
#endif

    // Create sensor reader task
    BaseType_t task_created = xTaskCreate(
        ld6002_reader_task,
        "ld6002_reader",
        8 * 1024,  // 8KB stack
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create reader task!");
        return;
    }
    
    ESP_LOGI(TAG, "✅ Sensor reader task started");
    ESP_LOGI(TAG, "═══════════════════════════════════════");

    // Keep main task alive
    while (true) {
#if ENABLE_WEB_INTERFACE
        // Periodic status update
        static uint32_t last_status = 0;
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now - last_status > 60000) {  // Every 60 seconds
            if (wifi_manager_is_connected()) {
                int clients = web_server_get_client_count();
                ESP_LOGI(TAG, "Web: %d client%s connected", clients, clients == 1 ? "" : "s");
            }
            last_status = now;
        }
#endif
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
