# Sistem Integrat de Administrare a Parcurilor Bucuresti
## Arhitectura Client-Server cu Qt + C++ + SQL Server

---

## Structura proiectului

```
ParcuriBucuresti/
│
├── main_server.cpp                  ← Punct de intrare — porneste serverul TCP
│
├── AppController.h                  ← Logica de business (neschimbata de UI)
│
├── interfaces/
│   ├── ISerializable.h              ← toString()
│   ├── IValidatable.h               ← isValid() + getValidationError()
│   ├── IObservable.h                ← Observer Pattern
│   └── IFirma.h                     ← Interfata abstracta firma
│
├── models/
│   ├── Obiecte.h                    ← Ierarhia obiectelor fizice din parc
│   ├── Zona.h                       ← Zona + Parc
│   ├── Utilizatori.h                ← Guest, Angajat, Admin
│   ├── Operatiuni.h                 ← Task, Sesizare, Raport, Notificare
│   └── EvenimenteFirme.h            ← Film, Concert, FoodFestival, Organizator, Distribuitor
│
├── db/
│   ├── DatabaseManager.h            ← Singleton ODBC Unicode (SQL Server)
│   └── Repositories.h               ← CRUD per entitate
│
├── utils/
│   ├── HashUtils.h                  ← SHA-256 (Win CryptoAPI)
│   └── json.hpp                     ← nlohmann/json — DE DESCARCAT (vezi mai jos)
│
└── server/
    ├── SocketServer.h               ← Server TCP Winsock, accepta conexiuni
    ├── ClientHandler.h              ← Gestioneaza un client pe thread separat
    └── JsonProtocol.h               ← Translateaza JSON <-> AppController
```

---

## Cerinte

| Cerinta                  | Implementare                                          |
|--------------------------|-------------------------------------------------------|
| Aplicatie client-server  | Backend C++ (server TCP) + Frontend Qt (client TCP)   |
| Comunicare prin socket   | Winsock2 — TCP pe portul 9000                         |
| Persistenta datelor      | SQL Server (SSMS) prin ODBC Driver 17                 |
| Arhitectura POO          | 4 niveluri mostenire, interfete, polimorfism, etc.    |
| Login si roluri          | Guest / Angajat / Admin cu verificare in fiecare metoda |
| CRUD prin query-uri SQL  | Repositories.h — INSERT, SELECT, UPDATE, DELETE       |
| Parsare JSON             | nlohmann/json — cereri si raspunsuri in format JSON   |
| Managementul parolelor   | SHA-256 prin Win CryptoAPI (HashUtils.h)              |

---

## Pasul 1 — Descarca json.hpp

Descarca fisierul de la urmatorul URL si salveaza-l in `utils/json.hpp`:

```
https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp
```

Sau din browser: `github.com/nlohmann/json` → Releases → Download `json.hpp`

---

## Pasul 2 — Setup Visual Studio (backend)

1. Creeaza un **Empty C++ Project** nou
2. Adauga toate fisierele `.h` si `main_server.cpp`
3. **Project Properties:**
   - C++ Language Standard: **ISO C++17** (`/std:c++17`)
   - Character Set: **Unicode**
   - Additional Dependencies: `odbc32.lib; advapi32.lib; ws2_32.lib`
4. Instaleaza **ODBC Driver 17 for SQL Server** (Microsoft)
5. Ruleaza scriptul SQL in SSMS pentru a crea baza de date

---

## Pasul 3 — Conectare la SQL Server

In `main_server.cpp`, modifica daca e necesar:
```cpp
db.conecteaza("localhost", "ParcuriBucuresti");
// sau cu autentificare SQL:
db.conecteaza("localhost", "ParcuriBucuresti", "sa", "parola_ta");
```

---

## Pasul 4 — Setup Qt (frontend, colegul tau)

In Qt, conectarea la server se face cu `QTcpSocket`:

```cpp
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

// Conectare
QTcpSocket* socket = new QTcpSocket(this);
socket->connectToHost("127.0.0.1", 9000);

// Trimitere cerere
QJsonObject cerere;
cerere["actiune"]  = "login";
cerere["username"] = "admin";
cerere["parola"]   = "admin123";

QByteArray date = QJsonDocument(cerere).toJson(QJsonDocument::Compact) + "\n";
socket->write(date);

// Primire raspuns
connect(socket, &QTcpSocket::readyRead, this, [=]() {
    QByteArray raspunsDate = socket->readLine();
    QJsonObject raspuns = QJsonDocument::fromJson(raspunsDate).object();

    if (raspuns["succes"].toBool()) {
        QString rol = raspuns["rol"].toString();
        // navigheaza la fereastra corespunzatoare rolului
    } else {
        QString eroare = raspuns["eroare"].toString();
        QMessageBox::warning(this, "Eroare", eroare);
    }
});
```

---

## Protocolul de comunicare

Fiecare mesaj este un JSON pe o singura linie, terminat cu `\n`.

### Autentificare

**Cerere:**
```json
{"actiune": "login", "username": "admin", "parola": "admin123"}
```
**Raspuns succes:**
```json
{"succes": true, "rol": "Admin", "id_user": 1, "mesaj": "Login reusit."}
```
**Raspuns esec:**
```json
{"succes": false, "eroare": "Credentiale invalide."}
```

### Sesizare (Guest sau oricine)

**Cerere:**
```json
{"actiune": "creeaza_sesizare", "id_zona": 3, "descriere": "Banca rupta"}
```
**Raspuns:**
```json
{"succes": true, "id_sesizare": 42}
```

### Lista taskuri (Angajat)

**Cerere:**
```json
{"actiune": "get_taskuri_mele"}
```
**Raspuns:**
```json
{
  "succes": true,
  "date": [
    {"id": 5, "tip": "Reparatie", "descriere": "Repara tobogan", "status": "InProgress"},
    {"id": 3, "tip": "Mentenanta", "descriere": "Tuns gazon", "status": "Done"}
  ]
}
```

### Creare task (Admin)

**Cerere:**
```json
{
  "actiune": "creeaza_task_daily",
  "id_angajat": 2,
  "id_zona": 3,
  "descriere": "Tuns gazon zona nord",
  "cost": 50.0,
  "deadline": "2026-04-30"
}
```
**Raspuns:**
```json
{"succes": true, "id_task": 12}
```

### Notificare admin -> angajat

**Cerere:**
```json
{
  "actiune": "cer_status_task",
  "id_task": 5,
  "id_angajat": 2,
  "mesaj": "Care este stadiul reparatiei toboganului?"
}
```
**Raspuns:**
```json
{"succes": true, "id_notif": 7}
```

---

## Lista completa actiuni

| Actiune                    | Rol minim  | Parametri principali                                      |
|----------------------------|------------|-----------------------------------------------------------|
| `login`                    | oricine    | `username`, `parola`                                      |
| `logout`                   | logat      | —                                                         |
| `register_angajat`         | Admin      | `username`, `parola`, `nume`, `prenume`, `email`, `id_zona` |
| `get_angajati`             | Admin      | —                                                         |
| `creeaza_sesizare`         | oricine    | `id_zona`, `descriere`                                    |
| `get_sesizari`             | Admin      | —                                                         |
| `ignora_sesizare`          | Admin      | `id_sesizare`                                             |
| `creeaza_task_din_sesizare`| Admin      | `id_angajat`, `id_zona`, `id_sesizare`, `descriere`, `cost`, `deadline` |
| `creeaza_task_daily`       | Admin      | `id_angajat`, `id_zona`, `descriere`, `cost`, `deadline`  |
| `get_taskuri_mele`         | Angajat    | —                                                         |
| `finalizeaza_task`         | Angajat    | `id_task`                                                 |
| `creeaza_raport`           | Angajat    | `id_task`, `descriere`                                    |
| `cer_status_task`          | Admin      | `id_task`, `id_angajat`, `mesaj`                          |
| `get_notificari`           | Angajat    | —                                                         |
| `citeste_notificare`       | Angajat    | `id_notif`                                                |
| `get_inventar_zona`        | logat      | `id_zona`                                                 |
| `adauga_in_depozit`        | Admin      | `id_categorie`, `cantitate`, `pret`, `data_achizitie`     |
| `muta_in_junk`             | Admin      | `id_obiect`                                               |
| `sterge_din_junk`          | Admin      | `id_obiect`                                               |
| `get_junk`                 | Admin      | —                                                         |
| `creeaza_eveniment`        | Admin      | `id_zona`, `tip`, `denumire`, `data`, `ora_start`, `ora_sfarsit`, `id_firma` |

---

## Principii POO aplicate

| Principiu         | Unde este aplicat                                                        |
|-------------------|--------------------------------------------------------------------------|
| Abstractizare     | `Obiect`, `TaskBase`, `Eveniment`, `User`, `IFirma` — clase abstracte    |
| Mostenire         | 4 niveluri: `Obiect → ObiectPlayground → ObiectSport → BaraTractiuni`   |
| Polimorfism       | `getTipGeneral()`, `getTipDescriptiv()`, `getActiunePrincipala()`        |
| Encapsulare       | Toti membrii `private`/`protected`, acces prin getteri/setteri           |
| Interfete         | `ISerializable`, `IValidatable`, `IObservable`, `IFirma`, `IRepository<T>` |
| Template          | `IRepository<T>` — contract CRUD generic                                 |
| Singleton         | `DatabaseManager` — o singura conexiune DB                               |
| Observer Pattern  | `TaskBase` (IObservable) notifica `Admin` (IObserver) la finalizare task |
| Move semantics    | `Zona::scoateObiect()` returneaza `unique_ptr` — transfer de ownership   |
| Smart pointers    | `unique_ptr<Obiect>` in `Zona`, `unique_ptr<Zona>` in `Parc`             |
| RAII              | `DatabaseManager` elibereaza ODBC in destructor                          |
