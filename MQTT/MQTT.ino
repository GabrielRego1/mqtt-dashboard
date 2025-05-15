#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Wi-Fi configuration
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

// MQTT configuration
const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_LED_COMMAND_TOPIC = "led/control";
const char* MQTT_LED_STATUS_TOPIC = "led/status";
const char* MQTT_GAS_TOPIC = "gas/value";
const char* MQTT_TEMP_TOPIC = "dht/temperature";
const char* MQTT_HUMIDITY_TOPIC = "dht/humidity";

// Hardware configuration
const int LED_PIN = 2;
const int GAS_SENSOR_PIN = 34;
const int DHT_PIN = 32;
#define DHT_TYPE DHT11

// Timing
unsigned long lastSensorPublishTime = 0;
const unsigned long SENSOR_PUBLISH_INTERVAL_MS = 1000;

// Objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
DHT dht(DHT_PIN, DHT_TYPE);

void connectToWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected. IP Address: ");
  Serial.println(WiFi.localIP());
}

void handleMqttMessage(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; ++i) {
    message += (char)payload[i];
  }

  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (message == "L") {
    digitalWrite(LED_PIN, HIGH);
    mqttClient.publish(MQTT_LED_STATUS_TOPIC, "LED on");
  } else if (message == "D") {
    digitalWrite(LED_PIN, LOW);
    mqttClient.publish(MQTT_LED_STATUS_TOPIC, "LED off");
  }
}

void connectToMqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Connected.");
      mqttClient.subscribe(MQTT_LED_COMMAND_TOPIC);
    } else {
      Serial.print("Failed. rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Retrying in 5s...");
      delay(5000);
    }
  }
}

void publishGasSensorValue() {
  int gasValue = analogRead(GAS_SENSOR_PIN);
  char payload[10];
  itoa(gasValue, payload, 10);
  mqttClient.publish(MQTT_GAS_TOPIC, payload);
  Serial.print("Gas value published: ");
  Serial.println(payload);
}

void publishDHTValues() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor.");
    return;
  }

  char tempPayload[10];
  dtostrf(temperature, 1, 2, tempPayload);
  mqttClient.publish(MQTT_TEMP_TOPIC, tempPayload);

  char humPayload[10];
  dtostrf(humidity, 1, 2, humPayload);
  mqttClient.publish(MQTT_HUMIDITY_TOPIC, humPayload);

  Serial.print("Temperature published: ");
  Serial.println(tempPayload);
  Serial.print("Humidity published: ");
  Serial.println(humPayload);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(GAS_SENSOR_PIN, INPUT);
  Serial.begin(115200);
  
  dht.begin();
  connectToWiFi();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(handleMqttMessage);
}

void loop() {
  if (!mqttClient.connected()) {
    connectToMqtt();
  }

  mqttClient.loop();

  unsigned long currentTime = millis();
  if (currentTime - lastSensorPublishTime > SENSOR_PUBLISH_INTERVAL_MS) {
    lastSensorPublishTime = currentTime;
    publishGasSensorValue();
    publishDHTValues();
  }
}
