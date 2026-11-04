# Waveshare Dust Sensor Arduino Library

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-blue.svg)](https://www.arduino.cc/)
[![PlatformIO Registry](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)

A lightweight, memory-efficient, non-blocking Arduino and PlatformIO library for the **[Waveshare Dust Sensor](https://www.waveshare.com/wiki/Dust_Sensor)** (powered by the **Sharp GP2Y1010AU0F** optical particulate sensor).

---

## Key Features

* **Lightweight & Zero-Heap Allocation:** Strict embedded design with zero dynamic memory (`malloc`/`new`), minimal RAM footprint (~28 bytes), and zero external dependencies.
* **Onboard Voltage Divider Compensation ($\times 11$):** Automatically compensates for the Waveshare module's built-in $11:1$ resistor divider on `AOUT`.
* **Platform & ADC Agnostic:** Works seamlessly across 5V and 3.3V boards (Arduino Uno/Nano, ESP32, ESP8266, RP2040 Raspberry Pi Pico, STM32) with configurable reference voltage and 10-bit / 12-bit / 16-bit ADC support.
* **Dual Execution Modes:**
  * **Synchronous (`read()`):** Minimal, easy-to-read blocking pulse for basic sketches.
  * **Non-Blocking State Machine (`update()`):** Microsecond-precision asynchronous sampling using `micros()`, freeing your MCU loop for WiFi, displays, and RTOS tasks.
* **Built-in Digital Filtering:** Optional Exponential Moving Average (EMA) filter to reject optical and atmospheric noise.
* **Clean-Air Zero Calibration:** Interactive calibration routine to measure and tune the baseline offset ($V_{\text{zero}}$).
* **Custom ADC Hook:** Attach external ADCs (such as ADS1115) via a clean function pointer callback.

---

## Hardware Overview & Pinout

Unlike the bare Sharp GP2Y1010AU0F sensor, the **Waveshare Dust Sensor breakout board** incorporates an onboard voltage booster circuit, an NPN transistor inverter for driving the IR diode, and an $11:1$ resistor divider on the analog output.

```
+--------------------------------------------------------------------------------+
|                             Waveshare Dust Sensor Board                        |
|                                                                                |
|  [ VCC ] (2.5V - 5.5V) ---> [ Onboard Voltage Booster (5V) ]                   |
|                                         |                                      |
|  [ ILED ] (Active HIGH) --> [ NPN Transistor Inverter ] --> [ Sharp IR LED ]   |
|                                                                 |  (Pulsed)    |
|                                                             [ Dust Chamber ]   |
|                                                                 |  (Scattering)|
|  [ AOUT ] <--- [ 11:1 Voltage Divider ] <------------------ [ Photodiode / AMP]|
|  [ GND  ] <------------------------------------------------ [ Common Ground ]  |
+--------------------------------------------------------------------------------+
```

### Wiring Guide

| Sensor Pin | Description | Arduino Uno / Nano | ESP32 | Raspberry Pi Pico (RP2040) |
| :--- | :--- | :--- | :--- | :--- |
| **VCC** | Power Supply (2.5V – 5.5V) | `5V` | `5V` (or `3.3V`) | `VBUS` (5V) or `3V3` |
| **GND** | Ground | `GND` | `GND` | `GND` |
| **AOUT** | Analog Data Output | `A0` | `GPIO 34` (ADC1) | `GP26` (ADC0) |
| **ILED** | Drive Pin for Internal IR LED | `D2` | `GPIO 25` | `GP15` |

> [!IMPORTANT]
> The Waveshare board's `ILED` pin is driven **active HIGH** (due to the onboard transistor), unlike the bare Sharp sensor which is active LOW.

---

## Operating Principle & Mathematical Formula

1. **Pulse Timing:**
   * Drive `ILED` **HIGH**.
   * Wait **$280\,\mu\text{s}$** ($0.28\,\text{ms}$) for the scattered optical signal to stabilize.
   * Sample the analog voltage on `AOUT` (during a $40\,\mu\text{s}$ window).
   * Drive `ILED` **LOW** (total pulse length $\le 320\,\mu\text{s}$).
   * Wait at least **$9.68\,\text{ms}$** before the next pulse (maximum $3.2\%$ duty cycle).

2. **Voltage & Density Math:**
   * **Measured Pin Voltage:**
     $$V_{\text{pin}} = \text{rawADC} \times \left(\frac{V_{\text{ref}}}{\text{ADC}_{\text{max}}}\right)$$
   * **Internal Module Voltage:**
     $$V_{\text{actual}} = V_{\text{pin}} \times 11.0$$
   * **Dust Concentration ($\mu\text{g/m}^3$):**
     $$\Delta V = \max(0,\, V_{\text{actual}} - V_{\text{zero}})$$
     $$\text{Dust Density } (\mu\text{g/m}^3) = \frac{\Delta V}{0.5\,\text{V}} \times 100 = \Delta V \times 200$$

---

## Quick Start Example

```cpp
#include <WaveshareDustSensor.h>

const uint8_t PIN_AOUT = A0;
const uint8_t PIN_ILED = 2;

// Parameters: AOUT pin, ILED pin, Vref = 5.0V, 10-bit ADC max = 1023
WaveshareDustSensor dustSensor(PIN_AOUT, PIN_ILED, 5.0f, 1023);

void setup() {
    Serial.begin(9600);
    dustSensor.begin();
    
    // Optional: Enable Exponential Moving Average (EMA) smoothing
    dustSensor.enableFilter(true, 0.20f);
}

void loop() {
    if (dustSensor.read()) {
        Serial.print("Dust Concentration: ");
        Serial.print(dustSensor.getDustDensity(), 1);
        Serial.print(" ug/m3 | Air Quality: ");
        Serial.println(dustSensor.getAirQualityString());
    }
    delay(1000);
}
```

---

## Non-Blocking Asynchronous Operation

For responsive applications, user interfaces, or network stacks (WiFi/Bluetooth on ESP32), use `update()` instead of `read()`:

```cpp
void loop() {
    // update() manages microsecond timing asynchronously
    if (dustSensor.update()) {
        // A complete pulse cycle finished and new data is ready!
        Serial.print("Density: ");
        Serial.println(dustSensor.getDustDensity());
    }

    // Other non-blocking tasks run concurrently with zero jitter
    runDisplayTask();
    runNetworkTask();
}
```

---

## Air Quality Categories

The library categorizes dust concentration based on standard particulate matter (PM) thresholds:

| Category Enum | Concentration ($\mu\text{g/m}^3$) | Qualitative Description |
| :--- | :--- | :--- |
| `DustAirQuality::EXCELLENT` | $0 - 35$ | Clean indoor air |
| `DustAirQuality::GOOD` | $35 - 75$ | Normal outdoor air |
| `DustAirQuality::MODERATE` | $75 - 115$ | Noticeable haze / moderate dust |
| `DustAirQuality::POOR` | $115 - 150$ | Unhealthy for sensitive groups |
| `DustAirQuality::VERY_POOR` | $150 - 250$ | Unhealthy air quality |
| `DustAirQuality::HAZARDOUS` | $> 250$ | Very unhealthy / hazardous |

---

## API Reference

### Initialization & Configuration
* `WaveshareDustSensor(uint8_t aoutPin, uint8_t iledPin, float vRef = 5.0f, uint16_t adcMax = 1023)`
* `void begin()`: Initializes GPIO directions and initial states.
* `void setVRef(float vRef)`: Sets ADC reference voltage (e.g. `3.3f` or `5.0f`).
* `void setAdcMax(uint16_t adcMax)`: Sets max ADC count (e.g. `1023` for 10-bit, `4095` for 12-bit).
* `void setVoltageDividerRatio(float ratio)`: Defaults to `11.0f`.
* `void setSensitivity(float sensitivityVPer100ug)`: Defaults to `0.5f` ($0.5\,\text{V}/100\,\mu\text{g/m}^3$).
* `void setBaselineVoltage(float vZeroActual)`: Sets clean-air baseline offset (nominal: `0.9f`).
* `float getBaselineVoltage() const`: Returns current baseline voltage.
* `void enableFilter(bool enable, float alpha = 0.25f)`: Enables EMA filter ($0.01 \le \alpha \le 1.0$).
* `void resetFilter()`: Clears filter history.
* `void setAdcCallback(DustSensorAdcReader customReader)`: Plugs in an external ADC callback.

### Measurement
* `bool read()`: Synchronous blocking read.
* `bool update()`: Asynchronous non-blocking state machine tick.
* `float calibrateZero(uint16_t sampleCount = 25, uint16_t delayBetweenSamplesMs = 20)`: Measures and sets clean-air baseline.

### Data Accessors
* `uint16_t getRawADC() const`: Returns the raw integer reading from the ADC.
* `float getPinVoltage() const`: Returns voltage measured at the MCU pin ($\text{V}$).
* `float getActualVoltage() const`: Returns calculated internal sensor voltage ($\text{V}$).
* `float getDustDensity() const`: Returns particulate concentration in $\mu\text{g/m}^3$.
* `float getDustDensityMg() const`: Returns particulate concentration in $\text{mg/m}^3$.
* `DustAirQuality getAirQuality() const`: Returns categorical enum.
* `const char* getAirQualityString() const`: Returns descriptive human-readable string.

---

## License

This library is released under the **MIT License**. See [LICENSE](LICENSE) for details.
