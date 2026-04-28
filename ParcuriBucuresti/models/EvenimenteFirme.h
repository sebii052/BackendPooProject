#pragma once
#include <string>
#include <vector>
#include "../interfaces/ISerializable.h"
#include "../interfaces/IValidatable.h"
#include "../interfaces/IFirma.h"

// ============================================================
// CLASA ABSTRACTA Eveniment
// ============================================================
class Eveniment : public ISerializable, public IValidatable {
protected:
    int         m_idEveniment;
    int         m_idZona;
    std::string m_denumire;
    std::string m_dataEveniment;
    std::string m_oraStart;
    std::string m_oraSfarsit;
    int         m_idFirmaOrganizator;
    std::string m_dataCreare;

public:
    Eveniment(int id, int idZona,
              const std::string& denumire,
              const std::string& dataEveniment,
              const std::string& oraStart,
              const std::string& oraSfarsit,
              int idFirma, const std::string& dataCreare)
        : m_idEveniment(id), m_idZona(idZona), m_denumire(denumire),
          m_dataEveniment(dataEveniment), m_oraStart(oraStart),
          m_oraSfarsit(oraSfarsit), m_idFirmaOrganizator(idFirma),
          m_dataCreare(dataCreare) {}

    virtual ~Eveniment() = default;

    virtual std::string getTipEveniment()    const = 0;
    virtual std::string getCerinteSpeciale() const = 0;

    bool seSuprapuneCu(const Eveniment& alt) const {
        if (m_idZona != alt.m_idZona) return false;
        if (m_dataEveniment != alt.m_dataEveniment) return false;
        return m_oraStart < alt.m_oraSfarsit
               && m_oraSfarsit > alt.m_oraStart;
    }

    int         getIdEveniment()        const { return m_idEveniment;        }
    int         getIdZona()             const { return m_idZona;             }
    std::string getDenumire()           const { return m_denumire;           }
    std::string getDataEveniment()      const { return m_dataEveniment;      }
    std::string getOraStart()           const { return m_oraStart;           }
    std::string getOraSfarsit()         const { return m_oraSfarsit;         }
    int         getIdFirmaOrganizator() const { return m_idFirmaOrganizator; }

    bool isValid() const override {
        return m_idEveniment > 0 && m_idZona > 0
               && !m_denumire.empty() && !m_dataEveniment.empty()
               && !m_oraStart.empty() && !m_oraSfarsit.empty()
               && m_oraStart < m_oraSfarsit;
    }
    std::string getValidationError() const override {
        if (m_denumire.empty())         return "Denumirea evenimentului nu poate fi goala.";
        if (m_dataEveniment.empty())    return "Data evenimentului este obligatorie.";
        if (m_oraStart >= m_oraSfarsit) return "Ora de sfarsit trebuie sa fie dupa ora de start.";
        return "";
    }
    std::string toString() const override {
        return "Eveniment[" + std::to_string(m_idEveniment) + "] " +
               getTipEveniment() + ": " + m_denumire +
               " | " + m_dataEveniment +
               " " + m_oraStart + "-" + m_oraSfarsit;
    }
};

class Film : public Eveniment {
    std::string m_titluFilm;
    std::string m_regizor;
    int         m_durataMinute;
    bool        m_areEcranExterior;
public:
    Film(int id, int idZona, const std::string& denumire,
         const std::string& data, const std::string& oraStart,
         const std::string& oraSfarsit, int idFirma,
         const std::string& dataCreare,
         const std::string& titlu, const std::string& regizor,
         int durata, bool ecranExterior = true)
        : Eveniment(id, idZona, denumire, data, oraStart,
                    oraSfarsit, idFirma, dataCreare),
          m_titluFilm(titlu), m_regizor(regizor),
          m_durataMinute(durata), m_areEcranExterior(ecranExterior) {}
    std::string getTipEveniment() const override { return "Film"; }
    std::string getCerinteSpeciale() const override {
        return m_areEcranExterior
               ? "Ecran exterior, generator, scaune"
               : "Proiector portabil";
    }
    std::string getTitluFilm()     const { return m_titluFilm;        }
    std::string getRegizor()       const { return m_regizor;          }
    int         getDurata()        const { return m_durataMinute;     }
    bool        areEcranExterior() const { return m_areEcranExterior; }
};

class Concert : public Eveniment {
    std::string m_artist;
    std::string m_gen;
    int         m_capacitateEstimata;
public:
    Concert(int id, int idZona, const std::string& denumire,
            const std::string& data, const std::string& oraStart,
            const std::string& oraSfarsit, int idFirma,
            const std::string& dataCreare,
            const std::string& artist, const std::string& gen, int capacitate)
        : Eveniment(id, idZona, denumire, data, oraStart,
                    oraSfarsit, idFirma, dataCreare),
          m_artist(artist), m_gen(gen), m_capacitateEstimata(capacitate) {}
    std::string getTipEveniment() const override { return "Concert"; }
    std::string getCerinteSpeciale() const override {
        return "Scena, instalatie sunet, iluminat scenic";
    }
    std::string getArtist()     const { return m_artist;              }
    std::string getGen()        const { return m_gen;                 }
    int         getCapacitate() const { return m_capacitateEstimata;  }
};

class FoodFestival : public Eveniment {
    int         m_nrStanduri;
    std::string m_tematica;
public:
    FoodFestival(int id, int idZona, const std::string& denumire,
                 const std::string& data, const std::string& oraStart,
                 const std::string& oraSfarsit, int idFirma,
                 const std::string& dataCreare,
                 int nrStanduri, const std::string& tematica)
        : Eveniment(id, idZona, denumire, data, oraStart,
                    oraSfarsit, idFirma, dataCreare),
          m_nrStanduri(nrStanduri), m_tematica(tematica) {}
    std::string getTipEveniment() const override { return "FoodFestival"; }
    std::string getCerinteSpeciale() const override {
        return "Corturi, mese, acces utilitati pentru " +
               std::to_string(m_nrStanduri) + " standuri";
    }
    int         getNrStanduri() const { return m_nrStanduri; }
    std::string getTematica()   const { return m_tematica;   }
};

// ============================================================
// FIRME
// ============================================================
class Organizator : public IFirma {
    std::vector<int> m_idEvenimenteOrganizate;
public:
    Organizator(int id, const std::string& nume, const std::string& cui,
                const std::string& adresa, const std::string& telefon,
                const std::string& email)
        : IFirma(id, nume, cui, adresa, telefon, email) {}

    std::string getTipFirma()          const override { return "Organizator"; }
    std::string getServiciuPrincipal() const override {
        return "Organizare evenimente publice in parcuri";
    }
    void adaugaEveniment(int id) { m_idEvenimenteOrganizate.push_back(id); }
    const std::vector<int>& getEvenimente() const { return m_idEvenimenteOrganizate; }
    int getNrEvenimente() const { return static_cast<int>(m_idEvenimenteOrganizate.size()); }
};

class Distribuitor : public IFirma {
    std::vector<int> m_idContracteAchizitie;
    double           m_discountProcentual;
public:
    Distribuitor(int id, const std::string& nume, const std::string& cui,
                 const std::string& adresa, const std::string& telefon,
                 const std::string& email, double discount = 0.0)
        : IFirma(id, nume, cui, adresa, telefon, email),
          m_discountProcentual(discount) {}

    std::string getTipFirma()          const override { return "Distribuitor"; }
    std::string getServiciuPrincipal() const override {
        return "Furnizare materiale si obiecte pentru inventar";
    }
    double getDiscount()               const { return m_discountProcentual; }
    void   setDiscount(double d)             { m_discountProcentual = d;    }
    void   adaugaContract(int id)            { m_idContracteAchizitie.push_back(id); }
    double calculeazaPretFinal(double pretBrut) const {
        return pretBrut * (1.0 - m_discountProcentual / 100.0);
    }
};
