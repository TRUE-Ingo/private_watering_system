# ESP8266 Watering System with Web API Integration

This is an ESP8266 version of the Arduino watering system that can communicate with a web API to send sensor data and pump status information.

## Features

- **WiFi Connectivity**: Connects to your WiFi network
- **API Communication**: Sends JSON data to your web API
- **4-Zone Watering**: Controls 4 separate watering zones
- **Moisture Sensing**: Monitors soil moisture levels
- **Automatic Pump Control**: Activates pumps when soil is dry
- **Real-time Monitoring**: Sends data every 30 seconds (configurable)

## Hardware Requirements

- ESP8266 (NodeMCU, Wemos D1 Mini, or similar)
- 4x Relay modules for pump control
- 4x Soil moisture sensors (or 1 sensor with multiplexer)
- 4x Water pumps
- Power supply for pumps
- Breadboard and jumper wires

## Pin Connections

| Component | ESP8266 Pin | Description |
|-----------|-------------|-------------|
| Pump 1 Relay | D4 (GPIO2) | Controls pump 1 |
| Pump 2 Relay | D3 (GPIO0) | Controls pump 2 |
| Pump 3 Relay | D2 (GPIO4) | Controls pump 3 |
| Pump 4 Relay | D1 (GPIO5) | Controls pump 4 |
| Moisture Sensor | A0 | Analog moisture reading |

## Setup Instructions

### 1. Install Required Libraries

In Arduino IDE, install these libraries:
- ESP8266WiFi
- ESP8266HTTPClient
- ArduinoJson (version 6.x)

### 2. Configure Settings

Edit `config.h` with your specific settings:

```cpp
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASSWORD "YourWiFiPassword"
#define API_URL "http://your-api-endpoint.com/api/watering-data"
#define API_KEY "your-api-key-if-needed"
```

### 3. API Data Format

The ESP8266 sends JSON data in this format:

```json
{
  "device_id": "ESP8266_WATERING_SYSTEM_01",
  "timestamp": 1234567890,
  "wifi_rssi": -45,
  "sensors": [
    {
      "id": 1,
      "moisture_value": 650,
      "pump_active": true
    },
    {
      "id": 2,
      "moisture_value": 520,
      "pump_active": false
    },
    {
      "id": 3,
      "moisture_value": 580,
      "pump_active": false
    },
    {
      "id": 4,
      "moisture_value": 610,
      "pump_active": true
    }
  ]
}
```

### 4. Important Notes

**ESP8266 Analog Pin Limitation**: The ESP8266 only has one analog pin (A0). For multiple moisture sensors, you have these options:

1. **Use a Multiplexer**: Connect multiple sensors through a CD4051 or similar multiplexer
2. **Use Digital Sensors**: Switch to digital moisture sensors
3. **Use Different ESP8266 Pins**: Some ESP8266 boards have additional analog inputs

**Current Implementation**: The current code simulates multiple sensors by adding random variations to the single sensor reading. For production use, implement one of the solutions above.

### 5. Customization Options

- **Moisture Threshold**: Adjust `MOISTURE_THRESHOLD` in config.h
- **API Send Interval**: Change `API_SEND_INTERVAL` to send data more/less frequently
- **Device ID**: Customize `DEVICE_ID` for multiple devices
- **Pump Control Logic**: Modify the `controlPump()` function for different activation logic

### 6. Troubleshooting

**WiFi Connection Issues**:
- Check SSID and password
- Ensure ESP8266 is within WiFi range
- Try restarting the device

**API Communication Issues**:
- Verify API endpoint URL
- Check if API requires authentication
- Monitor Serial output for error codes

**Pump Control Issues**:
- Verify relay connections
- Check power supply for pumps
- Ensure correct pin assignments

## API Endpoint Requirements

Your web API should accept POST requests with JSON data. The ESP8266 will send:
- Content-Type: application/json
- Authorization header (if API_KEY is configured)
- JSON payload with sensor and pump data

## Security Considerations

- Use HTTPS for API communication in production
- Implement proper API authentication
- Consider using WiFiManager for easier WiFi configuration
- Add error handling for network failures

## Future Enhancements

- WiFiManager integration for easy WiFi setup
- OTA (Over-The-Air) updates
- Local web interface for configuration
- Data logging to SD card
- MQTT support for IoT platforms
- Multiple sensor support with multiplexer 