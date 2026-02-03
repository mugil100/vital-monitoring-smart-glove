# Soldier Health Monitoring System - Project Overview

## 1. Core Concept
This project is an **IoT-based Health Monitoring Dashboard** designed to track soldiers in real-time. It aggregates live sensor data (heart rate, temperature, SpO₂, GPS location) from **ThingSpeak**, displays it on a React-based dashboard, and persistently logs history into **Firebase Firestore**.

## 2. Complete User Flow

### Step 1: Authentication
- **Entry Point:** `src/pages/Login.jsx` (`/` route)
- **Action:** User enters email and password.
- **Logic:** Authenticates using `firebase/auth` (`signInWithEmailAndPassword`).
- **Outcome:**
  - **Success:** Redirects to `/dashboard`.
  - **Failure:** Displays an alert with the error message.

### Step 2: The Dashboard
- **Entry Point:** `src/pages/Dashboard.jsx` (`/dashboard` route)
- **Structure:**
  - **Notification Panel:** Displays global alerts for any soldier in critical condition.
  - **Soldier Cards:** A grid of `SoldierVitals` components, one for each tracked soldier.
- **Data Source:** Hardcoded list of soldiers in `Dashboard.jsx`, each with a unique **ThingSpeak Channel ID** and **Read API Key**.

### Step 3: Real-Time Monitoring (Per Soldier)
- **Component:** `src/components/SoldierVitals.jsx`
- **Cycle (Every 10 Seconds):**
  1.  **Fetch:** Calls the ThingSpeak API to get the latest sensor feed.
  2.  **Store:** Writes the fetched data point into the `soldierVitals` collection in **Firebase Firestore**.
  3.  **Analyze:** Checks values against safety thresholds:
      - Heart Rate > 120 bpm
      - SpO₂ < 90%
      - Temperature > 39°C
      - Fall Detection (`field7 == "1"`)
      - Emergency Status (`field8 == "1"`)
  4.  **Visualize:**
      - **Live Stats:** Numerical display with conditional red highlighting for dangerous values.
      - **History Chart:** Uses `Recharts` to plot the last 10 data points fetched from Firestore.
      - **Live Map:** Uses `React-Leaflet` to render markers on an OpenStreetMap based on lat/long.

### Step 4: Centralized Alerting
- **Component:** `src/components/NotificationPanel.jsx` (Powered by logic in `Dashboard.jsx`)
- **Cycle (Every 5 Seconds):**
  - Queries Firestore for the latest record of every soldier.
  - specific check to identifying "critical" soldiers.
  - Updates the top notification bar if any critical states are found.

## 3. Data Flow Architecture

```mermaid
graph TD
    User((Soldier/IoT)) -->|Uploads Sensor Data| ThingSpeak(ThingSpeak cloud)
    Dashboard[React Dashboard] -->|1. Polls API (10s)| ThingSpeak
    Dashboard -->|2. Logs Data| Firestore(Firebase Firestore)
    Dashboard -->|3. Reads History| Firestore
    Dashboard --Auth--> FBAuth(Firebase Auth)

    subgraph "Visual Components"
    Stats[Live Vitals]
    Chart[History Line Chart]
    Map[GPS Location Map]
    end
    
    Dashboard --> Stats
    Dashboard --> Chart
    Dashboard --> Map
```

## 4. Key Technologies
- **Frontend:** React, Vite
- **Routing:** React Router DOM
- **State Management:** Local State (useState, useEffect)
- **Database:** Firebase Firestore (NoSQL)
- **Authentication:** Firebase Auth
- **Data Visualization:** Recharts (Graphs), React-Leaflet (Maps)
- **IoT Integration:** ThingSpeak REST API
