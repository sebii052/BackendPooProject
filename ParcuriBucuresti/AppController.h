#pragma once
#include <string>
#include <stdexcept>
#include <optional>
#include "db/DatabaseManager.h"
#include "db/Repositories.h"
#include "utils/HashUtils.h"
#include "models/Utilizatori.h"

// ============================================================
// CLASA AppController
// Punctul central al logicii de business.
// Folosit de ClientHandler (server) pentru toate operatiunile.
// Fiecare conexiune client are propriul AppController,
// deci sesiunea (user logat, rol) e izolata per client.
// ============================================================
class AppController {
private:
    DatabaseManager&     m_db;
    UtilizatorRepository m_utilizatorRepo;
    SesizareRepository   m_sesizareRepo;
    TaskRepository       m_taskRepo;
    InventarRepository   m_inventarRepo;
    EvenimentRepository  m_evenimentRepo;
    RaportRepository     m_raportRepo;
    NotificareRepository m_notificareRepo;
    AuditRepository      m_auditRepo;

    std::string m_usernameCurent;
    std::string m_rolCurent;
    int         m_idUserCurent = -1;

    void m_auditLog(const std::string& actiune, int idInregistrare) {
        m_auditRepo.logeaza(m_idUserCurent, actiune,
                            "Inventar", idInregistrare, "");
    }

public:
    explicit AppController(DatabaseManager& db)
        : m_db(db),
          m_utilizatorRepo(db), m_sesizareRepo(db),
          m_taskRepo(db),       m_inventarRepo(db),
          m_evenimentRepo(db),  m_raportRepo(db),
          m_notificareRepo(db), m_auditRepo(db) {}

    // --------------------------------------------------------
    // AUTENTIFICARE
    // --------------------------------------------------------
    bool login(const std::string& username,
               const std::string& parola) {
        std::string hash = HashUtils::sha256(parola);
        std::string rol;

        if (!m_utilizatorRepo.verificaLogin(username, hash, rol))
            return false;

        m_usernameCurent = username;
        m_rolCurent      = rol;
        m_idUserCurent   = m_utilizatorRepo.getIdAngajat(username).value_or(-1);

        m_auditRepo.logeaza(m_idUserCurent, "LOGIN",
                            "Utilizatori", m_idUserCurent,
                            "Login reusit: " + username);
        return true;
    }

    void logout() {
        m_auditRepo.logeaza(m_idUserCurent, "LOGOUT",
                            "Utilizatori", m_idUserCurent,
                            "Logout: " + m_usernameCurent);
        m_usernameCurent = "";
        m_rolCurent      = "";
        m_idUserCurent   = -1;
    }

    bool registerAngajat(const std::string& username,
                          const std::string& parola,
                          const std::string& nume,
                          const std::string& prenume,
                          const std::string& email,
                          int idZona) {
        if (!esteAdmin())
            throw std::runtime_error("Doar adminul poate crea conturi.");

        std::string hash = HashUtils::sha256(parola);
        bool ok = m_utilizatorRepo.inregistreazaAngajat(
            username, hash, nume, prenume, email, idZona);

        if (ok)
            m_auditRepo.logeaza(m_idUserCurent,
                "CREARE_CONT_ANGAJAT", "Utilizatori", -1,
                "Cont creat pentru: " + username);
        return ok;
    }

    // --------------------------------------------------------
    // GETTERI SESIUNE
    // --------------------------------------------------------
    std::string getRolCurent()      const { return m_rolCurent;             }
    std::string getUsernameCurent() const { return m_usernameCurent;        }
    int         getIdUserCurent()   const { return m_idUserCurent;          }
    bool        esteLogat()         const { return !m_rolCurent.empty();    }
    bool        esteAdmin()         const { return m_rolCurent == "Admin";  }
    bool        esteAngajat()       const { return m_rolCurent == "Angajat";}

    // --------------------------------------------------------
    // SESIZARI
    // --------------------------------------------------------
    int creeazaSesizare(int idZona, const std::string& descriere) {
        return m_sesizareRepo.creeaza(idZona, m_idUserCurent, descriere);
    }

    auto getSesizariNerezolvate() {
        if (!esteAdmin())
            throw std::runtime_error("Doar adminul vede toate sesizarile.");
        return m_sesizareRepo.getSesizariNerezolvate();
    }

    bool ignoraSesizare(int idSesizare) {
        if (!esteAdmin())
            throw std::runtime_error("Doar adminul poate ignora sesizari.");
        bool ok = m_sesizareRepo.ignora(idSesizare);
        if (ok)
            m_auditRepo.logeaza(m_idUserCurent, "IGNORA_SESIZARE",
                "Sesizari", idSesizare, "Sesizare ignorata de admin.");
        return ok;
    }

    // --------------------------------------------------------
    // TASKURI
    // --------------------------------------------------------
    int creeazaTaskDinSesizare(int idAngajat, int idZona,
                                int idSesizare,
                                const std::string& descriere,
                                double cost,
                                const std::string& deadline) {
        if (!esteAdmin())
            throw std::runtime_error("Doar adminul creeaza taskuri.");

        int id = m_taskRepo.creeazaOcazional(
            idAngajat, idZona, idSesizare, descriere, cost, deadline);

        if (id > 0) {
            m_sesizareRepo.marcheazaTransformata(idSesizare);
            m_auditRepo.logeaza(m_idUserCurent,
                "CREARE_TASK_REPARATIE", "Taskuri", id,
                "Task din sesizare #" + std::to_string(idSesizare));
        }
        return id;
    }

    int creeazaTaskDaily(int idAngajat, int idZona,
                          const std::string& descriere,
                          double cost, const std::string& deadline) {
        if (!esteAdmin())
            throw std::runtime_error("Doar adminul creeaza taskuri.");

        int id = m_taskRepo.creeazaDaily(
            idAngajat, idZona, descriere, cost, deadline);

        if (id > 0)
            m_auditRepo.logeaza(m_idUserCurent,
                "CREARE_TASK_MENTENANTA", "Taskuri", id,
                "Task zilnic creat de admin.");
        return id;
    }

    bool finalizeazaTask(int idTask) {
        if (!esteAngajat())
            throw std::runtime_error("Doar angajatul isi finalizeaza taskul.");

        bool ok = m_taskRepo.actualizeazaStatus(idTask, "Done");
        if (ok) {
            m_raportRepo.creeaza(idTask, m_idUserCurent,
                "Finalizare", "Task finalizat de angajat.");
            m_auditRepo.logeaza(m_idUserCurent,
                "FINALIZARE_TASK", "Taskuri", idTask, "");
        }
        return ok;
    }

    auto getTaskuriMele() {
        if (!esteAngajat())
            throw std::runtime_error("Doar angajatii au taskuri proprii.");
        return m_taskRepo.getTaskuriAngajat(m_idUserCurent);
    }

    // --------------------------------------------------------
    // NOTIFICARI
    // --------------------------------------------------------
    int cerStatusTask(int idTask, int idAngajat,
                       const std::string& mesaj) {
        if (!esteAdmin())
            throw std::runtime_error("Doar adminul trimite notificari de status.");
        return m_notificareRepo.trimiteNotificare(
            idTask, m_idUserCurent, idAngajat, mesaj);
    }

    auto getNotificariMele() {
        return m_notificareRepo.getNotificariAngajat(m_idUserCurent);
    }

    bool marcheazaNotificareCitita(int idNotif) {
        return m_notificareRepo.marcheazaCitita(idNotif);
    }

    // --------------------------------------------------------
    // RAPOARTE
    // --------------------------------------------------------
    int creeazaRaportStatus(int idTask, const std::string& descriere) {
        if (!esteAngajat())
            throw std::runtime_error("Doar angajatii creeaza rapoarte.");
        return m_raportRepo.creeaza(
            idTask, m_idUserCurent, "Status", descriere);
    }

    // --------------------------------------------------------
    // INVENTAR
    // --------------------------------------------------------
    int adaugaInDepozit(int idCategorie, int cantitate,
                         double pret, const std::string& dataAchizitie,
                         int idFurnizor = -1) {
        if (!esteAdmin())
            throw std::runtime_error("Doar adminul adauga in depozit.");
        int id = m_inventarRepo.adaugaInDepozit(
            idCategorie, idFurnizor, cantitate, pret, dataAchizitie);
        m_auditRepo.logeaza(m_idUserCurent, "ADD_INVENTAR",
            "Inventar", id, "Adaugat in depozit. Cantitate=" +
            std::to_string(cantitate));
        return id;
    }

    bool mutaInJunk(int idObiect) {
        if (!esteAdmin())
            throw std::runtime_error("Doar adminul muta in junk.");
        bool ok = m_inventarRepo.mutaInJunk(idObiect);
        if (ok) m_auditLog("MUTA_JUNK", idObiect);
        return ok;
    }

    bool stergeDinJunk(int idObiect) {
        if (!esteAdmin())
            throw std::runtime_error("Doar adminul sterge din junk.");
        bool ok = m_inventarRepo.stergeDinJunk(idObiect);
        if (ok)
            m_auditRepo.logeaza(m_idUserCurent, "DELETE_JUNK",
                "Inventar", idObiect, "Sters permanent din junk.");
        return ok;
    }

    auto getInventarZona(int idZona) {
        return m_inventarRepo.getInventarZona(idZona);
    }

    auto getJunk() {
        if (!esteAdmin())
            throw std::runtime_error("Doar adminul vede junk-ul.");
        return m_inventarRepo.getJunk();
    }

    auto getAngajatiCuStatus() {
        if (!esteAdmin())
            throw std::runtime_error("Doar adminul vede lista angajatilor.");
        return m_utilizatorRepo.getAngajatiCuStatus();
    }

    // --------------------------------------------------------
    // EVENIMENTE
    // --------------------------------------------------------
    int creeazaEveniment(int idZona, const std::string& tip,
                          const std::string& denumire,
                          const std::string& data,
                          const std::string& oraStart,
                          const std::string& oraSfarsit,
                          int idFirmaOrganizator) {
        if (!esteAdmin())
            throw std::runtime_error("Doar adminul creeaza evenimente.");

        int id = m_evenimentRepo.creeaza(
            idZona, tip, denumire, data,
            oraStart, oraSfarsit, idFirmaOrganizator);

        if (id > 0)
            m_auditRepo.logeaza(m_idUserCurent, "CREARE_EVENIMENT",
                "Evenimente", id,
                denumire + " in zona " + std::to_string(idZona));
        return id;
    }
};
