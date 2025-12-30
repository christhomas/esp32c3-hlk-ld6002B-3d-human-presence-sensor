# Live Configuration Interface Guide

**For HLK-LD6002B-3D Radar Sensor Web Interface**

This document explains the live configuration interface with 3D zone visualization implemented in the web dashboard.

---

## Overview

The live configuration interface allows real-time adjustment of sensor settings without reflashing firmware. All changes are:
- ✅ Sent via HTTP POST to the ESP32
- ✅ Processed through a thread-safe command queue
- ✅ Applied immediately to the sensor
- ✅ Broadcast to all connected clients via SSE
- ✅ Confirmed with visual feedback

---

## Accessing the Interface

1. Build and flash the firmware to your ESP32
2. Connect to WiFi (check serial output for IP address)
3. Open browser to http://radar.local or http://[IP_ADDRESS]
4. The configuration panel appears in the left sidebar

---

## Configuration Panel

### Sensitivity Control

**Purpose:** Adjusts how easily the radar detects targets.

| Setting | Use Case | Pros | Cons |
|---------|----------|------|------|
| **Low** | Large rooms, far distances | Fewer false positives, stable | May miss small movements |
| **Medium** | General purpose (default) | Balanced performance | Moderate sensitivity |
| **High** | Small rooms, multi-person tracking | Detects subtle movements | More false positives |

**How to use:**
1. Select desired sensitivity from dropdown
2. Change is applied immediately
3. Green checkmark confirms success
4. All connected clients see the update

**Technical details:**
- Sends POST to `/config` with `{"cmd":"sensitivity","value":"high"}`
- Command queued in radar task
- Sensor responds with updated sensitivity value
- Broadcast via SSE as `{"type":"config","data":{"sensitivity":2,...}}`

### Trigger Speed Control

**Purpose:** Controls how quickly the radar responds to movement.

| Setting | Response Time | Use Case |
|---------|--------------|----------|
| **Slow** | ~2-3 seconds | Minimal movement, reduce false triggers |
| **Medium** | ~1-2 seconds | General purpose (default) |
| **Fast** | <1 second | Active environments, quick response needed |

**How to use:**
1. Select desired speed from dropdown
2. Change is applied immediately
3. Green checkmark confirms success

**Technical details:**
- Sends POST to `/config` with `{"cmd":"trigger_speed","value":"fast"}`
- Command processed via queue
- Sensor responds with updated trigger speed
- Broadcast via SSE as `{"type":"config","data":{"trigger_speed":2,...}}`

### Zone Commands

#### Reset Detection Zone
**Purpose:** Restore detection zones to factory defaults (full 3m range).

**When to use:**
- After experimenting with custom zones
- When detection seems restricted
- To start fresh with zone configuration

**How to use:**
1. Click "Reset Detection" button
2. Wait for confirmation message
3. Zone visualization updates automatically
4. New zones broadcast to all clients

#### Clear Interference Zone
**Purpose:** Remove all learned interference zones.

**When to use:**
- After moving furniture or objects
- When legitimate targets are being ignored
- To reset interference learning

**How to use:**
1. Click "Clear Interference" button
2. Wait for confirmation message
3. Orange interference boxes disappear from 3D view

#### Auto-Gen Interference
**Purpose:** Automatically learn and create interference zones to ignore static objects.

⚠️ **WARNING:** Ensure no people are in the detection area during this process!

**How to use:**
1. Remove all people from detection area
2. Click "Auto-Gen Interference" button
3. Orange warning message appears
4. Wait 30-60 seconds for learning to complete
5. Interference zones appear as orange boxes in 3D view

**Technical details:**
- Learning runs for approximately 30-60 seconds
- Sensor analyzes static reflections
- Creates zones to ignore those areas
- Reduces false positives from furniture, walls, etc.

---

## 3D Zone Visualization

### Detection Zones (Blue)

**Appearance:**
- Semi-transparent blue boxes (opacity 0.15)
- Blue wireframe edges
- Text labels showing boundaries

**Information displayed:**
- Zone number (0-3)
- X-axis range (left/right)
- Y-axis range (front/back)
- Z-axis range (ground/height)

**Example:**
```
Zone 0
X: -1.5 to 1.5m
Y: -1.5 to 1.5m
Z: 0.0 to 2.5m
```

**Behavior:**
- Zones turn green when occupied (via presence detection)
- Updated automatically when configuration changes
- Hidden/shown with "Hide Zones" button

### Interference Zones (Orange)

**Appearance:**
- Semi-transparent orange boxes (opacity 0.1)
- Orange wireframe edges
- Only appear if configured

**Purpose:**
- Mark areas to ignore (furniture, walls, static objects)
- Reduce false positives
- Created by "Auto-Gen Interference" command

**Behavior:**
- Only displayed if non-zero boundaries
- Updated automatically when zones change
- Hidden/shown with "Hide Zones" button

### Zone Labels

**Appearance:**
- Floating text sprites above each zone
- Color-matched to zone type (blue/orange)
- Show min/max boundaries for X, Y, Z axes

**Technical implementation:**
- Canvas-based texture rendering
- THREE.Sprite for billboarding
- Positioned at top of zone box

---

## API Reference

### HTTP POST Endpoint: `/config`

**Request format:**
```json
POST /config
Content-Type: application/json

{
  "cmd": "sensitivity",
  "value": "high"
}
```

**Response format:**
```json
{
  "status": "ok"
}
```

**Supported commands:**

| Command | Value | Description |
|---------|-------|-------------|
| `sensitivity` | `"low"`, `"medium"`, `"high"` | Set detection sensitivity |
| `trigger_speed` | `"slow"`, `"medium"`, `"fast"` | Set trigger response time |
| `clear_interference` | _(none)_ | Clear interference zones |
| `reset_detection` | _(none)_ | Reset detection zones |
| `auto_interference` | _(none)_ | Auto-generate interference zones |
| `get_zones` | _(none)_ | Request current zone data |

**Error responses:**
- `400 Bad Request` - Invalid JSON or missing fields
- `500 Internal Server Error` - Command queue full or processing error

### SSE Messages: `/events`

**Detection Zones:**
```json
{
  "type": "detection_zones",
  "data": [
    {
      "x_min": -1.5,
      "x_max": 1.5,
      "y_min": -1.5,
      "y_max": 1.5,
      "z_min": 0.0,
      "z_max": 2.5
    },
    // ... 3 more zones
  ]
}
```

**Interference Zones:**
```json
{
  "type": "interference_zones",
  "data": [
    {
      "x_min": 0.0,
      "x_max": 0.5,
      "y_min": -0.5,
      "y_max": 0.0,
      "z_min": 0.0,
      "z_max": 1.0
    },
    // ... up to 4 zones (only non-zero zones sent)
  ]
}
```

**Configuration Updates:**
```json
{
  "type": "config",
  "data": {
    "sensitivity": 2,        // 0=Low, 1=Medium, 2=High
    "trigger_speed": 2,      // 0=Slow, 1=Medium, 2=Fast
    "install_method": 1      // 0=Top, 1=Side (255=unchanged)
  }
}
```

---

## Technical Architecture

### Backend (ESP32)

**Command Queue System:**
```c
// Define in web_server.h
typedef struct {
    radar_cmd_type_t type;
    uint8_t param;
} radar_cmd_t;

QueueHandle_t cmd_queue = xQueueCreate(10, sizeof(radar_cmd_t));
```

**POST Handler:**
```c
// In web_server.c
static esp_err_t config_post_handler(httpd_req_t *req) {
    // Parse JSON
    // Validate command
    // Queue command for radar task
    // Return success/error
}
```

**Radar Task Processing:**
```c
// In main.c
while (true) {
    // Check command queue
    if (xQueueReceive(cmd_queue, &cmd, 0) == pdTRUE) {
        // Send appropriate control command to sensor
        // Wait for response
        // Broadcast update via SSE
    }
    // Continue normal operation
}
```

**Zone Data Storage:**
```c
// Thread-safe zone storage with mutex
zone_bounds_t detection_zones[4];
zone_bounds_t interference_zones[4];
SemaphoreHandle_t zone_mutex;
```

### Frontend (JavaScript)

**Configuration Functions:**
```javascript
async function setSensitivity(value) {
    await sendConfigCommand('sensitivity', value);
}

async function sendConfigCommand(cmd, value) {
    const response = await fetch('/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ cmd, value })
    });
    showFeedback(response.ok ? 'Success' : 'Failed');
}
```

**3D Zone Rendering:**
```javascript
function createZoneBox(bounds, color, opacity) {
    // Calculate dimensions and center
    const width = bounds.x_max - bounds.x_min;
    const depth = bounds.y_max - bounds.y_min;
    const height = bounds.z_max - bounds.z_min;
    
    // Create semi-transparent box
    const geometry = new THREE.BoxGeometry(width, height, depth);
    const material = new THREE.MeshBasicMaterial({
        color: color,
        transparent: true,
        opacity: opacity
    });
    const box = new THREE.Mesh(geometry, material);
    
    // Create wireframe edges
    const edges = new THREE.EdgesGeometry(geometry);
    const line = new THREE.LineSegments(edges, 
        new THREE.LineBasicMaterial({ color: color }));
    
    return { box, line };
}
```

**SSE Event Handling:**
```javascript
es.onmessage = (event) => {
    const msg = JSON.parse(event.data);
    
    if (msg.type === 'detection_zones') {
        updateDetectionZones(msg.data);
    } else if (msg.type === 'interference_zones') {
        updateInterferenceZones(msg.data);
    } else if (msg.type === 'config') {
        updateConfigUI(msg.data);
    }
};
```

---

## Troubleshooting

### Configuration not applying

**Symptoms:** Buttons click but sensor doesn't change.

**Solutions:**
1. Check serial console for command processing logs
2. Verify sensor is responding (check "Received X bytes" messages)
3. Try increasing delay in command processing (edit `main.c`)
4. Check command queue isn't full (increase `CMD_QUEUE_SIZE` in `web_server.h`)

### Zones not displaying

**Symptoms:** No blue/orange boxes in 3D view.

**Solutions:**
1. Wait for initial zone data (sent at startup)
2. Click "Get Zones" button to request manually
3. Check browser console for JavaScript errors
4. Verify SSE connection is active (check "Connection: Connected" in HUD)
5. Try toggling "Hide Zones" button

### Feedback messages not showing

**Symptoms:** Button clicks have no visual response.

**Solutions:**
1. Check browser console for network errors
2. Verify `/config` endpoint is accessible (check network tab)
3. Ensure feedback message div exists in HTML
4. Check CSS for `#feedback-msg` styling

### Auto-Gen Interference fails

**Symptoms:** Orange warning appears but zones never created.

**Solutions:**
1. Ensure learning completed (wait full 60 seconds)
2. Check serial console for completion message
3. Manually request zones with "Get Zones" button
4. Try "Clear Interference" then retry
5. Verify sensor has clear view of static objects

---

## Best Practices

### For Multi-Person Detection

1. Set **Sensitivity: High**
2. Set **Trigger Speed: Fast**
3. **Clear Interference** zones
4. **Reset Detection** zones
5. Ensure targets maintain >0.5m separation

### For Stable Single-Person

1. Set **Sensitivity: Medium**
2. Set **Trigger Speed: Medium**
3. Use **Auto-Gen Interference** to ignore furniture
4. Leave detection zones at default

### For Minimal False Positives

1. Set **Sensitivity: Low**
2. Set **Trigger Speed: Slow**
3. Use **Auto-Gen Interference** extensively
4. Consider reducing detection zone size (requires firmware modification)

---

## Performance Considerations

### Memory Usage

- Command queue: ~120 bytes (10 commands × 12 bytes each)
- Zone storage: ~192 bytes (8 zones × 24 bytes each)
- SSE message buffer: 1024 bytes (shared)
- **Total added:** ~1.3 KB

### Processing Overhead

- POST request handling: <5ms
- Command queue operation: <1ms
- Zone parsing and broadcast: <10ms
- **Total impact:** Negligible on 160 MHz ESP32-C3

### Network Bandwidth

- Configuration POST: ~50 bytes per command
- Zone SSE broadcast: ~600 bytes per update
- Config SSE broadcast: ~80 bytes per update
- **Total:** <5 KB for full configuration cycle

---

## Future Enhancements

- [ ] Installation method toggle (top/side mounted) in UI
- [ ] Custom zone boundary editor with 3D handles
- [ ] Zone preset templates (living room, bedroom, etc.)
- [ ] Configuration profiles (save/load settings)
- [ ] Export zone data as JSON
- [ ] Advanced sensitivity curves
- [ ] Zone-specific sensitivity settings
- [ ] Heat map visualization of detection intensity

---

## References

- Main firmware: [`src/main.c`](../src/main.c)
- Web server: [`src/web_server.c`](../src/web_server.c)
- Web interface: [`src/webapp.js`](../src/webapp.js)
- Command reference: [`docs/sensor-configuration.md`](sensor-configuration.md)
- Protocol documentation: [`docs/tinyframe-protocol.md`](tinyframe-protocol.md)

---

**Last Updated:** December 30, 2024  
**Firmware Version:** 1.0 with Live Configuration Interface
