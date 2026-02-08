#include <iostream>
#include "ForzaData.h"
#include <winsock2.h> // Biblioteca pentru rețelistică pe Windows
#include <vector>
#include <fstream>
#include <thread>
#include <atomic>
#include <string>
std::atomic<int> globalSpeed(0);
std::atomic<int> globalRPM(0);
std::atomic<int> globalGear(0);

// Linkam biblioteca ws2_32.lib (necesar pentru Visual Studio/Linker)
#pragma comment(lib,"ws2_32.lib") 

using namespace std;

// Aceasta este structura EXACTĂ a datelor trimise de Forza Horizon (Formatul "Sled")
// Aici vezi puterea C++: mapam memoria direct pe variabile.


string GetCarClass(long pi) {
    if (pi <= 500) return "D";
    if (pi <= 600) return "C";
    if (pi <= 700) return "B";
    if (pi <= 800) return "A";
    if (pi <= 900) return "S1";
    if (pi <= 998) return "S2";
    return "X"; // 999+
}

void StartWebServer() {
    SOCKET webSocket;
    struct sockaddr_in server, client;

    webSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (webSocket == INVALID_SOCKET) return;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    if (bind(webSocket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) return;
    listen(webSocket, 5); // Coada de asteptare un pic mai mare

    int c = sizeof(struct sockaddr_in);

    while (true) {
        SOCKET clientSocket = accept(webSocket, (struct sockaddr*)&client, &c);
        if (clientSocket == INVALID_SOCKET) continue;

        // [NOU] Citim ce cere browserul (Request-ul)
        char request[4096];
        int bytesRead = recv(clientSocket, request, 4096, 0);
        if (bytesRead <= 0) {
            closesocket(clientSocket);
            continue;
        }

        // Convertim request-ul in string ca sa cautam usor in el
        string reqStr(request);

        // --- CAZUL 1: Browserul cere DATELE (API) ---
        // JavaScript-ul va cere asta de 20 de ori pe secunda
        if (reqStr.find("GET /data") != string::npos) {

            // Construim un raspuns JSON (JavaScript Object Notation)
            // Exemplu: { "speed": 120, "rpm": 4500 }
            string jsonBody = "{ \"speed\": " + to_string(globalSpeed) + ", \"rpm\": " + to_string(globalRPM) + " }";

            string response = "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n" // Permite oricui sa citeasca
                "Content-Length: " + to_string(jsonBody.length()) + "\r\n"
                "Connection: close\r\n\r\n" + jsonBody;

            send(clientSocket, response.c_str(), response.length(), 0);
        }

        // --- CAZUL 2: Browserul cere PAGINA (HTML) ---
        // Asta se intampla o singura data, cand deschizi site-ul
        else {
            string html = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
            html += "<html><head><title>Forza Dashboard</title>";
            html += "<meta name='viewport' content='width=device-width, initial-scale=1'>"; // Pt mobil
            html += "<style>";
            html += "body { background-color: #0f0f0f; color: #fff; font-family: 'Segoe UI', sans-serif; text-align: center; margin-top: 50px; }";
            html += ".speed-box { font-size: 120px; font-weight: 800; color: #00ffcc; text-shadow: 0 0 20px #00ffcc; }";
            html += ".rpm-box { font-size: 40px; color: #ff3333; margin-top: 20px; }";
            html += ".label { font-size: 14px; color: #666; letter-spacing: 2px; text-transform: uppercase; }";
            html += "</style>";
            html += "</head><body>";

            html += "<div class='label'>VITEZA (KM/H)</div>";
            html += "<div class='speed-box' id='speedDisplay'>0</div>"; // ID pentru JavaScript
            html += "<div class='label'>MOTOR (RPM)</div>";
            html += "<div class='rpm-box' id='rpmDisplay'>0</div>";     // ID pentru JavaScript

            // --- JAVASCRIPT-UL CARE FACE MAGIA ---
            html += "<script>";
            html += "setInterval(function() {"; // Ruleaza functia asta la infinit
            html += "   fetch('/data')";       // 1. Cere datele de la serverul nostru C++
            html += "   .then(response => response.json())"; // 2. Converteste raspunsul in JSON
            html += "   .then(data => {";
            html += "       document.getElementById('speedDisplay').innerText = data.speed;"; // 3. Actualizeaza Viteza
            html += "       document.getElementById('rpmDisplay').innerText = data.rpm;";     // 4. Actualizeaza RPM
            html += "   }).catch(err => console.log(err));";
            html += "}, 50);"; // 50ms = 20 de ori pe secunda (SUPER FLUID)
            html += "</script>";

            html += "</body></html>";

            send(clientSocket, html.c_str(), html.length(), 0);
        }

        closesocket(clientSocket);
    }
}

int main()
{
    // 1. Initializare Winsock (boilerplate obligatoriu pe Windows)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) 
    {
        cout << "Eroare la initializare Winsock: " << WSAGetLastError() << endl;
        return 1;
    }

    std::thread webThread(StartWebServer);
    webThread.detach(); 

    // 2. Creare Socket UDP (Internet Protocol, Datagrama, UDP)
    SOCKET server_socket;
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) 
    {
        cout << "Nu s-a putut crea socketul: " << WSAGetLastError() << endl;
        return 1;
    }

    // 3. Pregatim structura de adresa (unde ascultam?)
    struct sockaddr_in server, client;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY; // Asculta pe orice interfata de retea
    server.sin_port = htons(5300);       // Portul setat in joc (Host To Network Short)

    // 4. Bind (Legam socketul de port)
    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) 
    {
        cout << "Bind failed: " << WSAGetLastError() << endl;
        return 1;
    }

    cout << "--- Waiting for data Forza Horizon 5 on port 5300 ---" << endl;
    cout << "                    Start driving!" << endl;

    ofstream logFile;
    logFile.open("ForzaLog.csv");
    logFile << "Timestamp,Speed_KPM,RPM,Gear,Suspension_FL,Is_Cheating\n";

    char buffer[1024]; // Aici vom stoca datele brute primite
    int slen = sizeof(client);

    // [NOU] Variabila pentru Anti-Cheat
    float lastSpeed = 0.0f;

    // Variabile pentru logica noua
    bool isFirstPacket = true; // [FIX] Ignoram primul pachet ca sa nu dea eroare

    while (true) {

        int recv_len;
        if ((recv_len = recvfrom(server_socket, buffer, 1024, 0, (struct sockaddr*)&client, &slen)) == SOCKET_ERROR) {
            continue;
        }

        ForzaData* data = (ForzaData*)buffer;

        // Calcule viteza
        float speedMPS = sqrt(pow(data->VelocityX, 2) + pow(data->VelocityY, 2) + pow(data->VelocityZ, 2));
        float speedKPH = speedMPS * 3.6f;

        globalSpeed = (int)speedKPH;
        globalRPM = (int)data->CurrentEngineRpm; 

        string clasa = GetCarClass(data->CarPerformanceIndex);

        string cheatStatus = "NO";

        if (!isFirstPacket) {
            float deltaSpeed = speedKPH - lastSpeed;

            // Verificam daca acceleratia e suspecta
            if (speedKPH > 10 && deltaSpeed > 30.0f) {
                cheatStatus = "YES"; // [FIX] Aici doar schimbăm valoarea, nu o mai declarăm
                cout << "\n [!!!] CHEAT DETECTED: Jumped " << deltaSpeed << " km/h! \n";
            }
        }
        else {
            isFirstPacket = false;
        }

        lastSpeed = speedKPH;

        // --- GRAFICA ---
        float rpmPercent = data->CurrentEngineRpm / data->EngineMaxRpm;
        if (rpmPercent > 1.0f) rpmPercent = 1.0f;

        int barWidth = 20;
        int pos = barWidth * rpmPercent;
        string bar = "[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) bar += "=";
            else bar += " ";
        }
        bar += "]";

        logFile << data->TimestampMS << ","
            << speedKPH << ","
            << data->CurrentEngineRpm << ","
            << (int)data->CarClass << "," // Sau poti pune clasa (string)
            << data->NormalizedSuspensionTravelFrontLeft << ","
            << cheatStatus << "\n";

        printf("Clasa: %s | %s %3.0f km/h | RPM: %8.0f \r",
            clasa.c_str(),
            bar.c_str(),
            speedKPH,
            data->CurrentEngineRpm);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}