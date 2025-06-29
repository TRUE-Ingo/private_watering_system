const axios = require('axios');

const API_BASE_URL = 'https://private-watering-system.onrender.com/api';

// Sample data that matches ESP8266 format
const sampleData = {
  device_id: "ESP8266_WATERING_SYSTEM_01",
  timestamp: Date.now(),
  wifi_rssi: -45,
  uptime_seconds: 3600,
  total_api_calls: 10,
  failed_api_calls: 1,
  pump_activations: 5,
  sensors: [
    {
      id: 1,
      moisture_value: 450,
      pump_active: false,
      threshold: 580
    },
    {
      id: 2,
      moisture_value: 520,
      pump_active: false,
      threshold: 580
    },
    {
      id: 3,
      moisture_value: 380,
      pump_active: true,
      threshold: 580
    },
    {
      id: 4,
      moisture_value: 600,
      pump_active: true,
      threshold: 580
    }
  ]
};

async function testAPI() {
  console.log('🧪 Testing Watering System API...\n');

  try {
    // Test 1: Health check
    console.log('1. Testing health check...');
    const healthResponse = await axios.get(`${API_BASE_URL}/health`);
    console.log('✅ Health check passed:', healthResponse.data);
    console.log('');

    // Test 2: Send sample data
    console.log('2. Sending sample watering data...');
    const dataResponse = await axios.post(`${API_BASE_URL}/watering-data`, sampleData);
    console.log('✅ Data sent successfully:', dataResponse.data);
    console.log('');

    // Test 3: Get current status
    console.log('3. Getting current status...');
    const statusResponse = await axios.get(`${API_BASE_URL}/current-status`);
    console.log('✅ Current status retrieved:', statusResponse.data);
    console.log('');

    // Test 4: Get statistics
    console.log('4. Getting statistics...');
    const statsResponse = await axios.get(`${API_BASE_URL}/stats`);
    console.log('✅ Statistics retrieved:', statsResponse.data);
    console.log('');

    // Test 5: Get sensor data
    console.log('5. Getting sensor data...');
    const sensorResponse = await axios.get(`${API_BASE_URL}/sensor-data`);
    console.log('✅ Sensor data retrieved. Count:', sensorResponse.data.count);
    console.log('');

    console.log('🎉 All tests passed! API is working correctly.');
    console.log('\n📊 API Endpoints:');
    console.log(`   Health: ${API_BASE_URL}/health`);
    console.log(`   Send Data: ${API_BASE_URL}/watering-data (POST)`);
    console.log(`   Current Status: ${API_BASE_URL}/current-status`);
    console.log(`   Statistics: ${API_BASE_URL}/stats`);
    console.log(`   Sensor Data: ${API_BASE_URL}/sensor-data`);

  } catch (error) {
    console.error('❌ Test failed:', error.message);
    if (error.response) {
      console.error('Response data:', error.response.data);
      console.error('Status:', error.response.status);
    }
  }
}

// Run the test
testAPI(); 