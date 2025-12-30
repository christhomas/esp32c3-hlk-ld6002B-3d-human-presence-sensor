# HLK-LD6002B-3D Sensor Configuration Guide

**For 3D Presence Detection and Position Tracking**

This guide explains how to configure the HLK-LD6002B-3D radar sensor for optimal multi-target detection.

---

## Table of Contents

1. [Detection Range and Specifications](#detection-range-and-specifications)
2. [Sensitivity Settings](#sensitivity-settings)
3. [Trigger Speed Settings](#trigger-speed-settings)
4. [Detection Zones Configuration](#detection-zones-configuration)
5. [Multi-Target Detection](#multi-target-detection)
6. [Troubleshooting](#troubleshooting)
7. [Command Reference](#command-reference)

---

## Detection Range and Specifications

### Range Capabilities

| Parameter | Value |
|-----------|-------|
| **Maximum Detection Range** | 3 meters |
| **Minimum Detection Range** | ~0.4 meters |
| **Detection Angle** | ±60° horizontal, ±60° vertical |
| **Refresh Rate** | ~20 Hz (50ms per frame) |
| **Position Output** | 3D coordinates (X, Y, Z in meters) |

### Important Notes

⚠️ **Detection range is affected by:**
- Target RCS (Radar Cross Section) - larger targets detected further
- Environmental factors (walls, furniture, metal objects)
- Radar mounting position and orientation
- Current sensitivity settings

📏 **Coordinate System:**
- **X-axis**: Left (-) to Right (+)
- **Y-axis**: Front (+) to Back (-)
- **Z-axis**: Ground (0) to Height (+)

---

## Sensitivity Settings

Sensitivity controls how easily the radar detects targets.

### Available Levels

| Level | Code | Use Case | Pros | Cons |
|-------|------|----------|------|------|
| **Low** | `0x0A` | Large rooms, far distances | Less false positives, stable | May miss small movements, slower response |
| **Medium** | `0x0B` | General purpose (DEFAULT) | Balanced performance | Moderate sensitivity |
| **High** | `0x0C` | Small rooms, precise tracking | Detects subtle movements, faster response | More false positives, may detect unwanted objects |

### Current Setting

The firmware currently requests the sensitivity setting at startup with:
```c
send_control_command(CMD_GET_SENSITIVITY);  // 0x0D
```

### Changing Sensitivity

**To change sensitivity, modify [`src/main.c`](../src/main.c) in the `ld6002_reader_task` function:**

```c
// Around line 626-636, add one of these BEFORE the GET commands:

// For HIGH sensitivity (best for multiple person detection):
send_control_command(CMD_SET_SENSITIVITY_HIGH);      // 0x0C
vTaskDelay(pdMS_TO_TICKS(200));

// For MEDIUM sensitivity (balanced):
send_control_command(CMD_SET_SENSITIVITY_MEDIUM);    // 0x0B
vTaskDelay(pdMS_TO_TICKS(200));

// For LOW sensitivity (fewer false positives):
send_control_command(CMD_SET_SENSITIVITY_LOW);       // 0x0A
vTaskDelay(pdMS_TO_TICKS(200));
```

**Recommended for multi-person detection: HIGH sensitivity**

---

## Trigger Speed Settings

Trigger speed controls how quickly the radar responds to movement.

### Available Levels

| Level | Code | Response Time | Use Case |
|-------|------|---------------|----------|
| **Slow** | `0x0E` | ~2-3 seconds | Environments with minimal movement, reduce false triggers |
| **Medium** | `0x0F` | ~1-2 seconds | General purpose (DEFAULT) |
| **Fast** | `0x10` | <1 second | Quick response needed, active environments with multiple people |

### Current Setting

The firmware requests the trigger speed at startup:
```c
send_control_command(CMD_GET_TRIGGER_SPEED);  // 0x11
```

### Changing Trigger Speed

**To change trigger speed, modify [`src/main.c`](../src/main.c):**

```c
// Add one of these BEFORE the GET commands:

// For FAST trigger (best for active multi-person scenarios):
send_control_command(CMD_SET_TRIGGER_SPEED_FAST);    // 0x10
vTaskDelay(pdMS_TO_TICKS(200));

// For MEDIUM trigger:
send_control_command(CMD_SET_TRIGGER_SPEED_MEDIUM);  // 0x0F
vTaskDelay(pdMS_TO_TICKS(200));

// For SLOW trigger:
send_control_command(CMD_SET_TRIGGER_SPEED_SLOW);    // 0x0E
vTaskDelay(pdMS_TO_TICKS(200));
```

**Recommended for multi-person detection: FAST trigger speed**

---

## Detection Zones Configuration

The sensor supports 4 detection zones and 4 interference zones.

### Zone Types

1. **Detection Zones** - Areas where presence is actively monitored
2. **Interference Zones** - Areas to ignore (reduce false positives from static objects)

### Viewing Current Zones

The firmware requests zone information at startup:
```c
send_control_command(CMD_GET_ZONES);  // 0x02
```

This displays zones in the console output like:
```
📍 Detection Zones:
  Zone 0: X[-1.5 to 1.5] Y[-1.5 to 1.5] Z[0.0 to 2.5]m
  Zone 1: X[-3.0 to 3.0] Y[-3.0 to 3.0] Z[0.0 to 2.5]m
  ...
```

### Resetting Zones

To reset to factory default zones:
```c
// Reset detection zones to default
send_control_command(CMD_RESET_DETECTION_ZONE);  // 0x04
vTaskDelay(pdMS_TO_TICKS(500));

// Clear interference zones
send_control_command(CMD_CLEAR_INTERFERENCE_ZONE);  // 0x03
vTaskDelay(pdMS_TO_TICKS(500));
```

### Auto-Generate Interference Zones

The sensor can automatically learn and create interference zones:
```c
// Auto-generate interference zones (runs for ~30 seconds)
send_control_command(CMD_AUTO_GEN_INTERFERENCE_ZONE);  // 0x01
```

⚠️ **During auto-generation:**
- Ensure no people are in the detection area
- Keep the environment static
- Wait 30-60 seconds for learning to complete

---

## Multi-Target Detection

### Maximum Targets

The current implementation tracks up to **10 simultaneous targets** (defined by `MAX_TARGETS` in [`src/web_server.h`](../src/web_server.h)).

### Why Multi-Target Detection May Fail

If you're only seeing one target when multiple people are present:

#### 1. Sensitivity Too Low
- **Solution**: Set sensitivity to HIGH
- **Command**: `CMD_SET_SENSITIVITY_HIGH` (0x0C)

#### 2. Trigger Speed Too Slow
- **Solution**: Set trigger speed to FAST
- **Command**: `CMD_SET_TRIGGER_SPEED_FAST` (0x10)

#### 3. Targets Too Close Together
- **Issue**: When targets are within ~0.5m of each other, the radar may merge them
- **Solution**: Targets need to maintain >0.5m separation

#### 4. One Target Moving, One Stationary
- **Issue**: Radar may prioritize moving targets over stationary ones
- **Solution**: Ensure both targets are moving slightly

#### 5. Detection Zones Too Restrictive
- **Issue**: Zones may exclude parts of the area
- **Solution**: Reset zones with `CMD_RESET_DETECTION_ZONE`

#### 6. Interference Zones Blocking Targets
- **Issue**: Auto-generated interference zones may block real targets
- **Solution**: Clear interference zones with `CMD_CLEAR_INTERFERENCE_ZONE`

### Optimal Configuration for Multi-Person Detection

```c
// In ld6002_reader_task(), after "Initializing sensor...", add:

// 1. Clear any interference zones
send_control_command(CMD_CLEAR_INTERFERENCE_ZONE);
vTaskDelay(pdMS_TO_TICKS(200));

// 2. Reset detection zones to default (full 3m range)
send_control_command(CMD_RESET_DETECTION_ZONE);
vTaskDelay(pdMS_TO_TICKS(200));

// 3. Set HIGH sensitivity
send_control_command(CMD_SET_SENSITIVITY_HIGH);
vTaskDelay(pdMS_TO_TICKS(200));

// 4. Set FAST trigger speed
send_control_command(CMD_SET_TRIGGER_SPEED_FAST);
vTaskDelay(pdMS_TO_TICKS(200));

// 5. Enable target display (already done)
send_control_command(CMD_ENABLE_TARGET_DISPLAY);
vTaskDelay(pdMS_TO_TICKS(200));

// 6. Request current settings
send_control_command(CMD_GET_SENSITIVITY);
vTaskDelay(pdMS_TO_TICKS(100));
send_control_command(CMD_GET_TRIGGER_SPEED);
vTaskDelay(pdMS_TO_TICKS(100));
send_control_command(CMD_GET_ZONES);
vTaskDelay(pdMS_TO_TICKS(100));
```

---

## Troubleshooting

### Problem: Only One Target Detected When Two People Present

**Check these in order:**

1. **Console Output**
   - Look for: `🎯 X Targets detected:` in serial monitor
   - If showing > 1 but web shows 1: Frontend issue (check browser console)
   - If showing 1: Sensor configuration issue

2. **Test Target Separation**
   - Move people >1m apart
   - Ensure both are moving (wave arms, walk slowly)
   - Check if radar now sees 2 targets

3. **Increase Sensitivity**
   ```c
   send_control_command(CMD_SET_SENSITIVITY_HIGH);
   ```

4. **Increase Trigger Speed**
   ```c
   send_control_command(CMD_SET_TRIGGER_SPEED_FAST);
   ```

5. **Reset Configuration**
   ```c
   send_control_command(CMD_CLEAR_INTERFERENCE_ZONE);
   send_control_command(CMD_RESET_DETECTION_ZONE);
   ```

### Problem: Too Many False Targets

**Solutions:**

1. **Lower Sensitivity**
   ```c
   send_control_command(CMD_SET_SENSITIVITY_MEDIUM);
   // or
   send_control_command(CMD_SET_SENSITIVITY_LOW);
   ```

2. **Slower Trigger Speed**
   ```c
   send_control_command(CMD_SET_TRIGGER_SPEED_MEDIUM);
   ```

3. **Auto-Generate Interference Zones**
   - Remove all people from the area
   - Run: `send_control_command(CMD_AUTO_GEN_INTERFERENCE_ZONE);`
   - Wait 30-60 seconds

### Problem: Targets Merged in Frontend

This was fixed in the codebase with spatial proximity matching. If still occurring:

1. **Check TARGET_MATCH_DISTANCE**
   - In [`src/webapp.js`](../src/webapp.js) line 4
   - Current: `0.5` meters
   - Increase to `0.7` or `1.0` if targets are further apart

2. **Clear Browser Cache**
   - Hard refresh: Ctrl+Shift+R (Windows/Linux) or Cmd+Shift+R (Mac)

---

## Command Reference

### Complete Command List

| Command | Code | Description |
|---------|------|-------------|
| **Target Display** | | |
| Enable Target Display | 0x08 | Enable position tracking output |
| Disable Target Display | 0x09 | Disable position tracking output |
| **Point Cloud** | | |
| Enable Point Cloud | 0x06 | Enable point cloud data output |
| Disable Point Cloud | 0x07 | Disable point cloud data output |
| **Sensitivity** | | |
| Set Low | 0x0A | Reduce sensitivity |
| Set Medium | 0x0B | Default sensitivity |
| Set High | 0x0C | Maximum sensitivity |
| Get Current | 0x0D | Query current sensitivity |
| **Trigger Speed** | | |
| Set Slow | 0x0E | Slower response (~2-3s) |
| Set Medium | 0x0F | Default speed (~1-2s) |
| Set Fast | 0x10 | Fast response (<1s) |
| Get Current | 0x11 | Query current trigger speed |
| **Zones** | | |
| Auto-Generate Interference | 0x01 | Learn static objects (30-60s) |
| Get Zones | 0x02 | Query all zone configurations |
| Clear Interference Zones | 0x03 | Remove learned interference |
| Reset Detection Zones | 0x04 | Reset to factory default |
| Get Hold Delay | 0x05 | Query presence hold delay |
| **Installation** | | |
| Set Top-Mounted | 0x13 | Ceiling installation mode |
| Set Side-Mounted | 0x14 | Wall installation mode |
| Get Install Method | 0x15 | Query installation mode |
| **Advanced** | | |
| Get Z-Axis Range | 0x12 | Query vertical detection range |
| Enable Low Power Mode | 0x16 | Enable power saving |
| Disable Low Power Mode | 0x17 | Disable power saving |
| Get Low Power Mode | 0x18 | Query power mode status |
| Get Low Power Sleep Time | 0x19 | Query sleep duration |
| Reset No Person State | 0x1A | Clear presence state |

### Adding Commands to Firmware

Edit [`src/main.c`](../src/main.c), function `ld6002_reader_task()`, around line 626:

```c
// Add commands here (after "Initializing sensor...")
send_control_command(COMMAND_CODE);
vTaskDelay(pdMS_TO_TICKS(200));  // Wait for response
```

---

## Installation Mode

The sensor supports two installation modes:

### Side-Mounted (Default)
- Radar on wall at ~1-1.5m height
- Faces horizontally into room
- Best for: Rooms, hallways, doorways

### Top-Mounted
- Radar on ceiling (2.2-3.0m height)
- Faces downward
- Best for: Small enclosed spaces, bathrooms

**To change installation mode:**
```c
// For ceiling installation
send_control_command(CMD_SET_INSTALL_TOP_MOUNTED);  // 0x13

// For wall installation
send_control_command(CMD_SET_INSTALL_SIDE_MOUNTED); // 0x14
```

---

## Recommended Settings by Scenario

### Living Room (Multi-Person Tracking)
```c
send_control_command(CMD_CLEAR_INTERFERENCE_ZONE);
send_control_command(CMD_RESET_DETECTION_ZONE);
send_control_command(CMD_SET_SENSITIVITY_HIGH);
send_control_command(CMD_SET_TRIGGER_SPEED_FAST);
send_control_command(CMD_SET_INSTALL_SIDE_MOUNTED);
```

### Office/Study (Single Person)
```c
send_control_command(CMD_SET_SENSITIVITY_MEDIUM);
send_control_command(CMD_SET_TRIGGER_SPEED_MEDIUM);
send_control_command(CMD_SET_INSTALL_SIDE_MOUNTED);
```

### Bathroom (Small Space)
```c
send_control_command(CMD_SET_SENSITIVITY_HIGH);
send_control_command(CMD_SET_TRIGGER_SPEED_FAST);
send_control_command(CMD_SET_INSTALL_TOP_MOUNTED);
```

---

## References

- **Main Firmware:** [`src/main.c`](../src/main.c)
- **Protocol Documentation:** [`tinyframe-protocol.md`](tinyframe-protocol.md)
- **Product Overview:** [`product-overview.md`](product-overview.md)
- **Troubleshooting:** [`troubleshooting.md`](troubleshooting.md)

---

**Last Updated:** December 30, 2024  
**Firmware Version:** 1.0
