#include <iostream>
#include "ForzaData.h"
#include <winsock2.h> // Biblioteca pentru rețelistică pe Windows
#include <vector>

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

int main()
{
    // 1. Initializare Winsock (boilerplate obligatoriu pe Windows)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) 
    {
        cout << "Eroare la initializare Winsock: " << WSAGetLastError() << endl;
        return 1;
    }

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

    char buffer[1024]; // Aici vom stoca datele brute primite
    int slen = sizeof(client);

    // [NOU] Variabila pentru Anti-Cheat
    float lastSpeed = 0.0f;

    // Variabile pentru logica noua
    bool isFirstPacket = true; // [FIX] Ignoram primul pachet ca sa nu dea eroare

    while (true) {
        // [FIX 1] AM SCOS SLEEP-ul. 
        // recvfrom este "blocking", deci programul oricum asteapta aici pana vine pachetul.
        // Nu consuma CPU cand asteapta.

        int recv_len;
        if ((recv_len = recvfrom(server_socket, buffer, 1024, 0, (struct sockaddr*)&client, &slen)) == SOCKET_ERROR) {
            continue;
        }

        ForzaData* data = (ForzaData*)buffer;

        // Calcule viteza
        float speedMPS = sqrt(pow(data->VelocityX, 2) + pow(data->VelocityY, 2) + pow(data->VelocityZ, 2));
        float speedKPH = speedMPS * 3.6f;

        string clasa = GetCarClass(data->CarPerformanceIndex);

        // --- ANTI-CHEAT LOGIC V2 ---
        if (!isFirstPacket) { // Facem verificarea doar DACA NU e primul pachet
            float deltaSpeed = speedKPH - lastSpeed;

            // Verificam daca acceleratia e fizic imposibila
            // (Ex: creste cu 20km/h intr-o fracțiune de secundă, adica 1 frame)
            if (speedKPH > 5 && deltaSpeed > 20.0f) {
                cout << "\n [!!!] CHEAT DETECTED: Jumped " << deltaSpeed << " km/h in 1 frame! \n";
            }
        }
        else {
            // Daca e primul pachet, doar setam flag-ul pe false si mergem mai departe
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