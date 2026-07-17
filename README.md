# Vital Monitoring Smart Glove

An IoT-based wearable device designed to monitor real-time health vitals (such as heart rate, SpO2, and body temperature) and transmit the data to a web dashboard for continuous tracking.

---

## 🚀 Features

* **Real-time Vitals Tracking:** Monitors pulse rate, blood oxygen levels ($SpO_2$), and temperature.
* **IoT Connectivity:** Built using Arduino/ESP to process sensor data and transmit it via MQTT/HTTP.
* **Web Dashboard:** Responsive frontend built with React and Vite to view data analytics and live streams.
* **Backend Support:** Node.js backend to handle data storage, user authentication, and API requests.


## Screenshots:
<img width="1920" height="1080" alt="Screenshot 2026-03-10 153317" src="https://github.com/user-attachments/assets/717f4561-bfd2-4f42-8e58-74e008723ba4" />

---

## 🛠️ Tech Stack

* **Hardware / Firmware:** C++, Arduino IDE
* **Frontend:** React, Vite, CSS3
* **Backend:** Node.js, JavaScript

---

## 📂 Project Structure

```text
├── Arduino Codes/       # Firmware files for the microcontroller hardware
├── backend/             # Node.js backend server files
├── src/                 # React frontend source files
├── public/              # Static assets for the frontend
├── smartGlove.ino       # Main Arduino sketch file
