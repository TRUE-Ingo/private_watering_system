const fetch = require('node-fetch');

const API_BASE = 'http://localhost:3000/api';

async function testThresholds() {
    console.log('Testing threshold API endpoints...\n');
    
    try {
        // Test 1: Get current thresholds
        console.log('1. Testing GET /api/thresholds');
        const getResponse = await fetch(`${API_BASE}/thresholds`);
        const getData = await getResponse.json();
        console.log('Response:', getData);
        console.log('Status:', getResponse.status);
        console.log('');
        
        // Test 2: Update threshold for sensor 1
        console.log('2. Testing POST /api/thresholds/1');
        const updateResponse = await fetch(`${API_BASE}/thresholds/1`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ threshold: 650 })
        });
        const updateData = await updateResponse.json();
        console.log('Response:', updateData);
        console.log('Status:', updateResponse.status);
        console.log('');
        
        // Test 3: Test invalid threshold value
        console.log('3. Testing invalid threshold value');
        const invalidResponse = await fetch(`${API_BASE}/thresholds/2`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ threshold: 1500 })
        });
        const invalidData = await invalidResponse.json();
        console.log('Response:', invalidData);
        console.log('Status:', invalidResponse.status);
        console.log('');
        
        // Test 4: Test invalid sensor ID
        console.log('4. Testing invalid sensor ID');
        const invalidSensorResponse = await fetch(`${API_BASE}/thresholds/5`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ threshold: 600 })
        });
        const invalidSensorData = await invalidSensorResponse.json();
        console.log('Response:', invalidSensorData);
        console.log('Status:', invalidSensorResponse.status);
        console.log('');
        
        console.log('All tests completed!');
        
    } catch (error) {
        console.error('Test failed:', error);
    }
}

testThresholds(); 