#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Wi-Fi configuration
const char* WIFI_SSID = "Wokwi-GUEST"; 
const char* WIFI_PASSWORD = "";

// MQTT configuration
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* TOPIC_GAS_VALUE = "sensor/gas";
const char* TOPIC_TEMPERATURE = "sensor/temperature";
const char* TOPIC_HUMIDITY = "sensor/humidity";
const char* TOPIC_CURRENT = "sensor/current";
const char* TOPIC_VOLTAGE = "sensor/voltage";

// Hardware pin definitions
const int PIN_GAS_SENSOR = 33;
const int PIN_DHT_SENSOR = 32;
const int PIN_CURRENT_SENSOR = 34;
const int PIN_VOLTAGE_SENSOR = 35;

#define DHT_SENSOR_TYPE DHT22

// Timing
unsigned long lastPublishTimeMs = 0;
const unsigned long SENSOR_PUBLISH_INTERVAL_MS = 500;

// Global objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
DHT dhtSensor(PIN_DHT_SENSOR, DHT_SENSOR_TYPE);

// Connect to Wi-Fi network
void connectToWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected. IP address: " + WiFi.localIP().toString());
}


// Connect to MQTT broker and subscribe to necessary topics
void connectToMqttBroker() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT broker... ");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected.");
    } else {
      Serial.printf("connection failed (rc=%d). Retrying in 5 seconds...\n", mqttClient.state());
      delay(5000);
    }
  }
}

// Publish gas sensor reading via MQTT
void publishGasSensorReading() {
  int gasLevel = analogRead(PIN_GAS_SENSOR);
  char payload[10];
  itoa(gasLevel, payload, 10);
  mqttClient.publish(TOPIC_GAS_VALUE, payload);
  Serial.printf("Published gas level: %s\n", payload);
}

// Publish current sensor reading via MQTT
void publishCurrentSensorReading() {
  int currentLevel = analogRead(PIN_CURRENT_SENSOR);
  char payload[10];
  snprintf(payload, sizeof(payload), "%d", currentLevel);
  mqttClient.publish(TOPIC_CURRENT, payload);
  Serial.printf("Published current level: %s\n", payload);
}

// Publish voltage sensor reading via MQTT
void publishVoltageSensorReading() {
  int voltageLevel = analogRead(PIN_VOLTAGE_SENSOR);
  char payload[10];
  snprintf(payload, sizeof(payload), "%d", voltageLevel);
  mqttClient.publish(TOPIC_VOLTAGE, payload);
  Serial.printf("Published voltage level: %s\n", payload);
}


// Publish temperature and humidity readings via MQTT
void publishTemperatureAndHumidity() {
  float temperature = dhtSensor.readTemperature();
  float humidity = dhtSensor.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Error: Failed to read from DHT sensor.");
    return;
  }

  char tempPayload[10], humidityPayload[10];
  dtostrf(temperature, 1, 2, tempPayload);
  dtostrf(humidity, 1, 2, humidityPayload);

  mqttClient.publish(TOPIC_TEMPERATURE, tempPayload);
  mqttClient.publish(TOPIC_HUMIDITY, humidityPayload);

  Serial.printf("Published temperature: %s°C\n", tempPayload);
  Serial.printf("Published humidity: %s%%\n", humidityPayload);
}

void setup() {
  pinMode(PIN_GAS_SENSOR, INPUT);
  pinMode(PIN_CURRENT_SENSOR, INPUT);

  Serial.begin(115200);

  dhtSensor.begin();
  connectToWiFi();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
}

void loop() {
  if (!mqttClient.connected()) {
    connectToMqttBroker();
  }

  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastPublishTimeMs >= SENSOR_PUBLISH_INTERVAL_MS) {
    lastPublishTimeMs = now;
    publishGasSensorReading();
    publishTemperatureAndHumidity();
    publishCurrentSensorReading();
    publishVoltageSensorReading();
  }
}
