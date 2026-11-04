# Waveshare Dust Sensor - API Reference

Comprehensive programming manual for the `WaveshareDustSensor` C++ driver class.

---

## Table of Contents

* [Quick Method Index](#quick-method-index)
* [Pin Selection Guide](#pin-selection-guide)
* [Data Types & Enums](#data-types--enums)
* [Constructor & Initialization](#constructor--initialization)
* [Pin Management](#pin-management)
* [Measurement & Sampling](#measurement--sampling)
* [Calibration & Tuning](#calibration--tuning)
* [Signal Filtering](#signal-filtering)
* [Telemetry & Data Accessors](#telemetry--data-accessors)
* [External ADC Callback Hook](#external-adc-callback-hook)

---

## Quick Method Index

| Method Signature | Return Type | Blocking | Summary |
| :--- | :--- | :---: | :--- |
| [`WaveshareDustSensor()`](#wavesharedustsensor) | Instance | No | Default constructor (pins assigned later via `begin`). |
| [`WaveshareDustSensor(aout, iled, ...)`](#wavesharedustsensoraout-iled-) | Instance | No | Constructor with explicit pin assignments. |
| [`begin()`](#begin) | `void` | No | Initializes configured GPIO pins into safe states. |
| [`begin(aoutPin, iledPin)`](#beginaoutpin-iledpin) | `void` | No | Assigns pins and initializes hardware in one step. |
| [`setPins(aoutPin, iledPin)`](#setpinsaoutpin-iledpin) | `void` | No | Reassigns `AOUT` and `ILED` hardware pins dynamically. |
| [`getAoutPin()`](#getaoutpin--getiledpin) | `uint8_t` | No | Returns the currently configured `AOUT` pin. |
| [`getIledPin()`](#getaoutpin--getiledpin) | `uint8_t` | No | Returns the currently configured `ILED` pin. |
| [`read()`](#read) | `bool` | ~10 ms | Synchronous pulse and measurement. |
| [`update()`](#update) | `bool` | No (0 ms) | Non-blocking state machine tick. |
| [`calibrateZero(samples, delayMs)`](#calibratezerosamples-delayms) | `float` | Yes | Measures and calibrates clean-air baseline ($V_{\text{zero}}$). |
| [`setSampleIntervalMs(ms)`](#setsampleintervalmsms) | `void` | No | Configures autonomous rate for `update()` (default: 1000ms). |
| [`getSampleIntervalMs()`](#getsampleintervalms) | `uint32_t` | No | Returns current autonomous sampling interval. |
| [`setVRef(vRef)`](#setvrefvref--setadcmaxadcmax) | `void` | No | Sets ADC reference voltage in Volts (e.g. 5.0V or 3.3V). |
| [`setAdcMax(adcMax)`](#setvrefvref--setadcmaxadcmax) | `void` | No | Sets max integer ADC step count (e.g. 1023 or 4095). |
| [`setVoltageDividerRatio(ratio)`](#setvoltagedividerratioratio) | `void` | No | Overrides breakout board divider ratio (default: 11.0f). |
| [`setSensitivity(vPer100ug)`](#setsensitivityvper100ug) | `void` | No | Sets sensor slope (default: 0.5V per 100 µg/m³). |
| [`setBaselineVoltage(vZero)`](#setbaselinevoltagevzero) | `void` | No | Manually sets clean-air baseline offset in Volts. |
| [`getBaselineVoltage()`](#getbaselinevoltage) | `float` | No | Returns active clean-air baseline offset in Volts. |
| [`enableFilter(enable, alpha)`](#enablefilterenable-alpha) | `void` | No | Enables Exponential Moving Average (EMA) filter. |
| [`resetFilter()`](#resetfilter) | `void` | No | Clears filter history to prevent ramp-up lag. |
| [`setAdcCallback(customReader)`](#setadccallbackcustomreader) | `void` | No | Hooks an external ADC routine (e.g. ADS1115). |
| [`getRawADC()`](#getrawadc) | `uint16_t` | No | Returns raw integer count from microcontroller ADC. |
| [`getPinVoltage()`](#getpinvoltage) | `float` | No | Returns voltage measured directly at the MCU pin ($0 - V_{\text{ref}}$). |
| [`getActualVoltage()`](#getactualvoltage--getrawactualvoltage) | `float` | No | Returns internal sensor output ($V_{\text{pin}} \times 11.0$), smoothed if enabled. |
| [`getRawActualVoltage()`](#getactualvoltage--getrawactualvoltage) | `float` | No | Returns raw unfiltered internal sensor voltage. |
| [`getDustDensity()`](#getdustdensity--getdustdensitymg) | `float` | No | Returns particulate concentration in $\mu\text{g/m}^3$. |
| [`getDustDensityMg()`](#getdustdensity--getdustdensitymg) | `float` | No | Returns particulate concentration in $\text{mg/m}^3$. |
| [`isSaturated()`](#issaturated) | `bool` | No | Returns `true` if sensor reached optical ceiling ($\ge 500\,\mu\text{g/m}^3$). |
| [`getAirQuality()`](#getairquality--getairqualitystring) | `DustAirQuality`| No | Categorical air quality enum (`EXCELLENT` ... `HAZARDOUS`). |
| [`getAirQualityString()`](#getairquality--getairqualitystring) | `const char*` | No | Human-readable air quality string. |

---

## Pin Selection Guide

The library supports **any valid pin** on your microcontroller. When selecting pins, follow these electrical rules:

1. **`AOUT` (Analog Input):** Must connect to a pin capable of reading analog voltages via an internal Analog-to-Digital Converter (`analogRead`).
2. **`ILED` (Digital Output):** Can connect to any standard digital GPIO capable of output mode (`pinMode(pin, OUTPUT)`).

### Target Pin Compatibility Matrix

| Board | Allowable `AOUT` Pins (Analog In) | Allowable `ILED` Pins (Digital Out) | Recommended Default |
| :--- | :--- | :--- | :---: |
| **Arduino Uno / Nano** | Any analog pin: `A0`, `A1`, `A2`, `A3`, `A4`, `A5`, `A6`, `A7` | Any digital pin: `D2` through `D13` | `A0` & `D2` |
| **Arduino Mega 2560** | Any analog pin: `A0` through `A15` | Any digital pin: `2` through `53` | `A0` & `D2` |
| **ESP32 Dev Module** | **ADC1 Only:** `GPIO 32, 33, 34, 35, 36, 39`<br>*(Avoid ADC2 pins when WiFi/BLE is active)* | Any standard output GPIO:<br>`GPIO 2, 4, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33` | `GPIO 34` & `GPIO 25` |
| **ESP8266 (NodeMCU / D1 Mini)** | `A0` (only ADC channel on chip, max 1.0V) | Any general GPIO: `D1 (GPIO 5)`, `D2 (GPIO 4)`, `D5 (GPIO 14)`, `D6 (GPIO 12)` | `A0` & `D1` |
| **Raspberry Pi Pico (RP2040)** | Any ADC pin: `GP26 (ADC0)`, `GP27 (ADC1)`, `GP28 (ADC2)` | Any digital GPIO: `GP0` through `GP22` | `GP26` & `GP15` |
| **STM32 (Nucleo / BluePill)** | Any ADC-capable pin: `PA0`, `PA1`, `PA4`, `PB0` | Any digital pin: `PA2`, `PA3`, `PB10`, etc. | `PA0` & `PA1` |

---

## Data Types & Enums

### `enum class DustAirQuality : uint8_t`
Categorical classifications based on particulate concentration:

```cpp
enum class DustAirQuality : uint8_t {
    EXCELLENT = 1,          // Level I:   0 - 35 ug/m3  (AQI 0 - 50, Excellent)
    AVERAGE = 2,            // Level II:  35 - 75 ug/m3  (AQI 51 - 100, Average)
    LIGHT_POLLUTION = 3,    // Level III: 75 - 115 ug/m3 (AQI 101 - 150, Light pollution)
    MODERATE_POLLUTION = 4, // Level IV:  115 - 150 ug/m3 (AQI 151 - 200, Moderate pollution)
    HEAVY_POLLUTION = 5,    // Level V:   150 - 250 ug/m3 (AQI 201 - 300, Heavy pollution)
    SERIOUS_POLLUTION = 6,  // Level VI:  250 - 500 ug/m3 (AQI >= 300, Serious pollution)
    
    // Aliases for backwards compatibility:
    GOOD = AVERAGE,
    MODERATE = LIGHT_POLLUTION,
    POOR = MODERATE_POLLUTION,
    VERY_POOR = HEAVY_POLLUTION,
    HAZARDOUS = SERIOUS_POLLUTION
};
```

### `typedef uint16_t (*DustSensorAdcReader)(uint8_t pin)`
Function pointer signature for plugging in external ADCs (e.g. ADS1115):

```cpp
typedef uint16_t (*DustSensorAdcReader)(uint8_t pin);
```

---

## Constructor & Initialization

### `WaveshareDustSensor()`
Default constructor. Instantiates the driver without binding hardware pins immediately. Pins can be assigned later via `begin(aoutPin, iledPin)` or `setPins(aoutPin, iledPin)`.

```cpp
WaveshareDustSensor();
```

* **Example:**
  ```cpp
  WaveshareDustSensor dustSensor; // Global declaration

  void setup() {
      dustSensor.begin(A0, 2);   // Assigned and initialized in setup()
  }
  ```

---

### `WaveshareDustSensor(aout, iled, ...)`
Primary constructor with explicit pin assignments and optional ADC reference settings.

```cpp
WaveshareDustSensor(uint8_t aoutPin, uint8_t iledPin, float vRef = 5.0f, uint16_t adcMax = 1023);
```

| Parameter | Type | Default | Description |
| :--- | :--- | :---: | :--- |
| `aoutPin` | `uint8_t` | — | Analog input pin connected to `AOUT`. |
| `iledPin` | `uint8_t` | — | Digital output pin connected to `ILED`. |
| `vRef` | `float` | `5.0f` | ADC reference voltage in Volts (e.g. `5.0f` for Uno, `3.3f` for ESP32). |
| `adcMax` | `uint16_t` | `1023` | Max integer ADC step count (`1023` for 10-bit, `4095` for 12-bit). |

* **Example:**
  ```cpp
  // Standard Arduino Uno setup
  WaveshareDustSensor sensor(A0, 2);

  // ESP32 with 3.3V reference and 12-bit ADC
  WaveshareDustSensor espSensor(34, 25, 3.3f, 4095);
  ```

---

### `begin()`
Initializes hardware pins configured in the constructor. Configures `_iledPin` as `OUTPUT` (driven LOW) and `_aoutPin` as `INPUT`. Pre-arms the state machine timer and resets filters.

```cpp
void begin();
```

* **Example:**
  ```cpp
  WaveshareDustSensor sensor(A0, 2);

  void setup() {
      sensor.begin();
  }
  ```

---

### `begin(aoutPin, iledPin)`
Overloaded initialization method. Assigns or reassigns pins and initializes the hardware in a single call.

```cpp
void begin(uint8_t aoutPin, uint8_t iledPin);
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `aoutPin` | `uint8_t` | Any valid microcontroller analog pin connected to `AOUT`. |
| `iledPin` | `uint8_t` | Any valid microcontroller digital output pin connected to `ILED`. |

* **Example:**
  ```cpp
  WaveshareDustSensor sensor;

  void setup() {
      sensor.begin(A1, 4); // Use pin A1 for AOUT and pin 4 for ILED
  }
  ```

---

## Pin Management

### `setPins(aoutPin, iledPin)`
Dynamically reassigns hardware pins.

```cpp
void setPins(uint8_t aoutPin, uint8_t iledPin);
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `aoutPin` | `uint8_t` | Analog input pin connected to `AOUT`. |
| `iledPin` | `uint8_t` | Digital output pin connected to `ILED`. |

* **Example:**
  ```cpp
  sensor.setPins(A3, 7);
  sensor.begin(); // Re-apply pin configurations
  ```

---

### `getAoutPin()` & `getIledPin()`
Returns the currently assigned pin numbers.

```cpp
uint8_t getAoutPin() const;
uint8_t getIledPin() const;
```

* **Example:**
  ```cpp
  Serial.print("AOUT on pin: ");
  Serial.println(sensor.getAoutPin());
  ```

---

## Measurement & Sampling

### `read(sampleCount)`
Executes synchronous measurement.
- If `sampleCount == 1` (default): Executes a single measurement pulse (drives `ILED` HIGH, waits $280\,\mu\text{s}$, samples `AOUT`, turns `ILED` LOW, and enforces $9.68\,\text{ms}$ cooldown).
- If `sampleCount > 1` (e.g. `10`): Fires a burst of pulses spaced $10\,\text{ms}$ apart, collects the samples, and extracts the **median** reading to eliminate single-pulse optical jitter and electrical noise spikes.

```cpp
bool read(uint8_t sampleCount = 1);
```

| Parameter | Type | Default | Description |
| :--- | :--- | :---: | :--- |
| `sampleCount` | `uint8_t` | `1` | Number of pulse samples to capture and median-filter (recommended: `10` to `15`). |

* **Returns:** `true` if sampling was successful, `false` if pins are unassigned.
* **Blocking Time:** $\approx 10\,\text{ms} \times \text{sampleCount}$ (e.g. $\approx 100\,\text{ms}$ for 10 samples).
* **Example:**
  ```cpp
  // Recommended: 10-pulse burst median measurement
  if (sensor.read(10)) {
      Serial.print("Dust: ");
      Serial.print(sensor.getDustDensity());
      Serial.println(" ug/m3");
  }
  ```

---

### `readMedian(sampleCount)`
Convenience wrapper that explicitly invokes burst sampling with median filtering.

```cpp
bool readMedian(uint8_t sampleCount = 10);
```

| Parameter | Type | Default | Description |
| :--- | :--- | :---: | :--- |
| `sampleCount` | `uint8_t` | `10` | Number of pulses in the burst (max bounded at 25). |

* **Example:**
  ```cpp
  if (sensor.readMedian(10)) {
      Serial.println(sensor.getAirQualityString());
  }
  ```
---

### `update()`
Drives the asynchronous non-blocking state machine. Must be called frequently in `loop()`. Manages microsecond pulse timing, ADC sampling, and cooldown intervals in the background without blocking execution.

```cpp
bool update();
```

* **Returns:** `true` once per completed sample interval when fresh data is ready. Returns `false` while waiting.
* **Blocking Time:** $0\,\text{ms}$ (pure non-blocking).
* **Example:**
  ```cpp
  void loop() {
      if (sensor.update()) {
          // Fresh measurement ready!
          Serial.printf("Dust: %.1f ug/m3 [%s]\n", 
                        sensor.getDustDensity(), 
                        sensor.getAirQualityString());
      }
      
      // Other concurrent tasks run with zero jitter
  }
  ```

---

### `setSampleIntervalMs(ms)`
Sets the interval between automatic measurement cycles when using `update()`. Enforces a hardware minimum of $10\,\text{ms}$ to prevent infrared diode overheating.

```cpp
void setSampleIntervalMs(uint32_t intervalMs);
```

* **Default:** `1000` ms ($1\,\text{Hz}$).
* **Example:**
  ```cpp
  sensor.setSampleIntervalMs(2000); // Sample once every 2 seconds
  ```

---

### `getSampleIntervalMs()`
Returns the active non-blocking sampling interval in milliseconds.

```cpp
uint32_t getSampleIntervalMs() const;
```

---

## Calibration & Tuning

### `calibrateZero(samples, delayMs)`
Performs software baseline calibration by averaging $N$ consecutive pulses in clean air. Stores the result as the active zero-dust offset ($V_{\text{zero}}$) and recalibrates the latest cached density reading to $0.0\,\mu\text{g/m}^3$.

```cpp
float calibrateZero(uint16_t sampleCount = 25, uint16_t delayBetweenSamplesMs = 20);
```

| Parameter | Type | Default | Description |
| :--- | :--- | :---: | :--- |
| `sampleCount` | `uint16_t` | `25` | Number of pulses to average. |
| `delayBetweenSamplesMs`| `uint16_t` | `20` | Delay between calibration pulses in milliseconds. |

* **Returns:** Calibrated baseline voltage ($V_{\text{actual}}$) in Volts.
* **Example:**
  ```cpp
  Serial.println("Calibrating in clean air...");
  float baseline = sensor.calibrateZero(50, 20);
  Serial.printf("New clean-air baseline: %.4f V\n", baseline);
  ```

---

### `setBaselineVoltage(vZero)`
Manually sets the clean-air baseline voltage offset ($V_{\text{zero}}$) in Volts (measured internal sensor voltage).

```cpp
void setBaselineVoltage(float vZeroActual);
```

* **Default:** `0.9f` Volts.
* **Example:**
  ```cpp
  sensor.setBaselineVoltage(0.8842f);
  ```

---

### `getBaselineVoltage()`
Returns the currently active clean-air baseline voltage in Volts.

```cpp
float getBaselineVoltage() const;
```

---

### `setVRef(vRef)` & `setAdcMax(adcMax)`
Adjusts the ADC reference voltage and bit depth dynamically.

```cpp
void setVRef(float vRef);
void setAdcMax(uint16_t adcMax);
```

* **Example:**
  ```cpp
  sensor.setVRef(3.3f);
  sensor.setAdcMax(4095); // 12-bit ADC
  ```

---

### `setVoltageDividerRatio(ratio)`
Overrides the breakout board's onboard voltage divider ratio.

```cpp
void setVoltageDividerRatio(float ratio);
```

* **Default:** `11.0f` ($11:1$ ratio on the Waveshare breakout).

---

### `setSensitivity(vPer100ug)`
Sets the sensor's optical sensitivity slope in Volts per $100\,\mu\text{g/m}^3$.

```cpp
void setSensitivity(float sensitivityVPer100ug);
```

* **Default:** `0.5f` ($0.5\,\text{V} / 100\,\mu\text{g/m}^3$).

---

## Signal Filtering

### `enableFilter(enable, alpha)`
Enables or disables the Exponential Moving Average (EMA) filter.

```cpp
void enableFilter(bool enable, float alpha = 0.25f);
```

| Parameter | Type | Default | Description |
| :--- | :--- | :---: | :--- |
| `enable` | `bool` | — | `true` to activate filtering, `false` to disable. |
| `alpha` | `float` | `0.25f` | Smoothing factor ($0.01 \le \alpha \le 1.0$). Lower values provide heavier smoothing. |

* **Example:**
  ```cpp
  // Heavy smoothing for high-noise environments
  sensor.enableFilter(true, 0.15f);
  ```

---

### `resetFilter()`
Clears internal filter history, ensuring the next sample initializes the filter without ramp-up lag.

```cpp
void resetFilter();
```

---

## Telemetry & Data Accessors

All data getters return cached values from the most recent completed measurement:

```cpp
uint16_t getRawADC() const;
float getPinVoltage() const;
float getActualVoltage() const;
float getRawActualVoltage() const;
float getDustDensity() const;
float getDustDensityMg() const;
bool isSaturated() const;
DustAirQuality getAirQuality() const;
const char* getAirQualityString() const;
```

| Method | Return Type | Units / Range | Description |
| :--- | :--- | :---: | :--- |
| `getRawADC()` | `uint16_t` | $0 - \text{ADC}_{\text{max}}$ | Raw integer count directly from the ADC. |
| `getPinVoltage()` | `float` | $0.0 - V_{\text{ref}}\,\text{V}$ | Voltage measured directly at the MCU pin. |
| `getActualVoltage()` | `float` | $0.0 - 5.0\,\text{V}$ | Module internal voltage ($V_{\text{pin}} \times 11.0$). Smoothed if filter enabled. |
| `getRawActualVoltage()` | `float` | $0.0 - 5.0\,\text{V}$ | Unfiltered internal sensor voltage. |
| `getDustDensity()` | `float` | $\mu\text{g/m}^3$ | Calculated particulate matter concentration. |
| `getDustDensityMg()` | `float` | $\text{mg/m}^3$ | Particulate concentration in milligrams per cubic meter. |
| `isSaturated()` | `bool` | `true` / `false` | Returns `true` if reading reached optical ceiling ($\ge 500\,\mu\text{g/m}^3$ or $\ge 3.6\,\text{V}$). |
| `getAirQuality()` | `DustAirQuality` | Enum | Categorical air quality level tier (Level I to VI). |
| `getAirQualityLevel()` | `uint8_t` | $1 - 6$ | Numeric quality level (1 to 6) matching Roman numerals I to VI. |
| `getAirQualityLevelString()` | `const char*` | String | Roman numeral level string (`"I"`, `"II"`, `"III"`, `"IV"`, `"V"`, `"VI"`). |
| `getAirQualityString()` | `const char*` | String | Official Waveshare evaluation string (`"Excellent"`, `"Average"`, etc.). |
| `getAQI()` | `uint16_t` | $0 - 500$ | Calculated Air Quality Index using piecewise linear interpolation. |

* **Example:**
  ```cpp
  if (sensor.read()) {
      Serial.print("Raw ADC: ");
      Serial.println(sensor.getRawADC());
      
      Serial.print("Pin Volts: ");
      Serial.println(sensor.getPinVoltage(), 3);
      
      Serial.print("True Sensor Volts: ");
      Serial.println(sensor.getActualVoltage(), 3);
      
      Serial.print("Density: ");
      Serial.print(sensor.getDustDensity(), 1);
      Serial.println(" ug/m3");
      
      Serial.print("Air Quality: ");
      Serial.println(sensor.getAirQualityString());
      
      if (sensor.isSaturated()) {
          Serial.println("WARNING: Optical chamber saturated!");
      }
  }
  ```

---

## External ADC Callback Hook

### `setAdcCallback(customReader)`
Redirects ADC reads from the microcontroller's internal ADC to an external converter (e.g. Adafruit ADS1115).

```cpp
void setAdcCallback(DustSensorAdcReader customReader);
```

* **Signature:** `uint16_t (*customReader)(uint8_t pin)`
* **Example:**
  ```cpp
  #include <WaveshareDustSensor.h>
  #include <Adafruit_ADS1X15.h>

  Adafruit_ADS1115 ads;
  WaveshareDustSensor dustSensor(0, 2, 4.096f, 32767); // Channel 0, ILED pin 2

  // Custom callback function
  uint16_t readAds1115(uint8_t channel) {
      int16_t adc = ads.readADC_SingleEnded(channel);
      return (adc > 0) ? static_cast<uint16_t>(adc) : 0;
  }

  void setup() {
      ads.begin();
      dustSensor.begin();
      dustSensor.setAdcCallback(readAds1115); // Redirect all analog reads to the ADS1115
  }
  ```
