#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <iomanip> // Pentru formatare frumoasa

// Includem Winsock si Windows API
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")

#include "ForzaData.h"

using namespace std;

// --- GLOBALE ---
atomic<int> globalSpeed(0);
atomic<int> globalRPM(0);
atomic<int> globalGear(0);      // [NOU] Treapta de viteza
atomic<int> globalThrottle(0);  // [NOU] Cat la suta apesi acceleratia (0-100)
atomic<int> globalBrake(0);     // [NOU] Cat la suta apesi frana (0-100)
atomic<int> globalMaxRPM(0);    // [NOU] Ca sa stim cand sa dam FLASH

// --- UTILITARE PENTRU CONSOLA (GRAFICA) ---
void HideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE; // Ascunde cursorul care palpaie
    SetConsoleCursorInfo(consoleHandle, &info);
}

void GoToXY(int x, int y) {
    COORD c;
    c.X = x;
    c.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

// Culori: 7=Alb, 10=Verde, 12=Rosu, 14=Galben, 11=Cyan
void SetColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void DrawBox(int x, int y, int width, int height) {
    SetColor(8); // Gri inchis pentru margini
    // Colturile
    GoToXY(x, y); cout << "+";
    GoToXY(x + width, y); cout << "+";
    GoToXY(x, y + height); cout << "+";
    GoToXY(x + width, y + height); cout << "+";

    // Liniile orizontale
    for (int i = 1; i < width; i++) {
        GoToXY(x + i, y); cout << "-";
        GoToXY(x + i, y + height); cout << "-";
    }
    // Liniile verticale
    for (int i = 1; i < height; i++) {
        GoToXY(x, y + i); cout << "|";
        GoToXY(x + width, y + i); cout << "|";
    }
}

// --- SERVER WEB (SIMPLIFICAT PENTRU VITEZA) ---
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

        // --- API DATA (JSON) ---
        if (reqStr.find("GET /data") != string::npos) {
            string jsonBody = "{";
            jsonBody += "\"speed\": " + to_string(globalSpeed) + ",";
            jsonBody += "\"rpm\": " + to_string(globalRPM) + ",";
            jsonBody += "\"maxRpm\": " + to_string(globalMaxRPM) + ",";
            jsonBody += "\"gear\": " + to_string(globalGear) + ",";
            jsonBody += "\"throttle\": " + to_string(globalThrottle) + ",";
            jsonBody += "\"brake\": " + to_string(globalBrake);
            jsonBody += "}";

            string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n" + jsonBody;
            send(clientSocket, response.c_str(), response.length(), 0);
        }
        // --- DESIGN MODERN (HTML/CSS) ---
        else {
            string html = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
            html += "<html><head><meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=0'>";
            html += "<style>";
            // CSS - STILIZARE
            html += "body { background-color: #050505; color: white; font-family: 'Segoe UI', sans-serif; overflow: hidden; height: 100vh; margin: 0; display: flex; flex-direction: column; }";

            // Container Principal
            html += ".dashboard { flex: 1; display: flex; position: relative; }";

            // Pedale (Laterale)
            html += ".pedal-bar { width: 40px; height: 100%; background: #111; position: absolute; top: 0; display: flex; align-items: flex-end; }";
            html += ".left { left: 0; border-right: 2px solid #333; }";
            html += ".right { right: 0; border-left: 2px solid #333; }";
            html += ".pedal-fill { width: 100%; transition: height 0.05s linear; opacity: 0.8; }";
            html += "#brake-fill { background: #ff0040; height: 0%; box-shadow: 0 0 15px #ff0040; }";
            html += "#throttle-fill { background: #00ff80; height: 0%; box-shadow: 0 0 15px #00ff80; }";

            // Centru (Viteza si Gear)
            html += ".center-panel { flex: 1; display: flex; flex-direction: column; align-items: center; justify-content: center; z-index: 2; }";
            html += ".gear-display { font-size: 160px; font-weight: 900; color: #fff; text-shadow: 0 0 30px rgba(255,255,255,0.6); line-height: 1; }";
            html += ".speed-display { font-size: 50px; color: #00eaff; font-weight: bold; margin-top: 10px; }";
            html += ".speed-label { font-size: 16px; color: #555; letter-spacing: 3px; }";

            // RPM Bar (Jos)
            html += ".rpm-container { width: 100%; height: 60px; background: #111; position: relative; border-top: 2px solid #333; }";
            html += ".rpm-fill { height: 100%; background: linear-gradient(90deg, #0040ff, #00ff80, #ff0000); width: 0%; transition: width 0.05s linear; }";
            html += ".rpm-text { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); font-weight: bold; font-size: 20px; text-shadow: 1px 1px 2px black; }";

            // Animatie Shift Light
            html += "@keyframes flashRed { 0% { background-color: #ff0000; } 50% { background-color: #330000; } 100% { background-color: #ff0000; } }";
            html += ".shifting { animation: flashRed 0.08s infinite; }"; // Foarte agresiv

            html += "</style></head><body>";

            // LAYOUT
            html += "<div class='dashboard' id='dash-bg'>";
            html += "  <div class='pedal-bar left'><div class='pedal-fill' id='brake-fill'></div></div>"; // Frana
            html += "  <div class='center-panel'>";
            html += "     <div class='gear-display' id='gear'>N</div>";
            html += "     <div class='speed-display'><span id='speed'>0</span> <span class='speed-label'>KM/H</span></div>";
            html += "  </div>";
            html += "  <div class='pedal-bar right'><div class='pedal-fill' id='throttle-fill'></div></div>"; // Acceleratia
            html += "</div>";

            html += "<div class='rpm-container'>";
            html += "   <div class='rpm-fill' id='rpm-bar'></div>";
            html += "   <div class='rpm-text'><span id='rpm-val'>0</span> RPM</div>";
            html += "</div>";

            // JAVASCRIPT
            html += "<script>";
            html += "setInterval(() => { fetch('/data').then(r=>r.json()).then(d=>{";

            // 1. Viteza si RPM
            html += "  document.getElementById('speed').innerText = d.speed;";
            html += "  document.getElementById('rpm-val').innerText = d.rpm;";

            // 2. Bara RPM
            html += "  let rpmPct = 0;";
            html += "  if(d.maxRpm > 0) rpmPct = (d.rpm / d.maxRpm) * 100;";
            html += "  if(rpmPct > 100) rpmPct = 100;";
            html += "  document.getElementById('rpm-bar').style.width = rpmPct + '%';";

            // 3. GEAR LOGIC
            html += "  let g = d.gear;";
            html += "  let displayG = g;";

            html += "  if (g == 0) displayG = 'R';";       // 0 e Reverse
            html += "  else if (g == 11) displayG = 'N';"; // 11 e Neutral (deseori)
            // Daca vezi ca Neutral apare ca 'R', schimba conditia de mai sus

            html += "  document.getElementById('gear').innerText = displayG;";

            // 4. Pedale
            html += "  document.getElementById('throttle-fill').style.height = d.throttle + '%';";
            html += "  document.getElementById('brake-fill').style.height = d.brake + '%';";

            // 5. Shift Light (Doar bara de jos sau tot ecranul?)
            html += "  let bg = document.getElementById('dash-bg');";
            html += "  if(rpmPct > 92) { bg.classList.add('shifting'); }";
            html += "  else { bg.classList.remove('shifting'); }";

            html += "})}, 30);";
            html += "</script></body></html>";

            send(clientSocket, html.c_str(), html.length(), 0);
        }
        closesocket(clientSocket);
    }
}

// --- MAIN ---
int main() {
    // 1. Setup
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server, client;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(5300);
    bind(server_socket, (struct sockaddr*)&server, sizeof(server));

    // Pornim thread-ul web
    thread webThread(StartWebServer);
    webThread.detach();

    // 2. Initializare Grafica Consola
    system("cls"); // Curatam o singura data la inceput
    HideCursor();

    // Desenam interfata STATICA (Chenarele)
    SetColor(11); // Cyan
    GoToXY(2, 1); cout << "FORZA HORIZON 5 TELEMETRY SYSTEM v2.0";
    GoToXY(2, 2); cout << "=======================================";

    // Chenar Viteza
    DrawBox(2, 4, 20, 4);
    GoToXY(4, 5); SetColor(7); cout << "VITEZA (KM/H)";

    // Chenar RPM
    DrawBox(25, 4, 30, 4);
    GoToXY(27, 5); SetColor(7); cout << "RPM MOTOR";

    // Chenar Log
    DrawBox(2, 10, 53, 3);
    GoToXY(4, 11); cout << "STATUS: ";
    SetColor(10); cout << "CONECTAT SI INREGISTREAZA...";

    // Fisier CSV
    ofstream logFile;
    logFile.open("ForzaLog_V2.csv");
    logFile << "Timestamp,Speed,RPM,Gear\n";

    char buffer[1024];
    int slen = sizeof(client);

    // 3. Loop Principal
    while (true) {
        int recv_len = recvfrom(server_socket, buffer, 1024, 0, (struct sockaddr*)&client, &slen);

        if (recv_len > 0) {
            // --- DIAGNOSTICARE ---
            // Asta iti va spune exact ce se intampla
            GoToXY(0, 0);
            cout << "Marime Pachet: " << recv_len << " bytes   ";
            if (recv_len == 232) cout << "(FORMAT GRESIT! Schimba in CAR DASH)";
            if (recv_len == 324) cout << "(FORMAT CORECT: Car Dash)";

            // Accesam datele standard
            ForzaData* data = (ForzaData*)buffer;

            // Calcule Fizica
            float speedMPS = sqrt(pow(data->VelocityX, 2) + pow(data->VelocityY, 2) + pow(data->VelocityZ, 2));
            float speedKPH = speedMPS * 3.6f;

            // Update Globale
            globalSpeed = (int)speedKPH;
            globalRPM = (int)data->CurrentEngineRpm;
            globalMaxRPM = (int)data->EngineMaxRpm;

            // --- CITIRE PEDALE (OFFSET CORECT PENTRU CAR DASH) ---
            // In formatul "Car Dash" (324 bytes):
            // Accel = 315
            // Brake = 316
            // Gear  = 319

            unsigned char* rawBytes = (unsigned char*)buffer;

            if (recv_len >= 320) { // Citim doar daca avem pachetul mare
                unsigned char rawAccel = rawBytes[315];
                unsigned char rawBrake = rawBytes[316];
                unsigned char rawGear = rawBytes[319];

                globalThrottle = (int)((rawAccel / 255.0f) * 100.0f);
                globalBrake = (int)((rawBrake / 255.0f) * 100.0f);
                globalGear = (int)rawGear;
            }

            // --- AFISARE DEBUG PEDALE PE CONSOLA PC ---
            // Ca sa vedem daca le citim bine inainte sa le trimitem la telefon
            GoToXY(2, 18);
            cout << "Throttle Raw: " << globalThrottle << "%   ";
            GoToXY(2, 19);
            cout << "Brake Raw:    " << globalBrake << "%   ";
            GoToXY(2, 20);
            cout << "Gear Raw:     " << globalGear << "    ";

            // --- DESENARE GRAFICA VITEZA/RPM ---
            // 1. Viteza
            GoToXY(6, 7);
            SetColor(14);
            if (speedKPH > 200) SetColor(12);
            cout << setw(3) << (int)speedKPH;

            // 2. RPM Bar
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
}