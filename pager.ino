#include <Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID = "AH - Guest";
const char* WIFI_PASSWORD = "NoTimidSouls";
const char* DEVICE_ID = "picoA_cin";
const char* OTHER_ID = "picoB_cin";

const char* MQTT_BROKER = "broker.emqx.io";
const int MQTT_PORT = 1883;

char PUB_TOPIC[50];
char SUB_TOPIC[50];

double increment_delay = 200;
double hold_delay = 1000;

const int SERVO_PIN = 15;
const int BUTTON_PIN = 0;

Servo servo1;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void onMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.println("Recieved: " + msg);

  double increments = 10;
  double target = 90;
  if (msg ==  "pressed") {
    for (int i = 1; i < increments; i++) {
      servo1.write(target/increments * i);
      delay(increment_delay);
    }
    delay(2000);
    // servo1.write(90);
    // delay(10000);
    servo1.write(0);
    // servo1.write(90);
    // delay(10000);
  }
}

void connectWifi() {
  pinMode(LED_BUILTIN, OUTPUT);
  while (true) {
    Serial.println("Connecting to Wifi");
    WiFi.disconnect();
    delay(100);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(1000);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 5) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN, LOW);
      delay(250);
      Serial.println(".");
      attempts += 1;
    }

    if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("Connected! IP: " + WiFi.localIP().toString());
      return;
    }
    Serial.println("Failed after 5 attempts, waiting 10s...");
    for (int i = 10; i > 0; i--) {
      Serial.println("Retrying in " + String(i) + "s");
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(900);
    }
  }
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.println("Connecting to MQTT...");
    if (mqtt.connect(DEVICE_ID)) {
      Serial.println("connected!");
      mqtt.subscribe(SUB_TOPIC);
      Serial.println("Subscribed to: " + String(SUB_TOPIC));
    } else {
      Serial.println("failed (rc=)" + String(mqtt.state()) + "), retrying in 3s");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("setting up");
  snprintf(PUB_TOPIC, sizeof(PUB_TOPIC), "pico/cin/%s/button", DEVICE_ID);
  snprintf(SUB_TOPIC, sizeof(SUB_TOPIC), "pico/cin/%s/button", OTHER_ID);

  servo1.attach(SERVO_PIN, 500,2500);
  servo1.write(0);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  connectWifi();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMessage);
  connectMQTT();
}

int lastButton = HIGH;
bool first = true;

void loop() {
  // Serial.println("looping");
  if (!mqtt.connected()) {
    connectMQTT();
  }

  if (first) {
    Serial.println("loop");
    first = false;
  }
  mqtt.loop();

  int current = digitalRead(BUTTON_PIN);
  Serial.println(current);
  delay(100);
  if (current == LOW && lastButton == HIGH) {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Button pressed! Publishing...");
    mqtt.publish(PUB_TOPIC, "pressed");
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
  }

  lastButton = current;
}