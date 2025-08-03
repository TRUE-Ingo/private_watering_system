const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');
const fs = require('fs').promises;
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3000;
const NODE_ENV = process.env.NODE_ENV || 'development';

// In-memory storage variables (kept for compatibility but not used)
let inMemorySensorData = [];
let inMemoryHistoricalData = []; // Store historical sensor data
let inMemoryPumpActivity = []; // Store pump activity periods
let inMemoryStats = {
  total_api_calls: 0,
  failed_api_calls: 0,
  pump_activations: 0,
  daily_pump_runtime_1: 0,
  daily_pump_runtime_2: 0,
  daily_pump_runtime_3: 0,
  daily_pump_runtime_4: 0,
  max_daily_pump_runtime: 600000, // 10 minutes in milliseconds
  threshold_updates: {},
  last_updated: new Date().toISOString()
};

// Use file-based storage for persistence (even in production)
const useInMemoryStorage = false;

// Data storage path (used for persistence)
const DATA_DIR = path.join(__dirname, 'data');
const SENSOR_DATA_FILE = path.join(DATA_DIR, 'sensor_data.json');
const STATS_FILE = path.join(DATA_DIR, 'stats.json');
const HISTORICAL_DATA_FILE = path.join(DATA_DIR, 'historical_data.json');
const PUMP_ACTIVITY_FILE = path.join(DATA_DIR, 'pump_activity.json');

// Middleware
app.use(helmet({
  contentSecurityPolicy: {
    directives: {
      defaultSrc: ["'self'"],
      scriptSrc: ["'self'", "'unsafe-inline'", "https://cdn.jsdelivr.net"],
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

// Serve graphs page
app.get('/graphs', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'graphs.html'));
});

// Ensure data directory exists (for persistence)
async function ensureDataDir() {
  try {
    await fs.access(DATA_DIR);
  } catch {
    await fs.mkdir(DATA_DIR, { recursive: true });
  }
}

// Initialize data files if they don't exist (for persistence)
async function initializeDataFiles() {
  
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
      daily_pump_runtime_1: 0,
      daily_pump_runtime_2: 0,
      daily_pump_runtime_3: 0,
      daily_pump_runtime_4: 0,
      max_daily_pump_runtime: 600000, // 10 minutes in milliseconds
      last_updated: new Date().toISOString()
    };
    await fs.writeFile(STATS_FILE, JSON.stringify(initialStats, null, 2));
  }
  
  try {
    await fs.access(HISTORICAL_DATA_FILE);
  } catch {
    await fs.writeFile(HISTORICAL_DATA_FILE, JSON.stringify([], null, 2));
  }
  
  try {
    await fs.access(PUMP_ACTIVITY_FILE);
  } catch {
    await fs.writeFile(PUMP_ACTIVITY_FILE, JSON.stringify([], null, 2));
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
      
      // Store historical data for graphs
      const historicalEntry = {
        timestamp: data.server_timestamp,
        sensors: data.sensors.map(sensor => ({
          id: sensor.id,
          moisture_value: sensor.moisture_value,
          threshold: sensor.threshold,
          pump_active: sensor.pump_active
        }))
      };
      inMemoryHistoricalData.push(historicalEntry);
      
      // Keep only last 7 days of historical data (assuming data every 30 seconds = 2880 entries per day)
      const maxHistoricalEntries = 7 * 2880; // 7 days worth of data
      if (inMemoryHistoricalData.length > maxHistoricalEntries) {
        inMemoryHistoricalData = inMemoryHistoricalData.slice(-maxHistoricalEntries);
      }
      
      // Track pump activity periods
      data.sensors.forEach(sensor => {
        if (sensor.pump_active) {
          // Check if we need to start a new pump activity period
          const existingActivity = inMemoryPumpActivity.find(activity => 
            activity.pump_id === sensor.id && !activity.end_time
          );
          
          if (!existingActivity) {
            // Start new pump activity
            inMemoryPumpActivity.push({
              pump_id: sensor.id,
              start_time: data.server_timestamp,
              end_time: null
            });
          }
        } else {
          // End any existing pump activity for this sensor
          const existingActivity = inMemoryPumpActivity.find(activity => 
            activity.pump_id === sensor.id && !activity.end_time
          );
          
          if (existingActivity) {
            existingActivity.end_time = data.server_timestamp;
          }
        }
      });
      
      // Clean up old pump activity records (older than 7 days)
      const sevenDaysAgo = new Date(Date.now() - 7 * 24 * 60 * 60 * 1000);
      inMemoryPumpActivity = inMemoryPumpActivity.filter(activity => 
        new Date(activity.start_time) > sevenDaysAgo
      );
      
      inMemoryStats.total_api_calls++;
      inMemoryStats.last_updated = new Date().toISOString();
      
      // Count pump activations
      const activePumps = data.sensors.filter(sensor => sensor.pump_active).length;
      inMemoryStats.pump_activations += activePumps;
      
      // Update individual daily pump runtime if provided
      if (data.daily_pump_runtime_1 !== undefined) {
        inMemoryStats.daily_pump_runtime_1 = data.daily_pump_runtime_1;
      }
      if (data.daily_pump_runtime_2 !== undefined) {
        inMemoryStats.daily_pump_runtime_2 = data.daily_pump_runtime_2;
      }
      if (data.daily_pump_runtime_3 !== undefined) {
        inMemoryStats.daily_pump_runtime_3 = data.daily_pump_runtime_3;
      }
      if (data.daily_pump_runtime_4 !== undefined) {
        inMemoryStats.daily_pump_runtime_4 = data.daily_pump_runtime_4;
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
      
      // Store historical data for graphs
      let historicalData = [];
      try {
        const historicalDataContent = await fs.readFile(HISTORICAL_DATA_FILE, 'utf8');
        historicalData = JSON.parse(historicalDataContent);
      } catch (error) {
        historicalData = [];
      }
      
      const historicalEntry = {
        timestamp: data.server_timestamp,
        sensors: data.sensors.map(sensor => ({
          id: sensor.id,
          moisture_value: sensor.moisture_value,
          threshold: sensor.threshold,
          pump_active: sensor.pump_active
        }))
      };
      historicalData.push(historicalEntry);
      
      // Keep only last 7 days of historical data
      const maxHistoricalEntries = 7 * 2880; // 7 days worth of data
      if (historicalData.length > maxHistoricalEntries) {
        historicalData = historicalData.slice(-maxHistoricalEntries);
      }
      
      await fs.writeFile(HISTORICAL_DATA_FILE, JSON.stringify(historicalData, null, 2));
      
      // Track pump activity periods
      let pumpActivity = [];
      try {
        const pumpActivityContent = await fs.readFile(PUMP_ACTIVITY_FILE, 'utf8');
        pumpActivity = JSON.parse(pumpActivityContent);
      } catch (error) {
        pumpActivity = [];
      }
      
      data.sensors.forEach(sensor => {
        if (sensor.pump_active) {
          // Check if we need to start a new pump activity period
          const existingActivity = pumpActivity.find(activity => 
            activity.pump_id === sensor.id && !activity.end_time
          );
          
          if (!existingActivity) {
            // Start new pump activity
            pumpActivity.push({
              pump_id: sensor.id,
              start_time: data.server_timestamp,
              end_time: null
            });
          }
        } else {
          // End any existing pump activity for this sensor
          const existingActivity = pumpActivity.find(activity => 
            activity.pump_id === sensor.id && !activity.end_time
          );
          
          if (existingActivity) {
            existingActivity.end_time = data.server_timestamp;
          }
        }
      });
      
      // Clean up old pump activity records (older than 7 days)
      const sevenDaysAgo = new Date(Date.now() - 7 * 24 * 60 * 60 * 1000);
      pumpActivity = pumpActivity.filter(activity => 
        new Date(activity.start_time) > sevenDaysAgo
      );
      
      await fs.writeFile(PUMP_ACTIVITY_FILE, JSON.stringify(pumpActivity, null, 2));
      
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
      
      // Update individual daily pump runtime if provided
      if (data.daily_pump_runtime_1 !== undefined) {
        stats.daily_pump_runtime_1 = data.daily_pump_runtime_1;
      }
      if (data.daily_pump_runtime_2 !== undefined) {
        stats.daily_pump_runtime_2 = data.daily_pump_runtime_2;
      }
      if (data.daily_pump_runtime_3 !== undefined) {
        stats.daily_pump_runtime_3 = data.daily_pump_runtime_3;
      }
      if (data.daily_pump_runtime_4 !== undefined) {
        stats.daily_pump_runtime_4 = data.daily_pump_runtime_4;
      }
      if (data.max_daily_pump_runtime !== undefined) {
        stats.max_daily_pump_runtime = data.max_daily_pump_runtime;
      }
      
      await fs.writeFile(STATS_FILE, JSON.stringify(stats, null, 2));
    }
    
    // Data received from ESP8266
    
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
    
    // Store threshold update for ESP8266 to fetch
    if (useInMemoryStorage) {
      // Store in memory
      if (!inMemoryStats.threshold_updates) {
        inMemoryStats.threshold_updates = {};
      }
      inMemoryStats.threshold_updates[sensorId] = {
        threshold: threshold,
        timestamp: new Date().toISOString()
      };
      
      // Also update the latest sensor data so the web app shows the new threshold
      if (inMemorySensorData.length > 0) {
        const latestData = inMemorySensorData[inMemorySensorData.length - 1];
        if (latestData.sensors && Array.isArray(latestData.sensors)) {
          const sensor = latestData.sensors.find(s => s.id === sensorId);
          if (sensor) {
            sensor.threshold = threshold;
          }
        }
      }
    } else {
      // Store in file
      try {
        const statsData = await fs.readFile(STATS_FILE, 'utf8');
        const stats = JSON.parse(statsData);
        if (!stats.threshold_updates) {
          stats.threshold_updates = {};
        }
        stats.threshold_updates[sensorId] = {
          threshold: threshold,
          timestamp: new Date().toISOString()
        };
        await fs.writeFile(STATS_FILE, JSON.stringify(stats, null, 2));
        
        // Also update the sensor data file
        try {
          const sensorData = await fs.readFile(SENSOR_DATA_FILE, 'utf8');
          const sensorDataArray = JSON.parse(sensorData);
          
          if (sensorDataArray.length > 0) {
            const latestData = sensorDataArray[sensorDataArray.length - 1];
            
            if (latestData.sensors && Array.isArray(latestData.sensors)) {
              const sensor = latestData.sensors.find(s => s.id === sensorId);
              
              if (sensor) {
                sensor.threshold = threshold;
                await fs.writeFile(SENSOR_DATA_FILE, JSON.stringify(sensorDataArray, null, 2));
              }
            }
          }
        } catch (error) {
          console.error('Error updating sensor data file:', error);
        }
      } catch (error) {
        console.error('Error storing threshold update:', error);
      }
    }
    

    
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

// Get pending threshold updates for ESP8266
app.get('/api/threshold-updates', async (req, res) => {
  try {
    let thresholdUpdates = {};
    
    if (useInMemoryStorage) {
      thresholdUpdates = inMemoryStats.threshold_updates || {};
    } else {
      try {
        const statsData = await fs.readFile(STATS_FILE, 'utf8');
        const stats = JSON.parse(statsData);
        thresholdUpdates = stats.threshold_updates || {};
      } catch (error) {
        thresholdUpdates = {};
      }
    }
    
    res.json({
      success: true,
      threshold_updates: thresholdUpdates,
      timestamp: new Date().toISOString()
    });
    
  } catch (error) {
    console.error('Error reading threshold updates:', error);
    res.status(500).json({ 
      error: 'Failed to read threshold updates' 
    });
  }
});

// Clear threshold updates after ESP8266 has processed them
app.post('/api/clear-threshold-updates', async (req, res) => {
  try {
    if (useInMemoryStorage) {
      inMemoryStats.threshold_updates = {};
    } else {
      try {
        const statsData = await fs.readFile(STATS_FILE, 'utf8');
        const stats = JSON.parse(statsData);
        stats.threshold_updates = {};
        await fs.writeFile(STATS_FILE, JSON.stringify(stats, null, 2));
      } catch (error) {
        console.error('Error clearing threshold updates:', error);
      }
    }
    
    res.json({
      success: true,
      message: 'Threshold updates cleared'
    });
    
  } catch (error) {
    console.error('Error clearing threshold updates:', error);
    res.status(500).json({ 
      error: 'Failed to clear threshold updates' 
    });
  }
});

// Get historical data for graphs
app.get('/api/historical-data', async (req, res) => {
  try {
    const { timeRange = '24h', sensors } = req.query;
    const requestedSensors = sensors ? sensors.split(',').map(s => parseInt(s)) : [1, 2, 3, 4];
    
    // Calculate time range
    const now = new Date();
    let startTime;
    
    switch (timeRange) {
      case '1h':
        startTime = new Date(now.getTime() - 60 * 60 * 1000);
        break;
      case '6h':
        startTime = new Date(now.getTime() - 6 * 60 * 60 * 1000);
        break;
      case '24h':
        startTime = new Date(now.getTime() - 24 * 60 * 60 * 1000);
        break;
      case '7d':
        startTime = new Date(now.getTime() - 7 * 24 * 60 * 60 * 1000);
        break;
      case '30d':
        startTime = new Date(now.getTime() - 30 * 24 * 60 * 60 * 1000);
        break;
      default:
        startTime = new Date(now.getTime() - 24 * 60 * 60 * 1000);
    }
    
    let historicalData = [];
    let pumpActivity = [];
    
    if (useInMemoryStorage) {
      historicalData = inMemoryHistoricalData;
      pumpActivity = inMemoryPumpActivity;
    } else {
      // Read historical data from file
      try {
        const historicalDataContent = await fs.readFile(HISTORICAL_DATA_FILE, 'utf8');
        historicalData = JSON.parse(historicalDataContent);
      } catch (error) {
        historicalData = [];
      }
      
      // Read pump activity from file
      try {
        const pumpActivityContent = await fs.readFile(PUMP_ACTIVITY_FILE, 'utf8');
        pumpActivity = JSON.parse(pumpActivityContent);
      } catch (error) {
        pumpActivity = [];
      }
    }
    
    // Filter data by time range
    const filteredHistoricalData = historicalData.filter(entry => 
      new Date(entry.timestamp) >= startTime
    );
    
    const filteredPumpActivity = pumpActivity.filter(activity => 
      new Date(activity.start_time) >= startTime
    );
    
    // Organize sensor data by sensor ID
    const sensorData = {};
    requestedSensors.forEach(sensorId => {
      sensorData[sensorId] = [];
    });
    
    filteredHistoricalData.forEach(entry => {
      entry.sensors.forEach(sensor => {
        if (requestedSensors.includes(sensor.id)) {
          sensorData[sensor.id].push({
            timestamp: entry.timestamp,
            moisture_value: sensor.moisture_value,
            threshold: sensor.threshold,
            pump_active: sensor.pump_active
          });
        }
      });
    });
    
    // Filter pump activity by requested sensors
    const filteredPumpActivityBySensor = filteredPumpActivity.filter(activity =>
      requestedSensors.includes(activity.pump_id)
    );
    
    res.json({
      success: true,
      data: {
        sensors: sensorData,
        pump_activity: filteredPumpActivityBySensor,
        time_range: timeRange,
        start_time: startTime.toISOString(),
        end_time: now.toISOString()
      }
    });
    
  } catch (error) {
    console.error('Error reading historical data:', error);
    res.status(500).json({ 
      error: 'Failed to read historical data' 
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