#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// API Configuration
const char* apiUrl = "http://your-api-endpoint.com/api/watering-data";
const char* apiKey = "YOUR_API_KEY"; // Optional

// Pin Configuration
int IN1 = 2;  // D4 on ESP8266
int IN2 = 0;  // D3 on ESP8266
int IN3 = 4;  // D2 on ESP8266
int IN4 = 5;  // D1 on ESP8266

int Pin1 = A0; // Analog pin A0
int Pin2 = A0; // ESP8266 only has one analog pin, so we'll need to use a multiplexer or different approach
int Pin3 = A0;
int Pin4 = A0;

// Sensor values
float value1 = 0;
float value2 = 0;
float value3 = 0;
float value4 = 0;

// Pump status
bool pump1Active = false;
bool pump2Active = false;
bool pump3Active = false;
bool pump4Active = false;

// Timing variables
unsigned long lastApiCall = 0;
const unsigned long apiCallInterval = 30000; // Send data every 30 seconds

void setup() {
  Serial.begin(115200);
  
  // Configure pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  pinMode(Pin1, INPUT);
  
  // Initialize pumps to OFF state
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, HIGH);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  delay(1000);
}

void loop() {
  // Read sensor values
  // Note: ESP8266 only has one analog pin, so we'll simulate multiple sensors for now
  // In a real implementation, you'd use a multiplexer or different sensors
  
  value1 = analogRead(Pin1);
  value2 = value1 + random(-50, 50); // Simulate different sensor readings
  value3 = value1 + random(-50, 50);
  value4 = value1 + random(-50, 50);
  
  // Control pumps based on moisture levels
  controlPump(1, value1, 580);
  controlPump(2, value2, 580);
  controlPump(3, value3, 580);
  controlPump(4, value4, 580);
  
  // Print sensor readings
  Serial.println("=== MOISTURE SENSOR READINGS ===");
  Serial.print("Sensor 1: "); Serial.println(value1);
  Serial.print("Sensor 2: "); Serial.println(value2);
  Serial.print("Sensor 3: "); Serial.println(value3);
  Serial.print("Sensor 4: "); Serial.println(value4);
  Serial.println();
  
  // Send data to API periodically
  if (millis() - lastApiCall > apiCallInterval) {
    sendDataToApi();
    lastApiCall = millis();
  }
  
  delay(1000);
}

void controlPump(int pumpNumber, float moistureValue, int threshold) {
  int pumpPin;
  bool* pumpStatus;
  
  switch(pumpNumber) {
    case 1:
      pumpPin = IN1;
      pumpStatus = &pump1Active;
      break;
    case 2:
      pumpPin = IN2;
      pumpStatus = &pump2Active;
      break;
    case 3:
      pumpPin = IN3;
      pumpStatus = &pump3Active;
      break;
    case 4:
      pumpPin = IN4;
      pumpStatus = &pump4Active;
      break;
    default:
      return;
  }
  
  if (moistureValue > threshold) {
    digitalWrite(pumpPin, LOW); // Turn pump ON
    *pumpStatus = true;
    Serial.print("Pump "); Serial.print(pumpNumber); Serial.println(": ON");
  } else {
    digitalWrite(pumpPin, HIGH); // Turn pump OFF
    *pumpStatus = false;
    Serial.print("Pump "); Serial.print(pumpNumber); Serial.println(": OFF");
  }
}

void sendDataToApi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Attempting to reconnect...");
    WiFi.reconnect();
    return;
  }
  
  HTTPClient http;
  http.begin(apiUrl);
  http.addHeader("Content-Type", "application/json");
  
  // Add API key if required
  if (strlen(apiKey) > 0) {
    http.addHeader("Authorization", apiKey);
  }
  
  // Create JSON payload
  DynamicJsonDocument doc(1024);
  doc["device_id"] = "ESP8266_WATERING_SYSTEM";
  doc["timestamp"] = millis();
  doc["wifi_rssi"] = WiFi.RSSI();
  
  JsonArray sensors = doc.createNestedArray("sensors");
  
  JsonObject sensor1 = sensors.createNestedObject();
  sensor1["id"] = 1;
  sensor1["moisture_value"] = value1;
  sensor1["pump_active"] = pump1Active;
  
  JsonObject sensor2 = sensors.createNestedObject();
  sensor2["id"] = 2;
  sensor2["moisture_value"] = value2;
  sensor2["pump_active"] = pump2Active;
  
  JsonObject sensor3 = sensors.createNestedObject();
  sensor3["id"] = 3;
  sensor3["moisture_value"] = value3;
  sensor3["pump_active"] = pump3Active;
  
  JsonObject sensor4 = sensors.createNestedObject();
  sensor4["id"] = 4;
  sensor4["moisture_value"] = value4;
  sensor4["pump_active"] = pump4Active;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("Sending data to API:");
  Serial.println(jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response: ");
    Serial.println(response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
} 