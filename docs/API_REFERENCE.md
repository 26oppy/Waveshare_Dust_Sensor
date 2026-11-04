# Complete API Reference

Full formal API documentation for the **`WaveshareDustSensor`** C++ library.

---

## Enumerations

### `enum class DustAirQuality : uint8_t`
Categorical classifications based on particulate matter (PM) concentration:

| Value | Range ($\mu\text{g/m}^3$) | Description |
| :--- | :--- | :--- |
| `DustAirQuality::EXCELLENT` | $0 - 35$ | Clean indoor air |
| `DustAirQuality::GOOD` | $35 - 75$ | Normal outdoor air |
| `DustAirQuality::MODERATE` | $75 - 115$ | Noticeable haze / moderate dust |
| `DustAirQuality::POOR` | $115 - 150$ | Unhealthy for sensitive groups |
| `DustAirQuality::VERY_POOR` | $150 - 250$ | Unhealthy air quality |
| `DustAirQuality::HAZARDOUS` | $> 250$ | Hazardous particulate levels |

---

## Class: `WaveshareDustSensor`

### Constructor & Initialization

#### `WaveshareDustSensor(uint8_t aoutPin, uint8_t iledPin, float vRef = 5.0f, uint16_t adcMax = 1023)`
* **Description:** Instantiates the sensor driver with pin assignments and ADC reference characteristics.
* **Parameters:**
  * `aoutPin`: Analog input pin connected to the sensor's `AOUT`.
  * `iledPin`: Digital output pin connected to the sensor's `ILED`.
  * `vRef`: ADC reference voltage in Volts (e.g., `5.0f` for Uno, `3.3f` for ESP32/Pico). Default: `5.0f`.
  * `adcMax`: Maximum integer count of the ADC (e.g., `1023` for 10-bit, `4095` for 12-bit). Default: `1023`.

#### `void begin()`
* **Description:** Initializes GPIO pin modes, drives `ILED` LOW, pre-arms the non-blocking timer, and resets internal filters. Call inside `setup()`.
* **Blocking Time:** $< 1\,\mu\text{s}$.

---

### Sampling & Measurement Execution

#### `bool read()`
* **Description:** Performs a synchronous (blocking) measurement cycle: turns ON `ILED`, waits $280\,\mu\text{s}$, reads the ADC, immediately turns OFF `ILED`, and waits the required $9.68\,\text{ms}$ cooldown before calculating physical units.
* **Blocking Time:** $\approx 10\,\text{ms}$.
* **Returns:** `true` when the measurement completes.

#### `bool update()`
* **Description:** Drives the non-blocking state machine using microsecond timestamps (`micros()`). Call repeatedly inside `loop()`. Automatically samples at the configured interval without blocking CPU execution.
* **Blocking Time:** $0\,\text{ms}$ (non-blocking).
* **Returns:** `true` when a new measurement cycle has completed and fresh data is ready.

#### `float calibrateZero(uint16_t sampleCount = 25, uint16_t delayBetweenSamplesMs = 20)`
* **Description:** Averages $N$ consecutive pulses in clean air to establish the sensor's clean-air baseline offset ($V_{\text{zero}}$). Automatically updates the cached density calculation upon completion.
* **Parameters:**
  * `sampleCount`: Number of samples to collect and average. Default: `25`.
  * `delayBetweenSamplesMs`: Delay between successive pulses in milliseconds. Default: `20`.
* **Returns:** Measured clean-air baseline voltage ($V_{\text{actual}}$).

---

### Configuration Modifiers

#### `void setSampleIntervalMs(uint32_t intervalMs)`
* **Description:** Sets the non-blocking sampling interval for `update()`. The library enforces a hardware floor of $10\,\text{ms}$ to protect the internal infrared diode.
* **Default:** `1000` ms ($1\,\text{Hz}$).

#### `uint32_t getSampleIntervalMs() const`
* **Returns:** Currently configured non-blocking interval in milliseconds.

#### `void setVRef(float vRef)`
* **Description:** Dynamically modifies the ADC reference voltage.

#### `void setAdcMax(uint16_t adcMax)`
* **Description:** Dynamically modifies the maximum ADC integer step count.

#### `void setVoltageDividerRatio(float ratio)`
* **Description:** Modifies the onboard hardware divider compensation factor.
* **Default:** `11.0f`.

#### `void setSensitivity(float sensitivityVPer100ug)`
* **Description:** Sets the optical sensitivity slope in Volts per $100\,\mu\text{g/m}^3$.
* **Default:** `0.5f` ($0.5\,\text{V}/100\,\mu\text{g/m}^3$).

#### `void setBaselineVoltage(float vZeroActual)`
* **Description:** Sets the clean-air baseline voltage offset in Volts ($V_{\text{actual}}$).
* **Default:** `0.9f`.

#### `float getBaselineVoltage() const`
* **Returns:** The active clean-air baseline offset in Volts.

#### `void enableFilter(bool enable, float alpha = 0.25f)`
* **Description:** Enables or disables the built-in Exponential Moving Average (EMA) filter.
* **Parameters:**
  * `enable`: `true` to enable filtering, `false` to disable.
  * `alpha`: Smoothing factor between $0.01$ (heavy smoothing) and $1.0$ (no smoothing).

#### `void resetFilter()`
* **Description:** Clears internal filter state, causing the next reading to initialize the filter buffer directly without ramp-up lag.

#### `void setAdcCallback(DustSensorAdcReader customReader)`
* **Description:** Attaches an external ADC reader function pointer (e.g. for I2C ADCs like the ADS1115).
* **Signature:** `uint16_t (*customReader)(uint8_t pin)`.

---

### Data Accessors

#### `uint16_t getRawADC() const`
* **Returns:** Most recent raw ADC integer reading ($0 - \text{ADC}_{\text{max}}$).

#### `float getPinVoltage() const`
* **Returns:** Voltage measured directly at the microcontroller pin in Volts ($0 - V_{\text{ref}}$).

#### `float getActualVoltage() const`
* **Returns:** Calculated module internal voltage ($V_{\text{pin}} \times 11.0$). If filtering is enabled, returns the smoothed value.

#### `float getRawActualVoltage() const`
* **Returns:** Raw unfiltered module internal voltage ($V_{\text{pin}} \times 11.0$).

#### `float getDustDensity() const`
* **Returns:** Particulate concentration in micrograms per cubic meter ($\mu\text{g/m}^3$).

#### `float getDustDensityMg() const`
* **Returns:** Particulate concentration in milligrams per cubic meter ($\text{mg/m}^3$).

#### `bool isSaturated() const`
* **Returns:** `true` if the reading has reached the optical sensor's physical ceiling ($\ge 500\,\mu\text{g/m}^3$ or $\ge 3.6\,\text{V}$).

#### `DustAirQuality getAirQuality() const`
* **Returns:** Categorical enum representing current air quality.

#### `const char* getAirQualityString() const`
* **Returns:** Descriptive human-readable string (e.g. `"Excellent (Clean)"`, `"Moderate (Hazy)"`, `"Hazardous"`).
