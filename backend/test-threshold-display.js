const fetch = require('node-fetch');

const API_BASE = 'http://localhost:3000/api';

async function testThresholdDisplay() {
  console.log('Testing threshold display functionality...');
  
  try {
    // Test 1: Get current thresholds
    console.log('\n1. Getting current thresholds...');
    const thresholdsResponse = await fetch(`${API_BASE}/thresholds`);
    
    if (thresholdsResponse.ok) {
      const thresholdsResult = await thresholdsResponse.json();
      console.log('✅ Current thresholds:', thresholdsResult);
    } else {
      const error = await thresholdsResponse.json();
      console.log('❌ Failed to get thresholds:', error);
    }
    
    // Test 2: Update threshold for sensor 1
    console.log('\n2. Updating threshold for sensor 1 to 700...');
    const updateResponse = await fetch(`${API_BASE}/thresholds/1`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ threshold: 700 })
    });
    
    if (updateResponse.ok) {
      const updateResult = await updateResponse.json();
      console.log('✅ Threshold update successful:', updateResult);
    } else {
      const error = await updateResponse.json();
      console.log('❌ Threshold update failed:', error);
    }
    
    // Test 3: Get thresholds again to see if they're updated
    console.log('\n3. Getting thresholds again to verify update...');
    const thresholdsResponse2 = await fetch(`${API_BASE}/thresholds`);
    
    if (thresholdsResponse2.ok) {
      const thresholdsResult2 = await thresholdsResponse2.json();
      console.log('✅ Updated thresholds:', thresholdsResult2);
      
      // Check if sensor 1 threshold was updated
      if (thresholdsResult2.thresholds && thresholdsResult2.thresholds['1'] === 700) {
        console.log('✅ Sensor 1 threshold successfully updated to 700');
      } else {
        console.log('❌ Sensor 1 threshold was not updated correctly');
      }
    } else {
      const error = await thresholdsResponse2.json();
      console.log('❌ Failed to get updated thresholds:', error);
    }
    
  } catch (error) {
    console.error('❌ Test failed with error:', error);
  }
}

// Run the test
testThresholdDisplay(); 