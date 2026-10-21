# Project-1-ESP32-DHT11-OLED-Display
Temperature and Humidity Monitoring using ESP32, DHT11 and OLED Display
This project uses an ESP32 DevKit to read temperature and relative humidity from a DHT11 sensor, display device/network status on a 128 × 64 I²C OLED, and publish readings as JSON over MQTT to a Raspberry Pi / Node-RED system. The ESP32 subscribes to an acknowledgment (ACK) topic. A red LED indicates a transmission attempt and a green LED indicates that an ACK message was received.
Item	Details from supplied sketch
Controller	ESP32 DevKit; 3.3 V logic
Sensor	DHT11; data pin GPIO19
Display	SSD1306 OLED, 128 × 64, I²C address 0x3C
OLED I²C pins	SDA GPIO21, SCL GPIO22
Red TX LED	GPIO18
Green ACK LED	GPIO23
Wi-Fi SSID	Ecopro (credentials are configured in the sketch)
MQTT broker	172.16.0.132, TCP port 1883
Sensor publish topic	ecoprosus/esp32-02/temperature/humidity
ACK subscribe topic	ecoprosus/esp32-02/ack
Device ID in JSON	ESP32_Node_02
Publish interval	30 seconds
DHT read interval	2 seconds
System Architecture
Data path:
DHT11 → ESP32 → Wi-Fi → MQTT Broker (Raspberry Pi) → Node-RED Dashboard
Acknowledgment path:
Node-RED / Raspberry Pi → MQTT ACK topic → ESP32 → Green ACK LED
The OLED shows Wi-Fi state, MQTT state, date, temperature and humidity.
The sketch attempts NTP synchronization using pool.ntp.org.
The JSON timestamp is generated in UTC and ends with “Z”; the OLED time/date is displayed using an IST offset of UTC+05:30.
The ACK LED only blinks if a message is received on the subscribed ACK topic after a successful publish.
<img width="497" height="345" alt="image" src="https://github.com/user-attachments/assets/74ca903c-0bae-4b5d-b967-dbf06f4e2a94" />
