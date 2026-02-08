#include <iostream>
#include <winsock2.h> // Biblioteca pentru rețelistică pe Windows
#include <vector>

// Linkam biblioteca ws2_32.lib (necesar pentru Visual Studio/Linker)
#pragma comment(lib,"ws2_32.lib") 

using namespace std;

// Aceasta este structura EXACTĂ a datelor trimise de Forza Horizon (Formatul "Sled")
// Aici vezi puterea C++: mapam memoria direct pe variabile.
struct ForzaData {
    long IsRaceOn;      // = 1 cand esti in cursa, 0 in meniu
    unsigned long TimestampMS;
    float EngineMaxRpm;
    float EngineIdleRpm;
    float CurrentEngineRpm;
    float AccelerationX;
    float AccelerationY;
    float AccelerationZ;
    float VelocityX;
    float VelocityY;
    float VelocityZ;
    float AngularVelocityX;
    float AngularVelocityY;
    float AngularVelocityZ;
    float Yaw;
    float Pitch;
    float Roll;
    float NormalizedSuspensionTravelFrontLeft;
    float NormalizedSuspensionTravelFrontRight;
    float NormalizedSuspensionTravelRearLeft;
    float NormalizedSuspensionTravelRearRight;
    float TireSlipRatioFrontLeft;
    float TireSlipRatioFrontRight;
    float TireSlipRatioRearLeft;
    float TireSlipRatioRearRight;
    float WheelRotationSpeedFrontLeft;
    float WheelRotationSpeedFrontRight;
    float WheelRotationSpeedRearLeft;
    float WheelRotationSpeedRearRight;
    long WheelOnRumbleStripFrontLeft;
    long WheelOnRumbleStripFrontRight;
    long WheelOnRumbleStripRearLeft;
    long WheelOnRumbleStripRearRight;
    float WheelInPuddleDepthFrontLeft;
    float WheelInPuddleDepthFrontRight;
    float WheelInPuddleDepthRearLeft;
    float WheelInPuddleDepthRearRight;
    float SurfaceRumbleFrontLeft;
    float SurfaceRumbleFrontRight;
    float SurfaceRumbleRearLeft;
    float SurfaceRumbleRearRight;
    float TireSlipAngleFrontLeft;
    float TireSlipAngleFrontRight;
    float TireSlipAngleRearLeft;
    float TireSlipAngleRearRight;
    float TireCombinedSlipFrontLeft;
    float TireCombinedSlipFrontRight;
    float TireCombinedSlipRearLeft;
    float TireCombinedSlipRearRight;
    float SuspensionTravelMetersFrontLeft;
    float SuspensionTravelMetersFrontRight;
    float SuspensionTravelMetersRearLeft;
    float SuspensionTravelMetersRearRight;
    long CarOrdinal;
    long CarClass;
    long CarPerformanceIndex;
    long DrivetrainType;
    long NumCylinders;
};

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

    cout << "--- Astept date de la Forza Horizon pe portul 5300 ---" << endl;
    cout << "Intra in joc si condu masina!" << endl;

    char buffer[1024]; // Aici vom stoca datele brute primite
    int slen = sizeof(client);

    while (true) 
    {
        // 5. Primirea datelor (Blocheaza executia pana vin date)
        int recv_len;
        if ((recv_len = recvfrom(server_socket, buffer, 1024, 0, (struct sockaddr*)&client, &slen)) == SOCKET_ERROR)
        {
            cout << "recvfrom failed: " << WSAGetLastError() << endl;
            exit(1);
        }

        // 6. MAGIA POINTERILOR (Casting)
        // Transformam buffer-ul de char (bytes) direct in structura noastra
        // Asta e "unsafe" dar extrem de rapid. Exact ce se cere la ACS/Sisteme de Operare.
        ForzaData* data = (ForzaData*)buffer;

        // Calculam viteza (Viteza e vector 3D, facem magnitudine: sqrt(x^2 + y^2 + z^2))
        // Forza da viteza in metri pe secunda (m/s). Inmultim cu 3.6 pentru km/h.
        float speedMPS = sqrt(pow(data->VelocityX, 2) + pow(data->VelocityY, 2) + pow(data->VelocityZ, 2));
        float speedKPH = speedMPS * 3.6f;

        // Afisam datele
        // Folosim '\r' (carriage return) ca sa suprascriem linia, sa para animatie fluida
        string clasa = GetCarClass(data->CarPerformanceIndex);

       /* printf("Clasa: %s (%ld) | RPM: %8.0f | Viteza: %4.0f km/h \r",
            clasa.c_str(),
            data->CarPerformanceIndex,
            data->CurrentEngineRpm,
            speedKPH);*/
        // Calculam procentul de RPM (Cat de turata e masina, de la 0.0 la 1.0)
        float rpmPercent = data->CurrentEngineRpm / data->EngineMaxRpm;
        int barWidth = 70; // Lungimea barei grafice
        int pos = barWidth * rpmPercent;

        string bar = "[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) bar += "=";
            else bar += " ";
        }
        bar += "]";

        // Acum afisam totul frumos
        // \r ne duce la inceputul randului, cout flush forteaza afisarea
        cout << "Clasa: " << clasa << " | " << bar << " " << (int)speedKPH << " km/h      \r" << flush;
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}