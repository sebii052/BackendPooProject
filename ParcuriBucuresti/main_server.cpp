#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <string>
#include "db/DatabaseManager.h"
#include "server/SocketServer.h"

static const int SERVER_PORT = 9000;

int main() {
    std::cout << "==============================================\n";
    std::cout << "  Sistem Administrare Parcuri Bucuresti\n";
    std::cout << "  Server TCP pornit pe portul " << SERVER_PORT << "\n";
    std::cout << "==============================================\n\n";

    try {
        DatabaseManager& db = DatabaseManager::getInstance();
        db.conecteaza("localhost\\SQLEXPRESS", "ParcuriBucuresti");
        std::cout << "[OK] Conectat la SQL Server (ParcuriBucuresti).\n";

        SocketServer server(SERVER_PORT, db);
        server.porneste();

    }
    catch (const std::exception& e) {
        std::cerr << "[EROARE FATALA] " << e.what() << "\n";
        std::cerr << "Serverul nu a putut porni.\n";
        return 1;
    }

    return 0;
}
