#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN 15
#define DHTTYPE DHT22

#define LED 2
#define BUTTON 4
#define LDR 34
#define PIR 27
#define BUZZER 25
#define RELAY 26

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

const char* sensorTopic = "ece_task2/smart_home/sensors";
const char* ledTopic = "ece_task2/smart_home/led";

DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServer server(80);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String command = "";

  for (int i = 0; i < length; i++) {
    command += (char)payload[i];
  }

  if (command == "ON") {
    digitalWrite(LED, HIGH);
  }

  if (command == "OFF") {
    digitalWrite(LED, LOW);
  }
}

void handleHome() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int light = analogRead(LDR);
  int motion = digitalRead(PIR);

  String html = "<!DOCTYPE html><html>";
  html += "<head><title>ESP32 Smart Home</title></head>";
  html += "<body>";
  html += "<h1>ESP32 Smart Home</h1>";

  html += "<h2>Sensor Data</h2>";
  html += "<p>Temperature: " + String(temperature) + " &deg;C</p>";
  html += "<p>Humidity: " + String(humidity) + " %</p>";
  html += "<p>Light: " + String(light) + "</p>";
  html += "<p>Motion: " + String(motion) + "</p>";

  html += "<h2>LED Control</h2>";
  html += "<a href='/led/on'><button>LED ON</button></a>";
  html += "<a href='/led/off'><button>LED OFF</button></a>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

void ledOn() {
  digitalWrite(LED, HIGH);
  server.sendHeader("Location", "/");
  server.send(303);
}

void ledOff() {
  digitalWrite(LED, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");

    if (mqttClient.connect("ESP32_SmartHome_Final")) {
      Serial.println("Connected!");
      mqttClient.subscribe(ledTopic);
    } 
    else {
      Serial.print("Failed, state=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  dht.begin();

  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(PIR, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  server.on("/", handleHome);
  server.on("/led/on", ledOn);
  server.on("/led/off", ledOff);

  server.begin();

  Serial.println("Web Server Started!");
}

void loop() {

  if (!mqttClient.connected()) {
    reconnectMQTT();
  }

  mqttClient.loop();
  server.handleClient();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  int light = analogRead(LDR);
  int motion = digitalRead(PIR);
  int button = digitalRead(BUTTON);

  digitalWrite(BUZZER, motion);

  digitalWrite(RELAY, light < 1500);

  if (button == LOW) {
    digitalWrite(LED, !digitalRead(LED));
    delay(300);
  }

  String message = "Temperature: " + String(temperature) +
                   " C, Humidity: " + String(humidity) +
                   " %, Light: " + String(light) +
                   ", Motion: " + String(motion);

  mqttClient.publish(sensorTopic, message.c_str());

  Serial.println(message);

  delay(5000);
}