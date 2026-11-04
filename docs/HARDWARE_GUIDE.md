# Waveshare Dust Sensor - Hardware & Understanding Guide

This guide breaks down exactly how the **Waveshare Dust Sensor** and the **Sharp GP2Y1010AU0F** component work under the hood in plain, easy-to-understand language.

---

## 1. What Is Inside This Sensor?

The sensor module is divided into two parts:
1. **The Black Plastic Block (Sharp GP2Y1010AU0F):** The optical sensing chamber.
2. **The Blue PCB Breakout (Waveshare Board):** The electronics that power and adapt the sensor for microcontrollers.

```
       +--------------------------------------------------------+
       |                  SHARP OPTICAL SENSOR                  |
       |                                                        |
       |   [Infrared LED] --->                     [Lens]       |
       |                          \               /             |
       |                   (Dust Particles Enter Here)          |
       |                          /               \             |
       |                     [Lens]       [Phototransistor]     |
       +--------------------------------------------------------+
                                  |
                                  v
       +--------------------------------------------------------+
       |                WAVESHARE BREAKOUT BOARD                |
       |                                                        |
       |  * 5V Boost Regulator (accepts 2.5V - 5.5V input)      |
       |  * NPN Transistor Inverter (drives IR LED active-HIGH) |
       |  * 11:1 Resistor Divider (scales analog voltage down)  |
       +--------------------------------------------------------+
```

---

## 2. How Does It Detect Dust?

1. **Dark Chamber:** The inside of the sensor is a dark chamber.
2. **Infrared Beam:** An internal infrared diode emits a narrow beam of light across the chamber.
3. **Phototransistor (Light Detector):** The detector is positioned at an angle where the direct beam **cannot** hit it.
4. **Light Scattering:** When air carrying dust, smoke, or fine particles enters the chamber, the infrared light bounces (scatters) off the dust particles.
5. **Voltage Rise:** Some of the scattered light hits the phototransistor. The more dust particles in the air, the more light scatters, and the higher the analog output voltage becomes.

---

## 3. The Three Big Peculiarities of the Waveshare Board

When working with this sensor, there are 3 critical hardware facts that confuse many developers:

### A. Why do we pulse the IR LED instead of leaving it ON?
The infrared LED draws around **20 mA**. If you left it on constantly:
* The LED would heat up, and its brightness would drop drastically (thermal droop).
* The sensor's lifetime would shorten.
* The Sharp datasheet strictly specifies a **maximum pulse of 320 microseconds ($0.32\,\text{ms}$)** and a minimum cooldown of **9.68 milliseconds** between pulses (representing a maximum duty cycle of only $3.2\%$).

### B. Why is ILED driven HIGH on Waveshare (unlike raw Sharp sensors)?
On raw Sharp sensors, pulling the pin LOW turns the LED ON. But Waveshare added an **NPN transistor driver** on their breakout board. 
* Writing `digitalWrite(ILED, HIGH)` turns the IR LED **ON**.
* Writing `digitalWrite(ILED, LOW)` turns the IR LED **OFF**.

### C. The 11:1 Voltage Divider on `AOUT`
Inside the sensor, the phototransistor amplifier can output up to **$3.8\,\text{V}$**. 
To make sure this output never damages sensitive $3.3\,\text{V}$ microcontroller inputs (like ESP32 or Raspberry Pi Pico), Waveshare placed an **11:1 resistor voltage divider** on the board.
* If the sensor produces $1.1\,\text{V}$ internally, the pin on your Arduino only sees $0.1\,\text{V}$.
* Therefore, the library automatically multiplies the measured pin voltage by **$11$** to get the actual sensor reading ($V_{\text{actual}} = V_{\text{pin}} \times 11$).

---

## 4. The Linear Conversion Formula

Once we calculate the actual sensor voltage ($V_{\text{actual}}$), we calculate dust density using the sensor's characteristic sensitivity:

$$\text{Density } (\mu\text{g/m}^3) = (V_{\text{actual}} - V_{\text{zero}}) \times 200$$

* **$V_{\text{zero}}$ (Clean-Air Baseline):** When the air is completely clean, the sensor still outputs a small voltage (typically around $0.9\,\text{V}$ actual, or $\sim 0.08\,\text{V}$ on the pin).
* **$\Delta V = V_{\text{actual}} - V_{\text{zero}}$:** The change in voltage directly caused by dust particles.
* **$200$ Conversion Factor:** Sharp rates this sensor at $0.5\,\text{V}$ per $100\,\mu\text{g/m}^3$. 
  $$\frac{100\,\mu\text{g/m}^3}{0.5\,\text{V}} = 200\,\mu\text{g/m}^3\text{ per Volt}$$

---

## 5. Physical Installation & Best Practices

1. **Airflow Direction:** Mount the sensor so that the round air through-hole is **vertical** (pointing up and down). Natural convection will carry warm air and dust upwards through the hole.
2. **Never Clean with Liquids:** Do not use alcohol, water, or cotton swabs inside the hole. The optical lenses can get scratched or stained. Use gentle compressed air if cleaning is required.
3. **Avoid Bright Sunlight:** The optical chamber is shielded, but intense direct sunlight entering the hole can produce false dust readings.

