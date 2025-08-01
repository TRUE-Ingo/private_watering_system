const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');
const fs = require('fs').promises;
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3000;
const NODE_ENV = process.env.NODE_ENV || 'development';

// In-memory storage for Render free tier (ephemeral storage)
let inMemorySensorData = [];
let inMemoryStats = {
  total_api_calls: 0,
  failed_api_calls: 0,
  pump_activations: 0,
  daily_pump_runtime: 0,
  max_daily_pump_runtime: 600000, // 10 minutes in milliseconds
  last_updated: new Date().toISOString()
};

// Use in-memory storage on Render free tier
const useInMemoryStorage = NODE_ENV === 'production';

// Data storage path (only used in development)
const DATA_DIR = path.join(__dirname, 'data');
const SENSOR_DATA_FILE = path.join(DATA_DIR, 'sensor_data.json');
const STATS_FILE = path.join(DATA_DIR, 'stats.json');

// Middleware
app.use(helmet({
  contentSecurityPolicy: {
    directives: {
      defaultSrc: ["'self'"],
      scriptSrc: ["'self'", "'unsafe-inline'"],
      styleSrc: ["'self'", "'unsafe-inline'"],
      imgSrc: ["'self'", "data:", "https:"],
      connectSrc: ["'self'"],
      fontSrc: ["'self'", "https:", "data:"],
      objectSrc: ["'none'"],
      mediaSrc: ["'self'"],
      frameSrc: ["'none'"],
    },
  },
})); // Security headers
app.use(cors()); // Enable CORS for frontend
app.use(morgan('combined')); // Logging
app.use(express.json({ limit: '1mb' })); // Parse JSON bodies

// Serve static files from public directory
app.use(express.static(path.join(__dirname, 'public')));

// Serve dashboard at root
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// Ensure data directory exists (only in development)
async function ensureDataDir() {
  if (useInMemoryStorage) return;
  
  try {
    await fs.access(DATA_DIR);
  } catch {
    await fs.mkdir(DATA_DIR, { recursive: true });
  }
}

// Initialize data files if they don't exist (only in development)
async function initializeDataFiles() {
  if (useInMemoryStorage) return;
  
  try {
    await fs.access(SENSOR_DATA_FILE);
  } catch {
    await fs.writeFile(SENSOR_DATA_FILE, JSON.stringify([], null, 2));
  }
  
  try {
    await fs.access(STATS_FILE);
  } catch {
          const initialStats = {
        total_api_calls: 0,
        failed_api_calls: 0,
        pump_activations: 0,
        daily_pump_runtime: 0,
        max_daily_pump_runtime: 600000, // 10 minutes in milliseconds
        last_updated: new Date().toISOString()
      };
    await fs.writeFile(STATS_FILE, JSON.stringify(initialStats, null, 2));
  }
}

// API Routes

// Health check endpoint
app.get('/api/health', (req, res) => {
  res.json({ 
    status: 'ok', 
    timestamp: new Date().toISOString(),
    uptime: process.uptime()
  });
});

// Receive data from ESP8266
app.post('/api/watering-data', async (req, res) => {
  try {
    const data = req.body;
    
    // Basic validation
    if (!data.device_id || !data.sensors || !Array.isArray(data.sensors)) {
      return res.status(400).json({ 
        error: 'Invalid data format. Required: device_id, sensors array' 
      });
    }
    
    // Add timestamp if not provided
    if (!data.timestamp) {
      data.timestamp = Date.now();
    }
    
    // Add server timestamp
    data.server_timestamp = new Date().toISOString();
    
    if (useInMemoryStorage) {
      // Use in-memory storage for Render free tier
      inMemorySensorData.push(data);
      if (inMemorySensorData.length > 1000) {
        inMemorySensorData = inMemorySensorData.slice(-1000);
      }
      
      inMemoryStats.total_api_calls++;
      inMemoryStats.last_updated = new Date().toISOString();
      
      // Count pump activations
      const activePumps = data.sensors.filter(sensor => sensor.pump_active).length;
      inMemoryStats.pump_activations += activePumps;
      
      // Update daily pump runtime if provided
      if (data.daily_pump_runtime !== undefined) {
        inMemoryStats.daily_pump_runtime = data.daily_pump_runtime;
      }
      if (data.max_daily_pump_runtime !== undefined) {
        inMemoryStats.max_daily_pump_runtime = data.max_daily_pump_runtime;
      }
    } else {
      // Use file storage for development
      // Read existing data
      let sensorData = [];
      try {
        const existingData = await fs.readFile(SENSOR_DATA_FILE, 'utf8');
        sensorData = JSON.parse(existingData);
      } catch (error) {
        // File doesn't exist or is empty, start with empty array
        sensorData = [];
      }
      
      // Add new data (keep last 1000 entries)
      sensorData.push(data);
      if (sensorData.length > 1000) {
        sensorData = sensorData.slice(-1000);
      }
      
      // Save updated data
      await fs.writeFile(SENSOR_DATA_FILE, JSON.stringify(sensorData, null, 2));
      
      // Update statistics
      let stats = {
        total_api_calls: 0,
        failed_api_calls: 0,
        pump_activations: 0,
        last_updated: new Date().toISOString()
      };
      
      try {
        const statsData = await fs.readFile(STATS_FILE, 'utf8');
        stats = JSON.parse(statsData);
      } catch (error) {
        // Use default stats
      }
      
      stats.total_api_calls++;
      stats.last_updated = new Date().toISOString();
      
      // Count pump activations
      const activePumps = data.sensors.filter(sensor => sensor.pump_active).length;
      stats.pump_activations += activePumps;
      
      // Update daily pump runtime if provided
      if (data.daily_pump_runtime !== undefined) {
        stats.daily_pump_runtime = data.daily_pump_runtime;
      }
      if (data.max_daily_pump_runtime !== undefined) {
        stats.max_daily_pump_runtime = data.max_daily_pump_runtime;
      }
      
      await fs.writeFile(STATS_FILE, JSON.stringify(stats, null, 2));
    }
    
    // Log the received data
    console.log(`Received data from ${data.device_id}:`, {
      timestamp: data.server_timestamp,
      sensors: data.sensors.length,
      active_pumps: data.sensors.filter(sensor => sensor.pump_active).length,
      wifi_rssi: data.wifi_rssi,
      storage: useInMemoryStorage ? 'memory' : 'file'
    });
    
    res.json({ 
      success: true, 
      message: 'Data received successfully',
      timestamp: data.server_timestamp
    });
    
  } catch (error) {
    console.error('Error processing watering data:', error);
    
    // Update failed API calls count
    if (useInMemoryStorage) {
      inMemoryStats.failed_api_calls++;
      inMemoryStats.last_updated = new Date().toISOString();
    } else {
      try {
        const statsData = await fs.readFile(STATS_FILE, 'utf8');
        const stats = JSON.parse(statsData);
        stats.failed_api_calls++;
        stats.last_updated = new Date().toISOString();
        await fs.writeFile(STATS_FILE, JSON.stringify(stats, null, 2));
      } catch (statsError) {
        console.error('Error updating stats:', statsError);
      }
    }
    
    res.status(500).json({ 
      error: 'Internal server error',
      message: 'Failed to process data'
    });
  }
});

// Get latest sensor data
app.get('/api/sensor-data', async (req, res) => {
  try {
    let sensorData = [];
    
    if (useInMemoryStorage) {
      sensorData = inMemorySensorData;
    } else {
      const data = await fs.readFile(SENSOR_DATA_FILE, 'utf8');
      sensorData = JSON.parse(data);
    }
    
    // Return last 50 entries
    const recentData = sensorData.slice(-50);
    
    res.json({
      success: true,
      data: recentData,
      count: recentData.length
    });
    
  } catch (error) {
    console.error('Error reading sensor data:', error);
    res.status(500).json({ 
      error: 'Failed to read sensor data' 
    });
  }
});

// Get statistics
app.get('/api/stats', async (req, res) => {
  try {
    let stats;
    
    if (useInMemoryStorage) {
      stats = inMemoryStats;
    } else {
      const data = await fs.readFile(STATS_FILE, 'utf8');
      stats = JSON.parse(data);
    }
    
    res.json({
      success: true,
      stats: stats
    });
    
  } catch (error) {
    console.error('Error reading stats:', error);
    res.status(500).json({ 
      error: 'Failed to read statistics' 
    });
  }
});

// Get latest reading for each sensor
app.get('/api/current-status', async (req, res) => {
  try {
    let sensorData = [];
    
    if (useInMemoryStorage) {
      sensorData = inMemorySensorData;
    } else {
      const data = await fs.readFile(SENSOR_DATA_FILE, 'utf8');
      sensorData = JSON.parse(data);
    }
    
    if (sensorData.length === 0) {
      return res.json({
        success: true,
        current_status: null,
        message: 'No data available'
      });
    }
    
    const latestReading = sensorData[sensorData.length - 1];
    
    res.json({
      success: true,
      current_status: latestReading,
      timestamp: latestReading.server_timestamp
    });
    
  } catch (error) {
    console.error('Error reading current status:', error);
    res.status(500).json({ 
      error: 'Failed to read current status' 
    });
  }
});

// Get current thresholds for all sensors
app.get('/api/thresholds', async (req, res) => {
  try {
    let sensorData = [];
    
    if (useInMemoryStorage) {
      sensorData = inMemorySensorData;
    } else {
      const data = await fs.readFile(SENSOR_DATA_FILE, 'utf8');
      sensorData = JSON.parse(data);
    }
    
    if (sensorData.length === 0) {
      return res.json({
        success: true,
        thresholds: {
          1: 600,
          2: 600,
          3: 600,
          4: 600
        },
        message: 'No data available, using default thresholds'
      });
    }
    
    const latestReading = sensorData[sensorData.length - 1];
    const thresholds = {};
    
    if (latestReading.sensors && Array.isArray(latestReading.sensors)) {
      latestReading.sensors.forEach(sensor => {
        thresholds[sensor.id] = sensor.threshold || 600;
      });
    }
    
    res.json({
      success: true,
      thresholds: thresholds,
      timestamp: latestReading.server_timestamp
    });
    
  } catch (error) {
    console.error('Error reading thresholds:', error);
    res.status(500).json({ 
      error: 'Failed to read thresholds' 
    });
  }
});

// Update threshold for a specific sensor
app.post('/api/thresholds/:sensorId', async (req, res) => {
  try {
    const sensorId = parseInt(req.params.sensorId);
    const { threshold } = req.body;
    
    if (!threshold || typeof threshold !== 'number' || threshold < 0 || threshold > 1023) {
      return res.status(400).json({
        error: 'Invalid threshold value. Must be a number between 0 and 1023'
      });
    }
    
    if (sensorId < 1 || sensorId > 4) {
      return res.status(400).json({
        error: 'Invalid sensor ID. Must be 1-4'
      });
    }
    
    // For now, we'll just acknowledge the request
    // In a real implementation, you might want to send this to the Arduino
    // or store it in a database for the Arduino to fetch
    
    console.log(`Threshold update request: Sensor ${sensorId} -> ${threshold}`);
    
    res.json({
      success: true,
      message: `Threshold for sensor ${sensorId} updated to ${threshold}`,
      sensor_id: sensorId,
      threshold: threshold,
      timestamp: new Date().toISOString()
    });
    
  } catch (error) {
    console.error('Error updating threshold:', error);
    res.status(500).json({ 
      error: 'Failed to update threshold' 
    });
  }
});

// Error handling middleware
app.use((err, req, res, next) => {
  console.error('Unhandled error:', err);
  res.status(500).json({ 
    error: 'Internal server error' 
  });
});

// 404 handler
app.use((req, res) => {
  res.status(404).json({ 
    error: 'Endpoint not found' 
  });
});

// Start server
async function startServer() {
  try {
    if (useInMemoryStorage) {
      console.log('🚀 Starting server with in-memory storage (Render production mode)');
    } else {
      await ensureDataDir();
      await initializeDataFiles();
      console.log('🚀 Starting server with file storage (development mode)');
    }
    
    app.listen(PORT, () => {
      console.log(`🚀 Watering System API Server running on port ${PORT}`);
      console.log(`📊 Health check: http://localhost:${PORT}/api/health`);
      console.log(`📡 Data endpoint: http://localhost:${PORT}/api/watering-data`);
      console.log(`📈 Stats endpoint: http://localhost:${PORT}/api/stats`);
      console.log(`💾 Storage mode: ${useInMemoryStorage ? 'In-memory' : 'File-based'}`);
    });
    
  } catch (error) {
    console.error('Failed to start server:', error);
    process.exit(1);
  }
}

startServer(); 