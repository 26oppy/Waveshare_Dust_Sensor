/**
 * @file 02_FilteredReading.ino
 * @brief Demonstrates using the built-in Exponential Moving Average (EMA) filter
 *        to reject noise and particle fluctuations.
 * 
 * Hardware Connections:
 *   Waveshare Dust Sensor   ->   Arduino Uno / Nano
 *   VCC                     ->   5V
 *   GND                     ->   GND
 *   AOUT                    ->   A0
 *   ILED                    ->   D2
 */

#include <WaveshareDustSensor.h>

const uint8_t PIN_AOUT = A0;
const uint8_t PIN_ILED = 2;

WaveshareDustSensor dustSensor(PIN_AOUT, PIN_ILED, 5.0f, 1023);

void setup() {
    Serial.begin(9600);
    dustSensor.begin();

    // Enable EMA filter with an alpha of 0.20 (0.01 = heavy smoothing, 1.0 = raw/no filter)
    // A value of 0.15 - 0.25 provides an excellent balance of responsiveness and noise rejection.
    dustSensor.enableFilter(true, 0.20f);

    Serial.println(F("Waveshare Dust Sensor - Filtered Reading Started (EMA Alpha = 0.20)"));
}

void loop() {
    dustSensor.read();

    Serial.print(F("Smoothed Voltage: "));
    Serial.print(dustSensor.getActualVoltage(), 3);
    Serial.print(F(" V | Concentration: "));
    Serial.print(dustSensor.getDustDensity(), 2);
    Serial.print(F(" ug/m3 ("));
    Serial.print(dustSensor.getDustDensityMg(), 4);
    Serial.print(F(" mg/m3) | Air Quality: "));
    Serial.println(dustSensor.getAirQualityString());

    delay(200); // 5Hz sampling rate
}
