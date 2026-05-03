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


// CLASA ClientHandler
// Gestioneaza un singur client conectat
// Ruleaza pe un thread separat (std::thread)
// Fiecare client are propriul AppController => sesiune izolata
class ClientHandler {
private:
    SOCKET           m_socket;
    DatabaseManager& m_db;

    // Citeste un mesaj complet pana la '\n'
    // Returneaza "" daca clientul s-a deconectat
    std::string citesteMesaj() {
        std::string mesaj;
        char c;
        while (true) {
            int result = recv(m_socket, &c, 1, 0);
            if (result <= 0) return ""; // deconectat sau eroare
            if (c == '\n') break;
            if (c == '\r') continue;   // ignora \r din \r\n (Windows)
            mesaj += c;
        }
        return mesaj;
    }

    // Trimite mesaj complet - garanteaza ca toti bytes sunt trimisi
    void trimiteMesaj(const std::string& mesaj) {
        int total = static_cast<int>(mesaj.size());
        int trimis = 0;
        while (trimis < total) {
            int r = send(m_socket, mesaj.c_str() + trimis,
                total - trimis, 0);
            if (r <= 0) break; // eroare sau deconectat
            trimis += r;
        }
    }

    // Mascheaza parola din JSON pentru logging
    std::string mascheazaParola(const std::string& mesaj) {
        std::string rezultat = mesaj;
        const std::string cheie = "\"parola\":\"";
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
        std::cout.flush();

        AppController app(m_db);

        while (true) {
            std::string mesaj = citesteMesaj();
            if (mesaj.empty()) break; // client deconectat

            std::cout << "[RECV] " << mascheazaParola(mesaj) << "\n";
            std::cout.flush();

            std::string raspuns;
            try {
                raspuns = JsonProtocol::proceseaza(mesaj, app);
            }
            catch (const std::exception& e) {
                raspuns = std::string("{\"succes\":false,\"eroare\":\"Exceptie: ")
                    + e.what() + "\"}\n";
            }
            catch (...) {
                raspuns = "{\"succes\":false,\"eroare\":\"Eroare interna\"}\n";
            }

            std::cout << "[SEND] " << raspuns;
            std::cout.flush();
            trimiteMesaj(raspuns);
        }

        std::cout << "[CLIENT] Deconectat.\n";
        std::cout.flush();
        closesocket(m_socket);
    }

    void pornestePeThread() {
        ClientHandler* self = this;
        std::thread t([self]() {
            self->run();
            delete self; // stergem dupa ce run() s-a terminat
            });
        t.detach();
    }
};