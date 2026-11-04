/**
 * @file 01_BasicReading.ino
 * @brief Minimal synchronous example reading dust concentration from the Waveshare Dust Sensor.
 * 
 * Hardware Connections (Arduino Uno / Nano):
 *   Waveshare Dust Sensor   ->   Arduino
 *   ---------------------        -------
 *   VCC                     ->   5V
 *   GND                     ->   GND
 *   AOUT                    ->   A0
 *   ILED                    ->   D6 (or D2)
 */

#include <WaveshareDustSensor.h>

// Define hardware pins (adjust to match your physical wiring)
const uint8_t PIN_AOUT = A0; // Analog output from sensor
const uint8_t PIN_ILED = 6;  // Digital drive pin for internal IR LED (e.g. D6 or D2)

// Instantiate sensor driver (AOUT pin, ILED pin, Vref = 5.0V, 10-bit ADC = 1023)
WaveshareDustSensor dustSensor(PIN_AOUT, PIN_ILED, 5.0f, 1023);

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        ; // Wait for serial port to connect (needed for native USB boards)
    }

    Serial.println(F("========================================"));
    Serial.println(F("Waveshare Dust Sensor - Basic Reading"));
    Serial.println(F("========================================"));

    // Initialize hardware pins
    dustSensor.begin();

    // Set clean-air baseline voltage (internal voltage).
    // Sharp GP2Y1010AU0F nominal is ~0.9V (typically 0.70V - 0.95V in normal rooms).
    // Tip: Run example 04_ZeroCalibration or call dustSensor.calibrateZero(15)
    // to automatically tune the zero-floor to your ambient room conditions.
    dustSensor.setBaselineVoltage(0.85f);
}

void loop() {
    // Perform synchronous measurement.
    // Passing 10 samples performs burst oversampling with median filtering (~100ms total),
    // eliminating single-pulse electrical noise, USB power jitter, and transient spikes.
    // Call dustSensor.read() or dustSensor.read(1) if you want single-pulse mode.
    if (dustSensor.read(10)) {
        Serial.print(F("Raw ADC: "));
        Serial.print(dustSensor.getRawADC());

        Serial.print(F(" | Pin Volts: "));
        Serial.print(dustSensor.getPinVoltage(), 3);
        Serial.print(F(" V"));

        Serial.print(F(" | Actual Volts: "));
        Serial.print(dustSensor.getActualVoltage(), 3);
        Serial.print(F(" V"));

        Serial.print(F(" | Dust: "));
        Serial.print(dustSensor.getDustDensity(), 1);
        Serial.print(F(" ug/m3"));

        Serial.print(F(" ["));
        Serial.print(dustSensor.getAirQualityString());
        Serial.println(F("]"));
    } else {
        Serial.println(F("Sensor read failed."));
    }

    // Wait 1 second before the next measurement
    delay(1000);
}
