#pragma once
#include <vector>
#include <tuple>
#include <string>
#include <optional>
#include "DatabaseManager.h"
#include "../models/Utilizatori.h"
#include "../models/Zona.h"
#include "../models/Obiecte.h"
#include "../models/Operatiuni.h"
#include "../models/EvenimenteFirme.h"



// ============================================================
// INTERFATA GENERICA IRepository<T>
// ============================================================
template<typename T>
class IRepository {
public:
    virtual ~IRepository() = default;
    virtual bool             salveaza(const T& entitate) = 0;
    virtual bool             sterge(int id) = 0;
    virtual std::optional<T> gasesteDupaId(int id) = 0;
    virtual std::vector<T>   getToate() = 0;
};

// ============================================================
// UtilizatorRepository
// ============================================================
class UtilizatorRepository {
    DatabaseManager& m_db;
public:
    explicit UtilizatorRepository(DatabaseManager& db) : m_db(db) {}

    bool verificaLogin(const std::string& username,
        const std::string& parolaHash,
        std::string& rolOut) {
        bool found = false;
        std::string q =
            "SELECT rol FROM Utilizatori "
            "WHERE username='" + username +
            "' AND parola_hash='" + parolaHash +
            "' AND activ=1";
        m_db.interogheaza(q, [&](SQLHSTMT stmt) {
            rolOut = DatabaseManager::getColumn(stmt, 1);
            found = true;
            });
        return found;
    }

    // Returneaza true daca contul a fost creat
    // Returneaza false + seteaza mesajul de eroare daca username/email exista deja
    bool inregistreazaAngajat(const std::string& username,
        const std::string& parolaHash,
        const std::string& nume,
        const std::string& prenume,
        const std::string& email,
        int idZonaAlocata) {
        try {
            m_db.executa(
                "INSERT INTO Utilizatori "
                "(username, parola_hash, rol, nume, prenume, email, id_zona_alocata) "
                "VALUES ('" + username + "','" + parolaHash +
                "','Angajat','" + nume + "','" + prenume +
                "','" + email + "'," + std::to_string(idZonaAlocata) + ")");
            return true;
        }
        catch (const std::exception& e) {
            // SQL Server returneaza eroare 2627 pentru UNIQUE constraint violation
            // (username sau email duplicat)
            std::string err = e.what();
            if (err.find("2627") != std::string::npos ||
                err.find("UNIQUE") != std::string::npos ||
                err.find("duplicate") != std::string::npos) {
                throw std::runtime_error(
                    "Username-ul sau email-ul '" + username +
                    "' exista deja in sistem.");
            }
            throw; // re-arunca orice alta eroare
        }
    }

    struct InfoAngajat {
        int         id;
        std::string numeComplet;
        std::string username;
        int         idZona;
        bool        esteLiber;
        int         idTaskCurent;
    };

    std::vector<InfoAngajat> getAngajatiCuStatus() {
        std::vector<InfoAngajat> result;
        std::string q =
            "SELECT u.id_user, u.nume, u.prenume, u.username, "
            "       u.id_zona_alocata, "
            "       ISNULL(t.id_task, -1) as id_task_curent "
            "FROM Utilizatori u "
            "LEFT JOIN Taskuri t "
            "  ON t.id_angajat = u.id_user AND t.status = 'InProgress' "
            "WHERE u.rol = 'Angajat' AND u.activ = 1";

        m_db.interogheaza(q, [&](SQLHSTMT stmt) {
            InfoAngajat a;
            a.id = DatabaseManager::getColumnInt(stmt, 1);
            // col2=nume, col3=prenume => prenume + " " + nume
            a.numeComplet = DatabaseManager::getColumn(stmt, 3) + " " +
                DatabaseManager::getColumn(stmt, 2);
            a.username = DatabaseManager::getColumn(stmt, 4);
            a.idZona = DatabaseManager::getColumnInt(stmt, 5);
            a.idTaskCurent = DatabaseManager::getColumnInt(stmt, 6);
            a.esteLiber = (a.idTaskCurent == -1);
            result.push_back(a);
            });
        return result;
    }

    std::optional<int> getIdAngajat(const std::string& username) {
        std::optional<int> result;
        m_db.interogheaza(
            "SELECT id_user FROM Utilizatori WHERE username='" + username + "'",
            [&](SQLHSTMT stmt) {
                result = DatabaseManager::getColumnInt(stmt, 1);
            });
        return result;
    }
};

// ============================================================
// SesizareRepository
// ============================================================
class SesizareRepository {
    DatabaseManager& m_db;
public:
    explicit SesizareRepository(DatabaseManager& db) : m_db(db) {}

    int creeaza(int idZona, int idUser, const std::string& descriere) {
        std::string userVal = (idUser == -1) ? "NULL" : std::to_string(idUser);
        int newId = -1;
        m_db.executa(
            "INSERT INTO Sesizari (id_zona, id_user, descriere) "
            "VALUES (" + std::to_string(idZona) + "," +
            userVal + ",'" + descriere + "')");
        m_db.interogheaza("SELECT CAST(SCOPE_IDENTITY() AS INT)",
            [&](SQLHSTMT stmt) {
                newId = DatabaseManager::getColumnInt(stmt, 1);
            });
        return newId;
    }

    bool ignora(int idSesizare) {
        try {
            m_db.executa(
                "UPDATE Sesizari SET status='Ignorata', "
                "ignorata=1, data_procesare=GETDATE() "
                "WHERE id_sesizare=" + std::to_string(idSesizare));
            return true;
        }
        catch (...) { return false; }
    }

    bool marcheazaTransformata(int idSesizare) {
        try {
            m_db.executa(
                "UPDATE Sesizari SET status='TransformataTask', "
                "data_procesare=GETDATE() "
                "WHERE id_sesizare=" + std::to_string(idSesizare));
            return true;
        }
        catch (...) { return false; }
    }

    // Returneaza: (id_sesizare, id_zona, descriere, data_creare)
    std::vector<std::tuple<int, int, std::string, std::string>>
        getSesizariNerezolvate() {
        std::vector<std::tuple<int, int, std::string, std::string>> result;
        m_db.interogheaza(
            "SELECT id_sesizare, id_zona, descriere, data_creare "
            "FROM Sesizari WHERE status='Nerezolvata' "
            "ORDER BY data_creare ASC",
            [&](SQLHSTMT stmt) {
                int         id_ses = DatabaseManager::getColumnInt(stmt, 1);
                int         id_zona = DatabaseManager::getColumnInt(stmt, 2);
                std::string desc = DatabaseManager::getColumn(stmt, 3);
                std::string data = DatabaseManager::getColumn(stmt, 4);
                result.emplace_back(id_ses, id_zona, desc, data);
            });
        return result;
    }
};

// ============================================================
// TaskRepository
// ============================================================
class TaskRepository {
    DatabaseManager& m_db;
public:
    explicit TaskRepository(DatabaseManager& db) : m_db(db) {}

    int creeazaOcazional(int idAngajat, int idZona, int idSesizare,
        const std::string& descriere,
        double cost, const std::string& deadline) {
        int newId = -1;
        // INSERT fara OUTPUT (evita problema cu trigger + OUTPUT)
        m_db.executa(
            "INSERT INTO Taskuri "
            "(id_angajat, id_zona, id_sesizare, tip_task, descriere, cost_estimat, deadline) "
            "VALUES (" + std::to_string(idAngajat) + "," +
            std::to_string(idZona) + "," + std::to_string(idSesizare) +
            ",'Reparatie','" + descriere + "'," +
            std::to_string(cost) + ",'" + deadline + "')");
        // Obtine ID-ul generat prin SCOPE_IDENTITY()
        m_db.interogheaza("SELECT CAST(SCOPE_IDENTITY() AS INT)",
            [&](SQLHSTMT stmt) {
                newId = DatabaseManager::getColumnInt(stmt, 1);
            });
        return newId;
    }

    int creeazaDaily(int idAngajat, int idZona,
        const std::string& descriere,
        double cost, const std::string& deadline) {
        int newId = -1;
        m_db.executa(
            "INSERT INTO Taskuri "
            "(id_angajat, id_zona, tip_task, descriere, cost_estimat, deadline) "
            "VALUES (" + std::to_string(idAngajat) + "," +
            std::to_string(idZona) + ",'Mentenanta','" +
            descriere + "'," + std::to_string(cost) + ",'" + deadline + "')");
        m_db.interogheaza("SELECT CAST(SCOPE_IDENTITY() AS INT)",
            [&](SQLHSTMT stmt) {
                newId = DatabaseManager::getColumnInt(stmt, 1);
            });
        return newId;
    }

    bool actualizeazaStatus(int idTask, const std::string& status) {
        try {
            std::string finalizare = (status == "Done") ?
                ", data_finalizare=GETDATE()" : "";
            m_db.executa(
                "UPDATE Taskuri SET status='" + status + "'" +
                finalizare +
                " WHERE id_task=" + std::to_string(idTask));
            return true;
        }
        catch (...) { return false; }
    }

    // Returneaza taskurile finalizate: (data_finalizare, tip_task, descriere, status)
    std::vector<std::tuple<std::string, std::string, std::string, std::string>>
        getIstoricTaskuri(int idAngajat) {
        std::vector<std::tuple<std::string, std::string, std::string, std::string>> res;
        m_db.interogheaza(
            "SELECT ISNULL(CONVERT(VARCHAR(19), data_finalizare, 120), data_creare), "
            "       tip_task, descriere, status "
            "FROM Taskuri WHERE id_angajat=" + std::to_string(idAngajat) +
            " AND status IN ('Done','Anulat') "
            "ORDER BY data_creare DESC",
            [&](SQLHSTMT stmt) {
                std::string data = DatabaseManager::getColumn(stmt, 1);
                std::string tip = DatabaseManager::getColumn(stmt, 2);
                std::string desc = DatabaseManager::getColumn(stmt, 3);
                std::string status = DatabaseManager::getColumn(stmt, 4);
                res.emplace_back(data, tip, desc, status);
            });
        return res;
    }

    // Returneaza istoricul TUTUROR taskurilor finalizate (pentru Admin)
    // (data, tip, descriere, nume_angajat)
    std::vector<std::tuple<std::string, std::string, std::string, std::string>>
        getIstoricToate() {
        std::vector<std::tuple<std::string, std::string, std::string, std::string>> res;
        m_db.interogheaza(
            "SELECT ISNULL(CONVERT(VARCHAR(19), t.data_finalizare, 120), "
            "              CONVERT(VARCHAR(19), t.data_creare, 120)), "
            "       t.tip_task, t.descriere, "
            "       u.prenume + ' ' + u.nume "
            "FROM Taskuri t "
            "JOIN Utilizatori u ON u.id_user = t.id_angajat "
            "WHERE t.status IN ('Done','Anulat') "
            "ORDER BY t.data_creare DESC",
            [&](SQLHSTMT stmt) {
                std::string data = DatabaseManager::getColumn(stmt, 1);
                std::string tip = DatabaseManager::getColumn(stmt, 2);
                std::string desc = DatabaseManager::getColumn(stmt, 3);
                std::string angajat = DatabaseManager::getColumn(stmt, 4);
                res.emplace_back(data, tip, desc, angajat);
            });
        return res;
    }

    // Returneaza: (id_task, tip_task, descriere, status)
    std::vector<std::tuple<int, std::string, std::string, std::string>>
        getTaskuriAngajat(int idAngajat) {
        std::vector<std::tuple<int, std::string, std::string, std::string>> res;
        m_db.interogheaza(
            "SELECT id_task, tip_task, descriere, status "
            "FROM Taskuri WHERE id_angajat=" + std::to_string(idAngajat) +
            " ORDER BY data_creare DESC",
            [&](SQLHSTMT stmt) {
                // Citire in variabile locale - garanteaza ordinea col 1,2,3,4
                int         id = DatabaseManager::getColumnInt(stmt, 1);
                std::string tip = DatabaseManager::getColumn(stmt, 2);
                std::string desc = DatabaseManager::getColumn(stmt, 3);
                std::string status = DatabaseManager::getColumn(stmt, 4);
                res.emplace_back(id, tip, desc, status);
            });
        return res;
    }
};

// ============================================================
// InventarRepository
// ============================================================
class InventarRepository {
    DatabaseManager& m_db;
public:
    explicit InventarRepository(DatabaseManager& db) : m_db(db) {}

    int adaugaInDepozit(int idCategorie, int idFurnizor,
        int cantitate, double pret,
        const std::string& dataAchizitie) {
        int newId = -1;
        m_db.executa(
            "INSERT INTO Inventar "
            "(id_categorie, locatie, stare, cantitate, pret_achizitie, data_achizitie) "
            "VALUES (" + std::to_string(idCategorie) +
            ",'Depozit','Functional'," + std::to_string(cantitate) +
            "," + std::to_string(pret) + ",'" + dataAchizitie + "')");
        m_db.interogheaza("SELECT CAST(SCOPE_IDENTITY() AS INT)",
            [&](SQLHSTMT stmt) {
                newId = DatabaseManager::getColumnInt(stmt, 1);
            });
        return newId;
    }

    bool instaleazaInZona(int idObiect, int idZona,
        const std::string& dataInstalare) {
        try {
            m_db.executa(
                "UPDATE Inventar SET locatie='InUz', "
                "id_zona=" + std::to_string(idZona) +
                ", data_instalare='" + dataInstalare + "' "
                "WHERE id_obiect=" + std::to_string(idObiect));
            return true;
        }
        catch (...) { return false; }
    }

    bool mutaInJunk(int idObiect) {
        try {
            m_db.executa(
                "UPDATE Inventar SET locatie='Junk', "
                "in_junk=1, stare='Casat', data_mutare_junk=GETDATE() "
                "WHERE id_obiect=" + std::to_string(idObiect));
            return true;
        }
        catch (...) { return false; }
    }

    bool stergeDinJunk(int idObiect) {
        try {
            m_db.executa(
                "DELETE FROM Inventar "
                "WHERE id_obiect=" + std::to_string(idObiect) + " AND in_junk=1");
            return true;
        }
        catch (...) { return false; }
    }

    struct InfoObiect {
        int         id;
        std::string tipGeneral;
        std::string subtip;
        std::string stare;
        std::string locatie;
        double      pret;
    };

    std::vector<InfoObiect> getInventarZona(int idZona) {
        std::vector<InfoObiect> result;
        m_db.interogheaza(
            "SELECT i.id_obiect, c.tip_general, c.subtip, "
            "       i.stare, i.locatie, i.pret_achizitie "
            "FROM Inventar i "
            "JOIN CategoriiObiect c ON c.id_categorie=i.id_categorie "
            "WHERE i.id_zona=" + std::to_string(idZona) +
            " AND i.locatie='InUz' ORDER BY c.tip_general, c.subtip",
            [&](SQLHSTMT stmt) {
                InfoObiect o;
                o.id = DatabaseManager::getColumnInt(stmt, 1);
                o.tipGeneral = DatabaseManager::getColumn(stmt, 2);
                o.subtip = DatabaseManager::getColumn(stmt, 3);
                o.stare = DatabaseManager::getColumn(stmt, 4);
                o.locatie = DatabaseManager::getColumn(stmt, 5);
                o.pret = DatabaseManager::getColumnDouble(stmt, 6);
                result.push_back(o);
            });
        return result;
    }

    std::vector<InfoObiect> getJunk() {
        std::vector<InfoObiect> result;
        m_db.interogheaza(
            "SELECT i.id_obiect, c.tip_general, c.subtip, "
            "       i.stare, i.locatie, i.pret_achizitie "
            "FROM Inventar i "
            "JOIN CategoriiObiect c ON c.id_categorie=i.id_categorie "
            "WHERE i.in_junk=1",
            [&](SQLHSTMT stmt) {
                InfoObiect o;
                o.id = DatabaseManager::getColumnInt(stmt, 1);
                o.tipGeneral = DatabaseManager::getColumn(stmt, 2);
                o.subtip = DatabaseManager::getColumn(stmt, 3);
                o.stare = DatabaseManager::getColumn(stmt, 4);
                o.locatie = DatabaseManager::getColumn(stmt, 5);
                o.pret = DatabaseManager::getColumnDouble(stmt, 6);
                result.push_back(o);
            });
        return result;
    }

    int alocaPiesaAngajat(int idAngajat, int idObiect, int idTask) {
        int newId = -1;
        m_db.executa(
            "INSERT INTO InventarAngajat "
            "(id_angajat, id_obiect, id_task, stare_gestiune) "
            "VALUES (" + std::to_string(idAngajat) + "," +
            std::to_string(idObiect) + "," + std::to_string(idTask) + ",'Alocat')");
        m_db.interogheaza("SELECT CAST(SCOPE_IDENTITY() AS INT)",
            [&](SQLHSTMT stmt) {
                newId = DatabaseManager::getColumnInt(stmt, 1);
            });
        return newId;
    }
};

// ============================================================
// EvenimentRepository
// ============================================================
class EvenimentRepository {
    DatabaseManager& m_db;
public:
    explicit EvenimentRepository(DatabaseManager& db) : m_db(db) {}

    int creeaza(int idZona, const std::string& tip,
        const std::string& denumire, const std::string& data,
        const std::string& oraStart, const std::string& oraSfarsit,
        int idFirma) {
        int newId = -1;
        // SQL Server nu permite OUTPUT direct pe tabele cu triggere.
        // Solutie: OUTPUT ... INTO @tabel_temporar, apoi SELECT din el.
        m_db.executa(
            "INSERT INTO Evenimente "
            "(id_zona, tip_eveniment, denumire, data_eveniment, ora_start, ora_sfarsit) "
            "VALUES (" + std::to_string(idZona) + ",'" + tip +
            "','" + denumire + "','" + data +
            "','" + oraStart + "','" + oraSfarsit + "')");
        m_db.interogheaza("SELECT CAST(SCOPE_IDENTITY() AS INT)",
            [&](SQLHSTMT stmt) {
                newId = DatabaseManager::getColumnInt(stmt, 1);
            });
        if (newId > 0) {
            m_db.executa(
                "INSERT INTO ContracteEvenimente "
                "(id_firma, id_eveniment, data_semnare) "
                "VALUES (" + std::to_string(idFirma) +
                "," + std::to_string(newId) + ",GETDATE())");
        }
        return newId;
    }
};

// ============================================================
// RaportRepository
// ============================================================
class RaportRepository {
    DatabaseManager& m_db;
public:
    explicit RaportRepository(DatabaseManager& db) : m_db(db) {}

    int creeaza(int idTask, int idAngajat,
        const std::string& tip, const std::string& descriere) {
        int newId = -1;
        m_db.executa(
            "INSERT INTO Rapoarte "
            "(id_task, id_angajat, tip_raport, descriere) "
            "VALUES (" + std::to_string(idTask) + "," +
            std::to_string(idAngajat) + ",'" + tip + "','" + descriere + "')");
        m_db.interogheaza("SELECT CAST(SCOPE_IDENTITY() AS INT)",
            [&](SQLHSTMT stmt) {
                newId = DatabaseManager::getColumnInt(stmt, 1);
            });
        return newId;
    }
};

// ============================================================
// NotificareRepository
// ============================================================
class NotificareRepository {
    DatabaseManager& m_db;
public:
    explicit NotificareRepository(DatabaseManager& db) : m_db(db) {}

    int trimiteNotificare(int idTask, int idExpeditor,
        int idDestinatar, const std::string& mesaj) {
        int newId = -1;
        m_db.executa(
            "INSERT INTO Notificari "
            "(id_task, id_expeditor, id_destinatar, mesaj) "
            "VALUES (" + std::to_string(idTask) + "," +
            std::to_string(idExpeditor) + "," +
            std::to_string(idDestinatar) + ",'" + mesaj + "')");
        m_db.interogheaza("SELECT CAST(SCOPE_IDENTITY() AS INT)",
            [&](SQLHSTMT stmt) {
                newId = DatabaseManager::getColumnInt(stmt, 1);
            });
        return newId;
    }

    // Returneaza: (id_notif, id_task, mesaj, citita)
    std::vector<std::tuple<int, int, std::string, bool>>
        getNotificariAngajat(int idAngajat) {
        std::vector<std::tuple<int, int, std::string, bool>> result;
        m_db.interogheaza(
            "SELECT id_notif, id_task, mesaj, citita "
            "FROM Notificari WHERE id_destinatar=" + std::to_string(idAngajat) +
            " ORDER BY data_creare DESC",
            [&](SQLHSTMT stmt) {
                int         id_notif = DatabaseManager::getColumnInt(stmt, 1);
                int         id_task = DatabaseManager::getColumnInt(stmt, 2);
                std::string mesaj = DatabaseManager::getColumn(stmt, 3);
                bool        citita = (DatabaseManager::getColumnInt(stmt, 4) == 1);
                result.emplace_back(id_notif, id_task, mesaj, citita);
            });
        return result;
    }

    bool marcheazaCitita(int idNotif) {
        try {
            m_db.executa(
                "UPDATE Notificari SET citita=1, data_citire=GETDATE() "
                "WHERE id_notif=" + std::to_string(idNotif));
            return true;
        }
        catch (...) { return false; }
    }
};

// ============================================================
// AuditRepository
// ============================================================
class AuditRepository {
    DatabaseManager& m_db;
public:
    explicit AuditRepository(DatabaseManager& db) : m_db(db) {}

    void logeaza(int idUser, const std::string& actiune,
        const std::string& tabel, int idInregistrare,
        const std::string& detalii) {
        try {
            m_db.executa(
                "INSERT INTO AuditLog "
                "(id_user, actiune, tabel_afectat, id_inregistrare, detalii) "
                "VALUES (" + (idUser <= 0 ? std::string("NULL") : std::to_string(idUser)) +
                ",'" + actiune + "','" + tabel + "'," +
                std::to_string(idInregistrare) + ",'" + detalii + "')");
        }
        catch (...) { /* Logging nu blocheaza operatiunea */ }
    }
};