#pragma once
#include <string>
#include "ISerializable.h"
#include "IValidatable.h"

// ============================================================
// INTERFATA IFirma (clasa abstracta pura)
// Modeleaza conceptul de firma externa cu care lucram
// Doua implementari concrete: Organizator si Distribuitor
// ============================================================
class IFirma : public ISerializabil, public IValidatabil {
protected:
    int         m_idFirma;
    std::string m_nume;
    std::string m_cui;
    std::string m_adresa;
    std::string m_telefon;
    std::string m_email;

public:
    IFirma(int id,
           const std::string& nume,
           const std::string& cui,
           const std::string& adresa,
           const std::string& telefon,
           const std::string& email)
        : m_idFirma(id), m_nume(nume), m_cui(cui),
          m_adresa(adresa), m_telefon(telefon), m_email(email) {}

    virtual ~IFirma() = default;

    virtual std::string getTipFirma()         const = 0;
    virtual std::string getServiciuPrincipal() const = 0;

    int         getIdFirma()  const { return m_idFirma;  }
    std::string getNume()     const { return m_nume;     }
    std::string getCui()      const { return m_cui;      }
    std::string getAdresa()   const { return m_adresa;   }
    std::string getTelefon()  const { return m_telefon;  }
    std::string getEmail()    const { return m_email;    }

    void setNume(const std::string& n)    { m_nume    = n; }
    void setAdresa(const std::string& a)  { m_adresa  = a; }
    void setTelefon(const std::string& t) { m_telefon = t; }
    void setEmail(const std::string& e)   { m_email   = e; }

    bool isValid() const override {
        return !m_nume.empty() && !m_cui.empty();
    }
    std::string getValidationError() const override {
        if (m_nume.empty()) return "Numele firmei nu poate fi gol.";
        if (m_cui.empty())  return "CUI-ul firmei nu poate fi gol.";
        return "";
    }

    std::string toString() const override {
        return "[" + getTipFirma() + "] " + m_nume +
               " | CUI: " + m_cui +
               " | Tel: " + m_telefon;
    }
};
