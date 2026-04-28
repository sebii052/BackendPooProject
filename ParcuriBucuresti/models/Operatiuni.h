#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "../interfaces/ISerializable.h"
#include "../interfaces/IValidatable.h"
#include "../interfaces/IObservable.h"

enum class StatusTask     { InProgress, Done, Anulat };
enum class TipTask        { Reparatie, Mentenanta };
enum class StatusSesizare { Nerezolvata, Ignorata, TransformataTask };
enum class TipRaport      { Finalizare, Status };

inline std::string statusTaskToString(StatusTask s) {
    switch (s) {
        case StatusTask::InProgress: return "InProgress";
        case StatusTask::Done:       return "Done";
        case StatusTask::Anulat:     return "Anulat";
    }
    return "";
}

inline std::string tipTaskToString(TipTask t) {
    switch (t) {
        case TipTask::Reparatie:  return "Reparatie";
        case TipTask::Mentenanta: return "Mentenanta";
    }
    return "";
}

// ============================================================
// CLASA ABSTRACTA TaskBase
// ============================================================
class TaskBase : public ISerializable,
                 public IValidatable,
                 public IObservable {
protected:
    int         m_idTask;
    int         m_idAngajat;
    int         m_idZona;
    std::string m_descriere;
    double      m_costEstimat;
    std::string m_dataCreare;
    std::string m_deadline;
    StatusTask  m_status;
    std::string m_dataFinalizare;
    TipTask     m_tipTask;

    std::vector<IObserver*> m_observers;
    std::vector<int>        m_idRapoarte;

public:
    TaskBase(int id, int idAngajat, int idZona,
             const std::string& descriere, double costEstimat,
             const std::string& dataCreare, const std::string& deadline,
             TipTask tip)
        : m_idTask(id), m_idAngajat(idAngajat), m_idZona(idZona),
          m_descriere(descriere), m_costEstimat(costEstimat),
          m_dataCreare(dataCreare), m_deadline(deadline),
          m_status(StatusTask::InProgress), m_tipTask(tip) {}

    virtual ~TaskBase() = default;

    virtual std::string getTipDescriptiv() const = 0;

    virtual void setStatus(StatusTask novaStare) {
        m_status = novaStare;
        if (novaStare == StatusTask::Done)
            notifyObservers("TASK_FINALIZAT",
                "Task ID=" + std::to_string(m_idTask) +
                " marcat Done de angajat ID=" +
                std::to_string(m_idAngajat));
    }

    void addObserver(IObserver* obs) override {
        m_observers.push_back(obs);
    }
    void removeObserver(IObserver* obs) override {
        m_observers.erase(
            std::remove(m_observers.begin(), m_observers.end(), obs),
            m_observers.end());
    }
    void notifyObservers(const std::string& ev,
                         const std::string& det) override {
        for (auto* obs : m_observers)
            obs->onNotify(ev, det);
    }

    void adaugaRaport(int idRaport) { m_idRapoarte.push_back(idRaport); }

    int         getIdTask()      const { return m_idTask;      }
    int         getIdAngajat()   const { return m_idAngajat;   }
    int         getIdZona()      const { return m_idZona;      }
    std::string getDescriere()   const { return m_descriere;   }
    double      getCostEstimat() const { return m_costEstimat; }
    std::string getDataCreare()  const { return m_dataCreare;  }
    std::string getDeadline()    const { return m_deadline;    }
    StatusTask  getStatus()      const { return m_status;      }
    TipTask     getTipTask()     const { return m_tipTask;     }
    const std::vector<int>& getRapoarte() const { return m_idRapoarte; }

    bool isValid() const override {
        return m_idTask > 0 && m_idAngajat > 0
               && m_idZona > 0 && !m_descriere.empty()
               && m_costEstimat >= 0;
    }
    std::string getValidationError() const override {
        if (m_idTask <= 0)       return "ID task invalid.";
        if (m_idAngajat <= 0)    return "ID angajat invalid.";
        if (m_idZona <= 0)       return "ID zona invalid.";
        if (m_descriere.empty()) return "Descrierea nu poate fi goala.";
        if (m_costEstimat < 0)   return "Costul nu poate fi negativ.";
        return "";
    }
    std::string toString() const override {
        return "Task[" + std::to_string(m_idTask) + "] " +
               getTipDescriptiv() +
               " | Angajat=" + std::to_string(m_idAngajat) +
               " | Zona=" + std::to_string(m_idZona) +
               " | Status=" + statusTaskToString(m_status) +
               " | Deadline=" + m_deadline;
    }
};

// ============================================================
// CLASA TaskOcazional (Reparatie)
// ============================================================
class TaskOcazional : public TaskBase {
    int m_idSesizare;
    int m_idObiectDefect;
    int m_idObiectInlocuitor;

public:
    TaskOcazional(int id, int idAngajat, int idZona,
                  const std::string& descriere, double costEstimat,
                  const std::string& dataCreare, const std::string& deadline,
                  int idSesizare,
                  int idObiectDefect = -1,
                  int idObiectInlocuitor = -1)
        : TaskBase(id, idAngajat, idZona, descriere, costEstimat,
                   dataCreare, deadline, TipTask::Reparatie),
          m_idSesizare(idSesizare),
          m_idObiectDefect(idObiectDefect),
          m_idObiectInlocuitor(idObiectInlocuitor) {}

    std::string getTipDescriptiv() const override {
        return "Reparatie (din sesizare #" +
               std::to_string(m_idSesizare) + ")";
    }

    int getIdSesizare()         const { return m_idSesizare;         }
    int getIdObiectDefect()     const { return m_idObiectDefect;     }
    int getIdObiectInlocuitor() const { return m_idObiectInlocuitor; }
    void setObiectInlocuitor(int id)  { m_idObiectInlocuitor = id;  }
};

// ============================================================
// CLASA TaskDaily (Mentenanta)
// ============================================================
class TaskDaily : public TaskBase {
    std::string m_tipLucrare;
    std::string m_dataUltimeiExecutii;

public:
    TaskDaily(int id, int idAngajat, int idZona,
              const std::string& descriere, double costEstimat,
              const std::string& dataCreare, const std::string& deadline,
              const std::string& tipLucrare,
              const std::string& dataUltimaExecutie = "")
        : TaskBase(id, idAngajat, idZona, descriere, costEstimat,
                   dataCreare, deadline, TipTask::Mentenanta),
          m_tipLucrare(tipLucrare),
          m_dataUltimeiExecutii(dataUltimaExecutie) {}

    std::string getTipDescriptiv() const override {
        return "Mentenanta zilnica [" + m_tipLucrare + "]";
    }
    std::string getTipLucrare()        const { return m_tipLucrare;          }
    std::string getDataUltimaExecutie() const { return m_dataUltimeiExecutii; }
    void setDataUltimaExecutie(const std::string& d) { m_dataUltimeiExecutii = d; }
};

// ============================================================
// CLASA Sesizare
// ============================================================
class Sesizare : public ISerializable, public IValidatable {
    int            m_idSesizare;
    int            m_idZona;
    int            m_idUser;
    std::string    m_descriere;
    StatusSesizare m_status;
    std::string    m_dataCreare;
    std::string    m_dataProcesare;

public:
    Sesizare(int id, int idZona, int idUser,
             const std::string& descriere, const std::string& dataCreare)
        : m_idSesizare(id), m_idZona(idZona), m_idUser(idUser),
          m_descriere(descriere), m_status(StatusSesizare::Nerezolvata),
          m_dataCreare(dataCreare) {}

    void ignora(const std::string& dataProcesare) {
        m_status        = StatusSesizare::Ignorata;
        m_dataProcesare = dataProcesare;
    }
    void transformaInTask(const std::string& dataProcesare) {
        if (m_status != StatusSesizare::Nerezolvata)
            throw std::runtime_error("Sesizarea a fost deja procesata.");
        m_status        = StatusSesizare::TransformataTask;
        m_dataProcesare = dataProcesare;
    }

    bool           esteDelaGuest()    const { return m_idUser == -1;     }
    int            getIdSesizare()    const { return m_idSesizare;       }
    int            getIdZona()        const { return m_idZona;           }
    int            getIdUser()        const { return m_idUser;           }
    std::string    getDescriere()     const { return m_descriere;        }
    StatusSesizare getStatus()        const { return m_status;           }
    std::string    getDataCreare()    const { return m_dataCreare;       }
    std::string    getDataProcesare() const { return m_dataProcesare;    }

    bool isValid() const override {
        return m_idSesizare > 0 && m_idZona > 0 && !m_descriere.empty();
    }
    std::string getValidationError() const override {
        if (m_idSesizare <= 0)   return "ID sesizare invalid.";
        if (m_idZona <= 0)       return "ID zona invalid.";
        if (m_descriere.empty()) return "Descrierea nu poate fi goala.";
        return "";
    }
    std::string toString() const override {
        return "Sesizare[" + std::to_string(m_idSesizare) + "]" +
               " Zona=" + std::to_string(m_idZona) +
               " De la: " + (esteDelaGuest() ? "Guest" :
                   "User#" + std::to_string(m_idUser));
    }
};

// ============================================================
// CLASA Raport
// ============================================================
class Raport : public ISerializable, public IValidatable {
    int         m_idRaport;
    int         m_idTask;
    int         m_idAngajat;
    TipRaport   m_tipRaport;
    std::string m_descriere;
    std::string m_dataCreare;

public:
    Raport(int id, int idTask, int idAngajat,
           TipRaport tip, const std::string& descriere,
           const std::string& dataCreare)
        : m_idRaport(id), m_idTask(idTask), m_idAngajat(idAngajat),
          m_tipRaport(tip), m_descriere(descriere), m_dataCreare(dataCreare) {}

    int         getIdRaport()   const { return m_idRaport;  }
    int         getIdTask()     const { return m_idTask;    }
    int         getIdAngajat()  const { return m_idAngajat; }
    TipRaport   getTipRaport()  const { return m_tipRaport; }
    std::string getDescriere()  const { return m_descriere; }
    std::string getDataCreare() const { return m_dataCreare;}

    bool isValid() const override {
        return m_idRaport > 0 && m_idTask > 0
               && m_idAngajat > 0 && !m_descriere.empty();
    }
    std::string getValidationError() const override {
        if (m_descriere.empty()) return "Continutul raportului nu poate fi gol.";
        return "";
    }
    std::string toString() const override {
        return "Raport[" + std::to_string(m_idRaport) + "]" +
               " Tip=" + (m_tipRaport == TipRaport::Finalizare ? "Finalizare" : "Status") +
               " Task=" + std::to_string(m_idTask) +
               " Angajat=" + std::to_string(m_idAngajat);
    }
};

// ============================================================
// CLASA Notificare
// Admin -> Angajat: cere status lucrare
// ============================================================
class Notificare : public ISerializable {
    int         m_idNotif;
    int         m_idTask;
    int         m_idExpeditor;
    int         m_idDestinatar;
    std::string m_mesaj;
    bool        m_citita;
    std::string m_dataCreare;
    std::string m_dataCitire;

public:
    Notificare(int id, int idTask, int idExpeditor,
               int idDestinatar, const std::string& mesaj,
               const std::string& dataCreare)
        : m_idNotif(id), m_idTask(idTask),
          m_idExpeditor(idExpeditor), m_idDestinatar(idDestinatar),
          m_mesaj(mesaj), m_citita(false), m_dataCreare(dataCreare) {}

    void marcheazaCitita(const std::string& dataCitire) {
        m_citita     = true;
        m_dataCitire = dataCitire;
    }

    int         getIdNotif()      const { return m_idNotif;      }
    int         getIdTask()       const { return m_idTask;       }
    int         getIdExpeditor()  const { return m_idExpeditor;  }
    int         getIdDestinatar() const { return m_idDestinatar; }
    std::string getMesaj()        const { return m_mesaj;        }
    bool        esteCitita()      const { return m_citita;       }
    std::string getDataCreare()   const { return m_dataCreare;   }

    std::string toString() const override {
        return "Notificare[" + std::to_string(m_idNotif) + "]" +
               " De la Admin#" + std::to_string(m_idExpeditor) +
               " catre Angajat#" + std::to_string(m_idDestinatar) +
               " | " + (m_citita ? "Citita" : "Necitita");
    }
};
