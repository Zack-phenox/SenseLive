#include <WiFi.h>
#include <PubSubClient.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

// Access Point credentials
const char* apSSID = "ESP32_Device";         // Name of the Wi-Fi AP
const char* apPassword = "12345678";         // Password for the Wi-Fi AP

// MQTT details (use as required after connecting to Wi-Fi)
const char* mqtt_server = "dashboard.senselive.in";
const int mqtt_port = 1883;
const char* mqtt_user = "Sense2023";
const char* mqtt_password = "sense123";
const char* ledStatusTopic = "esp32/led_status";

// LED Pin
const int ledPin = 2; // Built-in LED (GPIO2)

// Web server and other instances
DNSServer dnsServer;
AsyncWebServer webServer(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Flags and variables
bool wifiConnected = false; // Indicates if Wi-Fi connection is successful
String deviceIP;            // Holds the device's dynamic IP address

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // Start in Access Point mode
  WiFi.softAP(apSSID, apPassword);
  IPAddress IP = WiFi.softAPIP();
  Serial.println("Access Point started");
  Serial.println("AP IP address: " + IP.toString());

  // Start DNS server for captive portal
  dnsServer.start(53, "*", IP);

  // Set up the captive portal
  setupCaptivePortal();

  Serial.println("Captive portal is ready.");
}

void loop() {
  if (!wifiConnected) {
    dnsServer.processNextRequest(); // Process DNS requests for captive portal
  } else {
    mqttClient.loop(); // Handle MQTT communication
  }
}

void setupCaptivePortal() {
  // Serve the configuration page
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    String html = "<!DOCTYPE html><html><head><title>Configure WiFi</title></head>";
    html += "<body><h1>WiFi Configuration</h1>";
    html += "<form action=\"/connect\" method=\"POST\">";
    html += "SSID: <input type=\"text\" name=\"ssid\"><br>";
    html += "Password: <input type=\"password\" name=\"password\"><br>";
    html += "<button type=\"submit\">Connect</button>";
    html += "</form></body></html>";
    request->send(200, "text/html", html);
  });

  // Handle Wi-Fi connection request
  webServer.on("/connect", HTTP_POST, [](AsyncWebServerRequest* request) {
    String newSSID = request->getParam("ssid", true)->value();
    String newPassword = request->getParam("password", true)->value();
    request->send(200, "text/html", "Connecting to WiFi...");

    // Attempt to connect to the provided Wi-Fi
    WiFi.softAPdisconnect(true); // Stop AP mode
    WiFi.begin(newSSID.c_str(), newPassword.c_str());

    // Wait for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      deviceIP = WiFi.localIP().toString(); // Get the dynamic IP
      Serial.println("\nConnected to WiFi!");
      Serial.println("IP Address: " + deviceIP);

      // Redirect user to the LED control page on the new IP
      String redirectURL = "http://" + deviceIP + "/";
      request->redirect(redirectURL);

      // Start the LED control web server
      startLEDControlServer();
    } else {
      Serial.println("\nFailed to connect to WiFi. Restarting AP mode.");
      WiFi.softAP(apSSID, apPassword);
    }
  });

  // Start the web server
  webServer.begin();
}

void startLEDControlServer() {
  // Handle LED ON/OFF
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    String html = "<!DOCTYPE html><html><head><title>ESP32 LED Control</title></head>";
    html += "<body><h1>ESP32 LED Control</h1>";
    html += "<p>Click the buttons below to control the LED:</p>";
    html += "<a href=\"/LED=ON\"><button style=\"font-size:20px\">ON</button></a>";
    html += "<a href=\"/LED=OFF\"><button style=\"font-size:20px\">OFF</button></a>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Handle LED ON
  webServer.on("/LED=ON", HTTP_GET, [](AsyncWebServerRequest* request) {
    digitalWrite(ledPin, HIGH);
    request->send(200, "text/html", "LED is ON");
  });

  // Handle LED OFF
  webServer.on("/LED=OFF", HTTP_GET, [](AsyncWebServerRequest* request) {
    digitalWrite(ledPin, LOW);
    request->send(200, "text/html", "LED is OFF");
  });

  webServer.begin(); // Start the web server
  Serial.println("LED control server started.");
}
