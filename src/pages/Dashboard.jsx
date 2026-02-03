import { useState } from "react";
import SoldierVitals from "../components/SoldierVitals";
import { useNavigate } from "react-router-dom";

export default function Dashboard() {
  const soldiers = [
    { name: "Soldier_1" }, // Matches 'soldierId' in ESP32 code and server.js
  ];


  const navigate = useNavigate();

  return (
    <div className="dashboard">
      {soldiers.map((s) => (
        <SoldierVitals
          key={s.name}
          soldierName={s.name}
        />
      ))}
    </div>
  );
}
