# LD6002C-60G Fall Detection Radar Module

**Technical Specification**  
**Version:** V1.1  
**Date:** June 28, 2024  
**Company:** Shenzhen Hi-Link Electronic Co., Ltd.

---

## Table of Contents

1. [Product Introduction](#product-introduction)
2. [Product Features](#product-features)
3. [Application Scenarios](#application-scenarios)
4. [Electrical Characteristics and Parameters](#electrical-characteristics-and-parameters)
5. [Hardware Description](#hardware-description)
6. [Usage and Configuration](#usage-and-configuration)
7. [Important Notes](#important-notes)
8. [Radar Radome Design](#radar-radome-design)
9. [Revision History](#revision-history)

---

## 1. Product Introduction

The LD6002C is a radar sensing module developed based on a high-performance domestic chip. It integrates:

- **57~64GHz RF transceiver system**
- **2T2R PCB microstrip antenna**
- **1MB Flash memory**
- **Radar signal processing unit**
- **ARM® Cortex®-M3 core**

This module is based on the FMCW (Frequency Modulated Continuous Wave) signal processing mechanism. Combined with radar signal processing algorithms, it achieves personnel status perception in specific locations and timely reports presence/absence information and fall status.

**Ideal for:** Ceiling installation in bathrooms, restrooms, and small single-person areas.

---

## 2. Product Features

- **FMCW Technology:** Based on Frequency Modulated Continuous Wave radar detection
- **Presence Detection:** Real-time human presence sensing
- **Fall Detection:** Accurate fall event detection
- **Universal UART Interface:** Provides communication protocol for data transmission
- **Parameter Adjustment:** UART-based parameter configuration to meet different scenario requirements
- **Compact Size:** Only 25×23mm, supports pin header connection
- **Environmental Immunity:** Unaffected by temperature, humidity, noise, airflow, dust, or lighting conditions

---

## 3. Application Scenarios

### Healthcare Monitoring
Bathrooms and restrooms in nursing facilities

### Home Security
Residential fall detection and safety monitoring

### Smart Home
Whole-house intelligent presence and safety systems

---

## 4. Electrical Characteristics and Parameters

### 4.1 Functional Parameters

| Parameter | Content | Min | Typical | Max | Unit |
|-----------|---------|-----|---------|-----|------|
| Fall monitoring detection range | 0.6×0.6 | - | 3×3 | - | m |
| Presence detection accuracy | - | - | 95 | - | % |
| Fall detection accuracy | - | - | 90 | - | % |

### 4.2 Electrical Characteristics

| Operational Parameter | Min | Typical | Max | Unit |
|----------------------|-----|---------|-----|------|
| Operating voltage (VCC) | 3.1 | 3.3 | 3.5 | V |
| Operating current (ICC) | 120 | - | 600 | mA |
| Operating temperature (TOP) | -20 | - | 85 | °C |
| Storage temperature (TST) | -40 | - | 85 | °C |

### 4.3 RF Characteristics

| Operational Parameter | Min | Typical | Max | Unit |
|----------------------|-----|---------|-----|------|
| Operating frequency | 58 | - | 62 | GHz |
| Transmit power (EIRP) | - | 12 | - | dBm |
| Antenna gain | - | 4 | - | dBi |
| Horizontal beamwidth (-3dB) | -60 | - | 60 | ° |
| Vertical beamwidth (-3dB) | -60 | - | 60 | ° |

---

## 5. Hardware Description

### 5.1 Dimensions

**Module Size:** 25mm × 23mm

### 5.2 Pin Definition

| Pin # | Pin Name | Description | Notes |
|-------|----------|-------------|-------|
| 1 | 3V3 | POWER INPUT | 3.3V power supply |
| 2 | GND | Ground | Ground connection |
| 3 | P19 | GPIO19 | Boot1 configuration |
| 4 | TX2 | GPIO20 | Fall detection output |
| 5 | AIO1 | Analog IO | Analog input/output |
| 6 | SCL0 | GPIO07 | I2C clock or GPIO |
| 7 | TX0 | UART TX | Connect to external serial TX |
| 8 | RX0 | UART RX | Connect to external serial RX |

**Note:** TX0/RX0 labels on module are from the module's perspective.

### 5.3 Module Peripheral Reference Design

**Power Supply:**
- Input voltage: 3.3V
- Power supply capability: >1A
- Low noise design recommended

**Communication Interface:**
- UART0: Default baud rate 115200, no parity
- I/O voltage level: 3.3V

**Digital Output:**
- TX2 pin: Outputs fall detection status
  - High level: Fall detected
  - Low level: No fall/No person

### 5.4 Boot Configuration

| BOOT1 | BOOT0 | Description |
|-------|-------|-------------|
| 0 | 1 | Boot from internal Flash |
| Pin Location | Pin 3 (P19) | - |

**Note:** Both BOOT1 and BOOT0 have internal pull-ups. BOOT1 must be pulled low before module startup.

---

## 6. Usage and Configuration

### 6.1 Typical Application Circuit

The LD6002C module provides two output methods:

1. **Digital Output (TX2 pin):**
   - Outputs detected target information
   - Fall detected: High level
   - No fall/No person: Low level

2. **UART0 Output:**
   - Follows specified protocol
   - Outputs detection results including fall information
   - Users can flexibly use based on application scenario

**Power Requirements:**
- Module power supply: 3.3V
- Input power capability: >1A
- Module I/O output voltage: 3.3V

**Communication Settings:**
- Default baud rate: 115200
- Parity: None

### 6.2 GUI Visualization Tool Application

#### 1. Device Connection

1. Select the corresponding serial port number
2. Set baud rate to **115200**
3. Click **[Connect]** button, module starts detection

#### 2. Data Viewing

1. Select **[3D view]** window
2. Drag and zoom the view to appropriate size using mouse (or press **H** key in English input mode for auto-zoom)
3. Check the **SystemLog** window below

#### 3. Fall Detection Status

**Normal Operation:**
- No fall detected: Displays "No abnormality detected"

**Fall Detected:**
- 3D View: Displays "Someone fell" text
- SystemLog window: Shows Chinese message "有人跌倒了！" (Someone has fallen!)

#### 4. Parameter Configuration

Select **Option** window → **[Fall]** button to enter parameter configuration interface:

##### Configuration Parameters

| Parameter | Description | Notes |
|-----------|-------------|-------|
| **MountingHeight** | Mounting height | Set according to actual installation height |
| **FallThreshold** | Fall height threshold | Determines fall based on point cloud distance from ground. Adjust according to usage scenario |
| **FallSensitivity** | Sensitivity setting | Range: 3-30, Default: 10. Lower values = faster reporting but higher false alarm rate. Default 10 = 15-20s continuous fall before reporting |
| **DetectRectXL** | Detection zone left boundary | X-axis left limit |
| **DetectRectXR** | Detection zone right boundary | X-axis right limit |
| **DetectRectZF** | Detection zone front boundary | Z-axis front limit |
| **DetectRectZB** | Detection zone back boundary | Z-axis back limit |

##### Configuration Steps

1. Fill in the desired parameter value for each configuration item
2. Click the corresponding **[Apply]** button to complete configuration
3. Confirm in the lower left **[Fall]** window that the parameter matches the set value
4. **[GetAllFallParas]** button: Read parameters set in the module
5. **Version** window → **[Get]** button: Obtain current firmware version number

##### User Log Control

**Open User Log:**
- Click **[Open userLog]** button
- Outputs target point cloud information
- 3D view shows point cloud distribution
- User log window displays:
  - Presence status (English string)
  - Fall status (English string)

**Close User Log:**
- Click **[Close userLog]** button
- Stops log display and point cloud data output

### 6.3 OTA Upgrade

Please refer to "Andar OTA Upgrade Tool User Manual_V1.0" document (for Hi-Link engineering development personnel only)

### 6.4 Installation Method and Sensing Range

#### Top-Mounted Installation

**Installation Height:** 2.2m - 3.0m  
**Maximum Sensing Range:** 3m × 3m

#### Installation Direction Labels

```
        ZF (Front)
         |
XL ------+------ XR
(Left)   |      (Right)
         |
        ZB (Back)
```

#### Installation Diagram

- Mount on ceiling directly above detection area
- Radar faces downward
- Height: H = 2.2m - 3.0m
- Coverage area: 3m × 3m at floor level

---

## 7. Important Notes

### 1. Detection Distance Variations
The radar module's detection distance is closely related to target RCS (Radar Cross Section) and environmental factors. Effective detection distance may vary with environment and target changes. Therefore, fluctuations in effective detection distance within a certain range are normal.

### 2. Power Supply Requirements
The radar module has extremely high power supply requirements:
- **Input voltage:** 3.1~3.5V
- **Power supply ripple:** ≤50mV
- **Current capability:** ≥1A
- **If using DC-DC power supply:** Switching frequency must not be lower than 2MHz

### 3. Application Limitations
This product is only suitable for:
- Small area scenarios (bathrooms, restrooms)
- Single-person situations

### 4. Interference Filtering
The product needs to be combined with application scenarios to filter out interference actions and movements.

---

## 8. Radar Radome Design

The radar radome protects the radar antenna from external environmental influences such as rain, sunlight, and wind. However, it affects the radar antenna in the following ways:

- **Dielectric and reflection losses** reduce effective radar power
- **Beam distortion** changes the radar coverage area
- **Shell reflections** degrade transmit-receive antenna isolation and may cause receiver saturation
- **Phase changes** affect angle measurement accuracy

Therefore, proper radome design is essential to minimize impact and improve radar performance.

### Design Requirements

#### 1. Material Selection

When choosing radome materials, while ensuring structural strength and low cost, select materials with the smallest dielectric constant and loss tangent to minimize impact on radar performance.

**Common Material Dielectric Constants and Dissipation Factors:**

| Material | Dielectric Constant (εr) | Dissipation Factor (tan δ) |
|----------|-------------------------|---------------------------|
| Polycarbonate | 2.9 | 0.012 |
| ABS | 2.0-3.5 | 0.0050-0.019 |
| PEEK | 3.2 | 0.0048 |
| PTFE (Teflon®) | 2.0 | <0.0002 |
| Plexiglass® | 2.6 | 0.009 |
| Glass | 5.75 | 0.003 |
| Ceramic | 9.8 | 0.0005 |
| PE (Polyethylene) | 2.3 | 0.0003 |
| PBT | 2.9-4.0 | 0.002 |

#### 2. Surface Requirements

- Radome surface must be smooth
- Thickness must be uniform and consistent

#### 3. Radome Thickness Design

**Formula:**

```
T = (c × N) / (2 × f × √εr)
```

Where:
- **T:** Radome thickness
- **c:** Speed of light = 3×10⁸ m/s
- **f:** Center frequency (60 GHz)
- **εr:** Material dielectric constant (DK)
- **N:** Integer (1, 2, 3...)

#### 4. Radar Antenna to Inner Shell Surface Height

**Formula:**

```
d = (c × N) / (2 × f)
```

Where:
- **c:** Speed of light = 3×10⁸ m/s
- **f:** Center frequency (60 GHz)
- **N:** Integer (1, 2, 3...)

**Example Calculation:**
```
c / (2 × f) = 3×10⁸ / (2 × 60×10⁹) = 2.5mm
```

---

## 9. Revision History

| Version | Release Date | Description |
|---------|-------------|-------------|
| V1.0 | 2023/10/10 | First release |
| V1.1 | 2024/06/28 | 1. Updated FallSensitivity setting range<br>2. Added presence detection function and description |

---

## Contact Information

**Company:** Shenzhen Hi-Link Electronic Co., Ltd.  
**Email:** stella@hlktech.com  
**Website:** www.hlktech.com

---

**Copyright © Shenzhen Hi-Link Electronic Co., Ltd. All rights reserved.**
