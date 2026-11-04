/**
 * @file 05_ESP32_3V3_Operation.ino
 * @brief Demonstrates using the Waveshare Dust Sensor on 3.3V logic microcontrollers
 *        with 12-bit ADCs (e.g. ESP32, ESP32-S3, RP2040 Raspberry Pi Pico).
 * 
 * Hardware Connections (ESP32):
 *   Waveshare Dust Sensor   ->   ESP32
 *   ---------------------        -----
 *   VCC                     ->   3.3V (or 5V if powering from VIN/VUSB)
 *   GND                     ->   GND
 *   AOUT                    ->   GPIO 34 (ADC1_CH6 - Input only pin)
 *   ILED                    ->   GPIO 25 (Digital Output)
 */

#include <WaveshareDustSensor.h>

// ESP32 ADC pin and digital output pin
const uint8_t PIN_AOUT = 34; // GPIO 34 (ADC1 channel)
const uint8_t PIN_ILED = 25; // GPIO 25

// Instantiate with 3.3V reference and 12-bit ADC max (4095)
WaveshareDustSensor dustSensor(PIN_AOUT, PIN_ILED, 3.3f, 4095);

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    Serial.println(F("\n============================================="));
    Serial.println(F("Waveshare Dust Sensor - ESP32 3.3V (12-bit ADC)"));
    Serial.println(F("============================================="));

    // Initialize hardware pins
    dustSensor.begin();

    // Enable smooth filtering
    dustSensor.enableFilter(true, 0.20f);
}

void loop() {
    if (dustSensor.read()) {
        Serial.printf("Raw ADC (12-bit): %4d | Pin Volts: %1.3f V | Actual Sensor: %1.3f V | Density: %3.1f ug/m3 [%s]\n",
                      dustSensor.getRawADC(),
                      dustSensor.getPinVoltage(),
                      dustSensor.getActualVoltage(),
                      dustSensor.getDustDensity(),
                      dustSensor.getAirQualityString());
    }

    delay(500);
}
