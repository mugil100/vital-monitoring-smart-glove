import { useState, useEffect } from "react";
import { db } from "../firebaseConfig";
import {
  collection,
  query,
  orderBy,
  limit,
  getDocs,
  where
} from "firebase/firestore";
import "./SoldierVitals.css";

export default function SoldierVitals({ soldierName }) {
  const [history, setHistory] = useState([]);
  const [error, setError] = useState(null);

  useEffect(() => {
    if (!soldierName) return;

    const fetchData = async () => {
      try {
        const q = query(
          collection(db, "soldierVitals"),
          // where("soldierName", "==", soldierName), // Removed to avoid index requirement
          orderBy("timestamp", "desc"),
          limit(50)
        );

        const snapshot = await getDocs(q);
        const data = [];
        snapshot.forEach((doc) => {
          const d = doc.data();
          // Client-side filtering to match soldierName
          if (d.soldierName === soldierName) {
            data.push({
              id: doc.id,
              time: d.timestamp ? d.timestamp.toDate().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }) : "N/A",
              hr: d.heartRate !== undefined ? Number(d.heartRate) : 0,
              spo2: d.spo2 !== undefined ? Number(d.spo2) : 0,
              temp: d.temperature !== undefined ? Number(d.temperature) : 0,
              accX: d.accX !== undefined ? Number(d.accX) : 0,
              accY: d.accY !== undefined ? Number(d.accY) : 0,
              accZ: d.accZ !== undefined ? Number(d.accZ) : 0,
              fall: d.fallDetection ? "YES" : "No",
            });
          }
        });
        // Since we query 50 mixed records, we might get fewer than 20 for this soldier. 
        // We slice to safeguard the view.
        setHistory(data.slice(0, 20));
        setError(null);
      } catch (err) {
        console.error("Firestore Error:", err);
        if (err.code === 'permission-denied') {
          setError("PERMISSION DENIED: Go to Firebase Console > Firestore > Rules and allow read/write.");
        } else {
          setError(`Error: ${err.message}`);
        }
      }
    };

    // Initial fetch
    fetchData();

    // Poll every 2 seconds
    const intervalId = setInterval(fetchData, 2000);

    return () => clearInterval(intervalId);
  }, [soldierName]);

  return (
    <div className="data-log-container">
      {error && (
        <div style={{ backgroundColor: '#fee2e2', color: '#b91c1c', padding: '1rem', borderRadius: '8px', marginBottom: '1rem', border: '1px solid #fecaca' }}>
          <strong>CRITICAL ERROR:</strong> {error}
        </div>
      )}
      <div className="log-header">
        <h3>📊 Soldier Vitals Log: {soldierName}</h3>
        <span className="live-badge">● Live</span>
      </div>

      <div className="table-responsive">
        <table className="log-table">
          <thead>
            <tr>
              <th>Time</th>
              <th>HR (bpm)</th>
              <th>SpO2 (%)</th>
              <th>Temp (°C)</th>
              <th>Acc X (g)</th>
              <th>Acc Y (g)</th>
              <th>Acc Z (g)</th>
              <th>Status</th>
            </tr>
          </thead>
          <tbody>
            {history.length === 0 ? (
              <tr><td colSpan="8" style={{ textAlign: 'center', padding: '2rem' }}>Waiting for data...</td></tr>
            ) : (
              history.map((row) => (
                <tr key={row.id} className={row.fall === "YES" ? "row-alert" : ""}>
                  <td>{row.time}</td>
                  <td>{row.hr}</td>
                  <td>{row.spo2}</td>
                  <td>{row.temp.toFixed(1)}</td>
                  <td>{row.accX.toFixed(2)}</td>
                  <td>{row.accY.toFixed(2)}</td>
                  <td>{row.accZ.toFixed(2)}</td>
                  <td>
                    {row.fall === "YES" ? (
                      <span className="badge-danger">FALL DETECTED</span>
                    ) : (
                      <span className="badge-normal">Normal</span>
                    )}
                  </td>
                </tr>
              ))
            )}
          </tbody>
        </table>
      </div>
    </div>
  );
}
