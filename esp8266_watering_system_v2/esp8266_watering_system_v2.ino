#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
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

// Individual thresholds for each sensor (stored in EEPROM)
int threshold1 = MOISTURE_THRESHOLD_1;
int threshold2 = MOISTURE_THRESHOLD_2;
int threshold3 = MOISTURE_THRESHOLD_3;
int threshold4 = MOISTURE_THRESHOLD_4;

// EEPROM addresses for storing thresholds
const int EEPROM_THRESHOLD1 = 0;
const int EEPROM_THRESHOLD2 = 4;
const int EEPROM_THRESHOLD3 = 8;
const int EEPROM_THRESHOLD4 = 12;

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
  
  // Initialize EEPROM
  EEPROM.begin(512);
  
  // Load thresholds from EEPROM
  loadThresholds();
  
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
  Serial.println("Watering System Ready");
  Serial.println("Commands: set_threshold <sensor> <value>, get_thresholds, help");
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
  
  // Test API connection on startup
  testApiConnection();
  
  // Test basic connectivity
  testBasicConnectivity();
  
  // Test HTTP connection
  testHttpConnection();
  
  // Test HTTPS GET connection
  testHttpsGet();
}

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "test") {
      testApiConnection();
    } else if (command == "ping") {
      testBasicConnectivity();
    } else if (command == "http") {
      testHttpConnection();
    } else if (command == "https") {
      testHttpsGet();
    } else if (command == "status") {
      printSensorReadings();
    } else if (command == "get_thresholds") {
      printThresholds();
    } else if (command.startsWith("set_threshold")) {
      handleSerialCommand(command);
    } else if (command == "help") {
      Serial.println("Available commands:");
      Serial.println("  test   - Test API connection");
      Serial.println("  ping   - Test basic connectivity");
      Serial.println("  http   - Test HTTP connection");
      Serial.println("  https  - Test HTTPS GET connection");
      Serial.println("  status - Show current sensor readings");
      Serial.println("  get_thresholds - Show current thresholds");
      Serial.println("  set_threshold <sensor> <value> - Set threshold for sensor (1-4)");
      Serial.println("  help   - Show this help message");
    }
  }
  
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
  controlPump(1, value1, threshold1);
  controlPump(2, value2, threshold2);
  controlPump(3, value3, threshold3);
  controlPump(4, value4, threshold4);
  
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
  
  #if DEBUG_MODE
  Serial.println("=== API COMMUNICATION DEBUG ===");
  Serial.print("WiFi Status: "); Serial.println(WiFi.status());
  Serial.print("WiFi RSSI: "); Serial.println(WiFi.RSSI());
  Serial.print("Target URL: "); Serial.println(API_URL);
  Serial.print("Device ID: "); Serial.println(DEVICE_ID);
  #endif
  
  #if DEBUG_MODE
  Serial.println("Initializing HTTP client...");
  #endif
  
  WiFiClientSecure client;
  HTTPClient http;
  
  #if DEBUG_MODE
  Serial.println("Configuring HTTPS client...");
  #endif
  
  // Configure SSL client for HTTPS
  client.setInsecure(); // Skip certificate verification
  client.setTimeout(10000);
  
  http.begin(client, API_URL);
  http.addHeader("Content-Type", "application/json");
  
  // Add API key if configured
  if (strlen(API_KEY) > 0) {
    http.addHeader("Authorization", API_KEY);
    #if DEBUG_MODE
    Serial.println("API Key added to headers");
    #endif
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
  sensor1["threshold"] = threshold1;
  
  JsonObject sensor2 = sensors.createNestedObject();
  sensor2["id"] = 2;
  sensor2["moisture_value"] = value2;
  sensor2["pump_active"] = pump2Active;
  sensor2["threshold"] = threshold2;
  
  JsonObject sensor3 = sensors.createNestedObject();
  sensor3["id"] = 3;
  sensor3["moisture_value"] = value3;
  sensor3["pump_active"] = pump3Active;
  sensor3["threshold"] = threshold3;
  
  JsonObject sensor4 = sensors.createNestedObject();
  sensor4["id"] = 4;
  sensor4["moisture_value"] = value4;
  sensor4["pump_active"] = pump4Active;
  sensor4["threshold"] = threshold4;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  #if DEBUG_MODE
  Serial.println("JSON Payload:");
  Serial.println(jsonString);
  Serial.print("Payload size: "); Serial.print(jsonString.length()); Serial.println(" characters");
  Serial.println("Sending HTTPS POST request...");
  #endif
  
  totalApiCalls++;
  unsigned long startTime = millis();
  Serial.println("POST request started...");
  
  int httpResponseCode = http.POST(jsonString);
  
  unsigned long endTime = millis();
  Serial.println("POST request completed!");
  
  Serial.print("Request completed in: ");
  Serial.print(endTime - startTime);
  Serial.println(" ms");
  
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    #if DEBUG_MODE
    Serial.print("Response: ");
    Serial.println(response);
    Serial.println("=== API CALL SUCCESSFUL ===");
    #endif
  } else {
    failedApiCalls++;
    #if DEBUG_MODE
    Serial.print("HTTP Error code: ");
    Serial.println(httpResponseCode);
    Serial.print("Error: ");
    Serial.println(http.errorToString(httpResponseCode));
    Serial.println("=== API CALL FAILED ===");
    #endif
  }
  
  http.end();
  #if DEBUG_MODE
  Serial.println("HTTP client closed");
  Serial.println("================================");
  #endif
}

// Test function to manually test API connection
void testApiConnection() {
  Serial.println("=== MANUAL API TEST ===");
  Serial.println("Testing API connection...");
  
  // Show the exact URL being called
  Serial.print("Target URL: ");
  Serial.println(API_URL);
  
  // Test basic connectivity first
  Serial.println("Testing basic connectivity...");
  WiFiClientSecure client;
  HTTPClient http;
  
  Serial.println("Initializing HTTPS client...");
  
  // Configure SSL client
  client.setInsecure(); // Skip certificate verification for testing
  client.setTimeout(5000); // Reduced to 5 seconds
  
  http.begin(client, API_URL);
  http.addHeader("Content-Type", "application/json");
  
  // Set timeout
  http.setTimeout(5000); // 5 second timeout
  
  // Create a simple test payload
  DynamicJsonDocument doc(512);
  doc["test"] = true;
  doc["device_id"] = DEVICE_ID;
  doc["timestamp"] = millis();
  doc["message"] = "API connectivity test from ESP8266";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("Sending test payload:");
  Serial.println(jsonString);
  Serial.println("Sending HTTPS POST request...");
  
  unsigned long startTime = millis();
  int httpResponseCode = http.POST(jsonString);
  unsigned long endTime = millis();
  
  Serial.print("Request completed in: ");
  Serial.print(endTime - startTime);
  Serial.println(" ms");
  
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  
  if (httpResponseCode > 0) {
    Serial.println("Reading response...");
    String response = http.getString();
    Serial.print("Response length: ");
    Serial.println(response.length());
    Serial.print("Response: ");
    Serial.println(response);
    Serial.println("=== API TEST APPEARS SUCCESSFUL ===");
  } else {
    Serial.print("HTTP Error code: ");
    Serial.println(httpResponseCode);
    Serial.print("Error: ");
    Serial.println(http.errorToString(httpResponseCode));
    Serial.println("=== API TEST FAILED ===");
  }
  
  Serial.println("Closing HTTP client...");
  http.end();
  Serial.println("=== API TEST COMPLETE ===");
}

// Test basic connectivity to the domain
void testBasicConnectivity() {
  Serial.println("=== BASIC CONNECTIVITY TEST ===");
  
  // Extract domain from API_URL
  String domain = "private-watering-system.onrender.com";
  Serial.print("Testing connectivity to: ");
  Serial.println(domain);
  
  WiFiClient client;
  if (client.connect(domain, 80)) {
    Serial.println("✅ Basic TCP connection successful");
    
    // Send a simple HTTP GET request
    client.println("GET /api/health HTTP/1.1");
    client.print("Host: ");
    client.println(domain);
    client.println("Connection: close");
    client.println();
    
    // Wait for response
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("❌ No response received (timeout)");
        client.stop();
        return;
      }
    }
    
    // Read response
    Serial.println("Response received:");
    while (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
    
    client.stop();
    Serial.println("✅ HTTP connectivity test successful");
  } else {
    Serial.println("❌ Failed to connect to domain");
  }
  
  Serial.println("=== CONNECTIVITY TEST COMPLETE ===");
}

// Test with HTTP instead of HTTPS
void testHttpConnection() {
  Serial.println("=== HTTP CONNECTION TEST ===");
  
  String httpUrl = "http://private-watering-system.onrender.com/api/health";
  Serial.print("Testing HTTP URL: ");
  Serial.println(httpUrl);
  
  WiFiClient client;
  HTTPClient http;
  
  Serial.println("Initializing HTTP client...");
  http.begin(client, httpUrl);
  http.setTimeout(5000); // 5 second timeout
  
  Serial.println("Sending HTTP GET request...");
  unsigned long startTime = millis();
  int httpResponseCode = http.GET();
  unsigned long endTime = millis();
  
  Serial.print("Request completed in: ");
  Serial.print(endTime - startTime);
  Serial.println(" ms");
  
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Response: ");
    Serial.println(response);
    Serial.println("✅ HTTP test successful");
  } else {
    Serial.print("Error: ");
    Serial.println(http.errorToString(httpResponseCode));
    Serial.println("❌ HTTP test failed");
  }
  
  http.end();
  Serial.println("=== HTTP TEST COMPLETE ===");
}

// Test HTTPS GET connection
void testHttpsGet() {
  Serial.println("=== HTTPS GET TEST ===");
  
  String httpsUrl = "https://private-watering-system.onrender.com/api/health";
  Serial.print("Testing HTTPS GET URL: ");
  Serial.println(httpsUrl);
  
  WiFiClientSecure client;
  HTTPClient http;
  
  Serial.println("Initializing HTTPS client...");
  client.setInsecure();
  client.setTimeout(5000);
  
  http.begin(client, httpsUrl);
  http.setTimeout(5000);
  
  Serial.println("Sending HTTPS GET request...");
  unsigned long startTime = millis();
  int httpResponseCode = http.GET();
  unsigned long endTime = millis();
  
  Serial.print("Request completed in: ");
  Serial.print(endTime - startTime);
  Serial.println(" ms");
  
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Response: ");
    Serial.println(response);
    Serial.println("✅ HTTPS GET test successful");
  } else {
    Serial.print("Error: ");
    Serial.println(http.errorToString(httpResponseCode));
    Serial.println("❌ HTTPS GET test failed");
  }
  
  http.end();
  Serial.println("=== HTTPS GET TEST COMPLETE ===");
}

// Function to handle serial commands for threshold management
void handleSerialCommand(String command) {
  if (command.startsWith("set_threshold")) {
    // Parse command: set_threshold <sensor> <value>
    int firstSpace = command.indexOf(' ');
    int secondSpace = command.indexOf(' ', firstSpace + 1);
    
    if (firstSpace == -1 || secondSpace == -1) {
      Serial.println("Error: Invalid format. Use: set_threshold <sensor> <value>");
      return;
    }
    
    String sensorStr = command.substring(firstSpace + 1, secondSpace);
    String valueStr = command.substring(secondSpace + 1);
    
    int sensor = sensorStr.toInt();
    int value = valueStr.toInt();
    
    if (sensor < 1 || sensor > 4) {
      Serial.println("Error: Sensor must be 1, 2, 3, or 4");
      return;
    }
    
    if (value < 0 || value > 1023) {
      Serial.println("Error: Threshold value must be between 0 and 1023");
      return;
    }
    
    setThreshold(sensor, value);
    Serial.print("Threshold for sensor "); Serial.print(sensor); Serial.print(" set to: "); Serial.println(value);
  }
}

// Function to set threshold for a specific sensor
void setThreshold(int sensor, int value) {
  switch(sensor) {
    case 1:
      threshold1 = value;
      EEPROM.put(EEPROM_THRESHOLD1, threshold1);
      break;
    case 2:
      threshold2 = value;
      EEPROM.put(EEPROM_THRESHOLD2, threshold2);
      break;
    case 3:
      threshold3 = value;
      EEPROM.put(EEPROM_THRESHOLD3, threshold3);
      break;
    case 4:
      threshold4 = value;
      EEPROM.put(EEPROM_THRESHOLD4, threshold4);
      break;
  }
  EEPROM.commit();
}

// Function to load thresholds from EEPROM
void loadThresholds() {
  int storedThreshold1, storedThreshold2, storedThreshold3, storedThreshold4;
  
  EEPROM.get(EEPROM_THRESHOLD1, storedThreshold1);
  EEPROM.get(EEPROM_THRESHOLD2, storedThreshold2);
  EEPROM.get(EEPROM_THRESHOLD3, storedThreshold3);
  EEPROM.get(EEPROM_THRESHOLD4, storedThreshold4);
  
  // Check if EEPROM values are valid (not -1 or 0, which indicates uninitialized)
  if (storedThreshold1 > 0 && storedThreshold1 <= 1023) {
    threshold1 = storedThreshold1;
  }
  if (storedThreshold2 > 0 && storedThreshold2 <= 1023) {
    threshold2 = storedThreshold2;
  }
  if (storedThreshold3 > 0 && storedThreshold3 <= 1023) {
    threshold3 = storedThreshold3;
  }
  if (storedThreshold4 > 0 && storedThreshold4 <= 1023) {
    threshold4 = storedThreshold4;
  }
  
  #if DEBUG_MODE
  Serial.println("Thresholds loaded from EEPROM:");
  Serial.print("Sensor 1: "); Serial.println(threshold1);
  Serial.print("Sensor 2: "); Serial.println(threshold2);
  Serial.print("Sensor 3: "); Serial.println(threshold3);
  Serial.print("Sensor 4: "); Serial.println(threshold4);
  #endif
}

// Function to print current thresholds
void printThresholds() {
  Serial.println("=== CURRENT THRESHOLDS ===");
  Serial.print("Sensor 1: "); Serial.println(threshold1);
  Serial.print("Sensor 2: "); Serial.println(threshold2);
  Serial.print("Sensor 3: "); Serial.println(threshold3);
  Serial.print("Sensor 4: "); Serial.println(threshold4);
  Serial.println("==========================");
} 