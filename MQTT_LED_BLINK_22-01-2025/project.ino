#include <WiFi.h>
#include <PubSubClient.h>

// Wi-Fi and MQTT credentials
const char* ssid = "GHRTBIF";                // Replace with your Wi-Fi SSID
const char* password = "Welcome2rgi";        // Replace with your Wi-Fi password
const char* mqtt_server = "dashboard.senselive.in"; // Your MQTT broker address
const int mqtt_port = 1883;                  // MQTT broker port
const char* mqtt_user = "Sense2023";         // MQTT username
const char* mqtt_password = "sense123";      // MQTT password

// MQTT Topic
const char* ledStatusTopic = "esp32/led_status";

// LED Pin
const int ledPin = 2; // Built-in LED (GPIO2)

// ESP32 web server
WiFiServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

// Counters for ON and OFF status
int onCount = 0;
int offCount = 0;

// Function prototypes
void connectToWiFi();
void connectToMQTT();
void handleWebClient();
void sendMQTTMessage(const char* message);

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // Connect to Wi-Fi
  connectToWiFi();

  // Set up MQTT client
  client.setServer(mqtt_server, mqtt_port);

  // Start web server
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  // Reconnect to Wi-Fi or MQTT if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected. Reconnecting...");
    connectToWiFi();
  }
  if (!client.connected()) {
    Serial.println("MQTT disconnected. Reconnecting...");
    connectToMQTT();
  }

  client.loop();

  // Handle incoming web client requests
  handleWebClient();
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.println("IP Address: " + WiFi.localIP().toString());
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT broker...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) { // Authenticate with username and password
      Serial.println("Connected to MQTT broker!");
    } else {
      Serial.print("Failed. MQTT State: ");
      Serial.println(client.state());
      Serial.println("Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void handleWebClient() {
  WiFiClient client = server.available(); // Check for new clients
  if (client) {
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    // Handle LED ON/OFF
    if (request.indexOf("/LED=ON") != -1) {
      digitalWrite(ledPin, HIGH);
      onCount++; // Increment ON count
      sendMQTTMessage("ON");
    } else if (request.indexOf("/LED=OFF") != -1) {
      digitalWrite(ledPin, LOW);
      offCount++; // Increment OFF count
      sendMQTTMessage("OFF");
    }

    // Web page response
    String html = "<!DOCTYPE html><html>";
    html += "<head><title>ESP32 LED Control</title></head>";
    html += "<body><h1>ESP32 LED Control</h1>";
    html += "<p>Click the buttons below to control the LED:</p>";
    html += "<a href=\"/LED=ON\"><button style=\"font-size:20px\">ON</button></a>";
    html += "<a href=\"/LED=OFF\"><button style=\"font-size:20px\">OFF</button></a>";
    html += "<h2>LED Status Count</h2>";
    html += "<table border=\"1\" style=\"font-size:20px; border-collapse:collapse;\">";
    html += "<tr><th>Status</th><th>Count</th></tr>";
    html += "<tr><td>ON</td><td>" + String(onCount) + "</td></tr>";
    html += "<tr><td>OFF</td><td>" + String(offCount) + "</td></tr>";
    html += "</table>";
    html += "</body></html>";

    client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    client.print(html);
    delay(1);
    client.stop();
  }
}

void sendMQTTMessage(const char* message) {
  if (client.publish(ledStatusTopic, message)) {
    Serial.println(String("Message sent: ") + message);
  } else {
    Serial.println("Failed to send message.");
  }
}
