# Quick Setup Guide - ESP8266 Watering System

## Step 1: Update WiFi Credentials

Edit `config.h` and replace the WiFi credentials with your actual network details:

```cpp
#define WIFI_SSID "YourActualWiFiName"
#define WIFI_PASSWORD "YourActualWiFiPassword"
```

## Step 2: Install Required Libraries

In Arduino IDE, install these libraries:
1. **ESP8266WiFi** (usually included with ESP8266 board package)
2. **ESP8266HTTPClient** (usually included with ESP8266 board package)
3. **ArduinoJson** (version 6.x) - Install via Library Manager

## Step 3: Select Board

In Arduino IDE:
- Tools → Board → ESP8266 Boards → NodeMCU 1.0 (ESP-12E Module)
- Or select your specific ESP8266 board

## Step 4: Upload Code

1. Open `esp8266_watering_system_v2.ino`
2. Make sure `config.h` is in the same folder
3. Click Upload

## Step 5: Monitor Serial Output

Open Serial Monitor (115200 baud) to see:
- WiFi connection status
- Sensor readings
- Pump activation status
- API communication attempts

## Current Configuration

- **API Send Interval**: 5 minutes
- **Moisture Threshold**: 580 (adjust in config.h if needed)
- **Debug Mode**: Enabled (shows detailed output)
- **API**: Placeholder URL (device will skip API calls until you update it)

## Hardware Connections

| Component | ESP8266 Pin | Description |
|-----------|-------------|-------------|
| Pump 1 Relay | D4 (GPIO2) | Controls pump 1 |
| Pump 2 Relay | D3 (GPIO0) | Controls pump 2 |
| Pump 3 Relay | D2 (GPIO4) | Controls pump 3 |
| Pump 4 Relay | D1 (GPIO5) | Controls pump 4 |
| Moisture Sensor | A0 | Analog moisture reading |

## Testing

1. **WiFi Connection**: Should connect automatically on startup
2. **Sensor Reading**: Check Serial Monitor for moisture values
3. **Pump Control**: Pumps activate when moisture > 580
4. **API Communication**: Currently skipped (placeholder URL)

## When Your Web App is Ready

1. Update `API_URL` in `config.h` with your actual endpoint
2. Add `API_KEY` if your API requires authentication
3. The device will automatically start sending data every 5 minutes

## Troubleshooting

- **WiFi Issues**: Check credentials and signal strength
- **Pump Not Working**: Verify relay connections and power supply
- **Sensor Issues**: Check wiring and power to moisture sensor
- **Compilation Errors**: Ensure all libraries are installed

## Next Steps

1. Test the basic watering functionality
2. Adjust moisture threshold if needed
3. Implement multiple sensors (multiplexer or digital sensors)
4. Create your web app API
5. Update API configuration when ready 