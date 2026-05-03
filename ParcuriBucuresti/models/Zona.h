#pragma once
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include "../interfaces/ISerializable.h"
#include "../interfaces/IValidatable.h"
#include "Obiecte.h"

enum class TipZona {
    Lac, Alee, LocJoaca,
    PistaBicicleta, Terenuri,
    ZonaVerde, SpatiuCaini
};

inline std::string tipZonaToString(TipZona t) {
    switch (t) {
        case TipZona::Lac:            return "Lac";
        case TipZona::Alee:           return "Alee";
        case TipZona::LocJoaca:       return "LocJoaca";
        case TipZona::PistaBicicleta: return "PistaBicicleta";
        case TipZona::Terenuri:       return "Terenuri";
        case TipZona::ZonaVerde:      return "ZonaVerde";
        case TipZona::SpatiuCaini:    return "SpatiuCaini";
    }
    return "Necunoscut";
}

// ============================================================
// CLASA Zona
// ============================================================
class Zona : public ISerializabil, public IValidatabil {
private:
    int         m_idZona;
    int         m_idParc;
    std::string m_numeZona;
    TipZona     m_tipZona;
    double      m_suprafataMP;
    std::string m_descriere;

    std::vector<std::unique_ptr<Obiect>> m_obiecte;

public:
    Zona(int id, int idParc,
         const std::string& nume, TipZona tip,
         double suprafata, const std::string& descriere = "")
        : m_idZona(id), m_idParc(idParc), m_numeZona(nume),
          m_tipZona(tip), m_suprafataMP(suprafata),
          m_descriere(descriere) {}

    Zona(const Zona&)            = delete;
    Zona& operator=(const Zona&) = delete;
    Zona(Zona&&)                 = default;
    Zona& operator=(Zona&&)      = default;

    void adaugaObiect(std::unique_ptr<Obiect> obj) {
        m_obiecte.push_back(std::move(obj));
    }

    std::unique_ptr<Obiect> scoateObiect(int idObiect) {
        auto it = std::find_if(m_obiecte.begin(), m_obiecte.end(),
            [idObiect](const std::unique_ptr<Obiect>& o) {
                return o->getIdObiect() == idObiect;
            });
        if (it == m_obiecte.end())
            throw std::runtime_error("Obiect ID=" +
                std::to_string(idObiect) + " negasit in zona.");
        std::unique_ptr<Obiect> obj = std::move(*it);
        m_obiecte.erase(it);
        return obj;
    }

    std::vector<const Obiect*> getObiecte() const {
        std::vector<const Obiect*> result;
        for (const auto& o : m_obiecte)
            result.push_back(o.get());
        return result;
    }

    std::vector<const Obiect*> getObiecteDefecte() const {
        std::vector<const Obiect*> result;
        for (const auto& o : m_obiecte)
            if (o->getStare() == StareObiect::Defect)
                result.push_back(o.get());
        return result;
    }

    int         getIdZona()    const { return m_idZona;      }
    int         getIdParc()    const { return m_idParc;      }
    std::string getNumeZona()  const { return m_numeZona;    }
    TipZona     getTipZona()   const { return m_tipZona;     }
    double      getSuprafata() const { return m_suprafataMP; }
    std::string getDescriere() const { return m_descriere;   }
    int         getNrObiecte() const { return static_cast<int>(m_obiecte.size()); }

    bool isValid() const override {
        return m_idZona > 0 && !m_numeZona.empty() && m_suprafataMP > 0;
    }
    std::string getValidationError() const override {
        if (m_idZona <= 0)      return "ID zona invalid.";
        if (m_numeZona.empty()) return "Numele zonei nu poate fi gol.";
        if (m_suprafataMP <= 0) return "Suprafata trebuie sa fie pozitiva.";
        return "";
    }
    std::string toString() const override {
        return "Zona[" + std::to_string(m_idZona) + "] " +
               m_numeZona + " (" + tipZonaToString(m_tipZona) + ")" +
               " | " + std::to_string(m_obiecte.size()) + " obiecte";
    }
};

// ============================================================
// CLASA Parc
// ============================================================
class Parc : public ISerializabil, public IValidatabil {
private:
    int         m_idParc;
    std::string m_nume;
    std::string m_adresa;
    double      m_suprafataMP;
    int         m_sector;
    std::string m_descriere;
    bool        m_activ;

    std::vector<std::unique_ptr<Zona>> m_zone;

public:
    Parc(int id, const std::string& nume, const std::string& adresa,
         double suprafata, int sector,
         const std::string& descriere = "", bool activ = true)
        : m_idParc(id), m_nume(nume), m_adresa(adresa),
          m_suprafataMP(suprafata), m_sector(sector),
          m_descriere(descriere), m_activ(activ) {}

    Parc(const Parc&)            = delete;
    Parc& operator=(const Parc&) = delete;
    Parc(Parc&&)                 = default;
    Parc& operator=(Parc&&)      = default;

    void adaugaZona(std::unique_ptr<Zona> zona) {
        m_zone.push_back(std::move(zona));
    }

    Zona* getZona(int idZona) const {
        for (const auto& z : m_zone)
            if (z->getIdZona() == idZona)
                return z.get();
        return nullptr;
    }

    std::vector<const Zona*> getZone() const {
        std::vector<const Zona*> result;
        for (const auto& z : m_zone)
            result.push_back(z.get());
        return result;
    }

    std::vector<const Zona*> getZoneDupaTip(TipZona tip) const {
        std::vector<const Zona*> result;
        for (const auto& z : m_zone)
            if (z->getTipZona() == tip)
                result.push_back(z.get());
        return result;
    }

    int         getIdParc()    const { return m_idParc;      }
    std::string getNume()      const { return m_nume;        }
    std::string getAdresa()    const { return m_adresa;      }
    double      getSuprafata() const { return m_suprafataMP; }
    int         getSector()    const { return m_sector;      }
    bool        esteActiv()    const { return m_activ;       }
    int         getNrZone()    const { return static_cast<int>(m_zone.size()); }

    void setActiv(bool a) { m_activ = a; }

    bool isValid() const override {
        return m_idParc > 0 && !m_nume.empty()
               && m_sector >= 1 && m_sector <= 6;
    }
    std::string getValidationError() const override {
        if (m_idParc <= 0)                return "ID parc invalid.";
        if (m_nume.empty())               return "Numele parcului nu poate fi gol.";
        if (m_sector < 1 || m_sector > 6) return "Sectorul trebuie sa fie intre 1 si 6.";
        return "";
    }
    std::string toString() const override {
        return "Parc[" + std::to_string(m_idParc) + "] " +
               m_nume + " | Sector " + std::to_string(m_sector) +
               " | " + std::to_string(m_zone.size()) + " zone";
    }
};
