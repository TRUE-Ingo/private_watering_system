const fetch = require('node-fetch');

const BASE_URL = 'http://localhost:3000';

async function testHistoricalData() {
    console.log('🧪 Testing Historical Data Endpoint...\n');
    
    try {
        // Test 1: Basic historical data request
        console.log('📊 Test 1: Basic historical data request (24h)');
        const response1 = await fetch(`${BASE_URL}/api/historical-data?timeRange=24h&sensors=1,2`);
        
        if (response1.ok) {
            const data1 = await response1.json();
            console.log('✅ Success!');
            console.log(`   Time range: ${data1.data.time_range}`);
            console.log(`   Sensors requested: 1,2`);
            console.log(`   Data points for sensor 1: ${data1.data.sensors[1]?.length || 0}`);
            console.log(`   Data points for sensor 2: ${data1.data.sensors[2]?.length || 0}`);
            console.log(`   Pump activity records: ${data1.data.pump_activity?.length || 0}`);
        } else {
            console.log('❌ Failed:', response1.status, response1.statusText);
        }
        
        console.log('\n' + '='.repeat(50) + '\n');
        
        // Test 2: Different time range
        console.log('📊 Test 2: 1 hour time range');
        const response2 = await fetch(`${BASE_URL}/api/historical-data?timeRange=1h&sensors=1,2,3,4`);
        
        if (response2.ok) {
            const data2 = await response2.json();
            console.log('✅ Success!');
            console.log(`   Time range: ${data2.data.time_range}`);
            console.log(`   Start time: ${data2.data.start_time}`);
            console.log(`   End time: ${data2.data.end_time}`);
        } else {
            console.log('❌ Failed:', response2.status, response2.statusText);
        }
        
        console.log('\n' + '='.repeat(50) + '\n');
        
        // Test 3: Health check
        console.log('📊 Test 3: Health check');
        const healthResponse = await fetch(`${BASE_URL}/api/health`);
        
        if (healthResponse.ok) {
            const healthData = await healthResponse.json();
            console.log('✅ Server is healthy!');
            console.log(`   Status: ${healthData.status}`);
            console.log(`   Uptime: ${healthData.uptime}s`);
        } else {
            console.log('❌ Health check failed:', healthResponse.status);
        }
        
        console.log('\n' + '='.repeat(50) + '\n');
        
        // Test 4: Graphs page accessibility
        console.log('📊 Test 4: Graphs page accessibility');
        const graphsResponse = await fetch(`${BASE_URL}/graphs`);
        
        if (graphsResponse.ok) {
            console.log('✅ Graphs page is accessible!');
            console.log(`   Status: ${graphsResponse.status}`);
            console.log(`   Content-Type: ${graphsResponse.headers.get('content-type')}`);
        } else {
            console.log('❌ Graphs page failed:', graphsResponse.status);
        }
        
    } catch (error) {
        console.error('❌ Test failed with error:', error.message);
    }
}

// Run the tests
testHistoricalData(); 