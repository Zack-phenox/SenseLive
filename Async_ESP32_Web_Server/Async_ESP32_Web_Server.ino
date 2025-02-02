#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

// Replace with your network credentials
const char* ssid = "GHRTBIF";
const char* password = "Welcome2rgi";

// Set LED GPIO
const int ledPin = 2;
// Stores LED state
String ledState;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Mutex for thread safety
SemaphoreHandle_t xMutex;

// Replaces placeholder with LED state value
String processor(const String& var) {
  Serial.println(var);
  if (var == "STATE") {
    if (digitalRead(ledPin)) {
      ledState = "ON";
    } else {
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Create a mutex for synchronization
  xMutex = xSemaphoreCreateMutex();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
      request->send(SPIFFS, "/index.html", String(), false, processor);
      xSemaphoreGive(xMutex);
    }
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
      request->send(SPIFFS, "/style.css", "text/css");
      xSemaphoreGive(xMutex);
    }
  });

  // Route to set GPIO to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
      digitalWrite(ledPin, HIGH);
      request->send(SPIFFS, "/index.html", String(), false, processor);
      xSemaphoreGive(xMutex);
    }
  });

  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
      digitalWrite(ledPin, LOW);
      request->send(SPIFFS, "/index.html", String(), false, processor);
      xSemaphoreGive(xMutex);
    }
  });

  // Start server
  server.begin();
}

void loop() {
  // Empty loop
}
