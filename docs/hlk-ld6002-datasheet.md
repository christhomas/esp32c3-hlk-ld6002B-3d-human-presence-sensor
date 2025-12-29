# HLK-LD6002 Breath and Heart Rate Detection Radar Module

**Technical Specification**  
**Version:** V1.1  
**Date:** December 2024  
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
9. [Appendix](#appendix)

---

## 1. Product Introduction

The HLK-LD6002 is a radar sensing module developed based on the ADT6101P chip. It integrates a 57~64GHz RF transceiver system on a single chip, featuring:

- **2T2R PCB microstrip antenna**
- **1MB Flash memory**
- **Radar signal processing unit**
- **ARM® Cortex®-M3 core**

This module is based on the FMCW (Frequency Modulated Continuous Wave) radar mechanism, detecting radar echoes reflected from the human body surface. Combined with radar signal processing algorithms, it achieves real-time measurement of single-person breathing and heart rate frequencies.

---

## 2. Product Features

- **FMCW Technology:** Based on Frequency Modulated Continuous Wave radar detection
- **Non-contact Sensing:** Detects human breathing and heart rate without physical contact
- **Maximum Detection Distance:** Up to 1.5m for breath and heart rate
- **Universal UART Interface:** Provides communication protocol for data transmission
- **Multiple I/O Ports:** Reserved GPIO and communication interfaces support secondary development for various scenarios
- **Compact Size:** Only 25×31.5mm, supports both pin header and SMT mounting
- **Environmental Immunity:** Unaffected by temperature, humidity, noise, airflow, dust, or lighting conditions

---

## 3. Application Scenarios

### Smart Home Applications
According to respiratory and heart rate measurements, achieve home empowerment and automation

### Health Management
Real-time monitoring of respiratory and heart rate data

### Smart Healthcare
Monitor elderly breathing and heart rate, immediately alert on abnormalities

---

## 4. Electrical Characteristics and Parameters

### 4.1 Functional Parameters

| Parameter | Min | Typical | Max | Unit |
|-----------|-----|---------|-----|------|
| Breathing and heartbeat detection distance (chest cavity) | 0.4 | - | 1.5 | m |
| Respiratory measurement accuracy | - | 90 | - | % |
| Respiratory rate measurement range | 9 | - | 48 | bpm |
| Heartbeat measurement accuracy | - | 90 | - | % |
| Heartbeat rate measurement range | 60 | - | 150 | bpm |
| Refresh time | - | 50 | - | ms |
| Detection establishment time | - | 1 | - | Min |
| Maximum number of detectable persons | - | 1 | - | - |

### 4.2 Electrical Characteristics

| Operational Parameter | Min | Typical | Max | Unit |
|----------------------|-----|---------|-----|------|
| Operating voltage (VCC) | 3.1 | 3.3 | 3.5 | V |
| Operating current (ICC) | - | - | 600 | mA |
| Operating temperature (TOP) | -20 | - | 85 | °C |
| Storage temperature (TST) | -40 | - | 85 | °C |

### 4.3 RF Characteristics

| Operational Parameter | Min | Typical | Max | Unit |
|----------------------|-----|---------|-----|------|
| Operating frequency | 58 | - | 62 | GHz |
| Transmit power (EIRP) | - | 12 | - | dBm |
| Antenna gain | - | 4 | - | dBi |
| Horizontal beamwidth (-3dB) | -60 | - | +60 | ° |
| Vertical beamwidth (-3dB) | -60 | - | +60 | ° |

---

## 5. Hardware Description

### 5.1 Dimensions

**Module Size:** 25mm × 31.5mm

### 5.2 Pin Definition

| Pin # | Pin Name | Description | Notes |
|-------|----------|-------------|-------|
| 1 | 3V3 | POWER INPUT | 3.3V power supply |
| 2 | GND | Ground | Ground connection |
| 3 | P19 | GPIO19 | Boot1 configuration |
| 4 | TX2 | GPIO20 | General purpose I/O |
| 5 | AIO1 | Analog IO | Analog input/output |
| 6 | SCL0 | GPIO07 | I2C clock or GPIO |
| 7 | TX0 | UART TX | Connect to external serial RX |
| 8 | RX0 | UART RX | Connect to external serial TX |

### 5.3 Module Peripheral Reference Design

**Power Supply:**
- Input voltage: 3.3V
- Power supply capability: >1A
- Low noise design recommended

**Communication Interface:**
- UART0: Default baud rate 1382400, no parity
- I/O voltage level: 3.3V

### 5.4 Boot Configuration

| BOOT1 | BOOT0 | Description |
|-------|-------|-------------|
| 0 | 1 | Boot from internal Flash |
| Pin Location | Pin 3 (P19) | Pin 12 |

**Note:** Both BOOT1 and BOOT0 have internal pull-ups. BOOT1 must be pulled low before module startup.

---

## 6. Usage and Configuration

### 6.1 Typical Application Circuit

The HLK-LD6002 module can directly use UART0 to output detection results according to the specified protocol. The serial data includes:
- Total phase
- Respiratory phase
- Heartbeat phase
- Respiratory rate
- Heartbeat rate

Users can flexibly use these parameters based on specific application scenarios.

**Power Requirements:**
- Module power supply: 3.3V
- Input power capability: >1A
- Module I/O output voltage: 3.3V

**Communication Settings:**
- Default baud rate: 1382400
- Parity: None

### 6.2 GUI Visualization Tool Application

#### Device Connection

1. In the upper right **Option** menu, select **Config** interface
2. Select the connected serial port
3. Set baud rate to **1382400**
4. Click **[Connect]** button to start measurement
5. For easier data viewing, drag and arrange the BreathPhase, HeartPhase, and TotalPhase windows

#### Data Viewing

1. Lower right corner displays breathing and heartbeat rate information
2. Lower left **[SystemLog]** window displays message information including:
   - Total phase data
   - Heartbeat phase
   - Respiratory phase
   - Respiratory rate
   - Heartbeat rate

### 6.3 OTA Upgrade

Please refer to the document "OTA Upgrade Tool User Manual_V1.0"

### 6.4 Installation Methods and Sensing Range

#### 1. Side Installation

**Recommended Setup:**
- Radar installation height should be consistent with the chest height of the person being measured
- Distance between module and chest position: ≤1.5m

#### 2. Tilted Installation

**For Sleep Breath and Heart Rate Detection:**

- Install radar directly above the headboard at 1m height
- Tilt downward at 45° toward the center of the bed
- Maintain radar-to-chest distance within 1.5m range
- Radar normal direction should point to the main detection position
- Ensure radar can detect breathing and heartbeat data

**Maximum Detection Distance:** 1.5m

---

## 7. Important Notes

### 1. Detection Distance Variations
The radar module's detection distance is closely related to target RCS (Radar Cross Section) and environmental factors. Effective detection distance may vary with environment and target changes. Therefore, fluctuations in effective detection distance within a certain range are normal.

### 2. Power Supply Requirements
The radar module has extremely high power supply requirements:
- **Input voltage:** 3.2~3.4V
- **Power supply ripple:** ≤50mV
- **Current capability:** ≥1A
- **If using DC-DC power supply:** Switching frequency must not be lower than 2MHz

### 3. Detection Time Requirements
Since breath and heart rate are weak reflection signals, the radar signal processing requires a period of data accumulation. Many factors can affect radar processing results during the accumulation process. Therefore, occasional detection failures are normal phenomena.

### 4. Single Person Detection
Currently, breath and heart rate measurement only supports single-person detection. Please ensure there is only one person in the detection area.

### 5. Resting State Measurement
Measurement must be performed in a resting state. Detection of large movements will stop the measurement.

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

## 9. Appendix

### A. Document Revision History

| Version | Revision Scope | Date |
|---------|---------------|------|
| V1.0 | Initial version | September 23, 2023 |
| V1.1 | Modified pin definitions and wiring diagram | December 11, 2024 |

---

## Contact Information

**Company:** Shenzhen Hi-Link Electronic Co., Ltd.  
**Email:** stella@hlktech.com  
**Website:** www.hlktech.com

---

**Copyright © Shenzhen Hi-Link Electronic Co., Ltd. All rights reserved.**
