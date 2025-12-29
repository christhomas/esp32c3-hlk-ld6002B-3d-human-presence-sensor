# TinyFrame Protocol Reference - Complete

**Version:** V1.2
**Revision Date:** June 6, 2025
**Models:** HLK-LD6002B (2D/3D Presence) & HLK-LD6002C (Presence + Fall Detection)
**Company:** Shenzhen Hi-Link Electronic Co., Ltd.

## Table of Contents

1. [TF Frame Description](#tf-frame-description)
2. [Application Projects](#application-projects)
   - 2.1 [2D/3D Presence Detection (LD6002B & LD6002C)](#21-2d3d-presence-detection-ld6002b--ld6002c)
   - 2.2 [Fall Detection Extension (LD6002C Only)](#22-fall-detection-extension-ld6002c-only)
3. [Programming Interface](#programming-interface)
4. [Appendix](#appendix)

---

## 1. TF Frame Description

### 1.1 Overview

TinyFrame is used as the communication protocol in Hi-Link millimeter wave radar products. Data is transmitted through the UART interface with the following default settings:

- **Baud Rate:** 115200
- **Data Bits:** 8
- **Stop Bits:** 1
- **Parity:** None
- **Flow Control:** None

Each frame consists of a header and a payload. Both parts can be protected by checksums to ensure rejection of frames with malformed headers (e.g., corrupted length fields) or corrupted payloads.

The frame header contains the Frame ID and message type. The Frame ID increments with each new message. For two peers, the highest bit of the ID field is fixed at 1 and 0 to avoid conflicts.

The Frame ID can be reused in responses to link two messages together. The value of the Type field is described later.

### 1.2 Frame Structure

The fields in the millimeter wave radar frame are configured as follows:

| Field | Length (Bytes) | Format | Description |
|-------|---------------|--------|-------------|
| **SOF** | 1 | uint8 | Start of Frame, typically fixed at `0x01` |
| **ID** | 2 | uint16 | Frame ID, MSB peer bit, represents the sending packet sequence (increments from 0 to 65535) |
| **LEN** | 2 | uint16 | Data frame length, represents the number of DATA bytes (maximum 1024 bytes) |
| **TYPE** | 2 | uint16 | Message type |
| **HEAD_CKSUM** | 1 | uint8 | Header checksum using TF_CKSUM_XOR (XOR all bytes from SOF to TYPE, then invert) |
| **DATA** | N | - | Data field of length LEN |
| **DATA_CKSUM** | 1 | uint8 | Data checksum using TF_CKSUM_XOR (XOR all DATA bytes, then invert) |

### 1.3 Important Notes

#### TF Frame Byte Order

- **SOF to HEAD_CKSUM:** Big-endian (high byte first, low byte last)
- **DATA_CKSUM high byte:** Big-endian
- **DATA field:** Little-endian (low byte first, high byte last)

**Example:**
- DATA type `uint32` with value `0x12345678` → transmitted as `0x78 0x56 0x34 0x12` (little-endian)
- ID type `uint16` with value `0x1234` → transmitted as `0x12 0x34` (big-endian)

#### Checksum Overflow

If `HEAD_CKSUM` or `DATA_CKSUM` exceeds 1 byte after calculation, only the lowest byte is used.  
**Example:** If `HEAD_CKSUM` = `0x1232`, only `0x32` is used.

#### Special Notes

1. **Acknowledgment:** When the lower device receives any TF frame command, it first replies with the same TYPE but without DATA to acknowledge receipt. If no reply is received, resend the configuration message.

2. **Low Power Mode Wake-up:** When low power mode is enabled and the device is sleeping, pull the RX0 pin of UART0 low to wake it, or send a configuration message to wake it. Confirm success by checking for a reply.

3. **Normal Mode Duration:** After receiving data or waking up, the device operates in normal mode for 10 seconds. If no person is detected after 10 seconds, it re-enters low power mode.

---

## 2. Application Projects

### 2.1 2D/3D Presence Detection (LD6002B & LD6002C)

#### 2.1.1 Message Type: Control Command `0x0201`

**Direction:** Host → Radar  
**Message Name:** `MSG_CFG_HUMAN_DETECTION_3D`

**Frame Structure:**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 04` | Data length = 4 |
| TYPE | 2 | uint16 | `02 01` | Frame type |
| HEAD_CKSUM | 1 | uint8 | `F9` | Header checksum |
| DATA | 4 | int32 | `01 00 00 00` | [command] |
| DATA_CKSUM | 1 | uint8 | `FE` | Data checksum |

**Command Values:**

| Command | Description |
|---------|-------------|
| `0x01` | Auto-generate interference zone |
| `0x02` | Get interference and detection zones |
| `0x03` | Clear interference zone |
| `0x04` | Reset detection zone |
| `0x05` | Get hold delay time |
| `0x06` | Enable point cloud display |
| `0x07` | Disable point cloud display |
| `0x08` | Enable target display |
| `0x09` | Disable target display |
| `0x0A` | Set detection sensitivity to Low |
| `0x0B` | Set detection sensitivity to Medium |
| `0x0C` | Set detection sensitivity to High |
| `0x0D` | Get detection sensitivity status |
| `0x0E` | Set trigger speed to Slow |
| `0x0F` | Set trigger speed to Medium |
| `0x10` | Set trigger speed to Fast |
| `0x11` | Get trigger speed status |
| `0x12` | Get Z-axis range (3D only) |
| `0x13` | Set installation method to Top-mounted (3D only) |
| `0x14` | Set installation method to Side-mounted (3D only) |
| `0x15` | Get installation method |
| `0x16` | Enable low power mode when no person detected |
| `0x17` | Disable low power mode when no person detected |
| `0x18` | Get low power mode status |
| `0x19` | Get low power mode sleep time |
| `0x1A` | Reset no-person state |

#### 2.1.2 Message Type: Set Area Coordinates `0x0202`

**Direction:** Host → Radar  
**Message Name:** `MSG_CFG_HUMAN_DETECTION_3D_AREA`

**Frame Structure:**

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | Start of frame |
| ID | 2 | uint16 | Frame ID |
| LEN | 2 | uint16 | `00 1C` (28 bytes) |
| TYPE | 2 | uint16 | `02 02` |
| HEAD_CKSUM | 1 | uint8 | Header checksum |
| DATA[0] | 4 | int32 | [area_id] - Zone ID (0-3: interference, 4-7: detection) |
| DATA[4] | 4 | float | [x_min] - Min X coordinate (meters) |
| DATA[8] | 4 | float | [x_max] - Max X coordinate (meters) |
| DATA[12] | 4 | float | [y_min] - Min Y coordinate (meters) |
| DATA[16] | 4 | float | [y_max] - Max Y coordinate (meters) |
| DATA[20] | 4 | float | [z_min] - Min Z coordinate (meters) |
| DATA[24] | 4 | float | [z_max] - Max Z coordinate (meters) |
| DATA_CKSUM | 1 | uint8 | Data checksum |

**Notes:**
- Total of 4 interference zones (ID 0-3) and 4 detection zones (ID 4-7)
- Only one zone can be set at a time
- For 2D mode: Set `z_min` = `-6.0m` and `z_max` = `6.0m`

#### 2.1.3 Message Type: Set Hold Delay Time `0x0203`

**Direction:** Host → Radar  
**Message Name:** `MSG_CFG_HUMAN_DETECTION_3D_PWM_DELAY`

**Frame Structure:**

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | `01` |
| ID | 2 | uint16 | `00 00` |
| LEN | 2 | uint16 | `00 04` |
| TYPE | 2 | uint16 | `02 03` |
| HEAD_CKSUM | 1 | uint8 | `FB` |
| DATA | 4 | uint32 | [pwm_delay] - Hold delay time in seconds |
| DATA_CKSUM | 1 | uint8 | Checksum |

**Default:** 30 seconds

#### 2.1.4 Message Type: Set Z-Axis Range `0x0204`

**Direction:** Host → Radar  
**Message Name:** `MSG_CFG_HUMAN_DETECTION_3D_Z`

**Frame Structure:**

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | `01` |
| ID | 2 | uint16 | `00 00` |
| LEN | 2 | uint16 | `00 08` |
| TYPE | 2 | uint16 | `02 04` |
| HEAD_CKSUM | 1 | uint8 | `FC` |
| DATA[0] | 4 | float | [z_min] - Min Z coordinate (meters) |
| DATA[4] | 4 | float | [z_max] - Max Z coordinate (meters) |
| DATA_CKSUM | 1 | uint8 | Checksum |

**Note:** This command is only applicable to 3D mode.

#### 2.1.5 Message Type: Set Low Power Mode Sleep Time `0x0205`

**Direction:** Host → Radar  
**Message Name:** `MSG_CFG_HUMAN_DETECTION_3D_LOW_POWER_MODE_TIME`

**Frame Structure:**

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | `01` |
| ID | 2 | uint16 | `00 00` |
| LEN | 2 | uint16 | `00 04` |
| TYPE | 2 | uint16 | `02 05` |
| HEAD_CKSUM | 1 | uint8 | `FD` |
| DATA | 4 | uint32 | [waitingPeriod] - Sleep time in milliseconds |
| DATA_CKSUM | 1 | uint8 | Checksum |

**Default:** 500 ms

#### 2.1.6 Message Type: Report Person Position `0x0A04` / `0x0A08`

**Direction:** Radar → Host  
**Message Name:** `MSG_IND_HUMAN_DETECTION_3D_TGT_RES`

- `0x0A04` = Target data
- `0x0A08` = Point cloud data

**Frame Structure:**

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | `01` |
| ID | 2 | uint16 | Frame ID |
| LEN | 2 | uint16 | Variable length |
| TYPE | 2 | uint16 | `0A 04` |
| HEAD_CKSUM | 1 | uint8 | Checksum |
| DATA[0] | 4 | int32 | [target_num] - Number of targets |
| DATA[4] | 4 | float | [x] - X coordinate (meters) |
| DATA[8] | 4 | float | [y] - Y coordinate (meters) |
| DATA[12] | 4 | float | [z] - Z coordinate (meters) |
| DATA[16] | 4 | int32 | [dop_idx] - Velocity index |
| DATA[20] | 4 | int32 | [cluster_id] - Cluster target ID |
| ... | ... | ... | Repeat for each target |
| DATA_CKSUM | 1 | uint8 | Checksum |

**Notes:**
- When N targets exist, x, y, z, dop_idx, and cluster_id repeat N times
- In 2D mode, Z-axis output is 0

#### 2.1.7 Message Type: Report 3D Point Cloud `0x0A08`

**Direction:** Radar → Host (auto-uploaded when UserLog enabled)  
**Message Name:** `MSG_IND_3D_CLOUD_RES`

**Frame Structure:**

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | `01` |
| ID | 2 | uint16 | Frame ID |
| LEN | 2 | uint16 | Variable |
| TYPE | 2 | uint16 | `0A 08` |
| HEAD_CKSUM | 1 | uint8 | Checksum |
| DATA[0] | 4 | int32 | [target_num] - Number of points |
| DATA[4] | 4 | int32 | [cluster_index] - Cluster ID |
| DATA[8] | 4 | float | [x_point] - X coordinate (m) |
| DATA[12] | 4 | float | [y_point] - Y coordinate (m) |
| DATA[16] | 4 | float | [z_point] - Z coordinate (m) |
| DATA[20] | 4 | float | [speed] - Speed (m/s) |
| ... | ... | ... | Repeat for each point |
| DATA_CKSUM | 1 | uint8 | Checksum |

**Note:** Points with similar distances share the same cluster_id.

#### 2.1.8 Message Type: Report Presence in Zone `0x0A0A`

**Direction:** Radar → Host  
**Message Name:** `MSG_IND_HUMAN_DETECTION_3D_RES`

**Frame Structure:**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 10` | 16 bytes |
| TYPE | 2 | uint16 | `0A 0A` | Message type |
| HEAD_CKSUM | 1 | uint8 | `FA` | Checksum |
| DATA[0] | 4 | uint32 | `01 00 00 00` | [detection_state_area0] - Zone 0 (1=occupied, 0=empty) |
| DATA[4] | 4 | uint32 | `01 00 00 00` | [detection_state_area1] - Zone 1 |
| DATA[8] | 4 | uint32 | `00 00 00 00` | [detection_state_area2] - Zone 2 |
| DATA[12] | 4 | uint32 | `01 00 00 00` | [detection_state_area3] - Zone 3 |
| DATA_CKSUM | 1 | uint8 | `FE` | Checksum |

Reports presence status for 4 detection zones: 1 = person detected, 0 = no person.

#### 2.1.9 Message Type: Report Zone Coordinates `0x0A0B` / `0x0A0C`

**Direction:** Radar → Host  
**Message Names:**
- `0x0A0B` = Interference zones
- `0x0A0C` = Detection zones

**Frame Structure:**

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | `01` |
| ID | 2 | uint16 | Frame ID |
| LEN | 2 | uint16 | Variable (96 bytes for 4 zones) |
| TYPE | 2 | uint16 | `0A 0B` or `0A 0C` |
| HEAD_CKSUM | 1 | uint8 | Checksum |
| DATA[0-23] | 24 | 6×float | Zone 0: x_min, x_max, y_min, y_max, z_min, z_max |
| DATA[24-47] | 24 | 6×float | Zone 1: x_min, x_max, y_min, y_max, z_min, z_max |
| DATA[48-71] | 24 | 6×float | Zone 2: x_min, x_max, y_min, y_max, z_min, z_max |
| DATA[72-95] | 24 | 6×float | Zone 3: x_min, x_max, y_min, y_max, z_min, z_max |
| DATA_CKSUM | 1 | uint8 | Checksum |

**Notes:**
- Sent in response to command `0x0201` with parameter `0x02`
- `0x0A0B` reports 4 interference zones
- `0x0A0C` reports 4 detection zones
- In 2D mode: z_min = -6m, z_max = 6m

#### 2.1.10 Message Type: Report Hold Delay Time `0x0A0D`

**Direction:** Radar → Host  
**Message Name:** `MSG_IND_HUMAN_DETECTION_3D_PWM_DELAY`

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start |
| ID | 2 | uint16 | `00 01` | Frame ID |
| LEN | 2 | uint16 | `00 04` | 4 bytes |
| TYPE | 2 | uint16 | `0A 0D` | Message type |
| HEAD_CKSUM | 1 | uint8 | `FC` | Checksum |
| DATA | 4 | uint32 | `05 00 00 00` | [pwmDelayTimer] - Delay in seconds |
| DATA_CKSUM | 1 | uint8 | `FA` | Checksum |

#### 2.1.11 Message Type: Report Detection Sensitivity `0x0A0E`

**Direction:** Radar → Host  
**Message Name:** `MSG_IND_HUMAN_DETECTION_3D_DETECT_SENSITIVITY`

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start |
| ID | 2 | uint16 | `00 01` | Frame ID |
| LEN | 2 | uint16 | `00 01` | 1 byte |
| TYPE | 2 | uint16 | `0A 0E` | Message type |
| HEAD_CKSUM | 1 | uint8 | `FF` | Checksum |
| DATA | 1 | uint8 | `01` | [detectSensitivity] - 0=Low, 1=Medium, 2=High |
| DATA_CKSUM | 1 | uint8 | `FE` | Checksum |

#### 2.1.12 Message Type: Report Trigger Speed `0x0A0F`

**Direction:** Radar → Host  
**Message Name:** `MSG_IND_HUMAN_DETECTION_3D_DETECT_TRIGGER`

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start |
| ID | 2 | uint16 | `00 01` | Frame ID |
| LEN | 2 | uint16 | `00 01` | 1 byte |
| TYPE | 2 | uint16 | `0A 0F` | Message type |
| HEAD_CKSUM | 1 | uint8 | `FE` | Checksum |
| DATA | 1 | uint8 | `02` | [detectTrigger] - 0=Slow, 1=Medium, 2=Fast |
| DATA_CKSUM | 1 | uint8 | `FD` | Checksum |

#### 2.1.13 Message Type: Report Z-Axis Range `0x0A10`

**Direction:** Radar → Host (3D only)  
**Message Name:** `MSG_IND_HUMAN_DETECTION_3D_DETECT_TRIGGER`

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | `01` |
| ID | 2 | uint16 | Frame ID |
| LEN | 2 | uint16 | `00 08` |
| TYPE | 2 | uint16 | `0A 10` |
| HEAD_CKSUM | 1 | uint8 | `E1` |
| DATA[0] | 4 | float | [z_min] - Min Z (meters) |
| DATA[4] | 4 | float | [z_max] - Max Z (meters) |
| DATA_CKSUM | 1 | uint8 | Checksum |

#### 2.1.14 Message Type: Report Installation Method `0x0A11`

**Direction:** Radar → Host  
**Message Name:** `MSG_IND_HUMAN_DETECTION_3D_INSTALL_SITE`

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | `01` |
| ID | 2 | uint16 | Frame ID |
| LEN | 2 | uint16 | `00 01` |
| TYPE | 2 | uint16 | `0A 11` |
| HEAD_CKSUM | 1 | uint8 | Checksum |
| DATA | 1 | uint8 | [installSite] - 0=Top-mounted, 1=Side-mounted |
| DATA_CKSUM | 1 | uint8 | Checksum |

#### 2.1.15 Message Type: Report Low Power Mode Status `0x0A12`

**Direction:** Radar → Host  
**Message Name:** `MSG_IND_HUMAN_DETECTION_3D_LOW_POWER_MODE`

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | `01` |
| ID | 2 | uint16 | Frame ID |
| LEN | 2 | uint16 | `00 01` |
| TYPE | 2 | uint16 | `0A 12` |
| HEAD_CKSUM | 1 | uint8 | Checksum |
| DATA | 1 | uint8 | [lowPowerMode] - 0=Disabled, 1=Enabled |
| DATA_CKSUM | 1 | uint8 | Checksum |

#### 2.1.16 Message Type: Report Low Power Sleep Time `0x0A13`

**Direction:** Radar → Host  
**Message Name:** `MSG_IND_HUMAN_DETECTION_3D_LOW_POWER_TIME`

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | `01` |
| ID | 2 | uint16 | Frame ID |
| LEN | 2 | uint16 | `00 04` |
| TYPE | 2 | uint16 | `0A 13` |
| HEAD_CKSUM | 1 | uint8 | Checksum |
| DATA | 4 | uint32 | [waitingPeriod] - Sleep time in milliseconds |
| DATA_CKSUM | 1 | uint8 | Checksum |

#### 2.1.17 Message Type: Report Working Mode `0x0A14`

**Direction:** Radar → Host  
**Message Name:** `MSG_IND_HUMAN_DETECTION_3D_MODE`

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | `01` |
| ID | 2 | uint16 | Frame ID |
| LEN | 2 | uint16 | `00 01` |
| TYPE | 2 | uint16 | `0A 14` |
| HEAD_CKSUM | 1 | uint8 | Checksum |
| DATA | 1 | uint8 | [detectStateMessager] - 0=Low power mode, 1=Normal mode |
| DATA_CKSUM | 1 | uint8 | Checksum |

**Note:** This message is sent when switching between low power and normal modes.

---

### 2.2 Fall Detection Extension (LD6002C Only)

The **LD6002C** extends the base TinyFrame protocol with specialized fall detection messages. These messages work **in addition to** the standard presence detection messages (section 2.1).

#### Message Type Ranges

| Range | Purpose | Direction |
|-------|---------|-----------|
| `0x0Exx` | Fall detection specific | Bidirectional |
| `0x21xx` | System initialization | Host → Radar |

#### 2.2.1 Message Type: Control User Log `0x0E01`

**Direction:** Host → Radar
**Message Name:** `MSG_CFG_USER_LOG`
**Transfer Mode:** One-way (command only)

Enables or disables user log information output (point cloud and status data).

**Frame Structure:**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 04` | Data length = 4 bytes |
| TYPE | 2 | uint16 | `0E 01` | Message type |
| HEAD_CKSUM | 1 | uint8 | `FB` | Header checksum |
| DATA | 4 | uint32 | `01 00 00 00` | **[value]** - Enable/Disable |
| DATA_CKSUM | 1 | uint8 | `FE` | Data checksum |

**Control Values:**

| Value | Action | Description |
|-------|--------|-------------|
| `0x00000000` | Disable | Turn off user log output |
| `0x00000001` | Enable | Turn on user log output |

#### 2.2.2 Message Type: Report Fall Status `0x0E02`

**Direction:** Radar → Host
**Message Name:** `MSG_IND_FALL_STATUS`
**Transfer Mode:** One-way (automatic reporting)

Reports real-time fall detection status.

**Frame Structure:**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 01` | Data length = 1 byte |
| TYPE | 2 | uint16 | `0E 02` | Message type |
| HEAD_CKSUM | 1 | uint8 | `F3` | Header checksum |
| DATA | 1 | uint8 | `01` | **[is_fall]** - Fall status |
| DATA_CKSUM | 1 | uint8 | `FE` | Data checksum |

**Fall Status Values:**

| Value | Status | Description |
|-------|--------|-------------|
| `0x00` | Normal | No fall detected |
| `0x01` | Fall | Fall event detected |

#### 2.2.3 Message Type: Set Installation Height `0x0E04`

**Direction:** Bidirectional
**Message Name:** `MSG_IND_FALL_SET_HIGH`
**Transfer Mode:** Command-Response

Configures the radar installation height for accurate fall detection.

**Command Frame (Host → Radar):**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 04` | Data length = 4 bytes |
| TYPE | 2 | uint16 | `0E 04` | Message type |
| HEAD_CKSUM | 1 | uint8 | `F0` | Header checksum |
| DATA | 4 | float | `00 00 20 40` | **[high]** - Installation height (meters) |
| DATA_CKSUM | 1 | uint8 | `9F` | Data checksum |

**Height Parameter:**
- Range: 1.0 - 5.0 meters
- Data Type: float (little-endian)
- Example: `0x40200000` = 2.5 meters

**Response Frame (Radar → Host):**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 01` | Data length = 1 byte |
| TYPE | 2 | uint16 | `0E 04` | Message type |
| HEAD_CKSUM | 1 | uint8 | `F5` | Header checksum |
| DATA | 1 | uint8 | `01` | **[result]** - 0=Failed, 1=Success |
| DATA_CKSUM | 1 | uint8 | `FE` | Data checksum |

#### 2.2.4 Message Type: Get Radar Parameters `0x0E06`

**Direction:** Bidirectional
**Message Name:** `MSG_IND_FALL_GET_HIGH`
**Transfer Mode:** Query-Response

Retrieves all current radar configuration parameters.

**Query Frame (Host → Radar):**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 00` | Data length = 0 (no data) |
| TYPE | 2 | uint16 | `0E 06` | Message type |
| HEAD_CKSUM | 1 | uint8 | `F6` | Header checksum |

**Response Frame (Radar → Host):**

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| SOF | 1 | uint8 | Start of frame (`0x01`) |
| ID | 2 | uint16 | Frame ID |
| LEN | 2 | uint16 | Data length (`00 1C` = 28 bytes) |
| TYPE | 2 | uint16 | Message type (`0E 06`) |
| HEAD_CKSUM | 1 | uint8 | Header checksum |
| DATA[0] | 4 | float | **[high]** - Installation height (meters) |
| DATA[4] | 4 | float | **[threshold]** - Fall threshold (meters) |
| DATA[8] | 4 | uint32 | **[sensitivity]** - Fall sensitivity (3-30) |
| DATA[12] | 4 | float | **[rect_XL]** - Detection zone left boundary (meters) |
| DATA[16] | 4 | float | **[rect_XR]** - Detection zone right boundary (meters) |
| DATA[20] | 4 | float | **[rect_ZF]** - Detection zone front boundary (meters) |
| DATA[24] | 4 | float | **[rect_ZB]** - Detection zone back boundary (meters) |
| DATA_CKSUM | 1 | uint8 | Data checksum |

**Parameter Descriptions:**

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| **high** | float | 1.0 - 5.0m | 2.2m | Radar installation height from floor |
| **threshold** | float | 0.3 - 1.5m | 0.6m | Height below which person is considered fallen |
| **sensitivity** | uint32 | 3 - 30 | 10 | Number of frames averaged (lower = faster, more false alarms) |
| **rect_XL** | float | 0.3 - 1.5m | 0.5m | Detection zone left edge distance |
| **rect_XR** | float | 0.3 - 1.5m | 0.5m | Detection zone right edge distance |
| **rect_ZF** | float | 0.3 - 1.5m | 0.5m | Detection zone front edge distance |
| **rect_ZB** | float | 0.3 - 1.5m | 0.5m | Detection zone back edge distance |

#### 2.2.5 Message Type: Set Fall Threshold `0x0E08`

**Direction:** Bidirectional
**Message Name:** `MSG_IND_FALL_SET_THRESHOLD`
**Transfer Mode:** Command-Response

Configures the height threshold below which a person is considered to have fallen.

**Command Frame (Host → Radar):**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 04` | Data length = 4 bytes |
| TYPE | 2 | uint16 | `0E 08` | Message type |
| HEAD_CKSUM | 1 | uint8 | `FC` | Header checksum |
| DATA | 4 | float | `9A 99 19 3F` | **[set_threshold]** - Threshold height (meters) |
| DATA_CKSUM | 1 | uint8 | `DA` | Data checksum |

**Threshold Parameter:**
- Range: 0.3 - 1.5 meters
- Default: 0.6 meters
- Data Type: float (little-endian)

**Response Frame (Radar → Host):**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 01` | Data length = 1 byte |
| TYPE | 2 | uint16 | `0E 08` | Message type |
| HEAD_CKSUM | 1 | uint8 | `F7` | Header checksum |
| DATA | 1 | uint8 | `01` | **[value]** - 0=Failed, 1=Success |
| DATA_CKSUM | 1 | uint8 | `FE` | Data checksum |

#### 2.2.6 Message Type: Set Fall Sensitivity `0x0E0A`

**Direction:** Bidirectional
**Message Name:** `MSG_IND_FALL_SET_SENSITIVITY`
**Transfer Mode:** Command-Response

Configures the fall detection sensitivity by setting the number of frames to average.

**Command Frame (Host → Radar):**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 04` | Data length = 4 bytes |
| TYPE | 2 | uint16 | `0E 0A` | Message type |
| HEAD_CKSUM | 1 | uint8 | `FB` | Header checksum |
| DATA | 4 | uint32 | `03 00 00 00` | **[sensitivity]** - Sensitivity value |
| DATA_CKSUM | 1 | uint8 | `FC` | Data checksum |

**Sensitivity Parameter:**
- Range: 3 - 30
- Default: 10
- Data Type: uint32 (little-endian)
- Meaning: Number of frames to average for fall detection

**Sensitivity Trade-offs:**

| Value | Response Time | False Alarm Rate | Recommended Use |
|-------|--------------|------------------|-----------------|
| 3-5 | Fast (3-8s) | Higher | Quick response needed |
| 10-15 | Medium (15-20s) | Balanced | General use (recommended) |
| 20-30 | Slow (30-45s) | Lower | Minimize false alarms |

**Response Frame (Radar → Host):**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 01` | Data length = 1 byte |
| TYPE | 2 | uint16 | `0E 0A` | Message type |
| HEAD_CKSUM | 1 | uint8 | `FB` | Header checksum |
| DATA | 1 | uint8 | `01` | **[value]** - 0=Failed, 1=Success |
| DATA_CKSUM | 1 | uint8 | `FE` | Data checksum |

#### 2.2.7 Message Type: Set Alarm Area Parameters `0x0E0C`

**Direction:** Bidirectional
**Message Name:** `MSG_CFG_FALL_ALARM_AREA`
**Transfer Mode:** Command-Response

Configures the detection zone boundaries for fall alarm triggering.

**Command Frame (Host → Radar):**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 10` | Data length = 16 bytes |
| TYPE | 2 | uint16 | `0E 0C` | Message type |
| HEAD_CKSUM | 1 | uint8 | `68` | Header checksum |
| DATA[0] | 4 | float | `00 00 00 3F` | **[rect_XL]** - Left boundary (meters) |
| DATA[4] | 4 | float | `00 00 00 3F` | **[rect_XR]** - Right boundary (meters) |
| DATA[8] | 4 | float | `00 00 00 3F` | **[rect_ZF]** - Front boundary (meters) |
| DATA[12] | 4 | float | `00 00 00 3F` | **[rect_ZB]** - Back boundary (meters) |
| DATA_CKSUM | 1 | uint8 | `FF` | Data checksum |

**Zone Parameters:**
- Range: 0.3 - 1.5 meters for each boundary
- Coordinate System:
  - XL: Left edge distance from center
  - XR: Right edge distance from center
  - ZF: Front edge distance from radar
  - ZB: Back edge distance from radar

**Zone Diagram:**
```
         ZF (Front)
          |
 XL ------+------ XR
(Left)    |      (Right)
          |
         ZB (Back)
```

**Example:** Setting 0.5m boundaries creates a 1.0m × 1.0m detection zone centered under the radar.

**Response Frame (Radar → Host):**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 01` | Data length = 1 byte |
| TYPE | 2 | uint16 | `0E 0C` | Message type |
| HEAD_CKSUM | 1 | uint8 | `FD` | Header checksum |
| DATA | 1 | uint8 | `01` | **[value]** - 0=Failed, 1=Success |
| DATA_CKSUM | 1 | uint8 | `FE` | Data checksum |

#### 2.2.8 Message Type: Report Height Information `0x0E0E`

**Direction:** Radar → Host
**Message Name:** `MSG_IND_FALL_HEIGHT_INFO`
**Transfer Mode:** One-way (automatic reporting)

Automatically reports the current height of detected targets.

**Frame Structure:**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 04` | Data length = 4 bytes |
| TYPE | 2 | uint16 | `0E 0E` | Message type |
| HEAD_CKSUM | 1 | uint8 | `F8` | Header checksum |
| DATA | 4 | uint32 | `01 00 00 00` | **[height]** - Current target height |
| DATA_CKSUM | 1 | uint8 | `FA` | Data checksum |

#### 2.2.9 Message Type: Radar Initialization `0x2110`

**Direction:** Host → Radar
**Message Name:** `MSG_CFG_RADAR_INIT`
**Transfer Mode:** One-way (command only)

Resets the radar to default factory settings for fall detection.

**Frame Structure:**

| Field | Bytes | Type | Example | Description |
|-------|-------|------|---------|-------------|
| SOF | 1 | uint8 | `01` | Start of frame |
| ID | 2 | uint16 | `00 00` | Frame ID |
| LEN | 2 | uint16 | `00 00` | Data length = 0 (no data) |
| TYPE | 2 | uint16 | `21 10` | Message type |
| HEAD_CKSUM | 1 | uint8 | `CF` | Header checksum |
| DATA_CKSUM | 1 | uint8 | `FF` | Data checksum (no data) |

**Default Parameters After Initialization:**

| Parameter | Default Value | Unit |
|-----------|---------------|------|
| Installation Height | 2.2 | meters |
| Fall Threshold | 0.5 | meters |
| Fall Sensitivity | 10 | frames |
| Detection Zone Left (XL) | 0.5 | meters |
| Detection Zone Right (XR) | 0.5 | meters |
| Detection Zone Front (ZF) | 0.5 | meters |
| Detection Zone Back (ZB) | 0.5 | meters |

**Note:** These default settings create a 1.0m × 1.0m detection zone centered under the radar at 2.2m installation height.

**Related Message:** The default values correspond to the parameter structure returned by message type [`0x0E06`](#33-get-radar-parameters-0x0e06).

---

## 3. Programming Interface

### 3.1 Encoding TF Messages

```c
void tinyFramefTx(TF_TYPE type, uint8 *data, TF_LEN len);
```

**Parameters:**
- `type`: Message type (uint16), e.g., `0x0A10` for person detection result report
- `data`: Pointer to data to send (uint8*)
- `len`: Length of data to send (uint16)

**Example: Enable User Log**
```c
uint8_t enable_log[] = {0x01, 0x00, 0x00, 0x00};  // uint32 = 1, little-endian
tinyFramefTx(0x0E01, enable_log, 4);
```

**Example: Set Fall Threshold to 0.6m**
```c
// 0.6 as float = 0x3F19999A
uint8_t threshold[] = {0x9A, 0x99, 0x19, 0x3F};  // little-endian
tinyFramefTx(0x0E08, threshold, 4);
```

### 3.2 Decoding TF Messages

```c
TinyFrameRx tinyFramefRx(void);
```

After successfully receiving a message, the data is returned in a `TinyFrameRx` type variable.

### 3.3 Float Conversion Helper

**Converting bytes to float:**

```c
float bytes_to_float(uint8_t *bytes) {
    // Combine bytes in little-endian order
    uint32_t value = (bytes[3] << 24) | (bytes[2] << 16) | 
                     (bytes[1] << 8) | bytes[0];
    return *(float*)&value;
}

// Usage example:
uint8_t data[] = {0x9A, 0x99, 0x19, 0x3F};
float threshold = bytes_to_float(data);  // Result: 0.6
```

**Converting float to bytes:**

```c
void float_to_bytes(float value, uint8_t *bytes) {
    uint32_t temp = *(uint32_t*)&value;
    bytes[0] = temp & 0xFF;          // Low byte
    bytes[1] = (temp >> 8) & 0xFF;
    bytes[2] = (temp >> 16) & 0xFF;
    bytes[3] = (temp >> 24) & 0xFF;  // High byte
}

// Usage example:
uint8_t buffer[4];
float_to_bytes(2.5, buffer);  // Converts 2.5m height to bytes
```

### 3.4 Checksum Calculation

Both `HEAD_CKSUM` and `DATA_CKSUM` use the XOR + invert algorithm:

```c
unsigned char getCksum(unsigned char *data, unsigned char len) {
    unsigned char ret = 0;
    
    for (int i = 0; i < len; i++)
        ret = ret ^ data[i];
    
    ret = ~ret;  // Invert all bits
    
    return ret;
}
```

**Example: Calculate header checksum**
```c
uint8_t header[] = {0x01, 0x00, 0x00, 0x00, 0x04, 0x0E, 0x02};
uint8_t head_cksum = getCksum(header, 7);  // From SOF to TYPE
```

### 3.5 Complete Example: Query Radar Parameters

```c
// Build query frame for message 0x0E06
void query_radar_parameters(void) {
    uint8_t frame[] = {
        0x01,              // SOF
        0x00, 0x00,        // ID = 0
        0x00, 0x00,        // LEN = 0 (no data)
        0x0E, 0x06,        // TYPE = 0x0E06
        0xF6               // HEAD_CKSUM (calculated)
    };
    
    uart_write(frame, sizeof(frame));
}

// Parse response
void parse_parameters(uint8_t *rx_buffer) {
    // Verify message type
    uint16_t msg_type = (rx_buffer[5] << 8) | rx_buffer[6];
    if (msg_type != 0x0E06) return;
    
    // Extract parameters (all little-endian)
    uint8_t *data = &rx_buffer[8];
    
    float height = bytes_to_float(&data[0]);        // Installation height
    float threshold = bytes_to_float(&data[4]);     // Fall threshold
    uint32_t sensitivity = data[8] | (data[9] << 8) | 
                          (data[10] << 16) | (data[11] << 24);
    float rect_XL = bytes_to_float(&data[12]);
    float rect_XR = bytes_to_float(&data[16]);
    float rect_ZF = bytes_to_float(&data[20]);
    float rect_ZB = bytes_to_float(&data[24]);
    
    printf("Height: %.2fm, Threshold: %.2fm, Sensitivity: %d\n",
           height, threshold, sensitivity);
    printf("Zone: XL=%.2f, XR=%.2f, ZF=%.2f, ZB=%.2f\n",
           rect_XL, rect_XR, rect_ZF, rect_ZB);
}
```

---

#### A. Extracting DATA Field

**Example: Converting int32 hex to float**

If `[x_point]` bytes are `0x66 0x66 0xA2 0x41`:
1. Combine into uint32 (little-endian): `0x41A26666`
2. Type-cast to float: `20.3`

```c
int main(void)
{
    unsigned int param = 0x41A26666;
    float res = *(float*)&param;
    
    printf("data: %f\n", res);  // Output: 20.3
    return 0;
}
```

#### B. Calculating Checksums

**HEAD_CKSUM:** XOR all bytes from SOF to the byte before HEAD_CKSUM, then invert.  
**DATA_CKSUM:** XOR all DATA bytes, then invert.

```c
unsigned char getCksum(unsigned char *data, unsigned char len)
{
    unsigned char ret = 0;
    
    for (int i = 0; i < len; i++)
        ret = ret ^ data[i];
    
    ret = ~ret;
    
    return ret;
}
```

### 3.3 Demo Code

For complete TF frame parsing demos (including Linux environment, Keil μVision5 environment C language demos, and Python language demos), please contact sales directly.

---

## 4. Appendix

### Revision History

| Version | Revision Scope | Date |
|---------|---------------|------|
| V1.0 | GUI description update, added module direction identification, power diagram | September 18, 2024 |
| V1.1 | Corrected checksum values | November 12, 2024 |
| V1.2 | Fixed documentation errors | June 6, 2025 |

---

## Quick Reference: Message Type Summary

### Commands (Host → Radar)

| Type | Name | Description |
|------|------|-------------|
| `0x0201` | Control Command | Execute various control operations |
| `0x0202` | Set Area | Configure interference/detection zones |
| `0x0203` | Set Delay | Configure hold delay time |
| `0x0204` | Set Z-Axis | Configure Z-axis range (3D only) |
| `0x0205` | Set Sleep Time | Configure low power sleep time |

### Reports (Radar → Host)

| Type | Name | Description |
|------|------|-------------|
| `0x0A04` | Target Position | Person position (target data) |
| `0x0A08` | Point Cloud | 3D point cloud data |
| `0x0A0A` | Presence Status | Occupancy status for 4 zones |
| `0x0A0B` | Interference Zones | Coordinates of interference zones |
| `0x0A0C` | Detection Zones | Coordinates of detection zones |
| `0x0A0D` | Delay Time | Current hold delay time |
| `0x0A0E` | Sensitivity | Current detection sensitivity |
| `0x0A0F` | Trigger Speed | Current trigger speed |
| `0x0A10` | Z-Axis Range | Current Z-axis range (3D) |
| `0x0A11` | Installation | Installation method (top/side) |
| `0x0A12` | Low Power Mode | Low power mode status |
| `0x0A13` | Sleep Time | Low power sleep duration |
| `0x0A14` | Working Mode | Current working mode |

---

### Fall Detection Commands (Host → Radar) - LD6002C Only

| Type | Name | Description |
|------|------|-------------|
| `0x0E01` | User Log Control | Enable/disable log output |
| `0x0E04` | Set Height | Set installation height (1-5m) |
| `0x0E06` | Get Parameters | Query all fall detection parameters |
| `0x0E08` | Set Threshold | Set fall threshold (0.3-1.5m) |
| `0x0E0A` | Set Sensitivity | Set detection sensitivity (3-30 frames) |
| `0x0E0C` | Set Alarm Area | Configure detection zone boundaries |
| `0x2110` | Initialize Radar | Reset to factory defaults |

### Fall Detection Reports (Radar → Host) - LD6002C Only

| Type | Name | Description |
|------|------|-------------|
| `0x0E02` | Fall Status | Report fall detection event (0=normal, 1=fall) |
| `0x0E04` | Height Response | Confirm height setting (0=failed, 1=success) |
| `0x0E06` | Parameter Response | Return all configuration parameters |
| `0x0E08` | Threshold Response | Confirm threshold setting |
| `0x0E0A` | Sensitivity Response | Confirm sensitivity setting |
| `0x0E0C` | Area Response | Confirm alarm area setting |
| `0x0E0E` | Height Info | Report target height information |


---

**Contact:** stella@hlktech.com  
**Copyright © Shenzhen Hi-Link Electronic Co., Ltd.**