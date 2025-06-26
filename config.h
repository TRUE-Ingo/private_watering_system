#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration - Update these with your actual WiFi credentials
#define WIFI_SSID "YourWiFiNetworkName"
#define WIFI_PASSWORD "YourWiFiPassword"

// API Configuration - Placeholder for now, will be updated when web app is ready
#define API_URL "http://placeholder-api.com/api/watering-data"
#define API_KEY ""  // Leave empty for now

// Device Configuration
#define DEVICE_ID "ESP8266_WATERING_SYSTEM_01"
#define MOISTURE_THRESHOLD 580  // Threshold for activating pumps
#define API_SEND_INTERVAL 300000 // Send data every 5 minutes (300,000 milliseconds)

// Pin Configuration for ESP8266
#define PUMP_1_PIN 2   // D4 on ESP8266
#define PUMP_2_PIN 0   // D3 on ESP8266  
#define PUMP_3_PIN 4   // D2 on ESP8266
#define PUMP_4_PIN 5   // D1 on ESP8266

#define MOISTURE_SENSOR_PIN A0  // ESP8266 only has one analog pin

// Optional: Enable debug mode
#define DEBUG_MODE true

#endif 