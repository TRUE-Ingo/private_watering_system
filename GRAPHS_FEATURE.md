# 📈 Historical Graphs Feature

## Overview

The watering system now includes a comprehensive historical data visualization feature that displays humidity levels and pump activity over time. This feature allows users to analyze trends, monitor system performance, and understand watering patterns.

## Features

### 📊 Interactive Charts
- **Real-time humidity tracking** for all 4 sensors
- **Pump activity visualization** showing when each pump was active
- **Time range selection** (1 hour, 6 hours, 24 hours, 7 days, 30 days)
- **Sensor selection** - choose which sensors to display
- **Responsive design** that works on desktop and mobile devices

### 🎨 Visual Elements
- **Color-coded sensors**: Each sensor has a distinct color
  - Sensor 1: Green (#4CAF50)
  - Sensor 2: Blue (#2196F3)
  - Sensor 3: Orange (#FF9800)
  - Sensor 4: Purple (#9C27B0)
- **Pump activity indicators**: Yellow background boxes show when pumps were active
- **Interactive tooltips**: Hover over data points to see detailed information
- **Progress bars**: Visual representation of moisture levels (0-1023 scale)

### 📱 User Interface
- **Navigation**: Easy access from main dashboard via "View Historical Graphs" button
- **Controls**: Time range dropdown and sensor checkboxes
- **Auto-refresh**: Data updates every 5 minutes automatically
- **Manual refresh**: "Refresh" button for immediate updates

## Technical Implementation

### Backend Changes

#### New API Endpoints
- `GET /api/historical-data` - Retrieves historical sensor data and pump activity
- `GET /graphs` - Serves the graphs HTML page

#### Data Storage
- **Historical sensor data**: Stored with timestamps for the last 7 days
- **Pump activity tracking**: Records start/end times for each pump activation
- **Automatic cleanup**: Old data is automatically removed to prevent storage bloat

#### File Structure
```
backend/
├── public/
│   ├── index.html          # Main dashboard
│   └── graphs.html         # New graphs page
├── data/
│   ├── historical_data.json # Historical sensor readings
│   └── pump_activity.json  # Pump activity periods
└── server.js               # Updated with new endpoints
```

### Frontend Changes

#### New Dependencies
- **Chart.js**: For creating interactive charts
- **Chart.js Date Adapter**: For proper time axis handling
- **Chart.js Annotation Plugin**: For pump activity overlays

#### Key Features
- **Time-based charts**: X-axis shows time, Y-axis shows moisture values
- **Pump activity overlays**: Background boxes indicate when pumps were running
- **Responsive design**: Charts adapt to different screen sizes
- **Real-time updates**: Data refreshes automatically

## Usage

### Accessing the Graphs
1. Open the main watering system dashboard
2. Click the "📈 View Historical Graphs" button in the header
3. Or navigate directly to `/graphs`

### Using the Controls
1. **Time Range**: Select from dropdown (1h, 6h, 24h, 7d, 30d)
2. **Sensors**: Check/uncheck sensors to show/hide them
3. **Load Data**: Click "📈 Load Data" to refresh with current settings

### Interpreting the Data
- **Moisture Values**: 0-1023 scale (0 = very wet, 1023 = very dry)
- **Percentage Display**: Y-axis shows moisture as percentage
- **Pump Activity**: Yellow background indicates pump was running
- **Trends**: Look for patterns in moisture levels and watering frequency

## Data Management

### Storage Limits
- **Historical data**: Last 7 days (approximately 20,160 data points)
- **Pump activity**: Last 7 days of pump activation records
- **Automatic cleanup**: Old data is removed to maintain performance

### Performance Considerations
- **Memory efficient**: Data is stored in chunks and cleaned regularly
- **Fast queries**: Time-based filtering for quick data retrieval
- **Scalable**: Can handle multiple concurrent users

## API Reference

### Historical Data Endpoint
```
GET /api/historical-data?timeRange=24h&sensors=1,2,3,4
```

**Parameters:**
- `timeRange` (optional): 1h, 6h, 24h, 7d, 30d (default: 24h)
- `sensors` (optional): Comma-separated sensor IDs (default: 1,2,3,4)

**Response:**
```json
{
  "success": true,
  "data": {
    "sensors": {
      "1": [
        {
          "timestamp": "2025-08-02T06:00:00.000Z",
          "moisture_value": 450,
          "threshold": 600,
          "pump_active": false
        }
      ]
    },
    "pump_activity": [
      {
        "pump_id": 1,
        "start_time": "2025-08-02T05:30:00.000Z",
        "end_time": "2025-08-02T05:35:00.000Z"
      }
    ],
    "time_range": "24h",
    "start_time": "2025-08-02T06:00:00.000Z",
    "end_time": "2025-08-02T06:00:00.000Z"
  }
}
```

## Future Enhancements

### Potential Improvements
- **Export functionality**: Download data as CSV/JSON
- **Statistical analysis**: Average moisture levels, watering frequency
- **Alert thresholds**: Visual indicators for unusual patterns
- **Multiple chart types**: Bar charts, heat maps, etc.
- **Custom time ranges**: User-defined start/end dates
- **Data comparison**: Compare different time periods

### Performance Optimizations
- **Data aggregation**: Store hourly/daily averages for long periods
- **Caching**: Cache frequently requested data
- **Compression**: Compress historical data files
- **Database migration**: Move to proper database for larger datasets

## Troubleshooting

### Common Issues
1. **No data showing**: Check if ESP8266 is sending data regularly
2. **Charts not loading**: Verify Chart.js dependencies are loaded
3. **Slow performance**: Check data file sizes and cleanup settings
4. **Missing pump activity**: Ensure pump status is being tracked correctly

### Debug Information
- Check browser console for JavaScript errors
- Verify API endpoints return expected data
- Monitor server logs for backend issues
- Test with different time ranges and sensor combinations

## Testing

The feature includes comprehensive testing:
- **API endpoint tests**: Verify data retrieval and filtering
- **Frontend tests**: Check chart rendering and interactions
- **Integration tests**: End-to-end functionality verification
- **Performance tests**: Ensure responsive behavior with large datasets

Run tests with:
```bash
cd backend
node test-historical-data.js
```

## Conclusion

The historical graphs feature provides valuable insights into the watering system's performance and helps users optimize their watering schedules. The interactive charts make it easy to identify patterns, troubleshoot issues, and ensure optimal plant health. 