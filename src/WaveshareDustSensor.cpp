/**
 * @file WaveshareDustSensor.cpp
 * @brief Implementation of the lightweight Waveshare Dust Sensor driver.
 * @author Waveshare Dust Sensor Library Contributors
 * @license MIT
 */

#include "WaveshareDustSensor.h"

// Optical pulse timing parameters (in microseconds) according to Sharp GP2Y1010AU0F specifications
static const uint8_t UNASSIGNED_PIN         = 255;  // Sentinel for unconfigured pins
static const uint32_t PULSE_SAMPLE_WAIT_US   = 280;  // 0.28ms LED stabilization time
static const uint32_t CYCLE_OFF_TIME_US      = 9680; // 9.68ms minimum off-time (total cycle >= 10ms)
static const uint32_t MIN_CYCLE_INTERVAL_MS  = 10;   // 10ms minimum sampling interval (100Hz max)

WaveshareDustSensor::WaveshareDustSensor()
    : WaveshareDustSensor(UNASSIGNED_PIN, UNASSIGNED_PIN, 5.0f, 1023) {}

WaveshareDustSensor::WaveshareDustSensor(uint8_t aoutPin, uint8_t iledPin, float vRef, uint16_t adcMax)
    : _aoutPin(aoutPin),
      _iledPin(iledPin),
      _vRef(vRef),
      _adcMax(adcMax > 0 ? adcMax : 1023),
      _dividerRatio(11.0f),       // Waveshare onboard 11:1 hardware voltage divider
      _sensitivity(0.5f),         // 0.5V per 100 ug/m3
      _vZero(0.9f),               // Nominal clean-air baseline internal voltage (~0.9V actual)
      _filterEnabled(false),
      _filterAlpha(0.25f),
      _filteredVoltage(0.0f),
      _filterInitialized(false),
      _customAdcReader(nullptr),
      _lastRawADC(0),
      _lastPinVoltage(0.0f),
      _lastRawActualVoltage(0.0f),
      _lastActualVoltage(0.0f),
      _lastDustDensity(0.0f),
      _state(State::IDLE),
      _stateTimerMicros(0),
      _lastSampleMillis(0),
      _sampleIntervalMs(1000) {}  // Default 1Hz (1000ms) interval for non-blocking update()

void WaveshareDustSensor::begin() {
    if (_iledPin != UNASSIGNED_PIN) {
        pinMode(_iledPin, OUTPUT);
        digitalWrite(_iledPin, LOW); // Ensure IR LED is off initially
    }
    if (_aoutPin != UNASSIGNED_PIN) {
        pinMode(_aoutPin, INPUT);    // Configure ADC pin in input mode
    }
    _state = State::IDLE;
    _stateTimerMicros = micros();
    // Pre-arm so first call to update() triggers immediately
    _lastSampleMillis = millis() - _sampleIntervalMs;
    resetFilter();
}

void WaveshareDustSensor::begin(uint8_t aoutPin, uint8_t iledPin) {
    setPins(aoutPin, iledPin);
    begin();
}

void WaveshareDustSensor::setPins(uint8_t aoutPin, uint8_t iledPin) {
    _aoutPin = aoutPin;
    _iledPin = iledPin;
}

uint8_t WaveshareDustSensor::getAoutPin() const {
    return _aoutPin;
}

uint8_t WaveshareDustSensor::getIledPin() const {
    return _iledPin;
}

bool WaveshareDustSensor::read() {
    // Fail-safe: ensure valid pins are configured
    if (_aoutPin == UNASSIGNED_PIN || _iledPin == UNASSIGNED_PIN) {
        return false;
    }

    // 1. Enable internal infrared emitting diode
    digitalWrite(_iledPin, HIGH);

    // 2. Wait 0.28ms for the optical signal to reach steady state
    delayMicroseconds(PULSE_SAMPLE_WAIT_US);

    // 3. Read analog voltage from sensor
    // Note: analogRead takes ~112us on AVR (16MHz) or ~10us on ESP32.
    // The ADC sample-and-hold captures the signal within the first ~12us.
    uint16_t rawValue = readAdcRaw();

    // 4. Turn off internal infrared diode immediately after reading
    // This prevents pulse width from exceeding 320us on slower microcontrollers
    digitalWrite(_iledPin, LOW);

    // 5. Complete the minimum off-time recovery period (9.68ms)
    delayMicroseconds(CYCLE_OFF_TIME_US);

    // 6. Reset state machine timer so non-blocking mode stays aligned
    _state = State::IDLE;
    _stateTimerMicros = micros();

    // 7. Calculate physical values
    processRawValue(rawValue);

    return true;
}

bool WaveshareDustSensor::update() {
    // Fail-safe: ensure valid pins are configured
    if (_aoutPin == UNASSIGNED_PIN || _iledPin == UNASSIGNED_PIN) {
        return false;
    }

    uint32_t currentMillis = millis();
    uint32_t currentMicros = micros();

    switch (_state) {
        case State::IDLE:
            // Check if the user-configured sample interval has elapsed
            if ((currentMillis - _lastSampleMillis) >= _sampleIntervalMs) {
                digitalWrite(_iledPin, HIGH); // Turn ON IR LED
                _stateTimerMicros = currentMicros;
                _state = State::WAITING_STABILIZATION;
            }
            break;

        case State::WAITING_STABILIZATION:
            // Wait 280us for signal stabilization
            if ((currentMicros - _stateTimerMicros) >= PULSE_SAMPLE_WAIT_US) {
                uint16_t rawValue = readAdcRaw();
                digitalWrite(_iledPin, LOW); // Turn OFF IR LED immediately

                processRawValue(rawValue);

                _lastSampleMillis = currentMillis;
                _stateTimerMicros = currentMicros;
                _state = State::WAITING_COOLDOWN;
                return true; // Fresh measurement ready!
            }
            break;

        case State::WAITING_COOLDOWN:
            // Enforce at least 9.68ms off-time before next cycle can trigger
            if ((currentMicros - _stateTimerMicros) >= CYCLE_OFF_TIME_US) {
                _state = State::IDLE;
            }
            break;
    }

    return false;
}

void WaveshareDustSensor::setSampleIntervalMs(uint32_t intervalMs) {
    if (intervalMs < MIN_CYCLE_INTERVAL_MS) {
        _sampleIntervalMs = MIN_CYCLE_INTERVAL_MS; // Enforce minimum 10ms datasheet limit
    } else {
        _sampleIntervalMs = intervalMs;
    }
}

uint32_t WaveshareDustSensor::getSampleIntervalMs() const {
    return _sampleIntervalMs;
}

float WaveshareDustSensor::calibrateZero(uint16_t sampleCount, uint16_t delayBetweenSamplesMs) {
    if (sampleCount == 0) {
        return _vZero;
    }

    float accumulatedVoltage = 0.0f;
    uint16_t validSamples = 0;

    for (uint16_t i = 0; i < sampleCount; ++i) {
        if (read()) {
            accumulatedVoltage += _lastRawActualVoltage;
            validSamples++;
        }
        if (delayBetweenSamplesMs > 0) {
            delay(delayBetweenSamplesMs);
        }
    }

    if (validSamples > 0) {
        _vZero = accumulatedVoltage / (float)validSamples;
        // Recalculate last reading with the newly calibrated baseline
        float deltaV = _lastActualVoltage - _vZero;
        if (deltaV < 0.0f) {
            deltaV = 0.0f;
        }
        _lastDustDensity = (deltaV / _sensitivity) * 100.0f;
    }

    return _vZero;
}

void WaveshareDustSensor::setVRef(float vRef) {
    if (vRef > 0.0f) {
        _vRef = vRef;
    }
}

void WaveshareDustSensor::setAdcMax(uint16_t adcMax) {
    if (adcMax > 0) {
        _adcMax = adcMax;
    }
}

void WaveshareDustSensor::setVoltageDividerRatio(float ratio) {
    if (ratio > 0.0f) {
        _dividerRatio = ratio;
    }
}

void WaveshareDustSensor::setSensitivity(float sensitivityVPer100ug) {
    if (sensitivityVPer100ug > 0.0f) {
        _sensitivity = sensitivityVPer100ug;
    }
}

void WaveshareDustSensor::setBaselineVoltage(float vZeroActual) {
    if (vZeroActual >= 0.0f) {
        _vZero = vZeroActual;
    }
}

float WaveshareDustSensor::getBaselineVoltage() const {
    return _vZero;
}

void WaveshareDustSensor::enableFilter(bool enable, float alpha) {
    _filterEnabled = enable;
    if (alpha > 0.0f && alpha <= 1.0f) {
        _filterAlpha = alpha;
    }
    resetFilter();
}

void WaveshareDustSensor::resetFilter() {
    _filterInitialized = false;
    _filteredVoltage = 0.0f;
}

void WaveshareDustSensor::setAdcCallback(DustSensorAdcReader customReader) {
    _customAdcReader = customReader;
}

uint16_t WaveshareDustSensor::getRawADC() const {
    return _lastRawADC;
}

float WaveshareDustSensor::getPinVoltage() const {
    return _lastPinVoltage;
}

float WaveshareDustSensor::getActualVoltage() const {
    return _lastActualVoltage;
}

float WaveshareDustSensor::getRawActualVoltage() const {
    return _lastRawActualVoltage;
}

float WaveshareDustSensor::getDustDensity() const {
    return _lastDustDensity;
}

float WaveshareDustSensor::getDustDensityMg() const {
    return _lastDustDensity / 1000.0f;
}

bool WaveshareDustSensor::isSaturated() const {
    return (_lastActualVoltage >= 3.6f || _lastDustDensity >= 500.0f);
}

DustAirQuality WaveshareDustSensor::getAirQuality() const {
    if (_lastDustDensity < 35.0f) {
        return DustAirQuality::EXCELLENT;
    } else if (_lastDustDensity < 75.0f) {
        return DustAirQuality::GOOD;
    } else if (_lastDustDensity < 115.0f) {
        return DustAirQuality::MODERATE;
    } else if (_lastDustDensity < 150.0f) {
        return DustAirQuality::POOR;
    } else if (_lastDustDensity < 250.0f) {
        return DustAirQuality::VERY_POOR;
    } else {
        return DustAirQuality::HAZARDOUS;
    }
}

const char* WaveshareDustSensor::getAirQualityString() const {
    switch (getAirQuality()) {
        case DustAirQuality::EXCELLENT: return "Excellent (Clean)";
        case DustAirQuality::GOOD:      return "Good";
        case DustAirQuality::MODERATE:  return "Moderate (Hazy)";
        case DustAirQuality::POOR:      return "Poor (Sensitive Warning)";
        case DustAirQuality::VERY_POOR: return "Very Poor (Unhealthy)";
        case DustAirQuality::HAZARDOUS: return "Hazardous";
        default:                        return "Unknown";
    }
}

uint16_t WaveshareDustSensor::readAdcRaw() {
    if (_customAdcReader != nullptr) {
        return _customAdcReader(_aoutPin);
    }
    return static_cast<uint16_t>(analogRead(_aoutPin));
}

void WaveshareDustSensor::processRawValue(uint16_t rawAdc) {
    _lastRawADC = rawAdc;

    // 1. Calculate voltage measured directly on the microcontroller ADC pin
    _lastPinVoltage = (static_cast<float>(rawAdc) * _vRef) / static_cast<float>(_adcMax);

    // 2. Account for Waveshare onboard 11:1 voltage divider to obtain actual sensor voltage
    _lastRawActualVoltage = _lastPinVoltage * _dividerRatio;

    // 3. Apply Exponential Moving Average (EMA) filter if enabled
    if (_filterEnabled) {
        if (!_filterInitialized) {
            _filteredVoltage = _lastRawActualVoltage;
            _filterInitialized = true;
        } else {
            _filteredVoltage = (_filterAlpha * _lastRawActualVoltage) + ((1.0f - _filterAlpha) * _filteredVoltage);
        }
        _lastActualVoltage = _filteredVoltage;
    } else {
        _lastActualVoltage = _lastRawActualVoltage;
    }

    // 4. Calculate delta above zero-dust clean-air baseline
    float deltaV = _lastActualVoltage - _vZero;
    if (deltaV < 0.0f) {
        deltaV = 0.0f; // Clip negative noise below clean baseline
    }

    // 5. Convert delta voltage to dust density: (deltaV / sensitivity) * 100 ug/m3
    _lastDustDensity = (deltaV / _sensitivity) * 100.0f;
}

