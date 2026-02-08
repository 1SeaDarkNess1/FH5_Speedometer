#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <thread>
#include <atomic>
#include <iomanip>

#include <winsock2.h>
#include <windows.h>

#pragma comment(lib,"ws2_32.lib")

#include "ForzaData.h"

using namespace std;

// --- GLOBALE (Shared Memory) ---
atomic<int> globalSpeed(0);
atomic<int> globalRPM(0);
atomic<int> globalMaxRPM(0);
atomic<int> globalGear(0);
atomic<int> globalThrottle(0);
atomic<int> globalBrake(0);

// [NOU] Variabile pentru G-Force (folosim float pentru precizie)
atomic<float> globalAccelX(0); // Stanga-Dreapta
atomic<float> globalAccelZ(0); // Fata-Spate

// --- UTILITARE CONSOLA ---
void HideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}

void GoToXY(int x, int y) {
    COORD c;
    c.X = x; c.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void SetColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// --- SERVER WEB (FULL COCKPIT HUD v2) ---
void StartWebServer() {
    SOCKET webSocket;
    struct sockaddr_in server, client;

    webSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (webSocket == INVALID_SOCKET) return;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    if (bind(webSocket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) return;
    listen(webSocket, 5);

    int c = sizeof(struct sockaddr_in);

    while (true) {
        SOCKET clientSocket = accept(webSocket, (struct sockaddr*)&client, &c);
        if (clientSocket == INVALID_SOCKET) continue;

        char request[4096];
        int bytesRead = recv(clientSocket, request, 4096, 0);
        if (bytesRead <= 0) { closesocket(clientSocket); continue; }
        string reqStr(request);

        if (reqStr.find("GET /data") != string::npos) {
            string jsonBody = "{";
            jsonBody += "\"speed\": " + to_string(globalSpeed) + ",";
            jsonBody += "\"rpm\": " + to_string(globalRPM) + ",";
            jsonBody += "\"maxRpm\": " + to_string(globalMaxRPM) + ",";
            jsonBody += "\"gear\": " + to_string(globalGear) + ",";
            jsonBody += "\"throttle\": " + to_string(globalThrottle) + ",";
            jsonBody += "\"brake\": " + to_string(globalBrake) + ",";
            jsonBody += "\"gX\": " + to_string(globalAccelX) + ",";
            jsonBody += "\"gZ\": " + to_string(globalAccelZ);
            jsonBody += "}";

            string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n" + jsonBody;
            send(clientSocket, response.c_str(), response.length(), 0);
        }
        else {
            string html = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
            html += "<html><head>";
            html += "<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=0'>";

            html += "<meta name='apple-mobile-web-app-capable' content='yes'>";
            html += "<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent'>";
            html += "<meta name='mobile-web-app-capable' content='yes'>";
            html += "<meta name='theme-color' content='#000000'>";

            html += "<link href='https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&display=swap' rel='stylesheet'>";

            html += "<style>";
            html += "body { background-color: #050505; color: #0ff; font-family: 'Orbitron', sans-serif; overflow: hidden; height: 100vh; margin: 0; display: flex; user-select: none; transition: background 0.1s; }";

            html += ".panel { height: 100%; display: flex; flex-direction: column; justify-content: center; align-items: center; position: relative; }";
            html += ".side { width: 25%; border: 1px solid #1a1a1a; background: radial-gradient(circle, #0a0a0a 0%, #000 100%); }";
            html += ".center { width: 50%; }";

            html += ".g-title { font-size: 12px; color: #555; position: absolute; top: 20px; letter-spacing: 2px; }";
            html += ".radar-circle { width: 140px; height: 140px; border: 2px solid #0ff; border-radius: 50%; position: relative; background: rgba(0, 255, 255, 0.05); box-shadow: 0 0 10px rgba(0,255,255,0.2); overflow: hidden; }";
            html += ".radar-cross { position: absolute; width: 100%; height: 100%; }";
            html += ".radar-cross::before { content:''; position: absolute; top: 50%; left: 0; width: 100%; height: 1px; background: rgba(0,255,255,0.3); }";
            html += ".radar-cross::after { content:''; position: absolute; left: 50%; top: 0; height: 100%; width: 1px; background: rgba(0,255,255,0.3); }";
            html += ".g-dot { width: 12px; height: 12px; background: #ff0055; border-radius: 50%; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); box-shadow: 0 0 10px #ff0055; transition: all 0.05s linear; }";

            html += ".telemetry-box { width: 60%; height: 60%; display: flex; justify-content: space-around; align-items: flex-end; }";
            html += ".bar-col { width: 20px; height: 100%; background: #111; position: relative; border-radius: 4px; overflow: hidden; }";
            html += ".t-bar { width: 100%; position: absolute; bottom: 0; transition: height 0.05s; }";
            html += "#t-acc { background: #0f0; height: 0%; box-shadow: 0 0 10px #0f0; }";
            html += "#t-brk { background: #f00; height: 0%; box-shadow: 0 0 10px #f00; }";
            html += ".bar-label { position: absolute; bottom: -25px; width: 100%; text-align: center; font-size: 10px; color: #555; }";

            html += "svg { position: absolute; transform: rotate(135deg); width: 100%; height: 100%; overflow: visible; }";
            html += "circle { fill: none; stroke-width: 15; stroke-linecap: round; transition: stroke-dashoffset 0.1s linear; }";
            html += ".bg-circle { stroke: #1a1a1a; }";
            html += ".fg-circle { stroke: url(#gradient); stroke-dasharray: 754; stroke-dashoffset: 754; filter: drop-shadow(0 0 5px #0ff); }";

            html += ".data-cluster { text-align: center; z-index: 2; }";
            html += ".speed-val { font-size: 80px; font-weight: 900; color: #fff; text-shadow: 0 0 15px #0ff; line-height: 0.8; }";
            html += ".unit { font-size: 14px; letter-spacing: 2px; color: #0aa; margin-bottom: 10px; }";
            html += ".gear-val { font-size: 40px; color: #ff0055; font-weight: bold; text-shadow: 0 0 10px #ff0055; margin-top: 5px; }";

            // --- ANIMATIE NUCLEARA ---
            // Flash violent: Rosu -> Negru, cu bordura alba stralucitoare
            html += "@keyframes nuke {";
            html += "  0% { background-color: #000; box-shadow: inset 0 0 0 0 #f00; }";
            html += "  50% { background-color: #ff0000; box-shadow: inset 0 0 100px 50px #fff; }"; // Rosu aprins + Glow interior alb
            html += "  100% { background-color: #000; box-shadow: inset 0 0 0 0 #f00; }";
            html += "}";

            // Cand shift light e activ, schimbam TOTUL
            html += ".shifting { animation: nuke 0.08s infinite; }"; // Foarte rapid (12Hz)

            // Cand e rosu, facem textul si elementele ALBE/NEGRE pentru contrast
            html += ".shifting .speed-val, .shifting .gear-val { color: #fff !important; text-shadow: 0 0 20px #000 !important; }";
            html += ".shifting .unit, .shifting .g-title, .shifting .bar-label { color: #000 !important; font-weight: bold; }";
            html += ".shifting .side { border-color: #fff !important; background: none !important; }";
            html += ".shifting .radar-circle { border-color: #fff !important; }";

            html += "#overlay { position: absolute; top: 0; left: 0; width: 100%; height: 100%; background: #000; z-index: 999; display: flex; justify-content: center; align-items: center; flex-direction: column; }";
            html += "#start-btn { padding: 20px 40px; font-family: 'Orbitron'; font-size: 20px; color: #0ff; background: transparent; border: 2px solid #0ff; cursor: pointer; box-shadow: 0 0 20px #0ff; transition: 0.2s; }";
            html += "#start-btn:active { background: #0ff; color: #000; }";
            html += ".warning { color: #555; margin-top: 20px; font-size: 12px; }";

            html += "</style></head><body id='body'>";

            html += "<div id='overlay'>";
            html += "  <button id='start-btn' onclick='goFullscreen()'>INITIALIZE DASHBOARD</button>";
            html += "  <div class='warning'>TAP TO ENGAGE FULLSCREEN MODE</div>";
            html += "</div>";

            html += "<div class='panel side'>";
            html += "  <div class='g-title'>G-FORCE</div>";
            html += "  <div class='radar-circle'><div class='radar-cross'></div><div class='g-dot' id='g-dot'></div></div>";
            html += "</div>";

            html += "<div class='panel center'>";
            html += "  <svg viewBox='0 0 300 300'>";
            html += "    <defs><linearGradient id='gradient' x1='0%' y1='0%' x2='100%' y2='0%'>";
            html += "      <stop offset='0%' style='stop-color:#00ffff;stop-opacity:1' />";
            html += "      <stop offset='100%' style='stop-color:#ff0055;stop-opacity:1' />";
            html += "    </linearGradient></defs>";
            html += "    <circle cx='150' cy='150' r='120' class='bg-circle' stroke-dasharray='565' stroke-dashoffset='0'></circle>";
            html += "    <circle cx='150' cy='150' r='120' class='fg-circle' id='rpm-arc' stroke-dasharray='754' stroke-dashoffset='754'></circle>";
            html += "  </svg>";
            html += "  <div class='data-cluster'>";
            html += "    <div class='unit'>KM/H</div>";
            html += "    <div class='speed-val' id='speed'>0</div>";
            html += "    <div class='gear-val' id='gear'>N</div>";
            html += "  </div>";
            html += "</div>";

            html += "<div class='panel side'>";
            html += "  <div class='g-title'>INPUTS</div>";
            html += "  <div class='telemetry-box'>";
            html += "    <div class='bar-col'><div class='t-bar' id='t-acc'></div><div class='bar-label'>THR</div></div>";
            html += "    <div class='bar-col'><div class='t-bar' id='t-brk'></div><div class='bar-label'>BRK</div></div>";
            html += "  </div>";
            html += "</div>";

            html += "<script>";
            html += "function goFullscreen() {";
            html += "  var elem = document.documentElement;";
            html += "  if (elem.requestFullscreen) { elem.requestFullscreen(); }";
            html += "  else if (elem.webkitRequestFullscreen) { elem.webkitRequestFullscreen(); }";
            html += "  else if (elem.msRequestFullscreen) { elem.msRequestFullscreen(); }";
            html += "  document.getElementById('overlay').style.display = 'none';";
            html += "  if ('wakeLock' in navigator) {";
            html += "     navigator.wakeLock.request('screen').catch((err) => { console.log(err); });";
            html += "  }";
            html += "}";

            html += "const circumference = 754;";
            html += "const maxArc = 754 * 0.75;";
            html += "setInterval(() => { fetch('/data').then(r=>r.json()).then(d=>{";

            html += "  document.getElementById('speed').innerText = d.speed;";
            html += "  let g = d.gear; let t = g;";
            html += "  if(g==0) t='R'; else if(g==11) t='N';";
            html += "  document.getElementById('gear').innerText = t;";

            html += "  let rpmPct = 0;";
            html += "  if(d.maxRpm > 0) rpmPct = d.rpm / d.maxRpm;";
            html += "  if(rpmPct > 1) rpmPct = 1;";
            html += "  let offset = circumference - (rpmPct * maxArc);";
            html += "  document.getElementById('rpm-arc').style.strokeDashoffset = offset;";

            html += "  if(rpmPct > 0.92) document.getElementById('body').classList.add('shifting');";
            html += "  else document.getElementById('body').classList.remove('shifting');";

            html += "  document.getElementById('t-acc').style.height = d.throttle + '%';";
            html += "  document.getElementById('t-brk').style.height = d.brake + '%';";

            html += "  let gX = d.gX * 3.0;";
            html += "  let gZ = d.gZ * 3.0;";
            html += "  if(gX > 48) gX = 48; if(gX < -48) gX = -48;";
            html += "  if(gZ > 48) gZ = 48; if(gZ < -48) gZ = -48;";
            html += "  let leftPos = 50 + gX;";
            html += "  let topPos = 50 - gZ;";
            html += "  let dot = document.getElementById('g-dot');";
            html += "  dot.style.left = leftPos + '%';";
            html += "  dot.style.top = topPos + '%';";

            html += "})}, 30);";
            html += "</script></body></html>";

            send(clientSocket, html.c_str(), html.length(), 0);
        }
        closesocket(clientSocket);
    }
}
// --- MAIN FUNCTION ---
int main() {
    // 1. Setup Winsock
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server, client;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(5300);
    bind(server_socket, (struct sockaddr*)&server, sizeof(server));

    // 2. Start Web Server Thread
    thread webThread(StartWebServer);
    webThread.detach();

    // 3. Setup Console UI
    system("cls");
    HideCursor();
    SetColor(11); // Cyan
    GoToXY(2, 1); cout << "FORZA HORIZON 5 TELEMETRY SYSTEM - FINAL EDITION";
    GoToXY(2, 2); cout << "==================================================";

    // Draw Boxes
    SetColor(8);
    // Speed Box
    GoToXY(2, 4); cout << "+--------------------+";
    GoToXY(2, 5); cout << "|                    |";
    GoToXY(2, 6); cout << "|                    |";
    GoToXY(2, 7); cout << "|                    |";
    GoToXY(2, 8); cout << "+--------------------+";
    GoToXY(4, 5); SetColor(7); cout << "VITEZA (KM/H)";

    // RPM Box
    SetColor(8);
    GoToXY(25, 4); cout << "+------------------------------+";
    GoToXY(25, 5); cout << "|                              |";
    GoToXY(25, 6); cout << "|                              |";
    GoToXY(25, 7); cout << "|                              |";
    GoToXY(25, 8); cout << "+------------------------------+";
    GoToXY(27, 5); SetColor(7); cout << "RPM MONITOR";

    // Debug info
    GoToXY(2, 10); SetColor(10); cout << "TELEMETRY ACTIVE. Connect phone to port 8080.";

    char buffer[1024];
    int slen = sizeof(client);

    // 4. Main Loop
    while (true) {
        int recv_len = recvfrom(server_socket, buffer, 1024, 0, (struct sockaddr*)&client, &slen);

        if (recv_len > 0) {
            ForzaData* data = (ForzaData*)buffer;

            // Physics Calculation
            float speedMPS = sqrt(pow(data->VelocityX, 2) + pow(data->VelocityY, 2) + pow(data->VelocityZ, 2));
            float speedKPH = speedMPS * 3.6f;

            // Update Globals
            globalSpeed = (int)speedKPH;
            globalRPM = (int)data->CurrentEngineRpm;
            globalMaxRPM = (int)data->EngineMaxRpm;

            // [NOU] ACTUALIZAM G-FORCE DIN STRUCTURA (NU RAW)
            // Acestea sunt sigure de citit direct
            globalAccelX = data->AccelerationX; // Lateral
            globalAccelZ = data->AccelerationZ; // Longitudinal

            // --- RAW MEMORY READING PENTRU PEDALE ---
            unsigned char* rawBytes = (unsigned char*)buffer;

            if (recv_len >= 320) {
                unsigned char rawAccel = rawBytes[315];
                unsigned char rawBrake = rawBytes[316];
                unsigned char rawGear = rawBytes[319];

                globalThrottle = (int)((rawAccel / 255.0f) * 100.0f);
                globalBrake = (int)((rawBrake / 255.0f) * 100.0f);
                globalGear = (int)rawGear;
            }

            // --- DESENARE CONSOLA ---
            GoToXY(6, 7);
            SetColor(14);
            if (speedKPH > 200) SetColor(12);
            cout << setw(3) << (int)speedKPH;

            // RPM Bar
            GoToXY(27, 7); SetColor(8); cout << "[                    ]";
            GoToXY(28, 7);

            float rpmPercent = 0;
            if (data->EngineMaxRpm > 0) rpmPercent = data->CurrentEngineRpm / data->EngineMaxRpm;
            int blocks = (int)(rpmPercent * 20.0f);
            if (blocks > 20) blocks = 20;

            for (int i = 0; i < blocks; i++) {
                if (i < 15) SetColor(10);
                else SetColor(12);
                cout << char(219);
            }
        }
    }

    return 0;
}