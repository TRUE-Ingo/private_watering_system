const fetch = require('node-fetch');

const API_BASE = 'http://localhost:3000/api';

async function testThresholdUpdates() {
  console.log('Testing threshold update functionality...');
  
  try {
    // Test 1: Update threshold for sensor 1
    console.log('\n1. Testing threshold update for sensor 1...');
    const updateResponse = await fetch(`${API_BASE}/thresholds/1`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ threshold: 650 })
    });
    
    if (updateResponse.ok) {
      const updateResult = await updateResponse.json();
      console.log('✅ Threshold update successful:', updateResult);
    } else {
      const error = await updateResponse.json();
      console.log('❌ Threshold update failed:', error);
    }
    
    // Test 2: Check if threshold update is stored
    console.log('\n2. Checking stored threshold updates...');
    const updatesResponse = await fetch(`${API_BASE}/threshold-updates`);
    
    if (updatesResponse.ok) {
      const updatesResult = await updatesResponse.json();
      console.log('✅ Threshold updates retrieved:', updatesResult);
    } else {
      const error = await updatesResponse.json();
      console.log('❌ Failed to get threshold updates:', error);
    }
    
    // Test 3: Clear threshold updates
    console.log('\n3. Clearing threshold updates...');
    const clearResponse = await fetch(`${API_BASE}/clear-threshold-updates`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      }
    });
    
    if (clearResponse.ok) {
      console.log('✅ Threshold updates cleared successfully');
    } else {
      const error = await clearResponse.json();
      console.log('❌ Failed to clear threshold updates:', error);
    }
    
    // Test 4: Verify updates are cleared
    console.log('\n4. Verifying updates are cleared...');
    const verifyResponse = await fetch(`${API_BASE}/threshold-updates`);
    
    if (verifyResponse.ok) {
      const verifyResult = await verifyResponse.json();
      console.log('✅ Verification result:', verifyResult);
    } else {
      const error = await verifyResponse.json();
      console.log('❌ Verification failed:', error);
    }
    
  } catch (error) {
    console.error('❌ Test failed with error:', error);
  }
}

// Run the test
testThresholdUpdates(); 