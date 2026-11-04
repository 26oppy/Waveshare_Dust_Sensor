/**
 * @file 03_NonBlocking.ino
 * @brief Demonstrates non-blocking state machine sampling using update().
 *        Allows the microcontroller to perform other operations (like blinking an LED)
 *        without blocking delays.
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
const uint8_t PIN_LED  = LED_BUILTIN; // Built-in heartbeat LED

WaveshareDustSensor dustSensor(PIN_AOUT, PIN_ILED, 5.0f, 1023);

// Timing variables for parallel heartbeat task
unsigned long lastHeartbeat = 0;
bool ledState = false;

void setup() {
    Serial.begin(9600);
    pinMode(PIN_LED, OUTPUT);

    dustSensor.begin();
    dustSensor.enableFilter(true, 0.25f);

    // Set non-blocking sampling rate to once per second (1000 ms)
    // The driver will handle the 280us pulse, sampling, and 9.68ms cooldown asynchronously!
    dustSensor.setSampleIntervalMs(1000);

    Serial.println(F("Waveshare Dust Sensor - Non-Blocking State Machine Demo"));
}

void loop() {
    // 1. Task 1: Non-blocking dust sensor state machine
    // update() manages microsecond timing internally and returns true when a sample completes
    if (dustSensor.update()) {
        Serial.print(F("[Sample Ready] Density: "));
        Serial.print(dustSensor.getDustDensity(), 1);
        Serial.print(F(" ug/m3 | Status: "));
        Serial.println(dustSensor.getAirQualityString());
    }

    // 2. Task 2: Completely independent non-blocking heartbeat task (toggles LED every 500ms)
    unsigned long currentMillis = millis();
    if (currentMillis - lastHeartbeat >= 500) {
        lastHeartbeat = currentMillis;
        ledState = !ledState;
        digitalWrite(PIN_LED, ledState ? HIGH : LOW);
    }

    // Any other application code or communication protocols can run freely here!
}

