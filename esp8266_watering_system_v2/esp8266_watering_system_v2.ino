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

// Water level sensor pin
int WATER_LEVEL_PIN = WATER_LEVEL_SENSOR_PIN;

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

// Pump runtime tracking
unsigned long pump1StartTime = 0;
unsigned long pump2StartTime = 0;
unsigned long pump3StartTime = 0;
unsigned long pump4StartTime = 0;

// Pump cooldown tracking
unsigned long pump1CooldownEnd = 0;
unsigned long pump2CooldownEnd = 0;
unsigned long pump3CooldownEnd = 0;
unsigned long pump4CooldownEnd = 0;

// Daily pump activation counter
unsigned long dailyPumpActivations = 0;
unsigned long lastDailyReset = 0;

// Water level status
bool waterLevelLow = false;
bool waterLevelCritical = false;

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
  Serial.println("Reading Cycle: 10 seconds");
  Serial.println("Pump Runtime: 10 seconds per cycle");
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
  
  // Configure water level sensor pin if enabled
  #if WATER_LEVEL_SENSOR_ENABLED
  pinMode(WATER_LEVEL_PIN, INPUT_PULLUP);
  #endif
  
  // Initialize pumps to OFF state
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, HIGH);
  
  // Initialize daily reset timer
  lastDailyReset = millis();
  
  // Connect to WiFi
  connectToWiFi();
  
  delay(1000);
  
  // Run sensor test to diagnose low values
  testSensorsDirectly();
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
  
  // Check water level status
  checkWaterLevel();
  
  // Check pump runtime limits
  checkPumpRuntime();
  
  // Reset daily counter if needed
  resetDailyCounter();
  
  // Read sensor values
  readSensors();
  
  // Control pumps based on moisture levels (with protection)
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
  
  delay(10000); // Changed from 1000ms to 10000ms (10 seconds)
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
  
  // Additional diagnostic information
  Serial.print("  MUX pins - S0:"); Serial.print((channel & 1) ? "HIGH" : "LOW");
  Serial.print(" S1:"); Serial.print((channel & 2) ? "HIGH" : "LOW");
  Serial.print(" S2:"); Serial.print((channel & 4) ? "HIGH" : "LOW");
  Serial.print(" S3:"); Serial.println((channel & 8) ? "HIGH" : "LOW");
  
  // Check if value is suspiciously low
  if (sensorValue < 10) {
    Serial.println("  WARNING: Very low sensor value detected!");
    Serial.println("  Possible issues:");
    Serial.println("  1. Sensor not connected properly");
    Serial.println("  2. Sensor not powered (VCC/GND)");
    Serial.println("  3. Multiplexer not working correctly");
    Serial.println("  4. Wrong channel selected");
  }
  #endif
  
  return sensorValue;
}

void printSensorReadings() {
  Serial.println("=== MOISTURE SENSOR READINGS (10s cycle) ===");
  Serial.print("Sensor 1: "); Serial.print(value1); Serial.print(" (Pump: "); Serial.print(pump1Active ? "ON" : "OFF"); Serial.println(")");
  Serial.print("Sensor 2: "); Serial.print(value2); Serial.print(" (Pump: "); Serial.print(pump2Active ? "ON" : "OFF"); Serial.println(")");
  Serial.print("Sensor 3: "); Serial.print(value3); Serial.print(" (Pump: "); Serial.print(pump3Active ? "ON" : "OFF"); Serial.println(")");
  Serial.print("Sensor 4: "); Serial.print(value4); Serial.print(" (Pump: "); Serial.print(pump4Active ? "ON" : "OFF"); Serial.println(")");
  
  // Display protection status
  Serial.println("=== PROTECTION STATUS ===");
  Serial.print("Water Level: "); Serial.println(waterLevelLow ? "LOW" : "OK");
  Serial.print("Daily Activations: "); Serial.print(dailyPumpActivations); Serial.print("/"); Serial.println(MAX_DAILY_PUMP_ACTIVATIONS);
  
  // Display pump runtime information
  if (pump1Active) {
    unsigned long runtime = (millis() - pump1StartTime) / 1000;
    Serial.print("Pump 1 Runtime: "); Serial.print(runtime); Serial.println(" seconds");
  }
  if (pump2Active) {
    unsigned long runtime = (millis() - pump2StartTime) / 1000;
    Serial.print("Pump 2 Runtime: "); Serial.print(runtime); Serial.println(" seconds");
  }
  if (pump3Active) {
    unsigned long runtime = (millis() - pump3StartTime) / 1000;
    Serial.print("Pump 3 Runtime: "); Serial.print(runtime); Serial.println(" seconds");
  }
  if (pump4Active) {
    unsigned long runtime = (millis() - pump4StartTime) / 1000;
    Serial.print("Pump 4 Runtime: "); Serial.print(runtime); Serial.println(" seconds");
  }
  
  Serial.println("--- Next reading in 10 seconds ---");
  Serial.println();
}

// Function to check water level status
void checkWaterLevel() {
  #if WATER_LEVEL_SENSOR_ENABLED
  // Read water level sensor (digital sensor - LOW means water detected)
  bool waterDetected = digitalRead(WATER_LEVEL_PIN) == LOW;
  
  // Update water level status
  waterLevelLow = !waterDetected;
  waterLevelCritical = !waterDetected;
  
  #if DEBUG_MODE
  if (waterLevelLow) {
    Serial.println("WARNING: Water level is LOW - Reservoir may be empty!");
  }
  #endif
  
  #else
  // If water level sensor is disabled, assume water is available
  waterLevelLow = false;
  waterLevelCritical = false;
  #endif
}

// Function to check if pump can be activated (runtime and cooldown protection)
bool canActivatePump(int pumpNumber) {
  // Check water level first
  if (waterLevelCritical) {
    #if DEBUG_MODE
    Serial.print("Pump "); Serial.print(pumpNumber); Serial.println(": Cannot activate - Water level critical!");
    #endif
    return false;
  }
  
  // Check daily activation limit
  if (dailyPumpActivations >= MAX_DAILY_PUMP_ACTIVATIONS) {
    #if DEBUG_MODE
    Serial.print("Pump "); Serial.print(pumpNumber); Serial.println(": Cannot activate - Daily limit reached!");
    #endif
    return false;
  }
  
  // Check cooldown period
  unsigned long currentTime = millis();
  unsigned long cooldownEnd = 0;
  
  switch(pumpNumber) {
    case 1: cooldownEnd = pump1CooldownEnd; break;
    case 2: cooldownEnd = pump2CooldownEnd; break;
    case 3: cooldownEnd = pump3CooldownEnd; break;
    case 4: cooldownEnd = pump4CooldownEnd; break;
    default: return false;
  }
  
  if (currentTime < cooldownEnd) {
    #if DEBUG_MODE
    Serial.print("Pump "); Serial.print(pumpNumber); Serial.println(": Cannot activate - In cooldown period!");
    #endif
    return false;
  }
  
  return true;
}

// Function to check pump runtime and enforce limits
void checkPumpRuntime() {
  unsigned long currentTime = millis();
  
  // Check pump 1 runtime
  if (pump1Active && (currentTime - pump1StartTime) > MAX_PUMP_RUNTIME) {
    digitalWrite(IN1, HIGH);
    pump1Active = false;
    pump1CooldownEnd = currentTime + PUMP_COOLDOWN_PERIOD;
    #if DEBUG_MODE
    Serial.println("Pump 1: Max runtime reached - Deactivated for cooldown");
    #endif
  }
  
  // Check pump 2 runtime
  if (pump2Active && (currentTime - pump2StartTime) > MAX_PUMP_RUNTIME) {
    digitalWrite(IN2, HIGH);
    pump2Active = false;
    pump2CooldownEnd = currentTime + PUMP_COOLDOWN_PERIOD;
    #if DEBUG_MODE
    Serial.println("Pump 2: Max runtime reached - Deactivated for cooldown");
    #endif
  }
  
  // Check pump 3 runtime
  if (pump3Active && (currentTime - pump3StartTime) > MAX_PUMP_RUNTIME) {
    digitalWrite(IN3, HIGH);
    pump3Active = false;
    pump3CooldownEnd = currentTime + PUMP_COOLDOWN_PERIOD;
    #if DEBUG_MODE
    Serial.println("Pump 3: Max runtime reached - Deactivated for cooldown");
    #endif
  }
  
  // Check pump 4 runtime
  if (pump4Active && (currentTime - pump4StartTime) > MAX_PUMP_RUNTIME) {
    digitalWrite(IN4, HIGH);
    pump4Active = false;
    pump4CooldownEnd = currentTime + PUMP_COOLDOWN_PERIOD;
    #if DEBUG_MODE
    Serial.println("Pump 4: Max runtime reached - Deactivated for cooldown");
    #endif
  }
}

// Function to reset daily pump activation counter
void resetDailyCounter() {
  unsigned long currentTime = millis();
  unsigned long oneDay = 86400000; // 24 hours in milliseconds
  
  if (currentTime - lastDailyReset > oneDay) {
    dailyPumpActivations = 0;
    lastDailyReset = currentTime;
    #if DEBUG_MODE
    Serial.println("Daily pump activation counter reset");
    #endif
  }
}

// Test function to read sensors directly without multiplexer
void testSensorsDirectly() {
  Serial.println("=== DIRECT SENSOR TEST (without multiplexer) ===");
  
  // Test reading from A0 directly
  int directReading = analogRead(ANALOG_IN);
  Serial.print("Direct A0 reading: "); Serial.println(directReading);
  
  // Test with different delays
  delay(100);
  directReading = analogRead(ANALOG_IN);
  Serial.print("Direct A0 reading (after 100ms): "); Serial.println(directReading);
  
  delay(500);
  directReading = analogRead(ANALOG_IN);
  Serial.print("Direct A0 reading (after 500ms): "); Serial.println(directReading);
  
  Serial.println("=== END DIRECT TEST ===");
  Serial.println();
}

void controlPump(int pumpNumber, float moistureValue, int threshold) {
  int pumpPin;
  bool* pumpStatus;
  unsigned long* pumpStartTime;
  
  switch(pumpNumber) {
    case 1:
      pumpPin = IN1;
      pumpStatus = &pump1Active;
      pumpStartTime = &pump1StartTime;
      break;
    case 2:
      pumpPin = IN2;
      pumpStatus = &pump2Active;
      pumpStartTime = &pump2StartTime;
      break;
    case 3:
      pumpPin = IN3;
      pumpStatus = &pump3Active;
      pumpStartTime = &pump3StartTime;
      break;
    case 4:
      pumpPin = IN4;
      pumpStatus = &pump4Active;
      pumpStartTime = &pump4StartTime;
      break;
    default:
      return;
  }
  
  bool shouldActivate = moistureValue > threshold;
  
  if (shouldActivate && !(*pumpStatus)) {
    // Check if pump can be activated (protection mechanisms)
    if (canActivatePump(pumpNumber)) {
      // Turning pump ON
      digitalWrite(pumpPin, LOW);
      *pumpStatus = true;
      *pumpStartTime = millis(); // Record start time
      pumpActivations++;
      dailyPumpActivations++;
      #if DEBUG_MODE
      Serial.print("Pump "); Serial.print(pumpNumber); Serial.println(": ACTIVATED (will run for 10s)");
      #endif
    } else {
      #if DEBUG_MODE
      Serial.print("Pump "); Serial.print(pumpNumber); Serial.println(": Activation blocked by protection");
      #endif
    }
  } else if (!shouldActivate && *pumpStatus) {
    // Turning pump OFF
    digitalWrite(pumpPin, HIGH);
    *pumpStatus = false;
    #if DEBUG_MODE
    Serial.print("Pump "); Serial.print(pumpNumber); Serial.println(": DEACTIVATED (moisture threshold met)");
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
  
  // Add protection status
  doc["water_level_low"] = waterLevelLow;
  doc["water_level_critical"] = waterLevelCritical;
  doc["daily_pump_activations"] = dailyPumpActivations;
  doc["max_daily_activations"] = MAX_DAILY_PUMP_ACTIVATIONS;
  
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