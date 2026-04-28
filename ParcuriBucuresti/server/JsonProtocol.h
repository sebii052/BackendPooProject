#pragma once
#include <string>
#include "../utils/json.hpp"
#include "../AppController.h"

using json = nlohmann::json;

// ============================================================
// CLASA JsonProtocol
// Translateaza mesaje JSON <-> apeluri AppController
//
// Format cerere:  { "actiune": "...", ...parametri... }
// Format raspuns: { "succes": true/false, "date": {...}, "eroare": "..." }
// ============================================================
class JsonProtocol {
public:

    // Proceseaza un mesaj JSON primit de la client
    // Returneaza raspunsul JSON ca string
    static std::string proceseaza(const std::string& mesajJson,
                                   AppController& app) {
        json raspuns;
        try {
            json cerere = json::parse(mesajJson);
            std::string actiune = cerere.value("actiune", "");

            // ------------------------------------------------
            // AUTH
            // ------------------------------------------------
            if (actiune == "login") {
                std::string username = cerere.value("username", "");
                std::string parola   = cerere.value("parola", "");

                if (app.login(username, parola)) {
                    raspuns["succes"] = true;
                    raspuns["rol"]    = app.getRolCurent();
                    raspuns["id_user"]= app.getIdUserCurent();
                    raspuns["mesaj"]  = "Login reusit.";
                } else {
                    raspuns["succes"] = false;
                    raspuns["eroare"] = "Credentiale invalide.";
                }
            }

            else if (actiune == "logout") {
                app.logout();
                raspuns["succes"] = true;
                raspuns["mesaj"]  = "Deconectat.";
            }

            else if (actiune == "register_angajat") {
                bool ok = app.registerAngajat(
                    cerere.value("username", ""),
                    cerere.value("parola", ""),
                    cerere.value("nume", ""),
                    cerere.value("prenume", ""),
                    cerere.value("email", ""),
                    cerere.value("id_zona", 0));
                raspuns["succes"] = ok;
                if (!ok) raspuns["eroare"] = "Nu s-a putut crea contul.";
            }

            // ------------------------------------------------
            // ANGAJATI
            // ------------------------------------------------
            else if (actiune == "get_angajati") {
                auto angajati = app.getAngajatiCuStatus();
                json lista = json::array();
                for (const auto& a : angajati) {
                    lista.push_back({
                        {"id",           a.id},
                        {"numeComplet",  a.numeComplet},
                        {"username",     a.username},
                        {"idZona",       a.idZona},
                        {"esteLiber",    a.esteLiber},
                        {"idTaskCurent", a.idTaskCurent}
                    });
                }
                raspuns["succes"] = true;
                raspuns["date"]   = lista;
            }

            // ------------------------------------------------
            // SESIZARI
            // ------------------------------------------------
            else if (actiune == "creeaza_sesizare") {
                int id = app.creeazaSesizare(
                    cerere.value("id_zona", 0),
                    cerere.value("descriere", ""));
                raspuns["succes"]     = (id > 0);
                raspuns["id_sesizare"]= id;
            }

            else if (actiune == "get_sesizari") {
                auto sesizari = app.getSesizariNerezolvate();
                json lista = json::array();
                for (const auto& [id, idZona, desc, data] : sesizari) {
                    lista.push_back({
                        {"id",        id},
                        {"idZona",    idZona},
                        {"descriere", desc},
                        {"data",      data}
                    });
                }
                raspuns["succes"] = true;
                raspuns["date"]   = lista;
            }

            else if (actiune == "ignora_sesizare") {
                bool ok = app.ignoraSesizare(cerere.value("id_sesizare", 0));
                raspuns["succes"] = ok;
            }

            // ------------------------------------------------
            // TASKURI
            // ------------------------------------------------
            else if (actiune == "creeaza_task_din_sesizare") {
                int id = app.creeazaTaskDinSesizare(
                    cerere.value("id_angajat", 0),
                    cerere.value("id_zona", 0),
                    cerere.value("id_sesizare", 0),
                    cerere.value("descriere", ""),
                    cerere.value("cost", 0.0),
                    cerere.value("deadline", ""));
                raspuns["succes"]  = (id > 0);
                raspuns["id_task"] = id;
            }

            else if (actiune == "creeaza_task_daily") {
                int id = app.creeazaTaskDaily(
                    cerere.value("id_angajat", 0),
                    cerere.value("id_zona", 0),
                    cerere.value("descriere", ""),
                    cerere.value("cost", 0.0),
                    cerere.value("deadline", ""));
                raspuns["succes"]  = (id > 0);
                raspuns["id_task"] = id;
            }

            else if (actiune == "get_taskuri_mele") {
                auto taskuri = app.getTaskuriMele();
                json lista = json::array();
                for (const auto& [id, tip, desc, status] : taskuri) {
                    lista.push_back({
                        {"id",        id},
                        {"tip",       tip},
                        {"descriere", desc},
                        {"status",    status}
                    });
                }
                raspuns["succes"] = true;
                raspuns["date"]   = lista;
            }

            else if (actiune == "finalizeaza_task") {
                bool ok = app.finalizeazaTask(cerere.value("id_task", 0));
                raspuns["succes"] = ok;
            }

            // ------------------------------------------------
            // NOTIFICARI
            // ------------------------------------------------
            else if (actiune == "cer_status_task") {
                int id = app.cerStatusTask(
                    cerere.value("id_task", 0),
                    cerere.value("id_angajat", 0),
                    cerere.value("mesaj", ""));
                raspuns["succes"]   = (id > 0);
                raspuns["id_notif"] = id;
            }

            else if (actiune == "get_notificari") {
                auto notificari = app.getNotificariMele();
                json lista = json::array();
                for (const auto& [id, idTask, mesaj, citita] : notificari) {
                    lista.push_back({
                        {"id",     id},
                        {"idTask", idTask},
                        {"mesaj",  mesaj},
                        {"citita", citita}
                    });
                }
                raspuns["succes"] = true;
                raspuns["date"]   = lista;
            }

            else if (actiune == "citeste_notificare") {
                bool ok = app.marcheazaNotificareCitita(
                    cerere.value("id_notif", 0));
                raspuns["succes"] = ok;
            }

            // ------------------------------------------------
            // RAPOARTE
            // ------------------------------------------------
            else if (actiune == "creeaza_raport") {
                int id = app.creeazaRaportStatus(
                    cerere.value("id_task", 0),
                    cerere.value("descriere", ""));
                raspuns["succes"]    = (id > 0);
                raspuns["id_raport"] = id;
            }

            // ------------------------------------------------
            // INVENTAR
            // ------------------------------------------------
            else if (actiune == "get_inventar_zona") {
                auto obiecte = app.getInventarZona(
                    cerere.value("id_zona", 0));
                json lista = json::array();
                for (const auto& o : obiecte) {
                    lista.push_back({
                        {"id",         o.id},
                        {"tipGeneral", o.tipGeneral},
                        {"subtip",     o.subtip},
                        {"stare",      o.stare},
                        {"locatie",    o.locatie},
                        {"pret",       o.pret}
                    });
                }
                raspuns["succes"] = true;
                raspuns["date"]   = lista;
            }

            else if (actiune == "adauga_in_depozit") {
                int id = app.adaugaInDepozit(
                    cerere.value("id_categorie", 0),
                    cerere.value("cantitate", 0),
                    cerere.value("pret", 0.0),
                    cerere.value("data_achizitie", ""),
                    cerere.value("id_furnizor", -1));
                raspuns["succes"]    = (id > 0);
                raspuns["id_obiect"] = id;
            }

            else if (actiune == "muta_in_junk") {
                bool ok = app.mutaInJunk(cerere.value("id_obiect", 0));
                raspuns["succes"] = ok;
            }

            else if (actiune == "sterge_din_junk") {
                bool ok = app.stergeDinJunk(cerere.value("id_obiect", 0));
                raspuns["succes"] = ok;
            }

            else if (actiune == "get_junk") {
                auto obiecte = app.getJunk();
                json lista = json::array();
                for (const auto& o : obiecte) {
                    lista.push_back({
                        {"id",         o.id},
                        {"tipGeneral", o.tipGeneral},
                        {"subtip",     o.subtip},
                        {"stare",      o.stare},
                        {"pret",       o.pret}
                    });
                }
                raspuns["succes"] = true;
                raspuns["date"]   = lista;
            }

            // ------------------------------------------------
            // EVENIMENTE
            // ------------------------------------------------
            else if (actiune == "creeaza_eveniment") {
                int id = app.creeazaEveniment(
                    cerere.value("id_zona", 0),
                    cerere.value("tip", ""),
                    cerere.value("denumire", ""),
                    cerere.value("data", ""),
                    cerere.value("ora_start", ""),
                    cerere.value("ora_sfarsit", ""),
                    cerere.value("id_firma", 0));
                raspuns["succes"]       = (id > 0);
                raspuns["id_eveniment"] = id;
            }

            // ------------------------------------------------
            // ACTIUNE NECUNOSCUTA
            // ------------------------------------------------
            else {
                raspuns["succes"] = false;
                raspuns["eroare"] = "Actiune necunoscuta: " + actiune;
            }

        } catch (const std::exception& e) {
            raspuns["succes"] = false;
            raspuns["eroare"] = std::string(e.what());
        } catch (...) {
            raspuns["succes"] = false;
            raspuns["eroare"] = "Eroare interna necunoscuta.";
        }

        return raspuns.dump() + "\n";
    }
};
