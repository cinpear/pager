#include <Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID = "AH - Guest";
const char* WIFI_PASSWORD = "NoTimidSouls";
const char* DEVICE_ID = "picoB_cin";
const char* OTHER_ID = "picoA_cin";

const char* MQTT_BROKER = "broker.emqx.io";
const int MQTT_PORT = 1883;

char PUB_TOPIC[50];
char SUB_TOPIC[50];

const int SERVO_PIN = 15;
const int BUTTON_PIN = 0;
const int LED_Y = 21;
const int LED_W = 26;

double increment_delay = 200;
double hold_delay = 1000;

String light_status;

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
    servo1.write(0);
  }

  // new NFC LED logic
  if (String(topic) == "pico/cin/nfc") {
    if (msg == "white") { analogWrite(LED_W, 255); analogWrite(LED_Y, 0); }
    if (msg == "yellow") { analogWrite(LED_W, 0); analogWrite(LED_Y, 255); }
    light_status = msg;
  }

  Serial.println("=== MESSAGE RECEIVED ===");
  Serial.println("Topic:   " + String(topic));
  Serial.println("Message: " + msg);
  Serial.println("SUB_TOPIC is: " + String(SUB_TOPIC));
  Serial.println("NFC topic is: pico/cin/nfc");
  Serial.println("Topic match: " + String(String(topic) == "pico/cin/nfc"));
  Serial.println("Msg match white: " + String(msg == "white"));
  Serial.println("Msg match yellow: " + String(msg == "yellow"));
  Serial.println("========================");
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
      Serial.println("MQTT connected!");
      
      bool sub1 = mqtt.subscribe(SUB_TOPIC);
      bool sub2 = mqtt.subscribe("pico/cin/nfc");
      
      Serial.println("Subscribed to SUB_TOPIC: " + String(sub1 ? "OK" : "FAILED"));
      Serial.println("Subscribed to pico/cin/nfc: " + String(sub2 ? "OK" : "FAILED"));
    } else {
      Serial.println("failed rc=" + String(mqtt.state()) + ", retrying in 3s");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  snprintf(PUB_TOPIC, sizeof(PUB_TOPIC), "pico/cin/picoB_cin/button", DEVICE_ID);
  snprintf(SUB_TOPIC, sizeof(SUB_TOPIC), "pico/cin/picoB_cin/button", OTHER_ID);

  servo1.attach(SERVO_PIN, 500, 2500);
  servo1.write(0);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_Y, OUTPUT);
  pinMode(LED_W, OUTPUT);
  analogWrite(LED_Y, 255);
  analogWrite(LED_W, 0);

  connectWifi();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(512);  // ← must be BEFORE connect
  connectMQTT();
}

int lastButton = HIGH;
bool first = true;

void loop() {
  if (!mqtt.connected()) {
    Serial.println("MQTT disconnected! Reconnecting...");
    connectMQTT();
  }

  mqtt.loop();  // only call once

  int current = digitalRead(BUTTON_PIN);

  if (current == LOW && lastButton == HIGH) {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Button pressed! Publishing...");
    bool ok = mqtt.publish(PUB_TOPIC, "pressed");
    Serial.println(ok ? "Published OK" : "Publish FAILED");
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);

    analogWrite(LED_Y, 255);
  }

  lastButton = current;
  // removed delay(100) — this was blocking mqtt.loop()
}