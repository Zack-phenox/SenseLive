#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>


const char* mqtt_server = "dashboard.senselive.io"; // Your ThingsBoard server
const char* access_token = "wehk8HkbdcyrHTJcmcim"; // Your device access token


WiFiClient espClient;
PubSubClient client(espClient);


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32", access_token, NULL)) {
      Serial.println("Connected to ThingsBoard.");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Trying again in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  
  Serial.begin(115200);
  Serial.println("Starting...");

  
  WiFiManager wifiManager;

  
  wifiManager.resetSettings();

  
  const char* portalSSID = "ESP32_Config_Portal";
  wifiManager.setConfigPortalTimeout(180); // Timeout after 3 minutes

  // Start configuration portal
  if (!wifiManager.startConfigPortal(portalSSID)) {
    Serial.println("Failed to start configuration portal. Restarting...");
    delay(3000);
    ESP.restart();
  }

  // If connected to WiFi
  Serial.println("WiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}


void loop() {
  // Ensure the ESP32 stays connected to MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Example telemetry data to send to ThingsBoard
  String payload = "{\"temperature\":25,\"humidity\":60}";
  client.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println("Telemetry sent: " + payload);

  delay(5000); // Send data every 5 seconds
}
