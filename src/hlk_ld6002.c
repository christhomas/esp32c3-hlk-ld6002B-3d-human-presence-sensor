// HLK-LD6002B-3D Radar Sensor API Implementation
// TinyFrame Protocol V1.2 for 60GHz FMCW radar

#include "hlk_ld6002.h"
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>
#include <math.h>

static const char *TAG = "HLK-LD6002";

// ========== GLOBAL STATE ==========

// Sensor statistics
static struct {
    uint32_t total_frames;
    uint32_t target_frames;
    uint32_t presence_frames;
} g_stats = {0};

// Registered callbacks
static hlk_callbacks_t g_callbacks = {0};

// Frame parser state
static struct {
    uint8_t frame_buf[HLK_FRAME_BUF_SIZE];
    uint16_t pos;
    bool syncing;
    uint16_t expected_frame_len;
} g_parser = {0};

// ========== UTILITY FUNCTIONS ==========

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

float hlk_calc_distance_3d(float x, float y, float z) {
    return sqrtf(x * x + y * y + z * z);
}

const char* hlk_sensitivity_to_string(uint8_t level) {
    const char *levels[] = {"Low", "Medium", "High"};
    return level < 3 ? levels[level] : "Unknown";
}

const char* hlk_trigger_speed_to_string(uint8_t speed) {
    const char *speeds[] = {"Slow", "Medium", "Fast"};
    return speed < 3 ? speeds[speed] : "Unknown";
}

const char* hlk_install_method_to_string(uint8_t method) {
    const char *methods[] = {"Top-mounted", "Side-mounted"};
    return method < 2 ? methods[method] : "Unknown";
}

// ========== MESSAGE PARSERS ==========

// Parse target position message (0x0A04)
static void parse_target_position(const uint8_t *data, uint16_t len) {
    if (len < 4) return;
    
    int32_t target_num = read_int32_le(&data[0]);
    g_stats.target_frames++;
    
    if (target_num > 0) {
        // Validate data length: 4 bytes header + (20 bytes per target)
        uint16_t expected_len = 4 + (target_num * 20);
        if (len < expected_len) {
            ESP_LOGW(TAG, "Incomplete target data: got %d bytes, expected %d", len, expected_len);
            return;
        }
        
        // Parse targets
        hlk_target_t targets[10];  // Support up to 10 targets
        int32_t num_to_process = target_num > 10 ? 10 : target_num;
        
        for (int i = 0; i < num_to_process; i++) {
            uint16_t offset = 4 + (i * 20);
            targets[i].x = read_float_le(&data[offset]);
            targets[i].y = read_float_le(&data[offset + 4]);
            targets[i].z = read_float_le(&data[offset + 8]);
            targets[i].velocity = read_int32_le(&data[offset + 12]);
            targets[i].cluster_id = read_int32_le(&data[offset + 16]);
        }
        
        // Trigger callback
        if (g_callbacks.on_target) {
            g_callbacks.on_target(targets, num_to_process);
        }
    } else {
        // No targets - trigger callback with count 0
        if (g_callbacks.on_target) {
            g_callbacks.on_target(NULL, 0);
        }
    }
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
    
    g_stats.presence_frames++;
    
    // Trigger callback
    if (g_callbacks.on_presence) {
        g_callbacks.on_presence(zone0, zone1, zone2, zone3);
    }
}

// Parse zone coordinates (0x0A0B or 0x0A0C)
static void parse_zones(const uint8_t *data, uint16_t len, bool is_interference) {
    if (len < 96) return;  // 4 zones * 6 floats * 4 bytes = 96 bytes
    
    hlk_zone_t zones[4];
    const char *zone_type = is_interference ? "Interference" : "Detection";
    
    ESP_LOGI(TAG, "📍 %s Zones:", zone_type);
    
    for (int i = 0; i < 4; i++) {
        uint16_t offset = i * 24;
        zones[i].x_min = read_float_le(&data[offset]);
        zones[i].x_max = read_float_le(&data[offset + 4]);
        zones[i].y_min = read_float_le(&data[offset + 8]);
        zones[i].y_max = read_float_le(&data[offset + 12]);
        zones[i].z_min = read_float_le(&data[offset + 16]);
        zones[i].z_max = read_float_le(&data[offset + 20]);
        
        ESP_LOGI(TAG, "  Zone %d: X[%.1f to %.1f] Y[%.1f to %.1f] Z[%.1f to %.1f]m",
                 i, zones[i].x_min, zones[i].x_max, 
                 zones[i].y_min, zones[i].y_max,
                 zones[i].z_min, zones[i].z_max);
    }
    
    // Trigger callback
    if (g_callbacks.on_zones) {
        g_callbacks.on_zones(zones, is_interference);
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
    
    // Verify header checksum
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
    
    g_stats.total_frames++;
    ESP_LOGD(TAG, "Frame #%lu: ID=0x%04X Type=0x%04X Len=%d", 
             g_stats.total_frames, frame_id, msg_type, data_len);
    
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
            parse_zones(data, data_len, true);
            break;
            
        case MSG_IND_HUMAN_DETECTION_3D_DETECTION_ZONES:
            parse_zones(data, data_len, false);
            break;
            
        default:
            // Other message types - pass to config callback
            if (g_callbacks.on_config) {
                g_callbacks.on_config(msg_type, data, data_len);
            }
            ESP_LOGD(TAG, "Message type: 0x%04X (len=%d)", msg_type, data_len);
            break;
    }
}

// ========== API IMPLEMENTATION ==========

esp_err_t hlk_ld6002_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = HLK_LD6002_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(HLK_LD6002_UART_PORT, HLK_UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(HLK_LD6002_UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(HLK_LD6002_UART_PORT, HLK_LD6002_TX_PIN, HLK_LD6002_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    ESP_LOGI(TAG, "Initialized UART%d (TX:%d RX:%d @ %d baud)",
             HLK_LD6002_UART_PORT, HLK_LD6002_TX_PIN, HLK_LD6002_RX_PIN, HLK_LD6002_BAUDRATE);
    
    return ESP_OK;
}

void hlk_ld6002_register_callbacks(const hlk_callbacks_t* callbacks) {
    if (callbacks) {
        g_callbacks = *callbacks;
        ESP_LOGI(TAG, "Callbacks registered");
    }
}

void hlk_ld6002_send_command(uint32_t cmd) {
    uint8_t frame[256];
    uint8_t pos = 0;
    
    // Build frame header
    frame[pos++] = TF_SOF;
    write_uint16_be(&frame[pos], 0x0000);  // Frame ID = 0
    pos += 2;
    write_uint16_be(&frame[pos], 4);  // Data length = 4
    pos += 2;
    write_uint16_be(&frame[pos], MSG_CFG_HUMAN_DETECTION_3D);
    pos += 2;
    
    // Calculate and add header checksum
    frame[pos] = calc_checksum(frame, 7);
    pos++;
    
    // Add data (command as int32_le)
    uint8_t data[4];
    write_int32_le(data, cmd);
    memcpy(&frame[pos], data, 4);
    pos += 4;
    
    // Calculate and add data checksum
    frame[pos] = calc_checksum(data, 4);
    pos++;
    
    // Send frame
    uart_write_bytes(HLK_LD6002_UART_PORT, frame, pos);
    
    ESP_LOGI(TAG, "📤 Sent command 0x%02lX", cmd);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, frame, pos, ESP_LOG_DEBUG);
}

int hlk_ld6002_process(uint32_t timeout_ms) {
    uint8_t byte;
    int bytes_processed = 0;
    int len = uart_read_bytes(HLK_LD6002_UART_PORT, &byte, 1, pdMS_TO_TICKS(timeout_ms));
    
    if (len == 1) {
        bytes_processed++;
        
        if (!g_parser.syncing) {
            // Looking for SOF
            if (byte == TF_SOF) {
                g_parser.frame_buf[0] = byte;
                g_parser.pos = 1;
                g_parser.syncing = true;
                g_parser.expected_frame_len = 0;
                ESP_LOGD(TAG, "SOF detected");
            }
        } else {
            // Building frame
            if (g_parser.pos < HLK_FRAME_BUF_SIZE) {
                g_parser.frame_buf[g_parser.pos++] = byte;
                
                // After receiving first 7 bytes, calculate expected frame length
                if (g_parser.pos == 7) {
                    uint16_t data_len = read_uint16_be(&g_parser.frame_buf[3]);
                    g_parser.expected_frame_len = 8 + data_len + 1;
                    
                    if (g_parser.expected_frame_len > HLK_FRAME_BUF_SIZE) {
                        ESP_LOGW(TAG, "Frame too large: %d bytes (max %d)", 
                                 g_parser.expected_frame_len, HLK_FRAME_BUF_SIZE);
                        g_parser.syncing = false;
                        g_parser.pos = 0;
                    } else if (g_parser.expected_frame_len < 9) {
                        ESP_LOGW(TAG, "Invalid frame length: %d", g_parser.expected_frame_len);
                        g_parser.syncing = false;
                        g_parser.pos = 0;
                    }
                }
                
                // Check if we have a complete frame
                if (g_parser.expected_frame_len > 0 && g_parser.pos >= g_parser.expected_frame_len) {
                    parse_tinyframe(g_parser.frame_buf, g_parser.expected_frame_len);
                    g_parser.syncing = false;
                    g_parser.pos = 0;
                    g_parser.expected_frame_len = 0;
                }
            } else {
                // Buffer overflow
                ESP_LOGW(TAG, "Frame buffer overflow");
                g_parser.syncing = false;
                g_parser.pos = 0;
                g_parser.expected_frame_len = 0;
            }
        }
    }
    
    return bytes_processed;
}

void hlk_ld6002_get_stats(uint32_t* total_frames, uint32_t* target_frames, uint32_t* presence_frames) {
    if (total_frames) *total_frames = g_stats.total_frames;
    if (target_frames) *target_frames = g_stats.target_frames;
    if (presence_frames) *presence_frames = g_stats.presence_frames;
}
