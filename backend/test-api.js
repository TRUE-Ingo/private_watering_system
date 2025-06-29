const fetch = require('node-fetch');

const BASE_URL = 'https://private-watering-system.onrender.com';

async function testAPI() {
    console.log('🧪 Testing API endpoints...\n');

    try {
        // Test 1: Health check
        console.log('1. Testing health endpoint...');
        const healthResponse = await fetch(`${BASE_URL}/api/health`);
        const healthData = await healthResponse.json();
        console.log('Health check result:', healthData);
        console.log('✅ Health check passed\n');

        // Test 2: Send test data
        console.log('2. Sending test data...');
        const testData = {
            device_id: "ESP8266_WATERING_SYSTEM_01",
            timestamp: Date.now(),
            wifi_rssi: -45,
            uptime_seconds: 3600,
            total_api_calls: 1,
            failed_api_calls: 0,
            pump_activations: 2,
            water_level_low: false,
            water_level_critical: false,
            daily_pump_activations: 5,
            max_daily_activations: 50,
            sensors: [
                {
                    id: 1,
                    moisture_value: 850,
                    pump_active: true,
                    threshold: 800
                },
                {
                    id: 2,
                    moisture_value: 750,
                    pump_active: false,
                    threshold: 800
                },
                {
                    id: 3,
                    moisture_value: 900,
                    pump_active: true,
                    threshold: 800
                },
                {
                    id: 4,
                    moisture_value: 600,
                    pump_active: false,
                    threshold: 800
                }
            ]
        };

        const dataResponse = await fetch(`${BASE_URL}/api/watering-data`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(testData)
        });

        const dataResult = await dataResponse.json();
        console.log('Data send result:', dataResult);
        console.log('✅ Test data sent successfully\n');

        // Test 3: Get current status
        console.log('3. Getting current status...');
        const statusResponse = await fetch(`${BASE_URL}/api/current-status`);
        const statusData = await statusResponse.json();
        console.log('Current status:', JSON.stringify(statusData, null, 2));
        console.log('✅ Current status retrieved\n');

        // Test 4: Get stats
        console.log('4. Getting stats...');
        const statsResponse = await fetch(`${BASE_URL}/api/stats`);
        const statsData = await statsResponse.json();
        console.log('Stats:', JSON.stringify(statsData, null, 2));
        console.log('✅ Stats retrieved\n');

        // Test 5: Get sensor data
        console.log('5. Getting sensor data...');
        const sensorResponse = await fetch(`${BASE_URL}/api/sensor-data`);
        const sensorData = await sensorResponse.json();
        console.log('Sensor data count:', sensorData.count);
        console.log('Latest sensor data:', JSON.stringify(sensorData.data[sensorData.data.length - 1], null, 2));
        console.log('✅ Sensor data retrieved\n');

        console.log('🎉 All tests completed successfully!');

    } catch (error) {
        console.error('❌ Test failed:', error.message);
        console.error('Full error:', error);
    }
}

// Run the test
testAPI(); 