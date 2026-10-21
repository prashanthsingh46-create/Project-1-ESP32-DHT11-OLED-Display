# Project-1-ESP32-DHT11-OLED-Display
Temperature and Humidity Monitoring using ESP32, DHT11 and OLED Displ

OLED • Wi-Fi • NTP Time • MQTT • JSON • Node-RED Acknowledgment
<img width="489" height="274" alt="image" src="https://github.com/user-attachments/assets/b1828c4b-da0c-4374-9a54-10e51410604c" />

1. Project Overview
This project uses an ESP32 DevKit to read temperature and relative humidity from a DHT11 sensor, display device/network status on a 128 × 64 I²C OLED, and publish readings as JSON over MQTT to a Raspberry Pi / Node-RED system. The ESP32 subscribes to an acknowledgment (ACK) topic. A red LED indicates a transmission attempt and a green LED indicates that an ACK message was received.
<img width="511" height="179" alt="image" src="https://github.com/user-attachments/assets/1f7e9072-8b9d-4fea-968e-85d136fc6ad0" />

2. System Architecture
Data path:
DHT11 → ESP32 → Wi-Fi → MQTT Broker (Raspberry Pi) → Node-RED Dashboard
Acknowledgment path:
Node-RED / Raspberry Pi → MQTT ACK topic → ESP32 → Green ACK LED
The OLED shows Wi-Fi state, MQTT state, date, temperature and humidity.
The sketch attempts NTP synchronization using pool.ntp.org.
The JSON timestamp is generated in UTC and ends with “Z”; the OLED time/date is displayed using an IST offset of UTC+05:30.
The ACK LED only blinks if a message is received on the subscribed ACK topic after a successful publish.
3. Wiring Diagram
Use this diagram as a wiring guide. Always verify the pin labels printed on your particular ESP32, OLED and DHT11 module before powering the circuit.

<img width="480" height="259" alt="image" src="https://github.com/user-attachments/assets/8a934ba5-2940-4675-800e-369f70a37e6f" />

4. Complete Wiring Connections
<img width="528" height="214" alt="image" src="https://github.com/user-attachments/assets/c22817a3-74ed-4429-8eea-ba8e35dff26e" />

DHT11 terminal naming: “+” means VCC, “−” means GND, and “OUT” means the digital data signal. This table assumes a 3-pin DHT11 module. A bare 4-pin DHT11 sensor has a different pinout and may require a pull-up resistor.

<img width="508" height="137" alt="image" src="https://github.com/user-attachments/assets/71cc84f8-fbf6-4b15-9903-8144d78ac750" />

Requirement mismatch to resolve: your stated requirement says the red LED should blink three times during data transmission. The attached sketch currently performs a double-blink sequence on GPIO18. The documentation records this difference rather than claiming the existing code already meets the three-blink requirement. To meet the requirement, update the TX LED function to use a three-iteration blink loop and call that function before client.publish().
Timing note: the sketch's Serial message says “Waiting 3 seconds for ACK”, but the actual loop waits 300 ms (0.3 seconds). If Node-RED takes longer than 300 ms to send the ACK, the ESP32 may report an ACK timeout even if an ACK arrives later.

6. MQTT Topics and JSON Payload
<img width="508" height="77" alt="image" src="https://github.com/user-attachments/assets/1ae5729c-d570-4bca-ad0e-2b0014c5f6fa" />

The supplied sketch builds a JSON object with the following fields:
<img width="508" height="131" alt="image" src="https://github.com/user-attachments/assets/f0f9482b-23a0-4f40-a6f4-11edf4871251" />
Illustrative payload structure (values are examples, not a captured reading):
{
  "device_id": "ESP32_Node_02",
  "online": true,
  "timestamp": "2026-10-21T10:00:00Z",
  "temperature_c": 25.0,
  "humidity_pct": 53.0,
  "logs": ["NTP_SYNC_OK", "MQTT_CONNECTED"]
}

7. Libraries and Software
WiFi.h — ESP32 Wi-Fi connectivity.
PubSubClient.h — MQTT client and topic subscription/publishing.
ArduinoJson.h — JSON payload creation.
Wire.h — I²C communication.
Adafruit_GFX.h and Adafruit_SSD1306.h — OLED display support.
DHT.h — DHT11 sensor reading.
time.h — time functions used with NTP.
Install the corresponding libraries through Arduino IDE Library Manager, and select the correct ESP32 board and serial port before uploading.

8. Setup and Commissioning Procedure
1.Power off the ESP32 and wire the DHT11, OLED and both LEDs using the wiring table.
2.Check that both LEDs have individual 220–330 Ω series resistors and all grounds are connected together.
3.In the sketch, verify the Wi-Fi SSID/password and the MQTT broker IP address (172.16.0.132) match the actual network.
4.Ensure Mosquitto is running on the Raspberry Pi and reachable from the ESP32 on TCP port 1883.
5.Confirm the Node-RED flow subscribes to ecoprosus/esp32-02/temperature/humidity and parses the JSON fields.
6.Configure Node-RED to publish an ACK message to ecoprosus/esp32-02/ack after receiving and processing the sensor message.
7.Upload the sketch and open Serial Monitor at 115200 baud.
8.Check that the OLED initializes and shows Wi-Fi/MQTT state, date, temperature and humidity.
9.Wait for the 30-second publish cycle; verify the MQTT message in Node-RED or with an MQTT subscriber.
10.Verify the ACK path by checking that Node-RED publishes the ACK topic and the green LED performs three blinks.
11.If the red LED must blink three times, update the supplied TX function before marking that requirement as complete.


9. Troubleshooting Guide
<img width="510" height="275" alt="image" src="https://github.com/user-attachments/assets/aaaf0ae6-5612-47c1-b073-faffeca99c62" />

10. Code Review Notes and Known Limitations
The code applies a fixed +36 percentage-point offset to DHT11 humidity. This is not a standard calibration method unless supported by measurements; verify it with a reference instrument.
The ACK waiting loop runs for 300 ms, not 3 seconds. Adjust the loop duration if the dashboard needs more processing time.
The red TX LED currently double-blinks, whereas the requested behavior is triple-blink.
The variable lastMQTTPublish starts at zero, so the first publish normally occurs after the first 30-second interval.
The JSON “online” field indicates MQTT connection status at the time the payload is built; it is not a separate cloud/device-health confirmation.
The provided sketch enables anonymous MQTT at the client level (no username/password shown). For deployment beyond a trusted test network, use authentication, access control and TLS where supported.
Do not publish the Wi-Fi password or other secrets in public code repositories or shared documentation.

11. Final Verification Checklist
☐ DHT11 readings appear on Serial Monitor and OLED.
☐ OLED displays Wi-Fi and MQTT connection states.
☐ ESP32 publishes JSON every 30 seconds to the correct sensor topic.
☐ Node-RED parses and displays temperature and humidity.
☐ Node-RED publishes ACK on the exact ACK topic.
☐ Green ACK LED blinks three times after a valid ACK.
☐ Red TX LED behavior is updated to three blinks if that is the final requirement.
☐ ACK timeout duration is consistent with the intended response time.
☐ Humidity offset is validated or removed if not required.

Appendix A — Pin Summary
<img width="508" height="106" alt="image" src="https://github.com/user-attachments/assets/498d35ca-b022-4754-86fc-8f2f484ea6ef" />

