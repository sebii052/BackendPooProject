#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "../db/DatabaseManager.h"
#include "ClientHandler.h"

// ============================================================
// CLASA SocketServer
// Server TCP care asculta pe un port dat.
// La fiecare conexiune noua, creeaza un ClientHandler
// care ruleaza pe un thread separat.
// ============================================================
class SocketServer {
private:
    int              m_port;
    SOCKET           m_serverSocket = INVALID_SOCKET;
    DatabaseManager& m_db;
    bool             m_running = false;

    void initWinsock() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
            throw std::runtime_error(
                "WSAStartup esuat. Cod: " + std::to_string(result));
    }

    void creeazaSocket() {
        m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_serverSocket == INVALID_SOCKET)
            throw std::runtime_error(
                "Nu s-a putut crea socket-ul server.");
    }

    void bindSocket() {
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(static_cast<u_short>(m_port));

        if (bind(m_serverSocket,
            reinterpret_cast<sockaddr*>(&serverAddr),
            sizeof(serverAddr)) == SOCKET_ERROR)
            throw std::runtime_error(
                "Bind esuat pe portul " + std::to_string(m_port));
    }

    void ascultaConexiuni() {
        if (listen(m_serverSocket, SOMAXCONN) == SOCKET_ERROR)
            throw std::runtime_error("Listen esuat.");
    }

public:
    SocketServer(int port, DatabaseManager& db)
        : m_port(port), m_db(db) {
    }

    ~SocketServer() { opreste(); }

    // --------------------------------------------------------
    // Porneste serverul si intra in bucla de acceptare
    // --------------------------------------------------------
    void porneste() {
        initWinsock();
        creeazaSocket();
        bindSocket();
        ascultaConexiuni();

        m_running = true;
        std::cout << "[SERVER] Pornit pe portul " << m_port << "\n";
        std::cout << "[SERVER] Asteapta conexiuni...\n";

        while (m_running) {
            sockaddr_in clientAddr{};
            int clientAddrLen = sizeof(clientAddr);

            SOCKET clientSocket = accept(
                m_serverSocket,
                reinterpret_cast<sockaddr*>(&clientAddr),
                &clientAddrLen);

            if (clientSocket == INVALID_SOCKET) {
                if (m_running)
                    std::cerr << "[SERVER] Accept esuat.\n";
                break;
            }

            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr,
                ipStr, sizeof(ipStr));
            std::cout << "[SERVER] Client conectat de la: "
                << ipStr << "\n";

            auto* handler = new ClientHandler(clientSocket, m_db);
            handler->pornestePeThread();
        }
    }

    void opreste() {
        m_running = false;
        if (m_serverSocket != INVALID_SOCKET) {
            closesocket(m_serverSocket);
            m_serverSocket = INVALID_SOCKET;
        }
        WSACleanup();
        std::cout << "[SERVER] Oprit.\n";
    }
};
