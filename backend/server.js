const express = require('express');
const admin = require('firebase-admin');
const cors = require('cors');
const rateLimit = require('express-rate-limit');
const fs = require('fs');
require('dotenv').config();

const app = express();
const PORT = process.env.PORT || 5000;

// --- Middleware ---
app.use(cors());
app.use(express.json());

// --- Rate Limiting ---
const limiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 1000, // Limit each IP to 100 requests per 15 mins (adjust as needed for ESP32 frequency)
  message: "Too many requests from this IP, please try again later."
});
app.use('/api', limiter);

// --- Firebase Admin Setup ---
const serviceAccountPath = './serviceAccountKey.json';

if (fs.existsSync(serviceAccountPath)) {
  const serviceAccount = require(serviceAccountPath);

  // Check if the file is still a placeholder
  if (serviceAccount.project_id === "REPLACE_WITH_YOUR_PROJECT_ID") {
    console.error("⚠️  ERROR: serviceAccountKey.json is still a placeholder!");
    console.error("👉  Please download your Firebase Admin SDK key and save it as 'serviceAccountKey.json' in the backend folder.");
    process.exit(1);
  }

  admin.initializeApp({
    credential: admin.credential.cert(serviceAccount)
  });
  console.log("🔥 Firebase Admin Initialized Successfully");
} else {
  console.error("⚠️  ERROR: serviceAccountKey.json not found!");
  console.error("👉  Please place your Firebase Admin SDK key in the backend folder.");
  process.exit(1);
}

const db = admin.firestore();

// --- Routes ---

// GET / - Health Check
app.get('/', (req, res) => {
  res.send('Smart Glove Backend is Running 🚀');
});

// POST /api/vitals - Receive Data from ESP32
app.post('/api/vitals', async (req, res) => {
  try {
    const { soldierId, hr, spo2, temp, fall, lat, lng, sos, accX, accY, accZ } = req.body;

    // 1. Validation (Basic)
    if (!soldierId) {
      return res.status(400).json({ error: "Missing soldierId" });
    }

    // 2. Determine Status
    let alertStatus = "Normal";
    let emergencyStatus = "None";

    // Normalize inputs to avoid NaN
    const heartRate = hr !== undefined ? Number(hr) : 0;
    const spo2Val = spo2 !== undefined ? Number(spo2) : 0;
    const tempVal = temp !== undefined ? Number(temp) : 0;
    const latitude = lat !== undefined ? Number(lat) : 0;
    const longitude = lng !== undefined ? Number(lng) : 0;
    const ax = accX !== undefined ? Number(accX) : 0;
    const ay = accY !== undefined ? Number(accY) : 0;
    const az = accZ !== undefined ? Number(accZ) : 0;
    const isFall = (fall === 1 || fall === true);
    const isSos = (sos === 1 || sos === true);

    if (isSos) {
      emergencyStatus = "Active";
      alertStatus = "SOS EMERGENCY";
    } else if (isFall) {
      alertStatus = "Fall Detected";
    } else if (heartRate > 120 || spo2Val < 90 || tempVal > 39) {
      alertStatus = "Abnormal Vitals";
    }

    const timestamp = admin.firestore.FieldValue.serverTimestamp();

    const dataPayload = {
      soldierName: soldierId,
      heartRate: heartRate,
      spo2: spo2Val,
      temperature: tempVal,
      latitude: latitude,
      longitude: longitude,
      accX: ax,
      accY: ay,
      accZ: az,
      fallDetection: isFall,
      emergencyStatus: emergencyStatus,
      alertStatus: alertStatus,
      timestamp: timestamp
    };

    // 3. Write to Firestore 'soldierVitals' collection
    await db.collection('soldierVitals').add(dataPayload);

    // Optional: Update a 'currentStatus' collection for faster "latest state" lookups
    // await db.collection('soldierCurrentStatus').doc(soldierId).set(dataPayload);

    console.log(`[${new Date().toISOString()}] Received: Soldier=${soldierId} | Acc X=${ax.toFixed(2)} Y=${ay.toFixed(2)} Z=${az.toFixed(2)} | Fall=${isFall} Temp=${tempVal} HR=${heartRate} SpO2=${spo2Val} Lat=${latitude} Lng=${longitude} SOS=${isSos}`);

    res.status(200).json({ status: "success", message: "Data logged" });

  } catch (error) {
    console.error("Error writing to Firestore:", error);
    res.status(500).json({ error: "Internal Server Error" });
  }
});

app.get("/")

// --- Start Server ---
app.listen(PORT, '0.0.0.0', () => {
  console.log(`✅ Server running on http://0.0.0.0:${PORT}`);
});
