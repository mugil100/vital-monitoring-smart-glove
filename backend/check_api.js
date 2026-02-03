const http = require('http');

const data = JSON.stringify({
    soldierId: "Soldier_1",
    hr: 85,
    spo2: 98,
    temp: 37.2,
    fall: 0,
    lat: 12.9716,
    lng: 80.24,
    sos: 0
});

const options = {
    hostname: 'localhost',
    port: 5000,
    path: '/api/vitals',
    method: 'POST',
    headers: {
        'Content-Type': 'application/json',
        'Content-Length': data.length
    }
};

const req = http.request(options, (res) => {
    console.log(`STATUS: ${res.statusCode}`);

    res.on('data', (chunk) => {
        console.log(`BODY: ${chunk}`);
    });
});

req.on('error', (e) => {
    console.error(`problem with request: ${e.message}`);
});

// Write data to request body
req.write(data);
req.end();
