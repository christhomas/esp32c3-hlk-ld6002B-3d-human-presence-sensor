# HLK-LD6002 Series Product Overview

**60GHz Radar Sensor Module Series**

This documentation provides a comprehensive overview and comparison of the HLK-LD6002 series radar sensor modules from Shenzhen Hi-Link Electronic Co., Ltd.

---

## ⚠️ IMPORTANT: Variant Capabilities Are Mutually Exclusive

The three variants (LD6002, LD6002B, LD6002C) share the same hardware platform but have **different firmware with mutually exclusive capabilities**:

- **HLK-LD6002** can ONLY do breath and heart rate detection
- **HLK-LD6002B** can ONLY do 3D presence detection and position tracking
- **HLK-LD6002C** can ONLY do fall detection and presence detection

**You cannot get all capabilities in one sensor.** Choose the variant based on your specific application requirements. The variants use completely different communication protocols and cannot be reconfigured to support features from other variants.

---

## Table of Contents

1. [Product Variants](#product-variants)
2. [Product Comparison Table](#product-comparison-table)
3. [Common Specifications](#common-specifications)
4. [Product Profile](#product-profile)
5. [Product Characteristics](#product-characteristics)
6. [Technical Specifications Summary](#technical-specifications-summary)
7. [Application Scenarios](#application-scenarios)
8. [Key Advantages](#key-advantages)
9. [References](#references)

---

## Product Variants

This documentation covers the following sensor models:

### HLK-LD6002 - Breath Detection
**Primary Use:** Non-contact breathing and heart rate monitoring

- **Detection Range:** 0.4m - 1.5m (chest cavity)
- **Measurements:** Respiratory rate (9-48 bpm), Heart rate (60-150 bpm)
- **Accuracy:** ≥90%
- **Application:** Smart homes, health monitoring, elderly care
- **Documentation:** [`hlk-ld6002-datasheet.md`](hlk-ld6002-datasheet.md)

### HLK-LD6002B - 3D Presence Detection
**Primary Use:** 3D spatial presence detection with point cloud data

- **Detection Range:** Up to 3m
- **Features:** 
  - 3D position tracking (X, Y, Z coordinates)
  - Point cloud data output
  - Multi-zone detection (4 interference zones + 4 detection zones)
  - Configurable sensitivity and trigger speed
  - Low power mode support
- **Application:** Smart homes, presence sensing, occupancy detection
- **Documentation:** [`tinyframe-protocol.md`](tinyframe-protocol.md) (Section 2.1)

### HLK-LD6002C - Fall Detection
**Primary Use:** Fall detection in small areas

- **Detection Range:** 3m × 3m (ceiling mounted at 2.2-3.0m height)
- **Accuracy:** Presence 95%, Fall detection 90%
- **Features:**
  - Top-mounted installation
  - Fall event detection
  - Presence detection
  - Configurable detection zones
- **Application:** Bathrooms, restrooms, elderly care facilities
- **Documentation:** [`hlk-ld6002c-datasheet.md`](hlk-ld6002c-datasheet.md) and [`tinyframe-protocol.md`](tinyframe-protocol.md) (Section 2.2)

---

## Product Comparison Table

| Specification | HLK-LD6002 | HLK-LD6002B | HLK-LD6002C |
|--------------|------------|-------------|-------------|
| **Application** | Breath Detection | 3D presence detection | Fall detection |
| **Frequency Range** | 58~61GHz | 58~61GHz | 58~61GHz |
| **Transmission Power** | 12dBm | 12dBm | 12dBm |
| **Power Consumption** | 600mA/3.3V | 600mA/3.3V | 120-600mA/3.3V |
| **Detection Range** | ≤1.5m | 3m | 2m (coverage: 3×3m) |
| **Detection Angle** | ±60° | ±60° | ±60° |
| **Default Baud Rate** | 1382400 | 115200 | 115200 |
| **Module Size** | 25×31.5mm | 25×31.5mm | 25×23mm |

---

## Common Specifications

All variants in the HLK-LD6002 series share these common specifications:

| Specification | Value |
|--------------|-------|
| **Operating Frequency** | 58-62 GHz |
| **Operating Voltage** | 3.1-3.5V (typical 3.3V) |
| **Operating Current** | 120-600mA |
| **Operating Temperature** | -20°C to 85°C |
| **Storage Temperature** | -40°C to 85°C |
| **Transmit Power (EIRP)** | 12 dBm |
| **Antenna Gain** | 4 dBi |
| **Beam Angle** | ±60° (horizontal and vertical) |
| **Interface** | UART (baud rate varies by variant) |
| **Technology** | FMCW (Frequency Modulated Continuous Wave) |
| **Processor** | ARM® Cortex®-M3 |
| **Flash Memory** | 1MB |

---

## Product Profile

HLK-LD6002/LD6002B/LD6002C is a radar sensor module developed based on ADT6101P chip (LD6002) or high-performance domestic chip (LD6002C). The module integrates:

- **57~64GHz RF Transceiver System** - Single chip integrated radio frequency system
- **2T2R PCB Microstrip Antenna** - 2 transmit, 2 receive antenna configuration
- **1MB Flash Memory** - On-board storage
- **Radar Signal Processing Unit** - Dedicated DSP for radar algorithms
- **ARM® Cortex®-M3 Core** - Main processor

### Technology

This module is based on the **FMCW (Frequency Modulated Continuous Wave) radar mechanism**. It:
1. Detects surface reflection radar echo from the human body
2. Combines radar signal processing algorithms
3. Realizes various detection capabilities depending on the variant:
   - **LD6002:** Single-person real-time respiratory and heart rate frequency measurement
   - **LD6002B:** 3D spatial presence detection with point cloud tracking
   - **LD6002C:** Personnel status perception and fall detection

---

## Product Characteristics

### Core Features

✅ **Radar detection based on FMCW signal**
- Frequency Modulated Continuous Wave technology
- High precision detection

✅ **No contact perception**
- Completely touchless sensing
- Privacy-preserving monitoring
- No cameras or images required

✅ **Detection capabilities vary by model:**
- **LD6002:** Breathing and heart rate monitoring (max 1.5m)
- **LD6002B:** 3D presence detection with position tracking (max 3m)
- **LD6002C:** Presence and fall detection (3×3m coverage)

✅ **Universal UART interface**
- Standard serial communication protocol provided
- Easy integration with microcontrollers
- Configurable parameters via UART

✅ **Multiple I/O ports and communication interfaces**
- Reserved GPIO ports for secondary development
- Support for custom applications across multiple scenarios
- Flexible integration options

✅ **Compact form factor**
- Dimensions vary by model:
  - **LD6002/B:** 25mm × 31.5mm
  - **LD6002C:** 25mm × 23mm
- Two connection methods supported:
  - Pin header connection (through-hole)
  - SMT/surface mount connection

✅ **Environmental stability**
- Not affected by temperature variations
- Immune to humidity changes
- Resistant to noise interference
- Unaffected by airflow
- Dust-resistant operation
- Light-independent (works in complete darkness)

---

## Technical Specifications Summary

### HLK-LD6002 (Breath Detection)

| Parameter | Value |
|-----------|-------|
| **RF Technology** | FMCW Radar, 57-64 GHz |
| **Processor** | ARM Cortex-M3 |
| **Antenna** | 2T2R PCB Microstrip |
| **Flash Memory** | 1MB |
| **Detection Range** | 0.4m - 1.5m (chest cavity) |
| **Max Detected Persons** | 1 person |
| **Breathing Accuracy** | ≥90% |
| **Heart Rate Accuracy** | ≥90% |
| **Breathing Range** | 9-48 breaths/min |
| **Heart Rate Range** | 60-150 bpm |
| **Refresh Rate** | 50ms |
| **Detection Setup Time** | ~1 minute |
| **Module Size** | 25mm × 31.5mm |
| **Operating Voltage** | 3.2-3.4V (nominal 3.3V) |
| **Operating Current** | ~600mA |
| **Supply Current** | ≥1A (recommended) |
| **Operating Temperature** | -20°C to +85°C |
| **Storage Temperature** | -40°C to +85°C |
| **Default Baud Rate** | 1382400 |
| **Communication** | UART (8N1, no parity) |

### HLK-LD6002B (3D Presence Detection)

| Parameter | Value |
|-----------|-------|
| **Detection Range** | Up to 3m |
| **Detection Zones** | 4 interference zones + 4 detection zones |
| **Position Output** | 3D coordinates (X, Y, Z in meters) |
| **Point Cloud Data** | Available with cluster IDs |
| **Sensitivity Levels** | Low, Medium, High |
| **Trigger Speed** | Slow, Medium, Fast |
| **Installation Mode** | Top-mounted or Side-mounted |
| **Low Power Mode** | Supported with configurable sleep time |
| **Hold Delay** | Configurable (default 30 seconds) |
| **Default Baud Rate** | 115200 |

### HLK-LD6002C (Fall Detection)

| Parameter | Value |
|-----------|-------|
| **Fall Monitoring Range** | 0.6×0.6m to 3×3m |
| **Presence Detection Accuracy** | ≥95% |
| **Fall Detection Accuracy** | ≥90% |
| **Installation Height** | 2.2m - 3.0m (ceiling mounted) |
| **Operating Current** | 120-600mA |
| **Fall Threshold** | Configurable (0.3-1.5m, default 0.6m) |
| **Fall Sensitivity** | Configurable (3-30 frames, default 10) |
| **Detection Zone** | Configurable rectangular area |
| **Digital Output** | TX2 pin (High=Fall, Low=Normal) |
| **Default Baud Rate** | 115200 |

---

## Application Scenarios

### Smart Home Applications
- Breathing and heart rate monitoring for home automation
- Presence detection and occupancy sensing
- Elderly care and safety monitoring
- Automated lighting and HVAC control based on presence
- Fall detection in high-risk areas (bathrooms, bedrooms)

### Health Management
- Real-time breathing and heart rate data monitoring
- Sleep quality assessment
- Vital signs tracking
- Non-intrusive patient monitoring

### Smart Healthcare & Elderly Care
- Elderly breathing and heart rate monitoring
- Abnormal condition detection and immediate alerts
- Non-intrusive health monitoring for seniors
- Hospital patient monitoring
- Fall detection in nursing facilities
- Bathroom and restroom safety monitoring

### Home Security
- Presence detection for security systems
- Intrusion detection
- Activity monitoring
- Fall detection and emergency response

---

## Key Advantages

1. **Non-contact**: No wearables or physical sensors required
2. **Privacy-preserving**: No cameras or images, only radar signals
3. **Reliable**: Works in all lighting conditions and weather
4. **Accurate**: 90%+ accuracy for breathing, heart rate, and fall detection
5. **Fast**: Real-time monitoring with minimal latency
6. **Compact**: Easy to integrate into products
7. **Stable**: Environmental factors don't affect performance
8. **Versatile**: Multiple variants for different use cases
9. **Configurable**: Parameters adjustable via UART for different scenarios
10. **Low Power Options**: LD6002B supports low power mode for battery applications

---

## Electrical Characteristics and Parameters

### Functional Parameters (LD6002 - Breath Detection)

| Parameter | Min | Typical | Max | Unit |
|-----------|-----|---------|-----|------|
| Breathing and heartbeat detection distance (chest cavity) | 0.4 | - | 1.5 | m |
| Respiratory measurement accuracy | - | 90 | - | % |
| Range of the number of respiratory measurements | 9 | - | 48 | bpm |
| Heartbeat measurement accuracy | - | 90 | - | % |
| Range of the number of heartbeat measurements | 60 | - | 150 | bpm |
| Refresh time | - | 50 | - | ms |
| Establish detection time | - | 1 | - | Min |
| Maximum number of tests | - | 1 | - | - |

### Functional Parameters (LD6002C - Fall Detection)

| Parameter | Content | Min | Typical | Max | Unit |
|-----------|---------|-----|---------|-----|------|
| Fall monitoring detection range | 0.6×0.6 | - | 3×3 | - | m |
| Presence detection accuracy | - | - | 95 | - | % |
| Fall detection accuracy | - | - | 90 | - | % |

### Electrical Characteristics (All Variants)

| Operational Parameter | Min | Type | Max | Unit |
|----------------------|-----|------|-----|------|
| Operating voltage (VCC) | 3.1 | 3.3 | 3.5 | V |
| Operating current (ICC) | 120 | - | 600 | mA |
| Operating temperature (TOP) | -20 | - | 85 | ℃ |
| Storage temperature (TST) | -40 | - | 85 | ℃ |

### RF Characteristics (All Variants)

| Operational Parameter | Min | Type | Max | Unit |
|----------------------|-----|------|-----|------|
| Service frequency | 58 | - | 62 | GHz |
| EIRP | - | 12 | - | dBm |
| Antenna gain | - | 4 | - | dBi |
| Horizontal beam (-3dB) | -60 | - | +60 | ° |
| Vertical beam (-3dB) | -60 | - | +60 | ° |

---

## References

### Documentation
- **LD6002 Datasheet:** [`hlk-ld6002-datasheet.md`](hlk-ld6002-datasheet.md)
- **LD6002B Protocol:** [`tinyframe-protocol.md`](tinyframe-protocol.md) (Section 2.1)
- **LD6002C Datasheet:** [`hlk-ld6002c-datasheet.md`](hlk-ld6002c-datasheet.md)
- **LD6002C Protocol:** [`tinyframe-protocol.md`](tinyframe-protocol.md) (Section 2.2)
- **Troubleshooting:** [`troubleshooting.md`](troubleshooting.md)

### Original Sources
- Chinese Technical Specification: `HLK-LD6002-呼吸心率检测雷达模组技术规格书 2024.12`
- Chinese PDF Datasheets in `docs/original/` folder
- Chip: ADT6101P (Analog Devices) for LD6002
- Manufacturer: Shenzhen Hi-Link Electronic Co., Ltd. (深圳市海凌科电子有限公司)

---

## Contact Information

**Company:** Shenzhen Hi-Link Electronic Co., Ltd.  
**Email:** stella@hlktech.com  
**Website:** www.hlktech.com

For technical support, documentation issues, or questions about this project, refer to the detailed documentation files listed in the References section above.

---

**Copyright © Shenzhen Hi-Link Electronic Co., Ltd. All rights reserved.**

**Last Updated:** December 29, 2024  
**Documentation Version:** 1.0
