#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// Pin Configuration using config.h
int IN1 = PUMP_1_PIN;
int IN2 = PUMP_2_PIN;
int IN3 = PUMP_3_PIN;
int IN4 = PUMP_4_PIN;

// Multiplexer control pins
int MUX_S0 = MUX_S0_PIN;
int MUX_S1 = MUX_S1_PIN;
int MUX_S2 = MUX_S2_PIN;
int MUX_S3 = MUX_S3_PIN;

// Analog input pin (connected to multiplexer output)
int ANALOG_IN = MOISTURE_SENSOR_PIN;

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
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 60000; // Check WiFi every 1 minute

// Statistics
unsigned long totalApiCalls = 0;
unsigned long failedApiCalls = 0;
unsigned long pumpActivations = 0;

// API status
bool apiEnabled = true; // Can be set to false if API is not ready

void setup() {
  Serial.begin(115200);
  
  #if DEBUG_MODE
  Serial.println("=== ESP8266 Watering System Starting ===");
  Serial.println("Version: 2.0");
  Serial.print("Device ID: ");
  Serial.println(DEVICE_ID);
  Serial.print("API Send Interval: ");
  Serial.print(API_SEND_INTERVAL / 1000);
  Serial.println(" seconds");
  #endif
  
  // Configure pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);
  
  // Initialize pumps to OFF state
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, HIGH);
  
  // Connect to WiFi
  connectToWiFi();
  
  delay(1000);
}

void loop() {
  // Check WiFi connection periodically
  if (millis() - lastWiFiCheck > wifiCheckInterval) {
    if (WiFi.status() != WL_CONNECTED) {
      #if DEBUG_MODE
      Serial.println("WiFi connection lost. Reconnecting...");
      #endif
      connectToWiFi();
    }
    lastWiFiCheck = millis();
  }
  
  // Read sensor values
  readSensors();
  
  // Control pumps based on moisture levels
  controlPump(1, value1, MOISTURE_THRESHOLD);
  controlPump(2, value2, MOISTURE_THRESHOLD);
  controlPump(3, value3, MOISTURE_THRESHOLD);
  controlPump(4, value4, MOISTURE_THRESHOLD);
  
  // Print sensor readings
  #if DEBUG_MODE
  printSensorReadings();
  #endif
  
  // Send data to API periodically (only if API is enabled and WiFi is connected)
  if (apiEnabled && WiFi.status() == WL_CONNECTED && millis() - lastApiCall > API_SEND_INTERVAL) {
    sendDataToApi();
    lastApiCall = millis();
  }
  
  delay(1000);
}

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  #if DEBUG_MODE
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  #endif
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi. Will retry later.");
    Serial.println("Device will continue to function for watering control.");
  }
}

void readSensors() {
  // Read from all 4 moisture sensors using the multiplexer
  value1 = readSensorFromMultiplexer(SENSOR_1_CHANNEL);
  value2 = readSensorFromMultiplexer(SENSOR_2_CHANNEL);
  value3 = readSensorFromMultiplexer(SENSOR_3_CHANNEL);
  value4 = readSensorFromMultiplexer(SENSOR_4_CHANNEL);
}

// Function to read from a specific multiplexer channel
int readSensorFromMultiplexer(int channel) {
  // Set the multiplexer channel selection pins
  digitalWrite(MUX_S0, (channel & 1) ? HIGH : LOW);      // Bit 0
  digitalWrite(MUX_S1, (channel & 2) ? HIGH : LOW);      // Bit 1
  digitalWrite(MUX_S2, (channel & 4) ? HIGH : LOW);      // Bit 2
  digitalWrite(MUX_S3, (channel & 8) ? HIGH : LOW);      // Bit 3
  
  // Small delay to allow multiplexer to settle
  delayMicroseconds(100);
  
  // Read the analog value
  int sensorValue = analogRead(ANALOG_IN);
  
  #if DEBUG_MODE
  Serial.print("Channel "); Serial.print(channel); Serial.print(": "); Serial.println(sensorValue);
  #endif
  
  return sensorValue;
}

void printSensorReadings() {
  Serial.println("=== MOISTURE SENSOR READINGS ===");
  Serial.print("Sensor 1: "); Serial.print(value1); Serial.print(" (Pump: "); Serial.print(pump1Active ? "ON" : "OFF"); Serial.println(")");
  Serial.print("Sensor 2: "); Serial.print(value2); Serial.print(" (Pump: "); Serial.print(pump2Active ? "ON" : "OFF"); Serial.println(")");
  Serial.print("Sensor 3: "); Serial.print(value3); Serial.print(" (Pump: "); Serial.print(pump3Active ? "ON" : "OFF"); Serial.println(")");
  Serial.print("Sensor 4: "); Serial.print(value4); Serial.print(" (Pump: "); Serial.print(pump4Active ? "ON" : "OFF"); Serial.println(")");
  Serial.println();
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
  
  bool shouldActivate = moistureValue > threshold;
  
  if (shouldActivate && !(*pumpStatus)) {
    // Turning pump ON
    digitalWrite(pumpPin, LOW);
    *pumpStatus = true;
    pumpActivations++;
    #if DEBUG_MODE
    Serial.print("Pump "); Serial.print(pumpNumber); Serial.println(": ACTIVATED");
    #endif
  } else if (!shouldActivate && *pumpStatus) {
    // Turning pump OFF
    digitalWrite(pumpPin, HIGH);
    *pumpStatus = false;
    #if DEBUG_MODE
    Serial.print("Pump "); Serial.print(pumpNumber); Serial.println(": DEACTIVATED");
    #endif
  }
}

void sendDataToApi() {
  if (WiFi.status() != WL_CONNECTED) {
    #if DEBUG_MODE
    Serial.println("Cannot send data: WiFi not connected");
    #endif
    failedApiCalls++;
    return;
  }
  
  // Check if API URL is still placeholder
  if (strstr(API_URL, "placeholder") != NULL) {
    #if DEBUG_MODE
    Serial.println("API URL is still placeholder. Skipping API call.");
    #endif
    return;
  }
  
  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");
  
  // Add API key if configured
  if (strlen(API_KEY) > 0) {
    http.addHeader("Authorization", API_KEY);
  }
  
  // Create JSON payload
  DynamicJsonDocument doc(2048);
  doc["device_id"] = DEVICE_ID;
  doc["timestamp"] = millis();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["uptime_seconds"] = millis() / 1000;
  doc["total_api_calls"] = totalApiCalls;
  doc["failed_api_calls"] = failedApiCalls;
  doc["pump_activations"] = pumpActivations;
  
  JsonArray sensors = doc.createNestedArray("sensors");
  
  JsonObject sensor1 = sensors.createNestedObject();
  sensor1["id"] = 1;
  sensor1["moisture_value"] = value1;
  sensor1["pump_active"] = pump1Active;
  sensor1["threshold"] = MOISTURE_THRESHOLD;
  
  JsonObject sensor2 = sensors.createNestedObject();
  sensor2["id"] = 2;
  sensor2["moisture_value"] = value2;
  sensor2["pump_active"] = pump2Active;
  sensor2["threshold"] = MOISTURE_THRESHOLD;
  
  JsonObject sensor3 = sensors.createNestedObject();
  sensor3["id"] = 3;
  sensor3["moisture_value"] = value3;
  sensor3["pump_active"] = pump3Active;
  sensor3["threshold"] = MOISTURE_THRESHOLD;
  
  JsonObject sensor4 = sensors.createNestedObject();
  sensor4["id"] = 4;
  sensor4["moisture_value"] = value4;
  sensor4["pump_active"] = pump4Active;
  sensor4["threshold"] = MOISTURE_THRESHOLD;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  #if DEBUG_MODE
  Serial.println("Sending data to API:");
  Serial.println(jsonString);
  #endif
  
  totalApiCalls++;
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    #if DEBUG_MODE
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response: ");
    Serial.println(response);
    #endif
  } else {
    failedApiCalls++;
    #if DEBUG_MODE
    Serial.print("HTTP Error code: ");
    Serial.println(httpResponseCode);
    Serial.println("API call failed, but device continues to function normally.");
    #endif
  }
  
  http.end();
} 