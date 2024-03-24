#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <DHT.h>

const char* ssid = "ssid";
const char* password = "pw";

const char* mqtt_server = "thingsboard.cloud";
const int mqtt_port = 1883;
const char* mqtt_user = "user";
const char* mqtt_pass = "pass";

WiFiClient espClient;
PubSubClient client(espClient);

Servo myservo;
int pos = 0;
const int irPin = 2;
const int trigPin = 18;
const int echoPin = 19;

#define DHTPIN 22
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

const int maxDistance = 400; // in cm
const int minDistance = 10;  // in cm

void setup() {
  Serial.begin(115200);
  myservo.attach(13);

  pinMode(irPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  dht.begin();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  client.setServer(mqtt_server, mqtt_port);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.print("Failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void loop() {
  int irValue = digitalRead(irPin);
  if (irValue == LOW) {
    Serial.println("Obstacle detected! Opening servo...");

    for (pos = 0; pos <= 90; pos += 1) {
      myservo.write(pos);
      delay(15);
    }

    Serial.println("Servo opened!");

    delay(10000);

    for (pos = 90; pos >= 0; pos -= 1) {
      myservo.write(pos);
      delay(15);
    }

    Serial.println("Servo closed!");
  } else {
    Serial.println("No obstacle");
  }

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance, 1);
  Serial.print(" cm");

  if (distance > maxDistance) {
    Serial.println(" - Far");
    String distanceStr = String(distance);
    client.publish("v1/devices/me/telemetry", ("{\"distance\":\""+distanceStr+"\"}").c_str(), true);
  } else if (distance < minDistance) {
    Serial.println(" - Near");
    String distanceStr = String(distance);
    client.publish("v1/devices/me/telemetry", ("{\"distance\":\""+distanceStr+"\"}").c_str(), true);
  } else {
    Serial.println("");
    String distanceStr = String(distance);
    client.publish("v1/devices/me/telemetry", ("{\"distance\":\""+distanceStr+"\"}").c_str(), true);
  }

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" *C ");
    String hStr = String(h);
    String tStr = String(t);
    client.publish("v1/devices/me/telemetry", ("{\"humidity\":\""+hStr+"\",\"temperature\":\""+tStr+"\"}").c_str(), true);
  }

  delay(100);
  client.loop();
}
