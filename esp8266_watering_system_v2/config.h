#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration - Update these with your actual WiFi credentials
#define WIFI_SSID "MountPanke"
#define WIFI_PASSWORD "Feige&Olive"

// API Configuration - Placeholder for now, will be updated when web app is ready
#define API_URL "http://placeholder-api.com/api/watering-data"
#define API_KEY ""  // Leave empty for now

// Device Configuration
#define DEVICE_ID "ESP8266_WATERING_SYSTEM_01"
#define MOISTURE_THRESHOLD 800  // Threshold for activating pumps
#define API_SEND_INTERVAL 300000 // Send data every 5 minutes (300,000 milliseconds)

// Pin Configuration for ESP8266
#define PUMP_1_PIN 2   // D4 on ESP8266
#define PUMP_2_PIN 0   // D3 on ESP8266  
#define PUMP_3_PIN 4   // D2 on ESP8266
#define PUMP_4_PIN 5   // D1 on ESP8266

#define MOISTURE_SENSOR_PIN A0  // ESP8266 only has one analog pin

// Water Level Sensor Configuration
#define WATER_LEVEL_SENSOR_PIN 16  // D0 on ESP8266 - Digital water level sensor
#define WATER_LEVEL_SENSOR_ENABLED false  // Set to false if not using water level sensor
#define WATER_LEVEL_LOW_THRESHOLD 200  // Analog value threshold for low water (if using analog sensor)

// Pump Runtime Protection
#define MAX_PUMP_RUNTIME 300000  // Maximum pump runtime: 5 minutes (300,000 ms)
#define PUMP_COOLDOWN_PERIOD 600000  // Cooldown period after max runtime: 10 minutes (600,000 ms)
#define MAX_DAILY_PUMP_ACTIVATIONS 50  // Maximum pump activations per day

// Multiplexer Control Pins for CD74HC4067 (16-channel)
#define MUX_S0_PIN 12  // D6 on ESP8266 - Channel select bit 0
#define MUX_S1_PIN 13  // D7 on ESP8266 - Channel select bit 1  
#define MUX_S2_PIN 14  // D5 on ESP8266 - Channel select bit 2
#define MUX_S3_PIN 15  // D8 on ESP8266 - Channel select bit 3

// Multiplexer channel assignments for moisture sensors
#define SENSOR_1_CHANNEL 0
#define SENSOR_2_CHANNEL 1
#define SENSOR_3_CHANNEL 2
#define SENSOR_4_CHANNEL 3

// Optional: Enable debug mode
#define DEBUG_MODE true

#endif 