#pragma once
#include <string>
#include <stdexcept>
#include "../interfaces/ISerializable.h"
#include "../interfaces/IValidatable.h"

// ============================================================
// ENUM-URI pentru starea si locatia obiectelor
// ============================================================
enum class StareObiect   { Functional, Defect, InReparatie, Casat };
enum class LocatieObiect { InUz, Depozit, Junk };

inline std::string stareToString(StareObiect s) {
    switch (s) {
        case StareObiect::Functional:  return "Functional";
        case StareObiect::Defect:      return "Defect";
        case StareObiect::InReparatie: return "InReparatie";
        case StareObiect::Casat:       return "Casat";
    }
    return "Necunoscut";
}

inline std::string locatieToString(LocatieObiect l) {
    switch (l) {
        case LocatieObiect::InUz:    return "InUz";
        case LocatieObiect::Depozit: return "Depozit";
        case LocatieObiect::Junk:    return "Junk";
    }
    return "Necunoscut";
}

// ============================================================
// CLASA ABSTRACTA Obiect
// ============================================================
class Obiect : public ISerializabil, public IValidatabil {
protected:
    int            m_idObiect;
    int            m_idCategorie;
    int            m_idZona;
    StareObiect    m_stare;
    LocatieObiect  m_locatie;
    std::string    m_descriere;
    std::string    m_dataAchizitie;
    double         m_pretAchizitie;

public:
    Obiect(int id, int idCategorie, int idZona,
           StareObiect stare, LocatieObiect locatie,
           const std::string& descriere,
           const std::string& dataAchizitie, double pret)
        : m_idObiect(id), m_idCategorie(idCategorie), m_idZona(idZona),
          m_stare(stare), m_locatie(locatie), m_descriere(descriere),
          m_dataAchizitie(dataAchizitie), m_pretAchizitie(pret) {}

    virtual ~Obiect() = default;

    virtual std::string getTipGeneral()        const = 0;
    virtual std::string getSubtip()            const = 0;
    virtual std::string getDescriereCompleta() const = 0;

    virtual void mutaIn(LocatieObiect nouaLocatie, int nouaZona = -1) {
        m_locatie = nouaLocatie;
        m_idZona  = nouaZona;
        if (nouaLocatie == LocatieObiect::Junk)
            m_stare = StareObiect::Casat;
    }

    int           getIdObiect()      const { return m_idObiect;      }
    int           getIdCategorie()   const { return m_idCategorie;   }
    int           getIdZona()        const { return m_idZona;        }
    StareObiect   getStare()         const { return m_stare;         }
    LocatieObiect getLocatie()       const { return m_locatie;       }
    std::string   getDescriere()     const { return m_descriere;     }
    std::string   getDataAchizitie() const { return m_dataAchizitie; }
    double        getPretAchizitie() const { return m_pretAchizitie; }

    void setStare(StareObiect s)           { m_stare     = s; }
    void setDescriere(const std::string& d) { m_descriere = d; }

    bool isValid() const override {
        return m_idObiect > 0 && m_pretAchizitie >= 0.0;
    }
    std::string getValidationError() const override {
        if (m_idObiect <= 0)       return "ID obiect invalid.";
        if (m_pretAchizitie < 0.0) return "Pretul nu poate fi negativ.";
        return "";
    }
    std::string toString() const override {
        return "[" + getTipGeneral() + "/" + getSubtip() + "] " +
               "ID=" + std::to_string(m_idObiect) +
               " Stare=" + stareToString(m_stare) +
               " Locatie=" + locatieToString(m_locatie);
    }
};

// ============================================================
// ObiectNatural
// ============================================================
class ObiectNatural : public Obiect {
public:
    ObiectNatural(int id, int idCategorie, int idZona,
                  StareObiect stare, LocatieObiect locatie,
                  const std::string& descriere,
                  const std::string& dataAchizitie, double pret)
        : Obiect(id, idCategorie, idZona, stare, locatie,
                 descriere, dataAchizitie, pret) {}
    std::string getTipGeneral() const override { return "Natural"; }
};

class Copac : public ObiectNatural {
    std::string m_specie;
    int         m_varstaAni;
public:
    Copac(int id, int idCategorie, int idZona,
          StareObiect stare, LocatieObiect locatie,
          const std::string& descriere,
          const std::string& dataAchizitie, double pret,
          const std::string& specie, int varsta)
        : ObiectNatural(id, idCategorie, idZona, stare, locatie,
                        descriere, dataAchizitie, pret),
          m_specie(specie), m_varstaAni(varsta) {}
    std::string getSubtip()  const override { return "Copac"; }
    std::string getSpecie()  const { return m_specie; }
    int         getVarsta()  const { return m_varstaAni; }
    std::string getDescriereCompleta() const override {
        return "Copac | Specie: " + m_specie +
               " | Varsta: " + std::to_string(m_varstaAni) + " ani";
    }
};

class FloriRond : public ObiectNatural {
    std::string m_tipFlori;
public:
    FloriRond(int id, int idCategorie, int idZona,
              StareObiect stare, LocatieObiect locatie,
              const std::string& descriere,
              const std::string& dataAchizitie, double pret,
              const std::string& tipFlori)
        : ObiectNatural(id, idCategorie, idZona, stare, locatie,
                        descriere, dataAchizitie, pret),
          m_tipFlori(tipFlori) {}
    std::string getSubtip()   const override { return "FloriRond"; }
    std::string getTipFlori() const { return m_tipFlori; }
    std::string getDescriereCompleta() const override {
        return "Rond de flori | Tip: " + m_tipFlori;
    }
};

class Iarba : public ObiectNatural {
    double m_suprafataMP;
public:
    Iarba(int id, int idCategorie, int idZona,
          StareObiect stare, LocatieObiect locatie,
          const std::string& descriere,
          const std::string& dataAchizitie, double pret,
          double suprafata)
        : ObiectNatural(id, idCategorie, idZona, stare, locatie,
                        descriere, dataAchizitie, pret),
          m_suprafataMP(suprafata) {}
    std::string getSubtip() const override { return "Iarba"; }
    std::string getDescriereCompleta() const override {
        return "Suprafata iarba | " + std::to_string(m_suprafataMP) + " mp";
    }
};

class Nisip : public ObiectNatural {
    double m_suprafataMP;
public:
    Nisip(int id, int idCategorie, int idZona,
          StareObiect stare, LocatieObiect locatie,
          const std::string& descriere,
          const std::string& dataAchizitie, double pret,
          double suprafata)
        : ObiectNatural(id, idCategorie, idZona, stare, locatie,
                        descriere, dataAchizitie, pret),
          m_suprafataMP(suprafata) {}
    std::string getSubtip() const override { return "Nisip"; }
    std::string getDescriereCompleta() const override {
        return "Suprafata nisip | " + std::to_string(m_suprafataMP) + " mp";
    }
};

// ============================================================
// ObiectUtilitate
// ============================================================
class ObiectUtilitate : public Obiect {
public:
    ObiectUtilitate(int id, int idCategorie, int idZona,
                    StareObiect stare, LocatieObiect locatie,
                    const std::string& descriere,
                    const std::string& dataAchizitie, double pret)
        : Obiect(id, idCategorie, idZona, stare, locatie,
                 descriere, dataAchizitie, pret) {}
    std::string getTipGeneral() const override { return "Utilitati"; }
};

class CosGunoi : public ObiectUtilitate {
    std::string m_material;
public:
    CosGunoi(int id, int idCategorie, int idZona,
             StareObiect stare, LocatieObiect locatie,
             const std::string& descriere,
             const std::string& dataAchizitie, double pret,
             const std::string& material = "metal")
        : ObiectUtilitate(id, idCategorie, idZona, stare, locatie,
                          descriere, dataAchizitie, pret),
          m_material(material) {}
    std::string getSubtip()    const override { return "CosGunoi"; }
    std::string getMaterial()  const { return m_material; }
    std::string getDescriereCompleta() const override {
        return "Cos gunoi | Material: " + m_material;
    }
};

class Toaleta : public ObiectUtilitate {
    bool m_esteAccesibila;
public:
    Toaleta(int id, int idCategorie, int idZona,
            StareObiect stare, LocatieObiect locatie,
            const std::string& descriere,
            const std::string& dataAchizitie, double pret,
            bool accesibila = true)
        : ObiectUtilitate(id, idCategorie, idZona, stare, locatie,
                          descriere, dataAchizitie, pret),
          m_esteAccesibila(accesibila) {}
    std::string getSubtip()       const override { return "Toaleta"; }
    bool        esteAccesibila()  const { return m_esteAccesibila; }
    std::string getDescriereCompleta() const override {
        return "Toaleta publica" +
               std::string(m_esteAccesibila ? " | Accesibila PMR" : "");
    }
};

class Cismea : public ObiectUtilitate {
    bool m_esteOperationala;
public:
    Cismea(int id, int idCategorie, int idZona,
           StareObiect stare, LocatieObiect locatie,
           const std::string& descriere,
           const std::string& dataAchizitie, double pret,
           bool operationala = true)
        : ObiectUtilitate(id, idCategorie, idZona, stare, locatie,
                          descriere, dataAchizitie, pret),
          m_esteOperationala(operationala) {}
    std::string getSubtip()         const override { return "Cismea"; }
    bool        esteOperationala()  const { return m_esteOperationala; }
    std::string getDescriereCompleta() const override {
        return "Cismea apa potabila | Stare: " +
               std::string(m_esteOperationala ? "Functionala" : "Defecta");
    }
};

// ============================================================
// ObiectDecorativ
// ============================================================
class ObiectDecorativ : public Obiect {
public:
    ObiectDecorativ(int id, int idCategorie, int idZona,
                    StareObiect stare, LocatieObiect locatie,
                    const std::string& descriere,
                    const std::string& dataAchizitie, double pret)
        : Obiect(id, idCategorie, idZona, stare, locatie,
                 descriere, dataAchizitie, pret) {}
    std::string getTipGeneral() const override { return "Decorative"; }
};

class Banca : public ObiectDecorativ {
    std::string m_material;
    int         m_nrLocuri;
public:
    Banca(int id, int idCategorie, int idZona,
          StareObiect stare, LocatieObiect locatie,
          const std::string& descriere,
          const std::string& dataAchizitie, double pret,
          const std::string& material = "lemn", int nrLocuri = 3)
        : ObiectDecorativ(id, idCategorie, idZona, stare, locatie,
                          descriere, dataAchizitie, pret),
          m_material(material), m_nrLocuri(nrLocuri) {}
    std::string getSubtip()    const override { return "Banca"; }
    std::string getMaterial()  const { return m_material; }
    int         getNrLocuri()  const { return m_nrLocuri; }
    std::string getDescriereCompleta() const override {
        return "Banca " + m_material + " | " +
               std::to_string(m_nrLocuri) + " locuri";
    }
};

class Statuie : public ObiectDecorativ {
    std::string m_autor;
    int         m_anInstalare;
public:
    Statuie(int id, int idCategorie, int idZona,
            StareObiect stare, LocatieObiect locatie,
            const std::string& descriere,
            const std::string& dataAchizitie, double pret,
            const std::string& autor, int an)
        : ObiectDecorativ(id, idCategorie, idZona, stare, locatie,
                          descriere, dataAchizitie, pret),
          m_autor(autor), m_anInstalare(an) {}
    std::string getSubtip()      const override { return "Statuie"; }
    std::string getAutor()       const { return m_autor; }
    int         getAnInstalare() const { return m_anInstalare; }
    std::string getDescriereCompleta() const override {
        return "Statuie | Autor: " + m_autor +
               " | Instalata: " + std::to_string(m_anInstalare);
    }
};

class MasaSah : public ObiectDecorativ {
public:
    MasaSah(int id, int idCategorie, int idZona,
            StareObiect stare, LocatieObiect locatie,
            const std::string& descriere,
            const std::string& dataAchizitie, double pret)
        : ObiectDecorativ(id, idCategorie, idZona, stare, locatie,
                          descriere, dataAchizitie, pret) {}
    std::string getSubtip() const override { return "MasaSah"; }
    std::string getDescriereCompleta() const override {
        return "Masa de sah exterior";
    }
};

// ============================================================
// ObiectPlayground
// ============================================================
class ObiectPlayground : public Obiect {
    int m_varstaMinima;
    int m_varstaMaxima;
public:
    ObiectPlayground(int id, int idCategorie, int idZona,
                     StareObiect stare, LocatieObiect locatie,
                     const std::string& descriere,
                     const std::string& dataAchizitie, double pret,
                     int varstaMin = 2, int varstaMax = 14)
        : Obiect(id, idCategorie, idZona, stare, locatie,
                 descriere, dataAchizitie, pret),
          m_varstaMinima(varstaMin), m_varstaMaxima(varstaMax) {}
    std::string getTipGeneral() const override { return "Playground"; }
    int getVarstaMinima() const { return m_varstaMinima; }
    int getVarstaMaxima() const { return m_varstaMaxima; }
};

class Tobogan : public ObiectPlayground {
    double m_inaltimeM;
public:
    Tobogan(int id, int idCategorie, int idZona,
            StareObiect stare, LocatieObiect locatie,
            const std::string& descriere,
            const std::string& dataAchizitie, double pret,
            double inaltime)
        : ObiectPlayground(id, idCategorie, idZona, stare, locatie,
                           descriere, dataAchizitie, pret),
          m_inaltimeM(inaltime) {}
    std::string getSubtip() const override { return "Tobogan"; }
    std::string getDescriereCompleta() const override {
        return "Tobogan | Inaltime: " + std::to_string(m_inaltimeM) + " m";
    }
};

class Balansoar : public ObiectPlayground {
    int m_nrLocuri;
public:
    Balansoar(int id, int idCategorie, int idZona,
              StareObiect stare, LocatieObiect locatie,
              const std::string& descriere,
              const std::string& dataAchizitie, double pret,
              int nrLocuri = 2)
        : ObiectPlayground(id, idCategorie, idZona, stare, locatie,
                           descriere, dataAchizitie, pret),
          m_nrLocuri(nrLocuri) {}
    std::string getSubtip()   const override { return "Balansoar"; }
    int         getNrLocuri() const { return m_nrLocuri; }
    std::string getDescriereCompleta() const override {
        return "Balansoar | " + std::to_string(m_nrLocuri) + " locuri";
    }
};

class Carusel : public ObiectPlayground {
    int m_capacitate;
public:
    Carusel(int id, int idCategorie, int idZona,
            StareObiect stare, LocatieObiect locatie,
            const std::string& descriere,
            const std::string& dataAchizitie, double pret,
            int capacitate = 6)
        : ObiectPlayground(id, idCategorie, idZona, stare, locatie,
                           descriere, dataAchizitie, pret),
          m_capacitate(capacitate) {}
    std::string getSubtip()     const override { return "Carusel"; }
    int         getCapacitate() const { return m_capacitate; }
    std::string getDescriereCompleta() const override {
        return "Carusel | Capacitate: " + std::to_string(m_capacitate) + " persoane";
    }
};

class Leagan : public ObiectPlayground {
    std::string m_tipLeagan;
public:
    Leagan(int id, int idCategorie, int idZona,
           StareObiect stare, LocatieObiect locatie,
           const std::string& descriere,
           const std::string& dataAchizitie, double pret,
           const std::string& tip = "simplu")
        : ObiectPlayground(id, idCategorie, idZona, stare, locatie,
                           descriere, dataAchizitie, pret),
          m_tipLeagan(tip) {}
    std::string getSubtip() const override { return "Leagan"; }
    std::string getDescriereCompleta() const override {
        return "Leagan " + m_tipLeagan;
    }
};

// ============================================================
// ObiectSport
// ============================================================
class ObiectSport : public ObiectPlayground {
public:
    ObiectSport(int id, int idCategorie, int idZona,
                StareObiect stare, LocatieObiect locatie,
                const std::string& descriere,
                const std::string& dataAchizitie, double pret)
        : ObiectPlayground(id, idCategorie, idZona, stare, locatie,
                           descriere, dataAchizitie, pret, 14, 99) {}
    std::string getTipGeneral() const override { return "Playground/Sport"; }
};

class BaraTractiuni : public ObiectSport {
public:
    BaraTractiuni(int id, int idCategorie, int idZona,
                  StareObiect stare, LocatieObiect locatie,
                  const std::string& descriere,
                  const std::string& dataAchizitie, double pret)
        : ObiectSport(id, idCategorie, idZona, stare, locatie,
                      descriere, dataAchizitie, pret) {}
    std::string getSubtip() const override { return "BaraTractiuni"; }
    std::string getDescriereCompleta() const override {
        return "Bara tractiuni exterior";
    }
};

class AparatMultifunctional : public ObiectSport {
    int m_nrStatii;
public:
    AparatMultifunctional(int id, int idCategorie, int idZona,
                          StareObiect stare, LocatieObiect locatie,
                          const std::string& descriere,
                          const std::string& dataAchizitie, double pret,
                          int nrStatii = 4)
        : ObiectSport(id, idCategorie, idZona, stare, locatie,
                      descriere, dataAchizitie, pret),
          m_nrStatii(nrStatii) {}
    std::string getSubtip()   const override { return "Multifunctional"; }
    int         getNrStatii() const { return m_nrStatii; }
    std::string getDescriereCompleta() const override {
        return "Aparat multifunctional | " + std::to_string(m_nrStatii) + " statii";
    }
};
