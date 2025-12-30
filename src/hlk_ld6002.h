// HLK-LD6002B-3D Radar Sensor API
// TinyFrame Protocol V1.2 implementation for 60GHz FMCW radar

#ifndef HLK_LD6002_H
#define HLK_LD6002_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/uart.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// ========== CONFIGURATION ==========

#ifndef HLK_LD6002_UART_PORT
#define HLK_LD6002_UART_PORT UART_NUM_1
#endif

#ifndef HLK_LD6002_TX_PIN
#define HLK_LD6002_TX_PIN GPIO_NUM_21  // D6 on XIAO → Pin 8 (RX0) on sensor
#endif

#ifndef HLK_LD6002_RX_PIN
#define HLK_LD6002_RX_PIN GPIO_NUM_20  // D7 on XIAO → Pin 7 (TX0) on sensor
#endif

#ifndef HLK_LD6002_BAUDRATE
#define HLK_LD6002_BAUDRATE 115200  // Default for LD6002B-3D
#endif

#define HLK_UART_BUF_SIZE 2048
#define HLK_FRAME_BUF_SIZE 1152  // Max: 1 + 2 + 2 + 2 + 1 + 1024 + 1 = 1033 bytes

// ========== TINYFRAME PROTOCOL ==========

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

// ========== DATA STRUCTURES ==========

// TinyFrame structure
typedef struct {
    uint8_t sof;           // Start of Frame (0x01)
    uint16_t id;           // Frame ID (big-endian)
    uint16_t len;          // Data length (big-endian)
    uint16_t type;         // Message type (big-endian)
    uint8_t head_cksum;    // Header checksum
    const uint8_t *data;   // Pointer to data (little-endian)
    uint8_t data_cksum;    // Data checksum
} hlk_tinyframe_t;

// Target data
typedef struct {
    float x;            // X coordinate (meters)
    float y;            // Y coordinate (meters)
    float z;            // Z coordinate (meters)
    int32_t velocity;   // Doppler velocity index
    int32_t cluster_id; // Cluster ID
} hlk_target_t;

// Zone bounds
typedef struct {
    float x_min;
    float x_max;
    float y_min;
    float y_max;
    float z_min;
    float z_max;
} hlk_zone_t;

// Parsed message callback types
typedef void (*hlk_target_callback_t)(const hlk_target_t* targets, int32_t count);
typedef void (*hlk_presence_callback_t)(uint32_t zone0, uint32_t zone1, uint32_t zone2, uint32_t zone3);
typedef void (*hlk_zones_callback_t)(const hlk_zone_t* zones, bool is_interference);
typedef void (*hlk_config_callback_t)(uint16_t msg_type, const uint8_t* data, uint16_t len);

// Sensor callbacks structure
typedef struct {
    hlk_target_callback_t on_target;
    hlk_presence_callback_t on_presence;
    hlk_zones_callback_t on_zones;
    hlk_config_callback_t on_config;
} hlk_callbacks_t;

// ========== API FUNCTIONS ==========

/**
 * Initialize the HLK-LD6002 sensor UART interface
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t hlk_ld6002_init(void);

/**
 * Register callbacks for sensor data
 * @param callbacks Callback functions for different message types
 */
void hlk_ld6002_register_callbacks(const hlk_callbacks_t* callbacks);

/**
 * Send a control command to the sensor
 * @param cmd Command code (CMD_*)
 */
void hlk_ld6002_send_command(uint32_t cmd);

/**
 * Parse incoming UART data and trigger callbacks
 * Should be called continuously from a task
 * @param timeout_ms Maximum time to wait for data
 * @return Number of bytes processed
 */
int hlk_ld6002_process(uint32_t timeout_ms);

/**
 * Get frame statistics
 * @param total_frames Total frames received (output)
 * @param target_frames Target frames received (output)
 * @param presence_frames Presence frames received (output)
 */
void hlk_ld6002_get_stats(uint32_t* total_frames, uint32_t* target_frames, uint32_t* presence_frames);

// ========== UTILITY FUNCTIONS ==========

/**
 * Calculate 3D distance from origin
 * @param x X coordinate
 * @param y Y coordinate
 * @param z Z coordinate
 * @return Distance in same units as input
 */
float hlk_calc_distance_3d(float x, float y, float z);

/**
 * Convert sensitivity level to string
 * @param level Sensitivity level (0-2)
 * @return String representation
 */
const char* hlk_sensitivity_to_string(uint8_t level);

/**
 * Convert trigger speed to string
 * @param speed Trigger speed (0-2)
 * @return String representation
 */
const char* hlk_trigger_speed_to_string(uint8_t speed);

/**
 * Convert installation method to string
 * @param method Installation method (0-1)
 * @return String representation
 */
const char* hlk_install_method_to_string(uint8_t method);

#ifdef __cplusplus
}
#endif

#endif // HLK_LD6002_H
