/*
    =====================================================
    ECOPROSUS ESP32 #2
    DHT11 + OLED + WiFi + NTP + MQTT + JSON
    Data Transmission (GPIO 18) & ACK Handling (GPIO 23)
    =====================================================

    TX LED:  GPIO 18 (Blinks twice on transmission)
    ACK LED: GPIO 23 (Blinks 3 times after 0.3s delay on ACK received)

    MQTT Out Topic: ecoprosus/esp32-02/temperature/humidity
    MQTT In Topic:  ecoprosus/esp32-02/ack
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include "time.h"

// ======================================================
// WIFI SETTINGS
// ======================================================

const char* ssid = "Ecopro";
const char* password = "ecopro$123";

// ======================================================
// DEVICE ID
// ======================================================

const char* DEVICE_ID = "ESP32_Node_02";

// ======================================================
// MQTT SETTINGS
// ======================================================

const char* mqtt_server = "172.16.0.132";
const int mqtt_port = 1883;

const char* sensorTopic = "ecoprosus/esp32-02/temperature/humidity";
const char* ackTopic    = "ecoprosus/esp32-02/ack";

WiFiClient espClient;
PubSubClient client(espClient);

// ======================================================
// OLED SETTINGS
// ======================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ======================================================
// NTP SETTINGS
// ======================================================

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec_local = 19800; // UTC +5:30 for India

// ======================================================
// DHT11 SETTINGS
// ======================================================

#define DHTPIN 19
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// ======================================================
// LED SETTINGS
// ======================================================

#define TX_LED_PIN 18
#define ACK_LED_PIN 23

// ======================================================
// SENSOR & STATE VARIABLES
// ======================================================

float humidity = 0.0;
float tempC = 0.0;
float tempF = 0.0;

bool lastDhtReadOk = false;
bool ntpSyncOk = false;
volatile bool ackReceived = false;

// ======================================================
// TIMERS
// ======================================================

unsigned long lastDisplayUpdate = 0;
unsigned long lastDHTRead = 0;
unsigned long lastMQTTPublish = 0;

unsigned long lastWiFiAttempt = 0;
unsigned long lastMQTTAttempt = 0;
unsigned long lastScrollUpdate = 0;

const unsigned long displayInterval = 50;
const unsigned long dhtInterval = 2000;
const unsigned long mqttInterval = 30000;

const unsigned long wifiRetryInterval = 5000;
const unsigned long mqttRetryInterval = 5000;

// Smooth OLED scrolling
const unsigned long scrollInterval = 80;
int scrollX = -60;
const int scrollSpeed = 6;

// ======================================================
// STATUS TEXT
// ======================================================

String wifiStatus = "STARTING";
String mqttStatus = "STARTING";

// ======================================================
// LED PATTERN FUNCTIONS
// ======================================================

void executeTxDoubleBlink()
{
  Serial.println("TX LED: ON - 0.3s");
  digitalWrite(TX_LED_PIN, HIGH);
  delay(300);

  Serial.println("TX LED: OFF - 0.2s");
  digitalWrite(TX_LED_PIN, LOW);
  delay(200);

  Serial.println("TX LED: ON - 0.3s");
  digitalWrite(TX_LED_PIN, HIGH);
  delay(300);

  Serial.println("TX LED: OFF");
  digitalWrite(TX_LED_PIN, LOW);
}

void executeAckTripleBlink()
{
  Serial.println("ACK LED: Starting triple blink sequence...");

  for (int i = 0; i < 3; i++)
  {
    digitalWrite(ACK_LED_PIN, HIGH);
    Serial.print("ACK LED: ON (");
    Serial.print(i + 1);
    Serial.println("/3)");
    delay(300);

    digitalWrite(ACK_LED_PIN, LOW);
    Serial.println("ACK LED: OFF");
    delay(200);
  }
}

// ======================================================
// MQTT CALLBACK
// ======================================================

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("MQTT Received [");
  Serial.print(topic);
  Serial.println("]");

  if (String(topic) == String(ackTopic))
  {
    ackReceived = true;
    Serial.println("--> ACK Received from RPi!");
  }
}

// ======================================================
// NON-BLOCKING WIFI CONNECTION
// ======================================================

void connectWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    wifiStatus = "CONNECTED";
    return;
  }

  unsigned long currentMillis = millis();

  if (currentMillis - lastWiFiAttempt < wifiRetryInterval)
  {
    return;
  }

  lastWiFiAttempt = currentMillis;
  Serial.println("Trying to connect to WiFi...");
  wifiStatus = "CONNECTING";

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
}

// ======================================================
// NON-BLOCKING MQTT CONNECTION
// ======================================================

void connectMQTT()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    mqttStatus = "NO WIFI";
    return;
  }

  if (client.connected())
  {
    mqttStatus = "CONNECTED";
    return;
  }

  unsigned long currentMillis = millis();

  if (currentMillis - lastMQTTAttempt < mqttRetryInterval)
  {
    mqttStatus = "BROKER OFF";
    return;
  }

  lastMQTTAttempt = currentMillis;
  Serial.print("Connecting MQTT...");
  mqttStatus = "CONNECTING";

  String clientID = String("ecoprosus-esp32-02-") + String((uint32_t)ESP.getEfuseMac(), HEX);

  if (client.connect(clientID.c_str()))
  {
    Serial.println("Connected");
    mqttStatus = "CONNECTED";

    client.subscribe(ackTopic);
    Serial.print("Subscribed to ACK Topic: ");
    Serial.println(ackTopic);
  }
  else
  {
    Serial.print("Failed, State=");
    Serial.println(client.state());
    mqttStatus = "BROKER OFF";
  }
}

// ======================================================
// TRUE UTC TIMESTAMP
// ======================================================

String getISOTimestamp()
{
  time_t now = time(nullptr);

  if (now < 100000)
  {
    return "1970-01-01T00:00:00Z";
  }

  struct tm utcTime;
  gmtime_r(&now, &utcTime);

  char timestamp[30];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &utcTime);

  return String(timestamp);
}

// ======================================================
// GET IST TIME FOR OLED
// ======================================================

bool getIstTime(struct tm* localTimeinfo)
{
  time_t now = time(nullptr);

  if (now < 100000)
  {
    return false;
  }

  time_t localEpoch = now + gmtOffset_sec_local;
  gmtime_r(&localEpoch, localTimeinfo);

  return true;
}

// ======================================================
// UPDATE OLED
// ======================================================

void updateOLED(const char* dateBuffer, const char* timeBuffer)
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(scrollX, 0);
  display.print("Ecoprosus");

  display.setCursor(0, 14);
  display.print("WiFi: ");
  display.println(wifiStatus);

  display.setCursor(0, 26);
  display.print("MQTT: ");
  display.println(mqttStatus);

  display.setCursor(0, 38);
  display.print("Date: ");
  display.println(dateBuffer);

  display.setCursor(0, 50);
  display.print("T:");
  display.print(tempC, 1);
  display.print("C H:");
  display.print(humidity, 1);
  display.print("%");

  display.display();
}

// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("======================================");
  Serial.println("        ECOPROSUS ESP32 #2");
  Serial.println("        DHT11 + OLED + MQTT + JSON");
  Serial.println("======================================");

  pinMode(TX_LED_PIN, OUTPUT);
  digitalWrite(TX_LED_PIN, LOW);

  pinMode(ACK_LED_PIN, OUTPUT);
  digitalWrite(ACK_LED_PIN, LOW);

  Wire.begin(21, 22);
  Wire.setClock(400000L);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
  {
    Serial.println("OLED Failed");
    while (true) { delay(1000); }
  }

  Serial.println("OLED Initialized");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 28);
  display.println("Ecoprosus");
  display.display();
  delay(1000);

  dht.begin();
  Serial.println("DHT11 Initialized");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  lastWiFiAttempt = millis() - wifiRetryInterval;
  wifiStatus = "CONNECTING";

  configTime(0, 0, ntpServer);
  Serial.println("NTP synchronization started");

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  client.setSocketTimeout(1);

  lastMQTTAttempt = millis() - mqttRetryInterval;
  Serial.println("MQTT configured");

  Serial.println("\nSystem Ready");
  Serial.println("======================================");
}

// ======================================================
// LOOP
// ======================================================

void loop()
{
  unsigned long currentMillis = millis();

  // ====================================================
  // WIFI RECONNECT
  // ====================================================
  if (WiFi.status() == WL_CONNECTED)
  {
    if (wifiStatus != "CONNECTED")
    {
      Serial.println("WiFi Connected");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    }
    wifiStatus = "CONNECTED";
  }
  else
  {
    wifiStatus = "OFFLINE";
    connectWiFi();
  }

  // ====================================================
  // MQTT RECONNECT
  // ====================================================
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!client.connected())
    {
      connectMQTT();
    }
    else
    {
      mqttStatus = "CONNECTED";
    }
    client.loop();
  }
  else
  {
    mqttStatus = "NO WIFI";
  }

  // ====================================================
  // GET IST TIME FOR OLED
  // ====================================================
  struct tm timeinfo;
  bool timeAvailable = getIstTime(&timeinfo);

  char dateBuffer[16];
  char timeBuffer[16];

  if (timeAvailable)
  {
    ntpSyncOk = true;
    strftime(dateBuffer, sizeof(dateBuffer), "%d-%m-%Y", &timeinfo);
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeinfo);
  }
  else
  {
    strcpy(dateBuffer, "NTP...");
    strcpy(timeBuffer, "Sync...");
  }

  // ====================================================
  // READ DHT11 EVERY 2 SECONDS (WITH CALIBRATION OFFSET)
  // ====================================================
  if (currentMillis - lastDHTRead >= dhtInterval)
  {
    lastDHTRead = currentMillis;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float tf = dht.readTemperature(true);

    if (!isnan(h) && !isnan(t) && !isnan(tf))
    {
      // Apply calibration offset to map 17% -> 53%
      float calibratedHumidity = h + 36.0;
      if (calibratedHumidity > 100.0) calibratedHumidity = 100.0;

      humidity = calibratedHumidity;
      tempC = t;
      tempF = tf;
      lastDhtReadOk = true;
    }
    else
    {
      lastDhtReadOk = false;
      Serial.println("DHT11 Read Failed");
    }
  }

  // ====================================================
  // MQTT TRANSMISSION & ACK SEQUENCE (EVERY 30S)
  // ====================================================
  if (currentMillis - lastMQTTPublish >= mqttInterval)
  {
    lastMQTTPublish = currentMillis;

    Serial.println();
    Serial.println(">>> MQTT TRANSMISSION START");

    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["online"] = client.connected();
    doc["timestamp"] = getISOTimestamp();
    doc["temperature_c"] = round(tempC * 10.0) / 10.0;
    doc["humidity_pct"] = round(humidity * 10.0) / 10.0;

    JsonArray logs = doc["logs"].to<JsonArray>();
    logs.add(timeAvailable ? "NTP_SYNC_OK" : "NTP_SYNC_FAIL");
    logs.add(client.connected() ? "MQTT_CONNECTED" : "MQTT_DISCONNECTED");
    if (!lastDhtReadOk) logs.add("DHT_READ_FAIL");

    char payload[350];
    size_t len = serializeJson(doc, payload, sizeof(payload));

    if (client.connected())
    {
      // Step 1: GPIO 18 Blinks twice during transmission
      executeTxDoubleBlink();

      bool published = client.publish(sensorTopic, payload, len);

      if (published)
      {
        Serial.println("MQTT Published Successfully.");

        // Step 2: 0.3-Second delay (keep processing incoming MQTT packets during this window)
        ackReceived = false;
        Serial.println("Waiting 3 seconds for ACK from RPi...");
        
        unsigned long delayStart = millis();
        while (millis() - delayStart < 300)
        {
          client.loop(); // Keep background processing active for incoming ACK
          delay(10);
        }

        // Step 3: GPIO 23 Blinks 3 times if ACK was received
        if (ackReceived)
        {
          Serial.println("ACK received successfully from RPi!");
          executeAckTripleBlink();
        }
        else
        {
          Serial.println("ACK Timeout: No acknowledgment message received from RPi.");
        }
      }
      else
      {
        Serial.println("MQTT Publish Failed!");
        mqttStatus = "PUBLISH FAIL";
      }
    }
    else
    {
      Serial.println("MQTT unavailable. Data not published.");
    }
  }

  // ====================================================
  // OLED DISPLAY UPDATE
  // ====================================================
  if (currentMillis - lastDisplayUpdate >= displayInterval)
  {
    lastDisplayUpdate = currentMillis;
    updateOLED(dateBuffer, timeBuffer);
  }

  // ====================================================
  // SMOOTH SCROLLING
  // ====================================================
  if (currentMillis - lastScrollUpdate >= scrollInterval)
  {
    lastScrollUpdate = currentMillis;
    scrollX += scrollSpeed;
    if (scrollX > SCREEN_WIDTH)
    {
      scrollX = -60;
    }
  }

  delay(1);
}