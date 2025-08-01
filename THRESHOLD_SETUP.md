# Individual Sensor Threshold Adjustment

This update adds the ability to adjust watering thresholds for each sensor individually through the web interface.

## Features Added

### 1. Arduino Code Changes (`arduino_Watering4.ino`)
- **EEPROM Storage**: Individual thresholds are stored in EEPROM for persistence
- **Serial Commands**: Added commands to adjust thresholds via serial monitor
- **Individual Control**: Each sensor now uses its own threshold value

#### Serial Commands:
- `set_threshold <sensor> <value>` - Set threshold for sensor (1-4)
- `get_thresholds` - Show current thresholds
- `help` - Show available commands

#### Example:
```
set_threshold 1 650
set_threshold 2 580
get_thresholds
```

### 2. ESP8266 Code Changes (`esp8266_watering_system_v2/`)
- **Individual Thresholds**: Updated config.h to support separate thresholds per sensor
- **API Integration**: Modified to send individual thresholds in API calls

### 3. Backend API Changes (`backend/server.js`)
- **GET /api/thresholds** - Retrieve current thresholds for all sensors
- **POST /api/thresholds/:sensorId** - Update threshold for specific sensor

#### API Examples:
```bash
# Get current thresholds
curl http://localhost:3000/api/thresholds

# Update sensor 1 threshold to 650
curl -X POST http://localhost:3000/api/thresholds/1 \
  -H "Content-Type: application/json" \
  -d '{"threshold": 650}'
```

### 4. Web Interface Changes (`backend/public/index.html`)
- **Threshold Controls**: Added UI section for adjusting thresholds
- **Real-time Updates**: Threshold values update automatically
- **Validation**: Input validation for threshold values (0-1023)
- **Success Feedback**: Visual confirmation when thresholds are updated

## How to Use

### 1. Upload Updated Arduino Code
1. Open `arduino_Watering4.ino` in Arduino IDE
2. Upload to your Arduino board
3. Open Serial Monitor to see threshold commands

### 2. Update ESP8266 Code (if using ESP8266)
1. Update `esp8266_watering_system_v2/config.h` with individual thresholds
2. Upload the updated code to your ESP8266

### 3. Start the Backend Server
```bash
cd backend
npm install
npm start
```

### 4. Access the Web Interface
1. Open your browser to `http://localhost:3000`
2. Navigate to the "Threshold Settings" section
3. Adjust thresholds for each sensor as needed

## Threshold Values

- **Range**: 0-1023 (analog sensor range)
- **Higher Values**: Trigger watering when soil is drier
- **Lower Values**: Trigger watering when soil is wetter
- **Default**: 600 (middle range)

## Technical Details

### EEPROM Storage
- Thresholds are stored in Arduino EEPROM for persistence
- Addresses: 0, 4, 8, 12 (4 bytes each)
- Validation ensures values are within valid range

### API Response Format
```json
{
  "success": true,
  "thresholds": {
    "1": 600,
    "2": 650,
    "3": 580,
    "4": 620
  },
  "timestamp": "2024-01-01T12:00:00.000Z"
}
```

### Error Handling
- Invalid threshold values (outside 0-1023 range)
- Invalid sensor IDs (must be 1-4)
- Network errors and timeouts
- EEPROM corruption recovery

## Testing

Run the test script to verify API functionality:
```bash
cd backend
node test-thresholds.js
```

## Troubleshooting

### Common Issues:
1. **Thresholds not persisting**: Check EEPROM usage and Arduino memory
2. **API not responding**: Verify server is running and ports are correct
3. **Web interface not updating**: Check browser console for JavaScript errors
4. **Invalid threshold values**: Ensure values are between 0-1023

### Debug Commands:
- Use Arduino Serial Monitor to check current thresholds
- Check browser developer tools for API errors
- Verify network connectivity between devices

## Future Enhancements

- **Database Storage**: Store thresholds in database instead of EEPROM
- **Real-time Updates**: Push threshold changes to Arduino via WiFi
- **Threshold History**: Track threshold changes over time
- **Plant Profiles**: Save threshold sets for different plant types
- **Auto-adjustment**: Machine learning to optimize thresholds based on plant health 