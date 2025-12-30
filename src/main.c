// ESP-IDF example for HLK-LD6002B-3D presence detection radar sensor
//
// IMPORTANT WIRING:
// HLK-LD6002 Pin 7 (TX0) → ESP32 GPIO20 (D7/RX)
// HLK-LD6002 Pin 8 (RX0) → ESP32 GPIO21 (D6/TX)
// HLK-LD6002 Pin 3 (P19) → GND (BOOT1 must be LOW!)
// HLK-LD6002 Pin 1 (3V3) → 3.3V (requires ≥1A supply!)
// HLK-LD6002 Pin 2 (GND) → GND

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mdns.h"

// Application modules
#include "hlk_ld6002.h"
#include "target_tracker.h"
#include "api.h"
#include "wifi_manager.h"
#include "web_server.h"

// Feature flags
#define ENABLE_WEB_INTERFACE 1  // Set to 0 to disable WiFi/web for debugging

static const char *TAG = "App";

// ========== SENSOR TASK ==========

static void sensor_task(void *arg) {
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "HLK-LD6002B-3D Sensor Task");
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    
    // Wait for sensor to stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Initialize sensor
    ESP_LOGI(TAG, "📡 Initializing sensor...");
    hlk_ld6002_send_command(CMD_ENABLE_TARGET_DISPLAY);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Request configuration
    hlk_ld6002_send_command(CMD_GET_SENSITIVITY);
    vTaskDelay(pdMS_TO_TICKS(100));
    hlk_ld6002_send_command(CMD_GET_TRIGGER_SPEED);
    vTaskDelay(pdMS_TO_TICKS(100));
    hlk_ld6002_send_command(CMD_GET_INSTALL_METHOD);
    vTaskDelay(pdMS_TO_TICKS(100));
    hlk_ld6002_send_command(CMD_GET_ZONES);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "✅ Ready - waiting for detections...");
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    
    while (true) {
        // Process web commands (via API layer)
        api_process_web_commands();
        
        // Process sensor data
        hlk_ld6002_process(10);  // 10ms timeout
        
        // Log statistics
        api_log_stats();
    }
}

// ========== MAIN APPLICATION ==========

void app_main(void) {
    ESP_LOGI(TAG, "╔═══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  HLK-LD6002B-3D Radar Sensor         ║");
    ESP_LOGI(TAG, "║  Clean Modular Architecture          ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════╝");
    
    // Initialize sensor hardware
    ESP_LOGI(TAG, "Initializing sensor communication...");
    if (hlk_ld6002_init() == ESP_OK) {
        ESP_LOGI(TAG, "✅ Sensor UART initialized");
    } else {
        ESP_LOGE(TAG, "❌ Failed to initialize sensor UART");
        return;
    }
    
    // Initialize target tracker
    target_tracker_init();
    
    // Initialize API layer
    api_init();
    
    // Register sensor callbacks through API layer
    hlk_callbacks_t callbacks = {
        .on_target = api_on_target_detected,
        .on_presence = api_on_presence_detected,
        .on_zones = api_on_zones_received,
        .on_config = api_on_config_received
    };
    hlk_ld6002_register_callbacks(&callbacks);

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
        ESP_LOGW(TAG, "Check WiFi credentials in wifi_credentials.h");
    }
#else
    ESP_LOGI(TAG, "Web interface disabled (ENABLE_WEB_INTERFACE=0)");
#endif

    // Create sensor processing task
    BaseType_t task_created = xTaskCreate(
        sensor_task,
        "sensor_task",
        8 * 1024,  // 8KB stack
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor task!");
        return;
    }
    
    ESP_LOGI(TAG, "✅ Sensor task started");
    ESP_LOGI(TAG, "═══════════════════════════════════════");

    // Main loop - monitor web clients
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
