#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>

// Define reset button GPIO pin
const int resetButtonPin = 0;  // GPIO0 (you can use a different pin)

const char* ap_ssid = "ESP32_ConfigPortal";
const char* ap_password = "12345678"; // Minimum 8 characters

WebServer server(80);
Preferences preferences;

// Variables to hold credentials
String ssid = "";
String password = "";
String staticIP = "";
String subnet = "";
String gateway = "";

const char* mqtt_server = "dashboard.senselive.in";
const int mqtt_port = 1883;
const char* mqtt_user = "Sense2023";
const char* mqtt_password = "sense123";
const char* ledStatusTopic = "esp32/led_status";

const int ledPin = 2;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

int onCount = 0;
int offCount = 0;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // Check if the reset button is pressed
  pinMode(resetButtonPin, INPUT_PULLUP); // Button pressed will be LOW
  if (digitalRead(resetButtonPin) == LOW) {
    preferences.begin("WiFiCreds", false);
    preferences.clear();  // Clear all preferences if reset button is pressed
    Serial.println("Preferences cleared, rebooting...");
    preferences.end();  // End the preferences session
    delay(2000);
    ESP.restart();
  }

  preferences.begin("WiFiCreds", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  staticIP = preferences.getString("staticIP", "");
  subnet = preferences.getString("subnet", "");
  gateway = preferences.getString("gateway", "");

  if (ssid != "" && password != "") {
    connectToWiFi();
  } else {
    startAPMode();
  }
}

void loop() {
  if (WiFi.getMode() == WIFI_AP) {
    server.handleClient();
  }

  if (WiFi.status() == WL_CONNECTED && mqttClient.connected()) {
    mqttClient.loop();
  }
}

void startAPMode() {
  Serial.println("Starting Access Point...");
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP IP Address: " + WiFi.softAPIP().toString());

  server.on("/", handleRoot);
  server.on("/submit", handleFormSubmit);
  server.begin();
  Serial.println("Configuration portal started.");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><title>ESP32 Config Portal</title></head>";
  html += "<body><h1>Wi-Fi Configuration</h1>";
  html += "<form action='/submit' method='GET'>";
  html += "<label for='ssid'>SSID:</label><br><input type='text' name='ssid'><br><br>";
  html += "<label for='password'>Password:</label><br><input type='password' name='password'><br><br>";
  html += "<label for='staticIP'>Static IP:</label><br><input type='text' name='staticIP'><br><br>";
  html += "<label for='subnet'>Subnet Mask:</label><br><input type='text' name='subnet'><br><br>";
  html += "<label for='gateway'>Gateway:</label><br><input type='text' name='gateway'><br><br>";
  html += "<input type='submit' value='Save & Connect'>";
  html += "</form></body></html>";

  server.send(200, "text/html", html);
}

void handleFormSubmit() {
  ssid = server.arg("ssid");
  password = server.arg("password");
  staticIP = server.arg("staticIP");
  subnet = server.arg("subnet");
  gateway = server.arg("gateway");

  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("staticIP", staticIP);
  preferences.putString("subnet", subnet);
  preferences.putString("gateway", gateway);

  String html = "<!DOCTYPE html><html><body><h1>Credentials Saved!</h1>";
  html += "<p>Device will now reboot and attempt to connect to the Wi-Fi network.</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);

  delay(2000);
  ESP.restart();
}

void connectToWiFi() {
  if (staticIP != "" && subnet != "" && gateway != "") {
    IPAddress ip, mask, gw;
    ip.fromString(staticIP);
    mask.fromString(subnet);
    gw.fromString(gateway);
    WiFi.config(ip, gw, mask);
  }

  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Connecting to Wi-Fi...");

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi!");
    Serial.println("IP Address: " + WiFi.localIP().toString());

    mqttClient.setServer(mqtt_server, mqtt_port);
    connectToMQTT();

    server.on("/LED=ON", handleLEDOn);
    server.on("/LED=OFF", handleLEDOff);
    server.on("/", handleLEDPage);
    server.begin();
    Serial.println("LED control page started.");
  } else {
    Serial.println("\nFailed to connect to Wi-Fi. Starting AP mode.");
    startAPMode();
  }
}

void connectToMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT broker...");
    if (mqttClient.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("Connected to MQTT broker!");
    } else {
      Serial.print("Failed. MQTT State: ");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void handleLEDPage() {
  String html = "<!DOCTYPE html><html><body><h1>ESP32 LED Control</h1>";
  html += "<p>Click the buttons below to control the LED:</p>";
  html += "<a href='/LED=ON'><button style='font-size:20px'>ON</button></a>";
  html += "<a href='/LED=OFF'><button style='font-size:20px'>OFF</button></a>";
  html += "<h2>LED Status Count</h2>";
  html += "<table border='1' style='font-size:20px; border-collapse:collapse;'>";
  html += "<tr><th>Status</th><th>Count</th></tr>";
  html += "<tr><td>ON</td><td>" + String(onCount) + "</td></tr>";
  html += "<tr><td>OFF</td><td>" + String(offCount) + "</td></tr>";
  html += "</table></body></html>";

  server.send(200, "text/html", html);
}

void handleLEDOn() {
  digitalWrite(ledPin, HIGH);
  onCount++;
  mqttClient.publish(ledStatusTopic, "ON");
  handleLEDPage();
}

void handleLEDOff() {
  digitalWrite(ledPin, LOW);
  offCount++;
  mqttClient.publish(ledStatusTopic, "OFF");
  handleLEDPage();
}
