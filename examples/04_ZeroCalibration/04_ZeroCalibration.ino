/**
 * @file 04_ZeroCalibration.ino
 * @brief Interactive zero-dust baseline calibration wizard.
 * 
 * Hardware Connections:
 *   Waveshare Dust Sensor   ->   Arduino Uno / Nano
 *   VCC                     ->   5V
 *   GND                     ->   GND
 *   AOUT                    ->   A0
 *   ILED                    ->   D2
 * 
 * Instructions:
 *   1. Place the sensor in a clean air environment (e.g. clean indoor room or sealed enclosure).
 *   2. Open the Serial Monitor at 9600 baud.
 *   3. Send 'c' or any character in the Serial Monitor to trigger a 50-sample calibration.
 *   4. Note the reported baseline voltage and pass it to setBaselineVoltage() in your main sketch.
 */

#include <WaveshareDustSensor.h>

const uint8_t PIN_AOUT = A0;
const uint8_t PIN_ILED = 6; // Pin 6 (or D2 depending on your wiring)

WaveshareDustSensor dustSensor(PIN_AOUT, PIN_ILED, 5.0f, 1023);

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        ;
    }

    dustSensor.begin();

    Serial.println(F("=================================================="));
    Serial.println(F("  Waveshare Dust Sensor Zero-Point Calibration    "));
    Serial.println(F("=================================================="));
    Serial.println(F("Instructions:"));
    Serial.println(F("1. Ensure the sensor is situated in clean, dust-free air."));
    Serial.println(F("2. Send 'c' via the Serial Monitor to start calibration."));
    Serial.println(F("=================================================="));
}

void loop() {
    if (Serial.available() > 0) {
        char ch = Serial.read();
        if (ch == 'c' || ch == 'C') {
            Serial.println(F("\n[Calibrating...] Averaging 50 consecutive pulses..."));

            // Calibrate by averaging 50 pulses with 20ms separation
            float newBaseline = dustSensor.calibrateZero(50, 20);

            Serial.print(F(">>> Calibration complete! Measured Baseline Voltage: "));
            Serial.print(newBaseline, 4);
            Serial.println(F(" V"));

            Serial.println(F("\nTo use this baseline in your projects, add this line in setup():"));
            Serial.print(F("  dustSensor.setBaselineVoltage("));
            Serial.print(newBaseline, 4);
            Serial.println(F("f);"));
            Serial.println(F("\nSend 'c' again to re-calibrate."));
        }
    }
}

