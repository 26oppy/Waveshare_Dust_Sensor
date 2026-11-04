/**
 * @file WaveshareDustSensor.h
 * @brief Lightweight, non-blocking library for the Waveshare Dust Sensor (Sharp GP2Y1010AU0F).
 * @author Waveshare Dust Sensor Library Contributors
 * @license MIT
 */

#ifndef WAVESHARE_DUST_SENSOR_H
#define WAVESHARE_DUST_SENSOR_H

#include <Arduino.h>

/**
 * @brief Air quality index categories based on particulate matter concentration.
 */
enum class DustAirQuality : uint8_t {
    EXCELLENT = 0,  ///< 0 - 35 ug/m3 (Clean indoor air)
    GOOD,           ///< 35 - 75 ug/m3 (Normal outdoor air)
    MODERATE,       ///< 75 - 115 ug/m3 (Noticeable haze/dust)
    POOR,           ///< 115 - 150 ug/m3 (Unhealthy for sensitive groups)
    VERY_POOR,      ///< 150 - 250 ug/m3 (Unhealthy air quality)
    HAZARDOUS       ///< > 250 ug/m3 (Hazardous air quality)
};

/**
 * @brief Function pointer signature for optional custom ADC reading routines.
 * Enables seamless integration with external ADCs (e.g. ADS1115) or custom HALs.
 */
typedef uint16_t (*DustSensorAdcReader)(uint8_t pin);

class WaveshareDustSensor {
public:
    /**
     * @brief Construct a new Waveshare Dust Sensor driver.
     * @param aoutPin Microcontroller analog pin connected to AOUT.
     * @param iledPin Microcontroller digital pin connected to ILED.
     * @param vRef ADC reference voltage in Volts (default: 5.0V).
     * @param adcMax Maximum ADC value (default: 1023 for 10-bit ADC, use 4095 for 12-bit).
     */
    WaveshareDustSensor(uint8_t aoutPin, uint8_t iledPin, float vRef = 5.0f, uint16_t adcMax = 1023);

    /**
     * @brief Initialize hardware pins. Call this in setup().
     */
    void begin();

    // -------------------------------------------------------------------------
    // Measurement Methods
    // -------------------------------------------------------------------------

    /**
     * @brief Perform a synchronous (blocking) measurement.
     * Generates the 320us LED pulse, samples AOUT at 280us, and applies cool-down.
     * @return true if measurement succeeded, false otherwise.
     */
    bool read();

    /**
     * @brief Non-blocking state machine updater.
     * Call this repeatedly inside loop().
     * Controls LED pulse timing and off-time cool-down asynchronously using micros().
     * @return true when a new complete measurement sample has been processed.
     */
    bool update();

    /**
     * @brief Calibrate the baseline zero-dust voltage in clean air.
     * @param sampleCount Number of samples to average (default: 25).
     * @param delayBetweenSamplesMs Delay between successive calibration pulses in ms (default: 20).
     * @return Calibrated baseline voltage (actual sensor internal voltage).
     */
    float calibrateZero(uint16_t sampleCount = 25, uint16_t delayBetweenSamplesMs = 20);

    // -------------------------------------------------------------------------
    // Configuration & Calibration Setters
    // -------------------------------------------------------------------------

    /**
     * @brief Set the ADC reference voltage (e.g. 3.3V or 5.0V).
     */
    void setVRef(float vRef);

    /**
     * @brief Set the maximum ADC count (e.g. 1023 for 10-bit, 4095 for 12-bit).
     */
    void setAdcMax(uint16_t adcMax);

    /**
     * @brief Set the onboard hardware voltage divider ratio.
     * The Waveshare module includes an onboard ~11:1 voltage divider. Default is 11.0f.
     */
    void setVoltageDividerRatio(float ratio);

    /**
     * @brief Set the optical sensor sensitivity in Volts per (100 ug/m3).
     * Sharp GP2Y1010AU0F nominal value is 0.5V per 100 ug/m3.
     */
    void setSensitivity(float sensitivityVPer100ug);

    /**
     * @brief Manually set the zero-dust baseline voltage (internal sensor voltage).
     * Default is typically around 0.9V (actual).
     */
    void setBaselineVoltage(float vZeroActual);

    /**
     * @brief Get the current zero-dust baseline voltage (internal sensor voltage).
     */
    float getBaselineVoltage() const;

    /**
     * @brief Configure exponential moving average (EMA) filter.
     * @param enable Enable or disable filtering.
     * @param alpha Smoothing factor between 0.01 (heavy smoothing) and 1.0 (no smoothing).
     */
    void enableFilter(bool enable, float alpha = 0.25f);

    /**
     * @brief Reset the internal filter state.
     */
    void resetFilter();

    /**
     * @brief Attach a custom ADC reader callback (e.g. for external ADS1115 ADC).
     * If nullptr, the library uses Arduino's standard analogRead().
     */
    void setAdcCallback(DustSensorAdcReader customReader);

    // -------------------------------------------------------------------------
    // Measurement Getters (From latest sample)
    // -------------------------------------------------------------------------

    /**
     * @brief Get the most recent raw ADC reading.
     */
    uint16_t getRawADC() const;

    /**
     * @brief Get the voltage measured directly on the MCU analog pin (in Volts).
     */
    float getPinVoltage() const;

    /**
     * @brief Get the actual internal sensor output voltage (PinVoltage * DividerRatio).
     */
    float getActualVoltage() const;

    /**
     * @brief Get the calculated dust density in micrograms per cubic meter (ug/m3).
     */
    float getDustDensity() const;

    /**
     * @brief Get the calculated dust density in milligrams per cubic meter (mg/m3).
     */
    float getDustDensityMg() const;

    /**
     * @brief Get the Air Quality qualitative category.
     */
    DustAirQuality getAirQuality() const;

    /**
     * @brief Get a human-readable string for the current air quality.
     */
    const char* getAirQualityString() const;

private:
    // Pin mapping
    uint8_t _aoutPin;
    uint8_t _iledPin;

    // Hardware parameters
    float _vRef;
    uint16_t _adcMax;
    float _dividerRatio;
    float _sensitivity;
    float _vZero;

    // Filter settings & state
    bool _filterEnabled;
    float _filterAlpha;
    float _filteredVoltage;
    bool _filterInitialized;

    // Custom ADC callback
    DustSensorAdcReader _customAdcReader;

    // Cached measurement results
    uint16_t _lastRawADC;
    float _lastPinVoltage;
    float _lastActualVoltage;
    float _lastDustDensity;

    // Non-blocking state machine definitions
    enum class State : uint8_t {
        IDLE = 0,
        WAITING_STABILIZATION,
        WAITING_CYCLE_END
    };
    State _state;
    uint32_t _stateTimerMicros;

    // Internal helper methods
    uint16_t readAdcRaw();
    void processRawValue(uint16_t rawAdc);
};

#endif // WAVESHARE_DUST_SENSOR_H
