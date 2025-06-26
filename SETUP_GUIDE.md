# Setup Guide - ESP8266 Watering System with Multiplexer

## Overview

This watering system uses an ESP8266 with a CD74HC4067 16-channel multiplexer to read from 4 moisture sensors and control 4 water pumps. The system automatically waters plants when moisture levels drop below the threshold.

## Hardware Components

### Required Components:
- **ESP8266 NodeMCU** (or compatible ESP8266 board)
- **CD74HC4067 16-Channel Multiplexer**
- **4x Moisture Sensors** (capacitive or resistive)
- **4x Relay Modules** (for pump control)
- **4x Water Pumps** (12V recommended)
- **Power Supply** (12V for pumps, 5V/3.3V for logic)
- **Breadboard and Jumper Wires**

### Optional Components:
- **Water Reservoir**
- **Tubing and Fittings**
- **Plant Pots with Drainage**

## Step 1: Hardware Connections

### Multiplexer Connections (CD74HC4067):
| Multiplexer Pin | ESP8266 Pin | Description |
|-----------------|-------------|-------------|
| S0 | D6 (GPIO12) | Channel select bit 0 |
| S1 | D7 (GPIO13) | Channel select bit 1 |
| S2 | D5 (GPIO14) | Channel select bit 2 |
| S3 | D8 (GPIO15) | Channel select bit 3 |
| Y (Common) | A0 | Analog input |
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| E (Enable) | GND | Enable (active low) |

### Moisture Sensor Connections:
| Sensor | Multiplexer Channel | Description |
|--------|-------------------|-------------|
| Sensor 1 | Y0 (Channel 0) | Plant 1 moisture |
| Sensor 2 | Y1 (Channel 1) | Plant 2 moisture |
| Sensor 3 | Y2 (Channel 2) | Plant 3 moisture |
| Sensor 4 | Y3 (Channel 3) | Plant 4 moisture |

### Pump Control Connections:
| Component | ESP8266 Pin | Description |
|-----------|-------------|-------------|
| Pump 1 Relay | D4 (GPIO2) | Controls pump 1 |
| Pump 2 Relay | D3 (GPIO0) | Controls pump 2 |
| Pump 3 Relay | D2 (GPIO4) | Controls pump 3 |
| Pump 4 Relay | D1 (GPIO5) | Controls pump 4 |

### Power Connections:
- **ESP8266**: USB power or 5V supply
- **Relay Modules**: 5V from ESP8266 or external supply
- **Water Pumps**: 12V external power supply
- **Multiplexer**: 3.3V from ESP8266

## Step 2: Software Configuration

### Update WiFi Credentials:
Edit `config.h` and replace the WiFi credentials:

```cpp
#define WIFI_SSID "YourActualWiFiName"
#define WIFI_PASSWORD "YourActualWiFiPassword"
```

### Adjust System Parameters (Optional):
```cpp
#define MOISTURE_THRESHOLD 580    // Adjust based on your sensors
#define API_SEND_INTERVAL 300000  // 5 minutes in milliseconds
#define DEVICE_ID "ESP8266_WATERING_SYSTEM_01"  // Unique device name
```

## Step 3: Install Required Libraries

In Arduino IDE, install these libraries:
1. **ESP8266WiFi** (included with ESP8266 board package)
2. **ESP8266HTTPClient** (included with ESP8266 board package)
3. **ArduinoJson** (version 6.x) - Install via Library Manager

## Step 4: Board Configuration

In Arduino IDE:
- **Tools → Board → ESP8266 Boards → NodeMCU 1.0 (ESP-12E Module)**
- **Tools → Upload Speed → 115200**
- **Tools → Port → Select your ESP8266 COM port**

## Step 5: Upload Code

1. Open `esp8266_watering_system_v2.ino`
2. Ensure `config.h` is in the same folder
3. Click **Upload**
4. Wait for upload to complete

## Step 6: Testing and Monitoring

### Open Serial Monitor:
- **Tools → Serial Monitor**
- **Baud Rate: 115200**
- **Line Ending: Both NL & CR**

### Expected Output:
```
=== ESP8266 Watering System Starting ===
Version: 2.0
Device ID: ESP8266_WATERING_SYSTEM_01
API Send Interval: 300 seconds
Connecting to WiFi: YourWiFiName
........
WiFi connected successfully!
IP address: 192.168.1.xxx
Signal strength (RSSI): -45 dBm

=== MOISTURE SENSOR READINGS ===
Channel 0: 450
Channel 1: 520
Channel 2: 380
Channel 3: 600
Sensor 1: 450 (Pump: OFF)
Sensor 2: 520 (Pump: OFF)
Sensor 3: 380 (Pump: ON)
Sensor 4: 600 (Pump: ON)
```

## System Operation

### Automatic Watering Logic:
- **Moisture > Threshold (580)**: Pump activates
- **Moisture ≤ Threshold**: Pump deactivates
- **Reading Interval**: Every 1 second
- **API Data Send**: Every 5 minutes (when configured)

### Multiplexer Operation:
- Cycles through 4 sensors (channels 0-3)
- 100μs delay between channel switches
- Real-time moisture monitoring

## Troubleshooting

### Common Issues:

**WiFi Connection Problems:**
- Check WiFi credentials in `config.h`
- Verify signal strength (RSSI should be > -70 dBm)
- Check router settings

**Sensor Reading Issues:**
- Verify multiplexer connections
- Check sensor power supply
- Test individual sensors with multimeter
- Ensure proper grounding

**Pump Control Issues:**
- Verify relay connections
- Check pump power supply (12V)
- Test relays with LED indicator
- Ensure proper wiring polarity

**Multiplexer Issues:**
- Verify all 4 control pins (S0-S3)
- Check enable pin (E) is grounded
- Ensure 3.3V power supply
- Test with known good sensor

### Debug Mode:
Enable detailed output by setting `DEBUG_MODE true` in `config.h`

## API Integration (Future)

When your web application is ready:

1. **Update API Configuration:**
```cpp
#define API_URL "https://your-api.com/watering-data"
#define API_KEY "your-api-key-here"
```

2. **Data Format Sent:**
```json
{
  "device_id": "ESP8266_WATERING_SYSTEM_01",
  "timestamp": 1234567890,
  "wifi_rssi": -45,
  "uptime_seconds": 3600,
  "sensors": [
    {
      "id": 1,
      "moisture_value": 450,
      "pump_active": false,
      "threshold": 580
    }
  ]
}
```

## Maintenance

### Regular Checks:
- **Weekly**: Check sensor readings and pump operation
- **Monthly**: Clean moisture sensors
- **Quarterly**: Check tubing and connections
- **As Needed**: Adjust moisture threshold based on plant needs

### Calibration:
- Test sensors in dry and wet soil
- Adjust `MOISTURE_THRESHOLD` based on readings
- Typical range: 300-700 (lower = drier)

## Safety Notes

- **Waterproof all electrical connections** near water
- **Use appropriate power supplies** for pumps
- **Install overflow protection** for water reservoir
- **Test system thoroughly** before leaving unattended
- **Monitor first few days** to ensure proper operation 