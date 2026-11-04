# Waveshare Dust Sensor Arduino Library

<p align="center">
  <strong>Lightweight, non-blocking, zero-allocation C++ driver for the Waveshare Dust Sensor (Sharp GP2Y1010AU0F / GP2Y1014AU0F)</strong>
</p>

<p align="center">
  <a href="https://github.com/26oppy/Waveshare_Dust_Sensor/actions"><img src="https://img.shields.io/badge/CI-Passing-2ea44f?style=flat-square&logo=githubactions&logoColor=white" alt="Build Status"></a>
  <a href="https://opensource.org/licenses/MIT"><img src="https://img.shields.io/badge/License-MIT-blue?style=flat-square" alt="License"></a>
  <a href="https://www.arduino.cc/"><img src="https://img.shields.io/badge/Arduino%20IDE-1.5%2B%20%7C%202.x-00979D?style=flat-square&logo=arduino&logoColor=white" alt="Arduino"></a>
  <a href="https://platformio.org/"><img src="https://img.shields.io/badge/PlatformIO-Registry-F06529?style=flat-square&logo=platformio&logoColor=white" alt="PlatformIO"></a>
  <a href="docs/ARCHITECTURE.md#5-resource-footprint--benchmarks"><img src="https://img.shields.io/badge/RAM%20Footprint-32%20bytes-success?style=flat-square" alt="RAM Footprint"></a>
</p>

---

## Overview

The **[Waveshare Dust Sensor](https://www.waveshare.com/wiki/Dust_Sensor)** is an optical air quality sensor built around the **Sharp GP2Y1010AU0F** detection core. It detects fine suspended particles ($> 0.8\,\mu\text{m}$) such as cigarette smoke, house dust, and pollen using optical scattering.

This library provides an industrial-grade, zero-allocation C++ driver designed for everything from 8-bit AVR microcontrollers to modern 32-bit platforms (ESP32, RP2040, STM32).

### Key Features

* **Flexible Pin Mapping:** Select **any** valid ADC pin for `AOUT` and **any** digital output pin for `ILED`, either in the constructor or dynamically via `begin(aoutPin, iledPin)` or `setPins()`.
* **Zero-Allocation Memory Footprint:** Pure static allocation (0 bytes dynamic heap), using only **32 bytes** of static RAM.
* **Onboard Hardware Divider Compensation ($\times 11$):** Automatically compensates for the Waveshare breakout board's built-in $11:1$ resistor divider on `AOUT`.
* **Dual Execution Modes:**
  * **Synchronous (`read()`):** Precision microsecond pulse for simple single-threaded applications.
  * **Asynchronous (`update()`):** Non-blocking state machine driven by `micros()`, freeing your loop for WiFi, displays, and FreeRTOS tasks.
* **Signal Smoothing:** Built-in configurable Exponential Moving Average (EMA) filter to reject atmospheric and optical noise.
* **Zero-Point Calibration:** Software baseline calibration routine to accurately account for ambient clean-air offsets.
* **Pluggable ADC Hook:** Seamlessly interface with external ADCs (such as the ADS1115) via a custom callback.

---

## Quick Wiring Reference

```
   Waveshare Dust Sensor                      Microcontroller
+-------------------------+                  +--------------------+
|  [VCC]  [GND] [AOUT][ILED]|                |                    |
|    |      |     |     |  |                 |                    |
+----+------+-----+-----+--+                 |                    |
     |      |     |     +--------------------> ILED Control Pin   |
     |      |     +--------------------------> ADC Analog Input   |
     |      +--------------------------------> GND (Common Ground)|
     +---------------------------------------> Power (2.5V - 5.5V)|
                                             +--------------------+
```

| Sensor Pin | Description | Arduino Uno / Nano | ESP32 (3.3V) | Raspberry Pi Pico (RP2040) | STM32 Nucleo |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **VCC** | Power (2.5V – 5.5V) | `5V` | `5V` (or `3V3`) | `VBUS` (5V) or `3V3` | `5V` or `3V3` |
| **GND** | Ground | `GND` | `GND` | `GND` | `GND` |
| **AOUT** | Analog Output | **Any Analog Pin** (e.g. `A0`–`A7`) | **Any ADC1 Pin** (`GPIO 32–39`) | **Any ADC Pin** (`GP26–28`) | **Any ADC Pin** (e.g. `PA0`) |
| **ILED** | IR LED Control | **Any Digital Pin** (e.g. `D2`–`D13`) | **Any Digital Out** (e.g. `GPIO 25`) | **Any GPIO** (e.g. `GP15`) | **Any GPIO** (e.g. `PA1`) |

> [!TIP]
> **Pin Selection Rules:**
> * `AOUT` connects to **any ADC-capable pin** on your microcontroller (for ESP32, choose from ADC1: GPIO 32, 33, 34, 35, 36, 39 to avoid conflicts with WiFi).
> * `ILED` connects to **any standard digital output pin**.
> * See the [Pin Selection Guide](docs/API_REFERENCE.md#pin-selection-guide) for board-specific matrices.

---

## Installation

### PlatformIO
Add the library to your `platformio.ini`:
```ini
[env:myboard]
platform = atmelavr
board = uno
framework = arduino
lib_deps =
    Waveshare Dust Sensor
```

### Arduino IDE
1. Download this repository as a `.zip` archive.
2. In the Arduino IDE, open **Sketch** $\rightarrow$ **Include Library** $\rightarrow$ **Add .ZIP Library...**
3. Select the downloaded `.zip` file.

---

## Quickstart

### 1. Minimal Synchronous Read
```cpp
#include <WaveshareDustSensor.h>

const uint8_t PIN_AOUT = A0;
const uint8_t PIN_ILED = 2;

// Constructor: (AOUT pin, ILED pin, Vref = 5.0V, ADC Max = 1023)
WaveshareDustSensor dustSensor(PIN_AOUT, PIN_ILED, 5.0f, 1023);

void setup() {
    Serial.begin(9600);
    dustSensor.begin();

    // Enable smooth EMA filtering (alpha = 0.20)
    dustSensor.enableFilter(true, 0.20f);
}

void loop() {
    if (dustSensor.read()) {
        Serial.print(F("Dust Density: "));
        Serial.print(dustSensor.getDustDensity(), 1);
        Serial.print(F(" ug/m3 | Air Quality: "));
        Serial.println(dustSensor.getAirQualityString());
    }
    delay(1000);
}
```

### 2. Non-Blocking Multitasking Read
```cpp
#include <WaveshareDustSensor.h>

WaveshareDustSensor dustSensor(A0, 2, 5.0f, 1023);

void setup() {
    Serial.begin(115200);
    dustSensor.begin();
    dustSensor.enableFilter(true, 0.25f);

    // Sample once per second (1000ms) without blocking
    dustSensor.setSampleIntervalMs(1000);
}

void loop() {
    // update() manages microsecond pulses and cool-downs asynchronously
    if (dustSensor.update()) {
        Serial.print(F("Dust: "));
        Serial.print(dustSensor.getDustDensity(), 1);
        Serial.println(F(" ug/m3"));
    }

    // High-priority concurrent tasks (Displays, WebServer, BLE) run jitter-free!
}
```

---

## Example Sketches

The library includes ready-to-run examples in the [`examples/`](examples/) directory:

| Example Sketch | Description |
| :--- | :--- |
| **[`01_BasicReading`](examples/01_BasicReading/)** | Simple synchronous sampling for beginners. |
| **[`02_FilteredReading`](examples/02_FilteredReading/)** | Noise reduction using the built-in Exponential Moving Average filter. |
| **[`03_NonBlocking`](examples/03_NonBlocking/)** | Concurrent multi-tasking execution alongside a heartbeat LED. |
| **[`04_ZeroCalibration`](examples/04_ZeroCalibration/)** | Interactive Serial wizard to calibrate clean-air baseline offset ($V_{\text{zero}}$). |
| **[`05_ESP32_3V3_Operation`](examples/05_ESP32_3V3_Operation/)** | Configuration for 3.3V logic and 12-bit ADC ($4095$ steps). |

---

## Detailed Documentation

For advanced details, engineering architecture, and complete API specifications, see the dedicated documentation guides:

* 📐 **[Technical Architecture & Signal Processing](docs/ARCHITECTURE.md):** Optical Mie scattering physics, $320\,\mu\text{s}$ pulse waveform diagrams, state machine flow, mathematical derivations, and memory footprint benchmarks.
* 📖 **[Complete API Reference](docs/API_REFERENCE.md):** Full formal specification of all classes, methods, parameters, return types, and complexity.
* 🛠️ **[Hardware & Understanding Guide](docs/HARDWARE_GUIDE.md):** Plain-English hardware breakdown, mounting orientation guidelines, and maintenance procedures.

---

## Air Quality Categories

| Category Enum | Density Range | Environmental Description |
| :--- | :---: | :--- |
| `DustAirQuality::EXCELLENT` | $0 - 35\,\mu\text{g/m}^3$ | 🟢 Clean indoor air; minimal particulate |
| `DustAirQuality::GOOD` | $35 - 75\,\mu\text{g/m}^3$ | 🟡 Normal ambient outdoor air |
| `DustAirQuality::MODERATE` | $75 - 115\,\mu\text{g/m}^3$ | 🟠 Noticeable dust or light haze |
| `DustAirQuality::POOR` | $115 - 150\,\mu\text{g/m}^3$ | 🔴 Unhealthy for sensitive groups |
| `DustAirQuality::VERY_POOR` | $150 - 250\,\mu\text{g/m}^3$ | 🟣 Unhealthy; smoke / pollution present |
| `DustAirQuality::HAZARDOUS` | $> 250\,\mu\text{g/m}^3$ | 🟤 Hazardous air quality |

---

## Troubleshooting at a Glance

* **Reading is always $0.0\,\mu\text{g/m}^3$:** Your clean ambient air voltage is lower than the default $0.9\,\text{V}$ baseline. Run [`04_ZeroCalibration.ino`](examples/04_ZeroCalibration/) to measure and store your sensor's exact clean-air offset.
* **Readings jump erratically:** Optical particle scattering is naturally sensitive to air turbulence. Call `dustSensor.enableFilter(true, 0.20f)` to activate EMA filtering.
* **Physical mounting:** Always mount the sensor with the through-hole **oriented vertically** so natural convection carries air upwards without dust settling on the internal lenses.

---

## Contributing

Contributions, bug reports, and pull requests are welcome! Please follow [Conventional Commits](https://www.conventionalcommits.org/) for commit messages.

---

## License

This library is open-source software licensed under the **[MIT License](LICENSE)**.
