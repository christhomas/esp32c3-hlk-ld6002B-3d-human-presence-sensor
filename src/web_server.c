// Web Server Implementation with Server-Sent Events (SSE) Streaming
// SSE provides real-time updates with ~100ms latency, perfect for 20Hz radar data

#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "WebServer";

// HTTP server handle
static httpd_handle_t server = NULL;

// Client tracking for statistics
static int client_count = 0;

// Shared message buffer for SSE
static char message_buffer[1024];
static SemaphoreHandle_t message_mutex = NULL;

// Command queue for radar control
static QueueHandle_t cmd_queue = NULL;

// Zone data storage
static zone_bounds_t detection_zones[4] = {0};
static zone_bounds_t interference_zones[4] = {0};
static SemaphoreHandle_t zone_mutex = NULL;

// Embedded HTML file (minified and combined from webapp.html/css/js)
extern const uint8_t index_html_start[] asm("_binary_webapp_min_html_start");
extern const uint8_t index_html_end[]   asm("_binary_webapp_min_html_end");

// Root handler - serves the embedded HTML file
static esp_err_t root_handler(httpd_req_t *req) {
    const size_t index_html_size = (index_html_end - index_html_start);
    ESP_LOGI(TAG, "📄 Root handler called, HTML size: %d bytes", index_html_size);
    
    if (index_html_size == 0 || index_html_size > 100000) {
        ESP_LOGE(TAG, "❌ Invalid HTML size: %d bytes - file may not be embedded!", index_html_size);
        const char *error_msg = "<html><body><h1>Error: HTML file not embedded</h1><p>Rebuild with 'pio run --target clean' then 'pio run'</p></body></html>";
        httpd_resp_set_type(req, "text/html");
        return httpd_resp_send(req, error_msg, strlen(error_msg));
    }
    
    ESP_LOGI(TAG, "✅ Serving HTML (%d bytes)", index_html_size);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    return httpd_resp_send(req, (const char *)index_html_start, index_html_size);
}

// SSE handler - streams events to connected clients
static esp_err_t sse_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/event-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    client_count++;
    ESP_LOGI(TAG, "SSE client connected (%d total)", client_count);
    
    // Send initial ping
    const char *init = "data: {\"type\":\"connected\"}\n\n";
    if (httpd_resp_send_chunk(req, init, strlen(init)) != ESP_OK) {
        client_count--;
        return ESP_FAIL;
    }
    
    // Keep connection alive and send data
    char last_msg[1024] = {0};
    for (int i = 0; i < 36000; i++) {  // Max 1 hour (36000 * 100ms)
        // Check if there's a new message
        if (message_mutex && xSemaphoreTake(message_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (strlen(message_buffer) > 0 && strcmp(message_buffer, last_msg) != 0) {
                // Format as SSE
                char sse_msg[1100];
                snprintf(sse_msg, sizeof(sse_msg), "data: %s\n\n", message_buffer);
                
                if (httpd_resp_send_chunk(req, sse_msg, strlen(sse_msg)) != ESP_OK) {
                    xSemaphoreGive(message_mutex);
                    break;
                }
                strncpy(last_msg, message_buffer, sizeof(last_msg) - 1);
            }
            xSemaphoreGive(message_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // Poll every 10ms for low latency
    }
    
    client_count--;
    ESP_LOGI(TAG, "SSE client disconnected (%d remaining)", client_count);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// POST handler for configuration commands
static esp_err_t config_post_handler(httpd_req_t *req) {
    char content[200];
    int ret = httpd_req_recv(req, content, sizeof(content));
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    ESP_LOGI(TAG, "Config POST: %s", content);
    
    // Parse JSON using cJSON
    cJSON *root = cJSON_Parse(content);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *cmd_json = cJSON_GetObjectItem(root, "cmd");
    cJSON *value_json = cJSON_GetObjectItem(root, "value");
    
    if (!cmd_json || !cJSON_IsString(cmd_json)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'cmd' field");
        return ESP_FAIL;
    }
    
    const char *cmd_str = cmd_json->valuestring;
    radar_cmd_t cmd = {0};
    bool valid_cmd = true;
    
    // Parse command type
    if (strcmp(cmd_str, "sensitivity") == 0) {
        cmd.type = RADAR_CMD_SET_SENSITIVITY;
        if (value_json && cJSON_IsString(value_json)) {
            const char *val = value_json->valuestring;
            if (strcmp(val, "low") == 0) cmd.param = 0;
            else if (strcmp(val, "medium") == 0) cmd.param = 1;
            else if (strcmp(val, "high") == 0) cmd.param = 2;
            else valid_cmd = false;
        } else {
            valid_cmd = false;
        }
    } else if (strcmp(cmd_str, "trigger_speed") == 0) {
        cmd.type = RADAR_CMD_SET_TRIGGER_SPEED;
        if (value_json && cJSON_IsString(value_json)) {
            const char *val = value_json->valuestring;
            if (strcmp(val, "slow") == 0) cmd.param = 0;
            else if (strcmp(val, "medium") == 0) cmd.param = 1;
            else if (strcmp(val, "fast") == 0) cmd.param = 2;
            else valid_cmd = false;
        } else {
            valid_cmd = false;
        }
    } else if (strcmp(cmd_str, "clear_interference") == 0) {
        cmd.type = RADAR_CMD_CLEAR_INTERFERENCE_ZONE;
    } else if (strcmp(cmd_str, "reset_detection") == 0) {
        cmd.type = RADAR_CMD_RESET_DETECTION_ZONE;
    } else if (strcmp(cmd_str, "auto_interference") == 0) {
        cmd.type = RADAR_CMD_AUTO_GEN_INTERFERENCE_ZONE;
    } else if (strcmp(cmd_str, "get_zones") == 0) {
        cmd.type = RADAR_CMD_GET_ZONES;
    } else {
        valid_cmd = false;
    }
    
    cJSON_Delete(root);
    
    if (!valid_cmd) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid command or value");
        return ESP_FAIL;
    }
    
    // Send command to queue
    if (cmd_queue && xQueueSend(cmd_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
        ESP_LOGI(TAG, "Command queued: type=%d param=%d", cmd.type, cmd.param);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Command queue full");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t web_server_init(void) {
    message_mutex = xSemaphoreCreateMutex();
    if (!message_mutex) {
        ESP_LOGE(TAG, "Failed to create message mutex");
        return ESP_FAIL;
    }
    
    zone_mutex = xSemaphoreCreateMutex();
    if (!zone_mutex) {
        ESP_LOGE(TAG, "Failed to create zone mutex");
        return ESP_FAIL;
    }
    
    // Create command queue
    cmd_queue = xQueueCreate(CMD_QUEUE_SIZE, sizeof(radar_cmd_t));
    if (!cmd_queue) {
        ESP_LOGE(TAG, "Failed to create command queue");
        return ESP_FAIL;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_open_sockets = 4;  // Max 7 allowed (3 used internally), using 4 for clients
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;
    config.stack_size = 8192;  // Increase from default 4096 to handle large HTML file
    
    ESP_LOGI(TAG, "Starting web server");
    
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start server");
        return ESP_FAIL;
    }
    
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &root_uri);
    
    httpd_uri_t sse_uri = {
        .uri = "/events",
        .method = HTTP_GET,
        .handler = sse_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &sse_uri);
    
    httpd_uri_t config_uri = {
        .uri = "/config",
        .method = HTTP_POST,
        .handler = config_post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &config_uri);
    
    ESP_LOGI(TAG, "✅ Web server started with SSE streaming and config API");
    return ESP_OK;
}

void web_server_deinit(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    if (message_mutex) {
        vSemaphoreDelete(message_mutex);
        message_mutex = NULL;
    }
    if (zone_mutex) {
        vSemaphoreDelete(zone_mutex);
        zone_mutex = NULL;
    }
    if (cmd_queue) {
        vQueueDelete(cmd_queue);
        cmd_queue = NULL;
    }
    ESP_LOGI(TAG, "Web server stopped");
}

bool web_server_is_running(void) {
    return server != NULL;
}

// Helper to queue message for SSE broadcast
static void queue_message(const char *json) {
    if (!message_mutex || !json) return;
    
    if (xSemaphoreTake(message_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        strncpy(message_buffer, json, sizeof(message_buffer) - 1);
        message_buffer[sizeof(message_buffer) - 1] = '\0';
        xSemaphoreGive(message_mutex);
    }
}

void web_server_send_targets(const hlk_target_t* targets, int32_t target_count) {
    if (!server || client_count == 0) return;
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "target");
    
    cJSON *data = cJSON_CreateArray();
    for (int i = 0; i < target_count; i++) {
        cJSON *target = cJSON_CreateObject();
        cJSON_AddNumberToObject(target, "x", targets[i].x);
        cJSON_AddNumberToObject(target, "y", targets[i].y);
        cJSON_AddNumberToObject(target, "z", targets[i].z);
        cJSON_AddNumberToObject(target, "v", targets[i].velocity);
        cJSON_AddNumberToObject(target, "c", targets[i].cluster_id);
        cJSON_AddItemToArray(data, target);
    }
    cJSON_AddItemToObject(root, "data", data);
    
    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        queue_message(json);
        free(json);
    }
    cJSON_Delete(root);
}

void web_server_send_point_cloud(int32_t point_count, const float* points, int max_points) {
    // Point cloud support (optional - can be enabled later)
    (void)point_count;
    (void)points;
    (void)max_points;
}

void web_server_send_presence(uint32_t zone0, uint32_t zone1, 
                              uint32_t zone2, uint32_t zone3) {
    if (!server || client_count == 0) return;
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "presence");
    
    cJSON *data = cJSON_CreateArray();
    cJSON_AddItemToArray(data, cJSON_CreateNumber(zone0));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(zone1));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(zone2));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(zone3));
    cJSON_AddItemToObject(root, "data", data);
    
    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        queue_message(json);
        free(json);
    }
    cJSON_Delete(root);
}

void web_server_send_config(uint8_t sensitivity, uint8_t trigger_speed, 
                            uint8_t install_method) {
    if (!server || client_count == 0) return;
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "config");
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "sensitivity", sensitivity);
    cJSON_AddNumberToObject(data, "trigger_speed", trigger_speed);
    cJSON_AddNumberToObject(data, "install_method", install_method);
    cJSON_AddItemToObject(root, "data", data);
    
    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        queue_message(json);
        free(json);
    }
    cJSON_Delete(root);
}

void web_server_send_zones(const zone_bounds_t* zones, bool is_interference) {
    if (!server || client_count == 0 || !zones) return;
    
    // Store zones
    if (zone_mutex && xSemaphoreTake(zone_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (is_interference) {
            memcpy(interference_zones, zones, sizeof(zone_bounds_t) * 4);
        } else {
            memcpy(detection_zones, zones, sizeof(zone_bounds_t) * 4);
        }
        xSemaphoreGive(zone_mutex);
    }
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", is_interference ? "interference_zones" : "detection_zones");
    
    cJSON *data = cJSON_CreateArray();
    for (int i = 0; i < 4; i++) {
        cJSON *zone = cJSON_CreateObject();
        cJSON_AddNumberToObject(zone, "x_min", zones[i].x_min);
        cJSON_AddNumberToObject(zone, "x_max", zones[i].x_max);
        cJSON_AddNumberToObject(zone, "y_min", zones[i].y_min);
        cJSON_AddNumberToObject(zone, "y_max", zones[i].y_max);
        cJSON_AddNumberToObject(zone, "z_min", zones[i].z_min);
        cJSON_AddNumberToObject(zone, "z_max", zones[i].z_max);
        cJSON_AddItemToArray(data, zone);
    }
    cJSON_AddItemToObject(root, "data", data);
    
    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        queue_message(json);
        free(json);
    }
    cJSON_Delete(root);
}

QueueHandle_t web_server_get_cmd_queue(void) {
    return cmd_queue;
}

int web_server_get_client_count(void) {
    return client_count;
}
