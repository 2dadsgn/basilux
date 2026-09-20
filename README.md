# 🌿 Basilux - IoT Smart Planter & Grow Light Controller

Basilux is an IoT automation system designed for moisture monitoring and lighting control of indoor crops such as arugula, parsley, tomatoes, and basil. The core of the system is housed in a compact control unit based on the ESP8266 architecture, which manages a LED grow light and polls a network of capacitive sensors to ensure optimal growth conditions.

All control logic and the web user interface are hosted locally on the microcontroller, eliminating the need for external cloud servers or third-party subscriptions.

<img src="https://github.com/2dadsgn/basilux/blob/main/source_img/front.jpg" alt="Basilux Dashboard" width="350"/>
<img src="https://github.com/2dadsgn/basilux/blob/main/source_img/ui.jpg" alt="Basilux Dashboard" width="350"/>

## ✨ Main Features

*   **Adaptive Lighting (PWM):** Smooth brightness control of the LED lamp (0-100%) via a TIP122 power transistor.
*   **High-Precision Multi-Sensor Management:** Simultaneous reading of three independent capacitive soil moisture sensors via an ADS1115 16-bit analog-to-digital converter (ADC) over the I2C bus.
*   **Biological Presets:**
    *   🌱 **Low Growth:** An 8-hour reduced intensity (50%) cycle, ideal for preventing premature bolting in herbs and leafy greens like arugula and parsley.
    *   🚀 **Fast Growth:** A 12-hour maximum intensity (100%) cycle, optimized for the intensive vegetative development of demanding crops like tomatoes.
*   **Integrated Web Dashboard:** A mobile-first interface served directly from the ESP8266's Flash memory (`PROGMEM`). It features real-time animated circular progress rings for each pot's hydration status, brightness sliders, and schedule selectors.
*   **Autonomous Time Synchronization:** Precise management of day/night cycles via the NTP protocol, with automatic time zone and daylight saving time adjustments.

## 🛠️ Hardware Architecture

The system is centralized within a dedicated case from which wiring branches out to the operational modules in the individual pots.

*   **Microcontroller:** Wemos D1 Mini (ESP8266).
*   **Analog Expansion:** ADS1115 ADC Module (16-bit, I2C interface).
*   **Moisture Sensors:** 3x Capacitive sensors v1.2 (galvanic corrosion resistant).
*   **Power Stage (Light):** TIP122 Darlington Transistor (PWM driven) coupled with the grow light.
*   **Power Supply:** 3.3V distribution for the sensor logic and a dedicated voltage for the lamp load.

   <img src="https://github.com/2dadsgn/basilux/blob/main/source_img/side.jpg" alt="Basilux Dashboard" width="350"/>

## 💻 Tech Stack

*   **Backend:** C++ (Arduino Core for ESP8266).
*   **Frontend:** HTML5, CSS3, Vanilla JavaScript.
*   **Protocols:** HTTP (Local RESTful Web Server), I2C (Sensor communication), NTP (Time synchronization).
*   **Libraries:** `ESP8266WiFi`, `ESP8266WebServer`, `Wire`, `Adafruit_ADS1X15`.

## 🚀 Setup and Installation

1.  **Hardware Configuration:** 
    *   Connect the `SDA` and `SCL` pins of the ADS1115 to the `D6` and `D5` pins of the Wemos D1 Mini, respectively.
    *   Connect the signal pins of the capacitive sensors to the `A0`, `A1`, and `A2` inputs of the ADS1115.
    *   Connect the base of the TIP122 to the `D2` pin (via a resistor) for PWM light control.
2.  **Software Configuration:**
    *   Clone the repository.
    *   Update the `ssid` and `password` constants in the main `.ino` file with your Wi-Fi network credentials.
    *   Set the desired static IP address by modifying the `local_IP` directive.
    *   Calibrate the `airValue` and `waterValue` variables by reading the raw data in the Serial Monitor to accurately map the percentage (0-100%) for your specific soil type.
3.  **Deployment:** Compile and upload the firmware to the board using the Arduino IDE. Access the configured static IP from the browser of any device connected to the same local network.
