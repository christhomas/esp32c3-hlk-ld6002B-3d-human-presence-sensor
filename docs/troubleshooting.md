# HLK-LD6002 Series Troubleshooting Guide

## Hardware Inventory

**Sensors Available:**
- HLK-LD6002B - 3D presence detection variant
- HLK-LD6002C - Fall detection variant

**Note:** These are different from the base HLK-LD6002 (respiratory and heart rate detection) model.

For product comparison, see [`product-overview.md`](product-overview.md).

## CRITICAL FINDINGS FROM DATASHEETS

### HLK-LD6002 (Breath/Heart Rate Detection)
**Default Baud Rate: 1382400** (confirmed in Chinese datasheet: "串口默认波特率为1382400")

See [`hlk-ld6002-datasheet.md`](hlk-ld6002-datasheet.md) for complete specifications.

### HLK-LD6002B & HLK-LD6002C (3D Presence & Fall Detection)
**Default Baud Rate: 115200**

See [`tinyframe-protocol.md`](tinyframe-protocol.md) for complete TinyFrame protocol specifications.

**IMPORTANT WIRING:**
The sensor has TWO UART ports:
- **UART0 (Pins 7/8)**: TX0/RX0 - **USE THESE** for data output
- **UART2 (Pins 4/5)**: TX2/RX2 - These are GPIO20, not for main output

**Correct Connection:**
```
HLK-LD6002 Pin 7 (TX0) → ESP32 RX (GPIO20/D7)
HLK-LD6002 Pin 8 (RX0) → ESP32 TX (GPIO21/D6)
HLK-LD6002 Pin 3 (P19) → GND (BOOT1, must be LOW)
HLK-LD6002 Pin 1 (3V3) → 3.3V (≥1A supply!)
HLK-LD6002 Pin 2 (GND) → GND
```

**Power Requirements:**
- Voltage: 3.2-3.4V (NOT 3.3V typical!)
- Current: ≥1A
- Ripple: ≤50mV
- If using DCDC: switching frequency ≥2MHz

---

# HLK-LD6002 Troubleshooting (Original)

## Current Status

The ESP32 is successfully receiving data from the HLK-LD6002-3D sensor, but we cannot decode it using the standard protocol. This indicates a configuration or hardware issue.

## Baud Rate Testing Results

| Baud Rate | Result |
|-----------|--------|
| 1382400 | All zeros - no valid signal |
| 460800 | Receives data but no 0x01 frame markers |
| 256000 | **Receives valid-looking data** (e.g., `06 3e 06 3e 00 c0 06 38`) but NO 0x01 frame markers |
| 230400 | Receives data but no 0x01 frame markers |
| 115200 | **Finds 0x01 bytes** but all frames corrupted (wrong lengths/checksums) |

## Analysis

### Most Promising: 256000 baud
At 256000 baud, we received data that looked structured:
```
06 3e 06 3e 00 c0 06 38 00 f8 38 00 00 00 00 00
```

This doesn't match the TinyFrame protocol (no 0x01 start byte), suggesting:
1. **Sensor is in raw data mode** - outputting unprocessed radar samples
2. **Different protocol variant** - your specific model uses different framing
3. **Needs initialization** - sensor requires setup commands before protocol mode

### Hardware Configuration

**Critical**: P19 pin must be pulled LOW for protocol mode (confirmed grounded).

**Wiring** (Seeed XIAO ESP32-C3):
```
Sensor P19 → GND
Sensor TX → GPIO20 (D7)
Sensor RX → GPIO21 (D6)
Sensor VCC → 5V
Sensor GND → GND
```

## Possible Issues

### 1. Wrong Sensor Model
- You have HLK-LD6002-**3D** (3D positioning radar)
- Arduino library is for HLK-LD6002 (respiratory/heartbeat)
- They may use different protocols despite similar names

### 2. Sensor Not in Protocol Mode
Even with P19 grounded, the sensor might need:
- Power cycle after P19 connection
- Initialization command via UART
- Different pin configuration (check for P20, P21, etc.)

### 3. Non-Standard Baud Rate
The sensor might be configured for a custom baud rate like:
- 691200 (half of 1382400)
- 9600 (old default)
- Something manufacturer-specific

## Recommendations

### Option 1: Contact Manufacturer
Ask HiLink for:
1. Confirmation of protocol for HL-LD6002-**3D** specifically
2. Default baud rate and any initialization sequence
3. Pin configuration requirements beyond P19

### Option 2: Try Raw Data Interpretation
At 256000 baud, analyze the raw byte stream:
- Look for repeating patterns
- Check if bytes correlate with distance (move object near sensor)
- May be outputting raw I/Q samples or range bins

### Option 3: Send Configuration Commands
Try sending these hex commands at 256000 baud to enable protocol mode:
```c
// Examples (need manufacturer docs):
uart_write_bytes(port, "\xFF\xAA\x01", 3);  // Enable protocol mode?
uart_write_bytes(port, "\xFD\xFC\xFB\xFA", 4);  // Reset?
```

### Option 4: Check for Other Models
Verify your sensor is actually HLK-LD6002-3D:
- Check PCB markings
- Verify part number on IC
- Compare with datasheet photos

## Code Status

Current implementation in `src/main.c`:
- ✅ Correct GPIO pins (20/21)
- ✅ Protocol parser ready (TinyFrame with checksums)
- ✅ Comprehensive debugging
- ❌ Cannot decode data (baud rate or protocol mismatch)

The parser will work correctly once the proper baud rate and protocol mode are determined.

## Quick Test Commands

Try these in `src/main.c` at line 28:

```c
// Test baud rate 1: 256000 (was receiving data)
#define HLK_LD6002_BAUDRATE 256000

// Test baud rate 2: 9600 (old default)
#define HLK_LD6002_BAUDRATE 9600

// Test baud rate 3: 691200 (half of 1382400)
#define HLK_LD6002_BAUDRATE 691200
```

For each test, check:
1. Are bytes being received? (look for "Received X bytes total")
2. Any 0x01 frame markers found?
3. Do frame lengths look reasonable (4-12 bytes)?
