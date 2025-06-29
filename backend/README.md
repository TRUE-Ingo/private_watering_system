# Watering System Backend API

A simple Node.js API server to receive and store data from the ESP8266 watering system.

## Features

- ✅ Receive data from ESP8266 watering system
- ✅ Store sensor readings and pump status
- ✅ Track statistics (API calls, pump activations)
- ✅ RESTful API endpoints
- ✅ CORS enabled for frontend access
- ✅ Basic security with Helmet
- ✅ Request logging with Morgan
- ✅ JSON file storage (no database required)

## Quick Start

### 1. Install Dependencies

```bash
cd backend
npm install
```

### 2. Start the Server

```bash
# Development mode (with auto-restart)
npm run dev

# Production mode
npm start
```

The server will start on `http://localhost:3000`

### 3. Test the API

```bash
# Install axios for testing
npm install axios

# Run the test script
node test-api.js
```

## API Endpoints

### POST `/api/watering-data`
Receive data from ESP8266

**Request Body:**
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

**Response:**
```json
{
  "success": true,
  "message": "Data received successfully",
  "timestamp": "2024-01-01T12:00:00.000Z"
}
```

### GET `/api/health`
Health check endpoint

**Response:**
```json
{
  "status": "ok",
  "timestamp": "2024-01-01T12:00:00.000Z",
  "uptime": 3600
}
```

### GET `/api/current-status`
Get the latest sensor readings

**Response:**
```json
{
  "success": true,
  "current_status": {
    "device_id": "ESP8266_WATERING_SYSTEM_01",
    "sensors": [...],
    "server_timestamp": "2024-01-01T12:00:00.000Z"
  }
}
```

### GET `/api/stats`
Get system statistics

**Response:**
```json
{
  "success": true,
  "stats": {
    "total_api_calls": 100,
    "failed_api_calls": 2,
    "pump_activations": 25,
    "last_updated": "2024-01-01T12:00:00.000Z"
  }
}
```

### GET `/api/sensor-data`
Get recent sensor data (last 50 entries)

**Response:**
```json
{
  "success": true,
  "data": [...],
  "count": 50
}
```

## Data Storage

Data is stored in JSON files in the `data/` directory:

- `sensor_data.json` - All sensor readings (keeps last 1000 entries)
- `stats.json` - System statistics

## Configuration

### Environment Variables

- `PORT` - Server port (default: 3000)

### ESP8266 Configuration

Update your ESP8266 `config.h` file:

```cpp
#define API_URL "http://your-server.com/api/watering-data"
#define API_KEY ""  // Leave empty for now
```

## Deployment

### Railway (Recommended - Free Tier)

1. Create account at [railway.app](https://railway.app)
2. Connect your GitHub repository
3. Deploy the backend folder
4. Get your deployment URL (e.g., `https://your-app.railway.app`)
5. Update ESP8266 config with the new URL

**Free Tier Includes:**
- 500 hours/month
- 512MB RAM
- Shared CPU
- Automatic HTTPS

### Render (Free Tier)

1. Create account at [render.com](https://render.com)
2. Create new Web Service
3. Connect your repository
4. Set build command: `npm install`
5. Set start command: `npm start`
6. Get your deployment URL

**Free Tier Includes:**
- 750 hours/month
- 512MB RAM
- Shared CPU
- Automatic HTTPS

### Vercel (Free Tier)

1. Create account at [vercel.com](https://vercel.com)
2. Install Vercel CLI: `npm i -g vercel`
3. Deploy: `vercel --prod`
4. Get your deployment URL

**Free Tier Includes:**
- Unlimited deployments
- 100GB bandwidth/month
- Serverless functions
- Automatic HTTPS

### Local Development with ngrok

For testing with ESP8266 before deployment:

1. Install ngrok: [ngrok.com](https://ngrok.com)
2. Start your local server: `npm start`
3. Expose local server:
   ```bash
   ngrok http 3000
   ```
4. Use the ngrok URL in your ESP8266 config
5. Free tier: 1 tunnel, 40 connections/minute

### Deployment Comparison

| Platform | Free Tier | Pros | Cons |
|----------|-----------|------|------|
| **Railway** | 500h/month | Easy setup, good docs | Limited hours |
| **Render** | 750h/month | More hours, reliable | Slower cold starts |
| **Vercel** | Unlimited | Fast, great performance | Serverless only |
| **ngrok** | 1 tunnel | Perfect for testing | Not for production |

## Testing

The `test-api.js` script sends sample data to test all endpoints:

```bash
node test-api.js
```

## Troubleshooting

### Common Issues

**Port already in use:**
```bash
# Find process using port 3000
lsof -i :3000
# Kill the process
kill -9 <PID>
```

**CORS errors:**
- CORS is enabled by default
- If issues persist, check frontend URL configuration

**Data not saving:**
- Check file permissions in `data/` directory
- Ensure disk space is available

### Logs

The server logs all requests and errors to console. Check for:
- Incoming data from ESP8266
- API call statistics
- Error messages

## Next Steps

1. ✅ Backend API (Complete)
2. 🔄 Frontend Dashboard (Next)
3. 📱 PWA Features
4. 🔔 Push Notifications
5. 🔍 Advanced Analytics 