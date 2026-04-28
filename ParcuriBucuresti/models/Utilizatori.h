#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "../interfaces/ISerializable.h"
#include "../interfaces/IValidatable.h"
#include "../interfaces/IObservable.h"

// ============================================================
// CLASA ABSTRACTA User
// ============================================================
class User : public ISerializable, public IValidatable {
protected:
    int         m_idUser;
    std::string m_rol;

public:
    explicit User(int id, const std::string& rol)
        : m_idUser(id), m_rol(rol) {}

    virtual ~User() = default;

    virtual bool        poateCreaTask()      const = 0;
    virtual bool        poateCreaRaport()    const = 0;
    virtual bool        poateCreaEveniment() const = 0;
    virtual bool        poateVedeaInventar() const = 0;
    virtual std::string getActiunePrincipala() const = 0;

    int         getIdUser() const { return m_idUser; }
    std::string getRol()    const { return m_rol;    }

    bool isValid() const override { return m_idUser >= 0; }
    std::string getValidationError() const override {
        if (m_idUser < 0) return "ID utilizator invalid.";
        return "";
    }
};

// ============================================================
// CLASA Guest
// ============================================================
class Guest : public User {
public:
    Guest() : User(-1, "Guest") {}

    bool poateCreaTask()      const override { return false; }
    bool poateCreaRaport()    const override { return false; }
    bool poateCreaEveniment() const override { return false; }
    bool poateVedeaInventar() const override { return false; }
    std::string getActiunePrincipala() const override {
        return "Creare sesizare";
    }
    bool isValid() const override { return true; }
    std::string toString() const override {
        return "[Guest] Utilizator anonim";
    }
};

// ============================================================
// CLASA ABSTRACTA AuthorizedUser
// ============================================================
class AuthorizedUser : public User, public IObserver {
protected:
    std::string      m_username;
    std::string      m_parolaHash;
    std::string      m_nume;
    std::string      m_prenume;
    std::string      m_email;
    std::string      m_telefon;
    bool             m_activ;
    std::vector<int> m_notificariNecitite;

public:
    AuthorizedUser(int id, const std::string& rol,
                   const std::string& username,
                   const std::string& parolaHash,
                   const std::string& nume,
                   const std::string& prenume,
                   const std::string& email,
                   const std::string& telefon = "",
                   bool activ = true)
        : User(id, rol),
          m_username(username), m_parolaHash(parolaHash),
          m_nume(nume), m_prenume(prenume),
          m_email(email), m_telefon(telefon), m_activ(activ) {}

    void onNotify(const std::string& eveniment,
                  const std::string& detalii) override {
        (void)eveniment;
        (void)detalii;
        m_notificariNecitite.push_back(
            static_cast<int>(m_notificariNecitite.size() + 1));
    }

    bool verificaParola(const std::string& parolaHash) const {
        return m_parolaHash == parolaHash;
    }

    std::string getUsername()    const { return m_username;  }
    std::string getNume()        const { return m_nume;      }
    std::string getPrenume()     const { return m_prenume;   }
    std::string getNumeComplet() const { return m_prenume + " " + m_nume; }
    std::string getEmail()       const { return m_email;     }
    std::string getTelefon()     const { return m_telefon;   }
    bool        esteActiv()      const { return m_activ;     }
    int         getNrNotificari() const {
        return static_cast<int>(m_notificariNecitite.size());
    }

    void setActiv(bool a)                    { m_activ      = a; }
    void setParolaHash(const std::string& h) { m_parolaHash = h; }

    bool isValid() const override {
        return m_idUser > 0 && !m_username.empty()
               && !m_parolaHash.empty() && !m_email.empty();
    }
    std::string getValidationError() const override {
        if (m_idUser <= 0)        return "ID utilizator invalid.";
        if (m_username.empty())   return "Username-ul nu poate fi gol.";
        if (m_parolaHash.empty()) return "Parola nu poate fi goala.";
        if (m_email.empty())      return "Email-ul nu poate fi gol.";
        return "";
    }
};

// ============================================================
// CLASA Angajat
// ============================================================
class Angajat : public AuthorizedUser {
private:
    int              m_idZonaAlocata;
    int              m_idTaskCurent;
    std::string      m_dataAngajare;
    std::vector<int> m_istoricTaskuri;
    std::vector<int> m_istoricSesizari;
    std::vector<int> m_istoricRapoarte;
    std::vector<int> m_idGestiuneObiecte;

public:
    Angajat(int id,
            const std::string& username,
            const std::string& parolaHash,
            const std::string& nume,
            const std::string& prenume,
            const std::string& email,
            int idZonaAlocata,
            const std::string& dataAngajare,
            const std::string& telefon = "")
        : AuthorizedUser(id, "Angajat", username, parolaHash,
                         nume, prenume, email, telefon),
          m_idZonaAlocata(idZonaAlocata),
          m_idTaskCurent(-1),
          m_dataAngajare(dataAngajare) {}

    bool poateCreaTask()      const override { return false; }
    bool poateCreaRaport()    const override { return true;  }
    bool poateCreaEveniment() const override { return false; }
    bool poateVedeaInventar() const override { return true;  }
    std::string getActiunePrincipala() const override {
        return "Executare taskuri si creare rapoarte";
    }

    void preiaTask(int idTask) {
        if (m_idTaskCurent != -1)
            throw std::runtime_error(
                "Angajatul " + m_username +
                " are deja un task activ (ID=" +
                std::to_string(m_idTaskCurent) + ").");
        m_idTaskCurent = idTask;
        m_istoricTaskuri.push_back(idTask);
    }

    void finalizeazaTask() {
        if (m_idTaskCurent == -1)
            throw std::runtime_error("Niciun task activ de finalizat.");
        m_idTaskCurent = -1;
    }

    void adaugaInGestiune(int idGestiune) {
        m_idGestiuneObiecte.push_back(idGestiune);
    }
    void scoateDinGestiune(int idGestiune) {
        auto it = std::find(m_idGestiuneObiecte.begin(),
                            m_idGestiuneObiecte.end(), idGestiune);
        if (it != m_idGestiuneObiecte.end())
            m_idGestiuneObiecte.erase(it);
    }

    void adaugaSesizare(int id) { m_istoricSesizari.push_back(id); }
    void adaugaRaport(int id)   { m_istoricRapoarte.push_back(id); }

    int         getIdZonaAlocata() const { return m_idZonaAlocata;     }
    int         getIdTaskCurent()  const { return m_idTaskCurent;      }
    bool        esteLiber()        const { return m_idTaskCurent == -1; }
    std::string getDataAngajare()  const { return m_dataAngajare;      }

    const std::vector<int>& getIstoricTaskuri()  const { return m_istoricTaskuri;    }
    const std::vector<int>& getIstoricSesizari() const { return m_istoricSesizari;   }
    const std::vector<int>& getIstoricRapoarte() const { return m_istoricRapoarte;   }
    const std::vector<int>& getGestiuneObiecte() const { return m_idGestiuneObiecte; }

    std::string toString() const override {
        return "[Angajat] " + getNumeComplet() +
               " (@" + m_username + ")" +
               " | Zona: " + std::to_string(m_idZonaAlocata) +
               " | Status: " + (esteLiber() ? "Liber" :
                   "Ocupat (TaskID=" + std::to_string(m_idTaskCurent) + ")");
    }
};

// ============================================================
// CLASA Admin
// ============================================================
class Admin : public AuthorizedUser {
private:
    std::vector<int> m_idTaskuriCreate;
    std::vector<int> m_idEvenimenteCreate;

public:
    Admin(int id,
          const std::string& username,
          const std::string& parolaHash,
          const std::string& nume,
          const std::string& prenume,
          const std::string& email,
          const std::string& telefon = "")
        : AuthorizedUser(id, "Admin", username, parolaHash,
                         nume, prenume, email, telefon) {}

    bool poateCreaTask()      const override { return true;  }
    bool poateCreaRaport()    const override { return false; }
    bool poateCreaEveniment() const override { return true;  }
    bool poateVedeaInventar() const override { return true;  }
    std::string getActiunePrincipala() const override {
        return "Management complet parcuri, taskuri, evenimente";
    }

    void inregistreazaTaskCreat(int id)      { m_idTaskuriCreate.push_back(id);    }
    void inregistreazaEvenimentCreat(int id) { m_idEvenimenteCreate.push_back(id); }

    const std::vector<int>& getTaskuriCreate()    const { return m_idTaskuriCreate;    }
    const std::vector<int>& getEvenimenteCreate() const { return m_idEvenimenteCreate; }

    std::string toString() const override {
        return "[Admin] " + getNumeComplet() + " (@" + m_username + ")";
    }
};
