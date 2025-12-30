# HLK-LD6002B-3D Radar Sensor with Web Visualization

ESP32-based firmware for the HLK-LD6002B-3D presence detection radar sensor with real-time 3D web visualization using SSE (Server-Sent Events) streaming and Three.js.

## Project Overview

This project provides a complete firmware solution for interfacing the **HLK-LD6002B-3D** 60GHz FMCW radar sensor with an ESP32-C3 microcontroller. The firmware implements the full TinyFrame communication protocol and provides:

- **Serial Communication:** Complete TinyFrame protocol parser for radar data acquisition
- **Web Dashboard:** Real-time 3D visualization accessible via WiFi
- **SSE Streaming:** Server-Sent Events for low-latency sensor data streaming (~100ms latency)
- **Production Ready:** Memory-efficient design with room for expansion

**Target Hardware:** Seeed XIAO ESP32-C3 (or any ESP32-C3 board)
**Sensor:** HLK-LD6002B-3D 60GHz presence detection radar
**Framework:** ESP-IDF (Espressif IoT Development Framework)
**Build Tools:** ESP-IDF native tools OR PlatformIO (VS Code integration)

### Build Tool Options

This project can be built using **ESP-IDF directly** or through **PlatformIO**:

| Tool | What It Is | Best For |
|------|-----------|----------|
| **ESP-IDF Native** | Official Espressif toolchain | Advanced users, command-line workflows, latest ESP-IDF features |
| **PlatformIO** | IDE wrapper that uses ESP-IDF under the hood | Beginners, VS Code users, unified project management |

**Important:** PlatformIO is NOT a separate framework - it's a build system that integrates ESP-IDF into VS Code with a simplified interface. Both approaches use the same ESP-IDF framework, libraries, and compiler, producing identical firmware. PlatformIO just makes it easier to manage ESP-IDF projects within VS Code.

## Quick Start

**TL;DR - Get Running in 5 Minutes:**

1. **Hardware:** Wire sensor to ESP32-C3 (see [Wiring](#wiring))
2. **Software:** Install PlatformIO VS Code extension OR ESP-IDF
3. **Configure WiFi:** Copy [`src/wifi_credentials.h.example`](src/wifi_credentials.h.example) to `src/wifi_credentials.h` and add your WiFi credentials
4. **Build:** `pio run --target upload --target monitor` OR `idf.py flash monitor`
5. **Access:** Open http://radar.local in your browser

See detailed instructions below for your chosen build system.

---

## Features

✨ **Complete TinyFrame Protocol Implementation**
- Full protocol parser according to V1.2 specification
- Proper big-endian header / little-endian data handling
- XOR checksum validation
- Support for all 13 message types

🎯 **3D Presence Detection**
- Target position tracking (X/Y/Z coordinates)
- Point cloud visualization
- Zone-based occupancy detection (4 zones)
- Motion detection and velocity tracking

🌐 **Web Interface**
- Real-time SSE (Server-Sent Events) streaming with ~100ms latency
- Interactive 3D visualization with Three.js
- **Live configuration interface** for sensor settings
- **3D zone visualization** with semi-transparent boundaries
- Responsive design with HUD overlay
- Supports multiple simultaneous viewers (up to 4 clients)
- Accessible via WiFi (http://radar.local or IP address)

⚙️ **Live Configuration Controls**
- Sensitivity adjustment (Low/Medium/High)
- Trigger speed control (Slow/Medium/Fast)
- Detection zone reset
- Interference zone management
- Auto-generation of interference zones
- Real-time feedback on configuration changes

📊 **Smart Logging**
- Movement-based filtering to reduce console spam
- Person entry/exit detection with duration tracking
- Periodic statistics reporting
- Configurable debug levels

## Hardware Requirements

- **ESP32-C3** (Seeed XIAO ESP32-C3 or compatible)
- **HLK-LD6002B-3D** radar sensor module
- **3.3V power supply** (≥1A capability)

## Wiring

| HLK-LD6002 Pin | ESP32 Pin | Description |
|---------------|-----------|-------------|
| Pin 1 (3V3) | 3.3V | Power supply (≥1A!) |
| Pin 2 (GND) | GND | Ground |
| Pin 3 (P19) | GND | BOOT1 (must be LOW!) |
| Pin 7 (TX0) | GPIO20 (D7) | UART TX → ESP32 RX |
| Pin 8 (RX0) | GPIO21 (D6) | UART RX → ESP32 TX |

⚠️ **CRITICAL:** Pin 3 (P19/BOOT1) must be connected to GND for proper operation!

## Software Setup

This project uses the **ESP-IDF framework** and can be built using:
- **ESP-IDF Native Tools** - Command-line build system (official Espressif toolchain)
- **PlatformIO** - VS Code integration that wraps ESP-IDF (simplified workflow)

Both methods use the same ESP-IDF framework, libraries, and compiler. PlatformIO simply provides a more user-friendly interface within VS Code.

### Option A: ESP-IDF Native Tools

#### 1. Install ESP-IDF

```bash
# Follow official ESP-IDF installation guide
https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/get-started/

# Or use the installer script
curl -LO https://github.com/espressif/esp-idf/releases/download/v5.1.2/esp-idf-installer-5.1.2-macos.dmg
```

#### 2. Configure WiFi Credentials

⚠️ **IMPORTANT:** You must create your WiFi credentials file before building!

```bash
# Copy the example file
cp src/wifi_credentials.h.example src/wifi_credentials.h

# Edit with your WiFi credentials
# Replace YOUR_WIFI_SSID_HERE and YOUR_WIFI_PASSWORD_HERE
nano src/wifi_credentials.h  # or use any text editor
```

The `wifi_credentials.h` file is in `.gitignore` and will never be committed to git, keeping your WiFi password secure.

#### 3. Build and Flash with ESP-IDF

```bash
# Set the target chip
idf.py set-target esp32c3

# Build the project
idf.py build

# Flash to device and open serial monitor
idf.py -p /dev/ttyUSB0 flash monitor

# Or just monitor if already flashed
idf.py -p /dev/ttyUSB0 monitor
```

**Note:** Replace `/dev/ttyUSB0` with your serial port:
- **macOS:** `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART`
- **Windows:** `COM3`, `COM4`, etc.
- **Linux:** `/dev/ttyUSB0`, `/dev/ttyACM0`, etc.

### Option B: PlatformIO (ESP-IDF via VS Code)

#### 1. Install PlatformIO

**VS Code Extension (Recommended):**
```bash
# Install VS Code
https://code.visualstudio.com/

# Install PlatformIO IDE extension from VS Code Marketplace
# Search for "PlatformIO IDE" and click Install
```

#### 2. Configure WiFi Credentials

⚠️ **IMPORTANT:** You must create your WiFi credentials file before building!

```bash
# Copy the example file
cp src/wifi_credentials.h.example src/wifi_credentials.h

# Edit with your WiFi credentials
# Replace YOUR_WIFI_SSID_HERE and YOUR_WIFI_PASSWORD_HERE
# You can use VS Code or any text editor
```

The `wifi_credentials.h` file is in `.gitignore` and will never be committed to git, keeping your WiFi password secure.

#### 3. Build and Flash with PlatformIO

**Using VS Code PlatformIO Extension:**
1. Open project folder in VS Code
2. Click PlatformIO icon in left sidebar
3. Under "Project Tasks" → "seeed_xiao_esp32c3":
   - Click **"Build"** to compile
   - Click **"Upload"** to flash
   - Click **"Monitor"** to view serial output
   - Or click **"Upload and Monitor"** to do both

**Using PlatformIO CLI:**
```bash
# Build the project
pio run

# Build and upload to device
pio run --target upload

# Build, upload, and open serial monitor
pio run --target upload --target monitor

# Just monitor (if already flashed)
pio device monitor
```

**Specify Serial Port (if needed):**
```bash
pio run --target upload --upload-port /dev/cu.usbserial-*
pio device monitor --port /dev/cu.usbserial-*
```

#### 4. PlatformIO Configuration

The project uses [`platformio.ini`](platformio.ini) configuration:

```ini
[env:seeed_xiao_esp32c3]
platform = espressif32      # ESP32 platform
board = seeed_xiao_esp32c3  # Seeed XIAO ESP32-C3
framework = espidf          # ESP-IDF framework
monitor_speed = 115200      # Serial monitor baud rate
```

**To add dependencies or customize build:**
```ini
# Example additions to platformio.ini
lib_deps =
    # Add libraries here if needed

build_flags =
    -DENABLE_WEB_INTERFACE=1
    # Add custom build flags

upload_speed = 921600  # Faster upload speed
```

## Web Interface Usage

### Accessing the Interface

Once the ESP32 connects to WiFi, access the web interface at:

- **By hostname:** http://radar.local
- **By IP:** http://[IP_ADDRESS] (shown in serial console)

### 3D View Controls

- **Mouse drag:** Rotate camera view
- **Mouse wheel:** Zoom in/out
- **Reset View:** Return camera to default position
- **Hide/Show Grid:** Toggle floor grid display
- **Hide/Show Zones:** Toggle detection zone wireframes
- **Trail Length Slider:** Adjust target trajectory history (0-100 steps)

### Configuration Panel

The live configuration panel allows real-time sensor adjustment without reflashing firmware:

#### Sensitivity Settings
- **Low:** Fewer false positives, stable detection for large rooms
- **Medium:** Balanced performance (default)
- **High:** Detects subtle movements, best for multi-person tracking

#### Trigger Speed Settings
- **Slow:** 2-3 second response time, minimal false triggers
- **Medium:** 1-2 second response (default)
- **Fast:** <1 second response, ideal for active environments

#### Zone Commands
- **Reset Detection:** Restore detection zones to factory defaults (full 3m range)
- **Clear Interference:** Remove all learned interference zones
- **Auto-Gen Interference:** Automatically learn and create interference zones
  - ⚠️ **Warning:** Ensure no people are in the detection area during this 30-60 second process

All configuration changes are:
- ✅ Sent via secure HTTP POST to `/config` endpoint
- ✅ Processed by command queue (thread-safe)
- ✅ Confirmed with visual feedback
- ✅ Applied immediately to the sensor
- ✅ Broadcast to all connected clients via SSE

### 3D Visualization

#### Target Display
- **Colored Spheres:** Detected targets with unique IDs
  - Each target gets a persistent color from a 10-color palette
  - 🏃 Bright/glowing: Moving target
  - 🧍 Dimmed: Stationary target
- **Trail Lines:** Smooth curved trajectories showing target movement history
  - Adjustable length (0-100 steps)
  - Color-matched to target

#### Zone Visualization
- **Detection Zones (Blue):**
  - Semi-transparent blue boxes showing 4 detection zones
  - Edges highlighted with wireframe
  - Labels display zone boundaries (X/Y/Z min/max in meters)
  - Turn green when occupied
- **Interference Zones (Orange):**
  - Semi-transparent orange boxes for learned interference areas
  - Only displayed if configured
  - Help reduce false positives from static objects

#### Scene Elements
- **3D Grid:** Floor plane with coordinate axes
  - X-axis (Red): Left (-) to Right (+)
  - Y-axis (Green): Front (+) to Back (-)
  - Z-axis (Blue): Ground (0) to Height (+)
- **Direction Arrow:** Red arrow labeled "FRONT" shows sensor orientation
- **HUD:** Real-time statistics
  - Connection status
  - Target count
  - Zone occupancy (4 zones)
  - Frame count and FPS

## Configuration

### Disable Web Interface

To disable WiFi and web interface for serial-only debugging:

Edit [`src/main.c`](src/main.c:25):

```c
#define ENABLE_WEB_INTERFACE 0  // Set to 0 to disable
```

### Sensor Commands

The firmware supports sending configuration commands to the sensor:

```c
// Examples (modify src/main.c in ld6002_reader_task)
send_control_command(CMD_SET_SENSITIVITY_HIGH);
send_control_command(CMD_SET_TRIGGER_SPEED_FAST);
send_control_command(CMD_ENABLE_POINT_CLOUD);
```

See [`src/main.c`](src/main.c:70-96) for all available commands.

## Protocol Details

### TinyFrame Structure

| Field | Size | Endianness | Description |
|-------|------|-----------|-------------|
| SOF | 1 byte | - | Start of Frame (0x01) |
| ID | 2 bytes | Big-endian | Frame ID |
| LEN | 2 bytes | Big-endian | Data length |
| TYPE | 2 bytes | Big-endian | Message type |
| HEAD_CKSUM | 1 byte | - | Header checksum (XOR + invert) |
| DATA | N bytes | Little-endian | Payload data |
| DATA_CKSUM | 1 byte | - | Data checksum (XOR + invert) |

### Supported Message Types

**Sensor Reports (Radar → Host):**
- `0x0A04` - Target positions (X/Y/Z coordinates)
- `0x0A08` - Point cloud data
- `0x0A0A` - Zone presence status
- `0x0A0B` - Interference zones
- `0x0A0C` - Detection zones
- `0x0A0D` - Hold delay time
- `0x0A0E` - Detection sensitivity
- `0x0A0F` - Trigger speed
- `0x0A10` - Z-axis range
- `0x0A11` - Installation method
- `0x0A12` - Low power mode status
- `0x0A13` - Low power sleep time
- `0x0A14` - Working mode

**Control Commands (Host → Radar):**
- `0x0201` - Control command (with 27 sub-commands)
- `0x0202` - Set area coordinates
- `0x0203` - Set hold delay time
- `0x0204` - Set Z-axis range
- `0x0205` - Set low power sleep time

## SSE (Server-Sent Events) Protocol

The web interface connects via SSE at `/events` endpoint using the EventSource API. SSE provides unidirectional server-to-client streaming with automatic reconnection and ~100ms latency, perfect for 20Hz radar data.

### Why SSE Instead of WebSockets?

**ESP-IDF native HTTP server does not support WebSockets.** SSE is the best alternative because:
- ✅ Built into ESP-IDF HTTP server
- ✅ Automatic reconnection handled by browser
- ✅ Lower overhead than WebSockets for one-way streaming
- ✅ Perfect for sensor data streaming (no need for client-to-server messages)
- ✅ Works through most firewalls and proxies

### Message Format (JSON over SSE)

SSE messages are sent as `data:` lines with JSON payloads:

**Target Update:**
```
data: {"type":"target","data":[{"x":-0.16,"y":-0.17,"z":0.43,"v":0,"c":1}]}

```

**Presence Update:**
```
data: {"type":"presence","data":[1,0,0,0]}

```

**Configuration:**
```
data: {"type":"config","data":{"sensitivity":1,"trigger_speed":2,"install_method":0}}

```

**JavaScript Client Example:**
```javascript
const eventSource = new EventSource('/events');

eventSource.onmessage = (event) => {
    const data = JSON.parse(event.data);
    if (data.type === 'target') {
        updateTargets(data.data);
    } else if (data.type === 'presence') {
        updatePresence(data.data);
    }
};

eventSource.onerror = () => {
    console.log('Connection lost, will auto-reconnect...');
};
```

## Memory Usage

Approximate memory footprint on ESP32-C3:

| Component | RAM | Flash |
|-----------|-----|-------|
| WiFi Stack | ~40 KB | ~200 KB |
| HTTP Server | ~30 KB | ~80 KB |
| SSE Streaming | ~10 KB | ~15 KB |
| Sensor Parser | ~15 KB | ~40 KB |
| HTML/CSS/JS | - | ~8 KB |
| **Total** | **~175 KB** | **~343 KB** |
| **Available** | ~270 KB | ~3.7 MB |
| **Remaining** | **~95 KB** | **~3.3 MB** |

✅ Plenty of memory available for expansion!

## Troubleshooting

### WiFi Credentials Not Found Error

If you see a build error about missing `wifi_credentials.h`:

```bash
fatal error: wifi_credentials.h: No such file or directory
```

**Solution:**
```bash
# Copy the example file to create your credentials file
cp src/wifi_credentials.h.example src/wifi_credentials.h

# Edit it with your WiFi network name and password
nano src/wifi_credentials.h  # or use any text editor
```

The `wifi_credentials.h` file must exist but is never committed to git (it's in `.gitignore`).

### WiFi Connection Issues

1. **Check credentials** in [`src/wifi_credentials.h`](src/wifi_credentials.h) - ensure SSID and password are correct
2. **Signal strength** - ensure ESP32 is within range
3. **2.4GHz network** - ESP32-C3 only supports 2.4GHz WiFi (not 5GHz)
4. **Serial output** shows connection status and errors
5. **Special characters** - if your WiFi password has special characters, ensure they're properly escaped in C strings

### Sensor Not Responding

1. **Check wiring** - especially BOOT1 (Pin 3) must be LOW (GND)
2. **Power supply** - must provide ≥1A at 3.3V
3. **Baud rate** - 115200 for 3D variant
4. **Serial monitor** shows "Received X bytes" if data is flowing

### Web Interface Not Loading

1. **Check WiFi connection** - serial log shows IP address
2. **Browser compatibility** - use modern browser with WebGL support
3. **Network firewall** - may block SSE connections (port 80)
4. **mDNS issues** - try IP address instead of radar.local
5. **Max clients reached** - only 4 simultaneous SSE clients supported

### Can't See 3D Visualization

1. **WebGL support** - check browser console for errors
2. **Three.js loading** - requires internet to load from CDN
3. **SSE connection** - check "Connection" status in HUD (should show "Connected")
4. **Sensor data** - verify sensor is detecting (check serial output)
5. **Browser console** - open DevTools and check for JavaScript errors

### Build Errors

**ESP-IDF:**
- **Missing IDF_PATH** - ensure ESP-IDF is properly installed and sourced
  ```bash
  . $HOME/esp/esp-idf/export.sh  # Run this before building
  ```
- **CMake errors** - delete `build/` folder and try again
  ```bash
  rm -rf build && idf.py build
  ```
- **Component errors** - check [`dependencies.lock`](dependencies.lock) is present
- **Python errors** - ensure you're using Python 3.8 or newer
  ```bash
  python --version  # Should be 3.8+
  ```

**PlatformIO:**
- **Platform not found** - ensure PlatformIO is up to date
  ```bash
  pio upgrade
  pio platform update espressif32
  ```
- **Compilation errors** - try cleaning the project
  ```bash
  pio run --target clean
  rm -rf .pio/build
  ```
- **Upload errors** - check serial port permissions
  ```bash
  # Linux/macOS - grant permission
  sudo chmod 666 /dev/ttyUSB0
  
  # Or add user to dialout group (Linux, requires logout/login)
  sudo usermod -a -G dialout $USER
  ```

For more detailed troubleshooting, see [`docs/troubleshooting.md`](docs/troubleshooting.md).

## Project Structure

```
hlk_ld6002/
├── src/
│   ├── main.c              # Main application + TinyFrame parser
│   ├── wifi_manager.h/c    # WiFi connection management
│   ├── web_server.h/c      # HTTP server + SSE streaming
│   └── CMakeLists.txt      # Source build configuration
├── docs/
│   ├── product-overview.md           # Product comparison & specifications
│   ├── tinyframe-protocol.md         # Complete TinyFrame protocol (LD6002B/C)
│   ├── hlk-ld6002-datasheet.md       # Base model datasheet (breath/heart)
│   ├── hlk-ld6002c-datasheet.md      # Fall detection datasheet
│   ├── troubleshooting.md            # Hardware and software troubleshooting
│   └── original/                     # Original Chinese PDF datasheets
├── lib/                    # External libraries (if any)
├── test/                   # Unit tests (if any)
├── include/                # Global headers (if any)
├── CMakeLists.txt          # ESP-IDF project configuration
├── platformio.ini          # PlatformIO project configuration
├── sdkconfig.defaults      # ESP-IDF default configuration
├── dependencies.lock       # ESP-IDF component dependencies
└── README.md               # This file
```

## Documentation

### Hardware & Product Information
- [`docs/product-overview.md`](docs/product-overview.md) - **START HERE** - Product comparison, specifications, and variant capabilities
- [`docs/hlk-ld6002-datasheet.md`](docs/hlk-ld6002-datasheet.md) - Base model datasheet (breath/heart rate detection)
- [`docs/hlk-ld6002c-datasheet.md`](docs/hlk-ld6002c-datasheet.md) - Fall detection variant datasheet

### Protocol & Communication
- [`docs/tinyframe-protocol.md`](docs/tinyframe-protocol.md) - **Complete TinyFrame protocol specification** for LD6002B (3D presence) and LD6002C (fall detection)

### Troubleshooting & Support
- [`docs/troubleshooting.md`](docs/troubleshooting.md) - Hardware wiring, baud rates, common issues

⚠️ **Important:** The LD6002, LD6002B, and LD6002C variants have **mutually exclusive capabilities**. See [`product-overview.md`](docs/product-overview.md) for details.

## Configuration API

### HTTP POST Endpoint

The web interface communicates with the ESP32 via HTTP POST to `/config`:

```javascript
// Example: Set sensitivity to high
fetch('/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ cmd: 'sensitivity', value: 'high' })
});

// Example: Reset detection zones
fetch('/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ cmd: 'reset_detection' })
});
```

### Supported Commands

| Command | Value Options | Description |
|---------|--------------|-------------|
| `sensitivity` | `low`, `medium`, `high` | Adjust detection sensitivity |
| `trigger_speed` | `slow`, `medium`, `fast` | Adjust trigger response time |
| `clear_interference` | - | Clear interference zones |
| `reset_detection` | - | Reset detection zones to defaults |
| `auto_interference` | - | Auto-generate interference zones |
| `get_zones` | - | Request current zone configuration |

### SSE Message Types

The server broadcasts these message types via SSE at `/events`:

```javascript
// Target positions
{"type":"target","data":[{"x":-0.16,"y":-0.17,"z":0.43,"v":0,"c":1},...]}

// Presence status (4 zones)
{"type":"presence","data":[1,0,0,0]}

// Configuration updates
{"type":"config","data":{"sensitivity":2,"trigger_speed":2,"install_method":1}}

// Detection zones (4 zones with boundaries)
{"type":"detection_zones","data":[{"x_min":-1.5,"x_max":1.5,"y_min":-1.5,"y_max":1.5,"z_min":0.0,"z_max":2.5},...]}

// Interference zones
{"type":"interference_zones","data":[...]}
```

## Future Enhancements

- [ ] Offline mode (embed Three.js instead of CDN)
- [ ] SPIFFS support for separate HTML file updates
- [x] **Bidirectional control commands** (adjust settings from web UI using POST requests) ✅
- [x] **3D zone visualization** with semi-transparent boundaries ✅
- [ ] Data logging to SD card
- [ ] MQTT support for Home Assistant integration
- [ ] Mobile-responsive controls
- [ ] Historical data visualization
- [ ] Multi-sensor support
- [ ] Custom zone boundary editor
- [ ] Installation method toggle (top/side mounted)

## License

This project is provided as-is for educational and development purposes.

## Credits

- **Hardware:** HLK-LD6002B-3D by Shenzhen Hi-Link Electronic Co., Ltd.
- **Visualization:** Three.js (https://threejs.org/)
- **Framework:** ESP-IDF by Espressif Systems

## Support

For issues or questions:
1. Check serial console output for error messages
2. Verify hardware connections and power supply
3. Review protocol documentation in `docs/` folder
4. Check ESP-IDF version compatibility

---

**Made with ❤️ for the ESP32 and radar sensor community**
