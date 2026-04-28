#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <thread>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "../AppController.h"
#include "JsonProtocol.h"

// ============================================================
// CLASA ClientHandler
// Gestioneaza un singur client conectat.
// Ruleaza pe un thread separat (std::thread).
// Fiecare client are propriul AppController => sesiune izolata.
// ============================================================
class ClientHandler {
private:
    SOCKET           m_socket;
    DatabaseManager& m_db;

    // Citeste un mesaj complet pana la '\n'
    std::string citesteMesaj() {
        std::string mesaj;
        char c;
        while (true) {
            int result = recv(m_socket, &c, 1, 0);
            if (result <= 0) break;  // client deconectat
            if (c == '\n') break;    // sfarsit mesaj
            mesaj += c;
        }
        return mesaj;
    }

    void trimiteMesaj(const std::string& mesaj) {
        send(m_socket, mesaj.c_str(),
            static_cast<int>(mesaj.size()), 0);
    }

public:
    ClientHandler(SOCKET socket, DatabaseManager& db)
        : m_socket(socket), m_db(db) {
    }

    // Bucla principala — apelata pe thread separat
    void run() {
        std::cout << "[CLIENT] Conexiune noua acceptata.\n";

        AppController app(m_db);

        while (true) {
            std::string mesaj = citesteMesaj();
            if (mesaj.empty()) break;

            std::cout << "[RECV] " << mesaj << "\n";
            std::string raspuns = JsonProtocol::proceseaza(mesaj, app);
            std::cout << "[SEND] " << raspuns;
            trimiteMesaj(raspuns);
        }

        std::cout << "[CLIENT] Deconectat.\n";
        closesocket(m_socket);
        delete this;  // se sterge singur la final de thread
    }

    void pornestePeThread() {
        std::thread t([this]() { this->run(); });
        t.detach();
    }
};
