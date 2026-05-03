#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <thread>
#include <iostream>
#include <sstream>

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
            if (result <= 0) break;
            if (c == '\n') break;
            if (c == '\r') continue; // ignora \r din \r\n
            mesaj += c;
        }
        return mesaj;
    }

    void trimiteMesaj(const std::string& mesaj) {
        send(m_socket, mesaj.c_str(),
            static_cast<int>(mesaj.size()), 0);
    }

    // Mascheaza parola din JSON pentru logging
    // {"actiune":"login","parola":"admin123",...}
    // -> {"actiune":"login","parola":"***",...}
    std::string mascheazaParola(const std::string& mesaj) {
        // Cauta "parola":"..." si inlocuieste valoarea cu ***
        std::string rezultat = mesaj;
        std::string cheie = "\"parola\":\"";
        size_t pos = rezultat.find(cheie);
        if (pos == std::string::npos) return rezultat;

        size_t start = pos + cheie.size();
        size_t end = rezultat.find('"', start);
        if (end == std::string::npos) return rezultat;

        rezultat.replace(start, end - start, "***");
        return rezultat;
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

            // Logam mesajul cu parola mascata
            std::cout << "[RECV] " << mascheazaParola(mesaj) << "\n";

            std::string raspuns = JsonProtocol::proceseaza(mesaj, app);
            std::cout << "[SEND] " << raspuns;
            trimiteMesaj(raspuns);
        }

        std::cout << "[CLIENT] Deconectat.\n";
        closesocket(m_socket);
        delete this;
    }

    void pornestePeThread() {
        std::thread t([this]() { this->run(); });
        t.detach();
    }
};