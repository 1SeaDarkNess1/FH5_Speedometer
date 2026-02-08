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

        // API PENTRU DASHBOARD
        if (reqStr.find("GET /data") != string::npos) {
            string jsonBody = "{ \"speed\": " + to_string(globalSpeed) + ", \"rpm\": " + to_string(globalRPM) + " }";
            string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n" + jsonBody;
            send(clientSocket, response.c_str(), response.length(), 0);
        }
        // HTML MODERN
        else {
            string html = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
            html += "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
            html += "<style>";
            html += "body { background: #121212; color: white; font-family: 'Arial', sans-serif; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0; }";
            html += ".card { background: #1e1e1e; padding: 20px; border-radius: 15px; width: 80%; max-width: 400px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); text-align: center; }";
            html += ".value { font-size: 80px; font-weight: bold; color: #00ffcc; }";
            html += ".label { color: #888; letter-spacing: 2px; font-size: 14px; margin-bottom: 5px; }";
            html += ".rpm-bar-bg { width: 100%; height: 10px; background: #333; border-radius: 5px; margin-top: 20px; overflow: hidden; }";
            html += ".rpm-bar-fill { height: 100%; background: linear-gradient(90deg, #00ffcc, #ffff00, #ff0000); width: 0%; transition: width 0.1s linear; }";
            html += "</style></head><body>";

            html += "<div class='card'>";
            html += "<div class='label'>VITEZA</div>";
            html += "<div class='value' id='spd'>0</div>";
            html += "<div class='label'>KM/H</div>";
            html += "<div class='rpm-bar-bg'><div class='rpm-bar-fill' id='rpm'></div></div>";
            html += "<div class='label' style='margin-top: 5px;'>RPM MONITOR</div>";
            html += "</div>";

            html += "<script>";
            html += "setInterval(() => { fetch('/data').then(r=>r.json()).then(d=>{";
            html += "  document.getElementById('spd').innerText = d.speed;";
            html += "  document.getElementById('rpm').style.width = (d.rpm / 9000 * 100) + '%';"; // Presupunem max 9000 RPM
            html += "})}, 50);"; // 20 FPS
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
            ForzaData* data = (ForzaData*)buffer;

            // Calcule
            float speedMPS = sqrt(pow(data->VelocityX, 2) + pow(data->VelocityY, 2) + pow(data->VelocityZ, 2));
            float speedKPH = speedMPS * 3.6f;

            // Update Globale
            globalSpeed = (int)speedKPH;
            globalRPM = (int)data->CurrentEngineRpm;

            // --- DESENARE DINAMICA (Doar valorile) ---

            // 1. Afisare Viteza (Centrat in casuta)
            GoToXY(6, 7);
            SetColor(14); // Galben
            if (speedKPH > 200) SetColor(12); // Rosu daca e viteza mare
            cout << setw(3) << (int)speedKPH; // setw aliniaza frumos

            // 2. Afisare RPM (Bara progresiva in consola)
            GoToXY(27, 7);
            SetColor(8); cout << "[                    ]"; // Stergem bara veche (background)
            GoToXY(28, 7);

            // Calculam cati "patratele" sa desenam
            float rpmPercent = data->CurrentEngineRpm / data->EngineMaxRpm;
            int blocks = (int)(rpmPercent * 20.0f); // Bara are 20 caractere lungime
            if (blocks > 20) blocks = 20;

            // Desenam bara colorata
            for (int i = 0; i < blocks; i++) {
                if (i < 15) SetColor(10); // Verde
                else SetColor(12);       // Rosu pe zona rosie
                cout << char(219);       // Caracterul 'block' (plin)
            }

            // CSV Log
            logFile << data->TimestampMS << "," << speedKPH << "," << data->CurrentEngineRpm << "\n";
        }
    }
}