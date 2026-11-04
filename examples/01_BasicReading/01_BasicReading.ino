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
 *   ILED                    ->   D2
 */

#include <WaveshareDustSensor.h>

// Define hardware pins
const uint8_t PIN_AOUT = A0; // Analog output from sensor
const uint8_t PIN_ILED = 2;  // Digital drive pin for internal IR LED

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

    // Optional: Set your known clean-air baseline voltage (internal voltage)
    // Nominal default is 0.9V. Run the 04_ZeroCalibration sketch to measure your exact baseline.
    dustSensor.setBaselineVoltage(0.9f);
}

void loop() {
    // Perform synchronous measurement pulse (blocks for ~10ms)
    if (dustSensor.read()) {
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

