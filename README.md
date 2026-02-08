# 🏎️ Forza Horizon 5 Telemetry Server

A high-performance C++ application that captures real-time telemetry data from Forza Horizon 5 via UDP sockets and visualizes it through a dual-interface system: a Console Dashboard and a Mobile Web Interface.

## 🚀 Features

* **Real-Time Data Acquisition:** Parses UDP packets from the Forza game engine (Car Dash format).
* **Memory Hacking:** Reads raw byte offsets to bypass struct alignment issues for Pedals and Gear.
* **Multithreaded Architecture:** * **Thread A:** UDP Listener & Data Processing.
    * **Thread B:** TCP HTTP Server for mobile devices.
* **Professional Console UI:** Custom-built Text User Interface (TUI) using Windows API.
* **Mobile Dashboard (IoT):** Hosts a local web server allowing any phone on the WiFi to act as a digital dashboard.
* **Shift Light Logic:** Mobile screen flashes red when RPM > 90%.

## 🛠️ Tech Stack

* **Language:** C++ (Standard 17)
* **Networking:** Winsock2 (UDP & TCP/IP)
* **Concurrency:** `std::thread`, `std::atomic`
* **Frontend:** HTML5, CSS3, JavaScript (Fetch API)

## ⚙️ How it works

1.  **The Game** sends telemetry data to `127.0.0.1:5300` (UDP).
2.  **The C++ Backend** captures the packet, calculates physics, and reads raw memory bytes for Throttle/Brake.
3.  **The Console** renders the data locally.
4.  **The HTTP Server** serves a JSON API (`/data`) to the phone, updating the UI at 30Hz.
