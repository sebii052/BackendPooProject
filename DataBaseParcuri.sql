-- ============================================================
-- SISTEM INTEGRAT DE ADMINISTRARE A PARCURILOR BUCURESTI
-- Script creare baza de date - SQL Server (SSMS)
-- Versiunea corectata - compatibila cu backend-ul C++
-- ============================================================

USE master;
GO

IF EXISTS (SELECT name FROM sys.databases WHERE name = N'ParcuriBucuresti')
    DROP DATABASE ParcuriBucuresti;
GO

CREATE DATABASE ParcuriBucuresti
    COLLATE Romanian_CI_AI;
GO

USE ParcuriBucuresti;
GO

-- ============================================================
-- 1. PARCURI
-- ============================================================
CREATE TABLE Parcuri (
    id_parc         INT PRIMARY KEY IDENTITY(1,1),
    nume            VARCHAR(100)    NOT NULL,
    adresa          VARCHAR(200),
    suprafata_mp    FLOAT           CHECK (suprafata_mp > 0),
    sector          INT             CHECK (sector BETWEEN 1 AND 6),
    descriere       VARCHAR(500),
    activ           BIT             NOT NULL DEFAULT 1,
    data_creare     DATETIME        NOT NULL DEFAULT GETDATE()
);
GO

-- ============================================================
-- 2. ZONE
-- ============================================================
CREATE TABLE Zone (
    id_zona         INT PRIMARY KEY IDENTITY(1,1),
    id_parc         INT             NOT NULL,
    nume_zona       VARCHAR(100)    NOT NULL,
    tip_zona        VARCHAR(50)     NOT NULL
                        CHECK (tip_zona IN (
                            'Lac', 'Alee', 'LocJoaca', 'PistaBicicleta',
                            'Terenuri', 'ZonaVerde', 'SpatiuCaini'
                        )),
    suprafata_mp    FLOAT           CHECK (suprafata_mp > 0),
    descriere       VARCHAR(500),

    CONSTRAINT FK_Zone_Parc FOREIGN KEY (id_parc)
        REFERENCES Parcuri(id_parc)
        ON DELETE CASCADE
);
GO

-- ============================================================
-- 3. FIRME (Organizator / Distribuitor)
-- ============================================================
CREATE TABLE IFirma (
    id_firma        INT PRIMARY KEY IDENTITY(1,1),
    tip             VARCHAR(20)     NOT NULL
                        CHECK (tip IN ('Organizator', 'Distribuitor')),
    nume            VARCHAR(150)    NOT NULL,
    cui             VARCHAR(20)     UNIQUE,
    adresa          VARCHAR(200),
    telefon         VARCHAR(20),
    email           VARCHAR(100),
    data_creare     DATETIME        NOT NULL DEFAULT GETDATE()
);
GO

-- ============================================================
-- 4. UTILIZATORI
-- ============================================================
CREATE TABLE Utilizatori (
    id_user         INT PRIMARY KEY IDENTITY(1,1),
    username        VARCHAR(50)     NOT NULL UNIQUE,
    parola_hash     VARCHAR(256)    NOT NULL,
    rol             VARCHAR(10)     NOT NULL
                        CHECK (rol IN ('Admin', 'Angajat')),
    nume            VARCHAR(100)    NOT NULL,
    prenume         VARCHAR(100)    NOT NULL,
    email           VARCHAR(100)    UNIQUE,
    telefon         VARCHAR(20),
    id_zona_alocata INT             NULL,
    activ           BIT             NOT NULL DEFAULT 1,
    data_creare     DATETIME        NOT NULL DEFAULT GETDATE(),

    CONSTRAINT FK_Utilizatori_Zona FOREIGN KEY (id_zona_alocata)
        REFERENCES Zone(id_zona)
        ON DELETE SET NULL
);
GO

-- ============================================================
-- 5. CATEGORII OBIECT
-- ============================================================
CREATE TABLE CategoriiObiect (
    id_categorie    INT PRIMARY KEY IDENTITY(1,1),
    tip_general     VARCHAR(30)     NOT NULL
                        CHECK (tip_general IN (
                            'Utilitati', 'Decorative', 'Naturale', 'Playground'
                        )),
    subtip          VARCHAR(50)     NOT NULL,
    descriere       VARCHAR(300),

    CONSTRAINT UQ_Categorie UNIQUE (tip_general, subtip)
);
GO

-- ============================================================
-- 6. INVENTAR
-- FIX #1: adaugata coloana 'in_junk' (folosita de mutaInJunk si getJunk din Repositories.h)
-- ============================================================
CREATE TABLE Inventar (
    id_obiect       INT PRIMARY KEY IDENTITY(1,1),
    id_categorie    INT             NOT NULL,
    id_zona         INT             NULL,
    locatie         VARCHAR(10)     NOT NULL DEFAULT 'Depozit'
                        CHECK (locatie IN ('InUz', 'Depozit', 'Junk')),
    stare           VARCHAR(20)     NOT NULL DEFAULT 'Functional'
                        CHECK (stare IN (
                            'Functional', 'Defect', 'InReparatie', 'Casat'
                        )),
    cantitate       INT             NOT NULL DEFAULT 1
                        CHECK (cantitate > 0),
    pret_achizitie  DECIMAL(10,2)   CHECK (pret_achizitie >= 0),
    data_achizitie  DATE,
    data_instalare  DATE,
    descriere       VARCHAR(500),
    in_junk         BIT             NOT NULL DEFAULT 0,    -- FIX #1
    data_mutare_junk DATETIME       NULL,
    data_creare     DATETIME        NOT NULL DEFAULT GETDATE(),

    CONSTRAINT FK_Inventar_Categorie FOREIGN KEY (id_categorie)
        REFERENCES CategoriiObiect(id_categorie),
    CONSTRAINT FK_Inventar_Zona FOREIGN KEY (id_zona)
        REFERENCES Zone(id_zona)
        ON DELETE SET NULL
);
GO

-- ============================================================
-- 7. INVENTAR ANGAJAT
-- ============================================================
CREATE TABLE InventarAngajat (
    id_gestiune     INT PRIMARY KEY IDENTITY(1,1),
    id_angajat      INT             NOT NULL,
    id_obiect       INT             NOT NULL,
    id_task         INT             NULL,
    stare_gestiune  VARCHAR(20)     NOT NULL DEFAULT 'Alocat'
                        CHECK (stare_gestiune IN (
                            'Alocat', 'Utilizat', 'Returnat'
                        )),
    data_alocare    DATETIME        NOT NULL DEFAULT GETDATE(),
    data_utilizare  DATETIME        NULL,
    data_returnare  DATETIME        NULL,

    CONSTRAINT FK_InvAngajat_Angajat FOREIGN KEY (id_angajat)
        REFERENCES Utilizatori(id_user),
    CONSTRAINT FK_InvAngajat_Obiect FOREIGN KEY (id_obiect)
        REFERENCES Inventar(id_obiect)
);
GO

-- ============================================================
-- 8. CONTRACTE FURNIZORI
-- ============================================================
CREATE TABLE ContracteFurnizori (
    id_contract     INT PRIMARY KEY IDENTITY(1,1),
    id_firma        INT             NOT NULL,
    valoare_totala  DECIMAL(12,2)   NOT NULL CHECK (valoare_totala > 0),
    data_semnare    DATE            NOT NULL DEFAULT GETDATE(),
    data_livrare    DATE,
    fisier_ref      VARCHAR(300),
    observatii      VARCHAR(500),
    data_creare     DATETIME        NOT NULL DEFAULT GETDATE(),

    CONSTRAINT FK_ContractFurnizor_Firma FOREIGN KEY (id_firma)
        REFERENCES IFirma(id_firma)
);
GO

CREATE TABLE ContracteFurnizori_Inventar (
    id              INT PRIMARY KEY IDENTITY(1,1),
    id_contract     INT             NOT NULL,
    id_obiect       INT             NOT NULL,
    cantitate       INT             NOT NULL DEFAULT 1 CHECK (cantitate > 0),
    pret_unitar     DECIMAL(10,2)   NOT NULL CHECK (pret_unitar >= 0),

    CONSTRAINT FK_CFI_Contract FOREIGN KEY (id_contract)
        REFERENCES ContracteFurnizori(id_contract)
        ON DELETE CASCADE,
    CONSTRAINT FK_CFI_Obiect FOREIGN KEY (id_obiect)
        REFERENCES Inventar(id_obiect)
);
GO

-- ============================================================
-- 9. SESIZARI
-- ============================================================
CREATE TABLE Sesizari (
    id_sesizare     INT PRIMARY KEY IDENTITY(1,1),
    id_zona         INT             NOT NULL,
    id_user         INT             NULL,
    descriere       VARCHAR(1000)   NOT NULL,
    status          VARCHAR(20)     NOT NULL DEFAULT 'Nerezolvata'
                        CHECK (status IN (
                            'Nerezolvata', 'Ignorata', 'TransformataTask'
                        )),
    ignorata        BIT             NOT NULL DEFAULT 0,
    data_creare     DATETIME        NOT NULL DEFAULT GETDATE(),
    data_procesare  DATETIME        NULL,

    CONSTRAINT FK_Sesizari_Zona FOREIGN KEY (id_zona)
        REFERENCES Zone(id_zona),
    CONSTRAINT FK_Sesizari_User FOREIGN KEY (id_user)
        REFERENCES Utilizatori(id_user)
        ON DELETE SET NULL
);
GO

-- ============================================================
-- 10. TASKURI
-- ============================================================
CREATE TABLE Taskuri (
    id_task         INT PRIMARY KEY IDENTITY(1,1),
    id_angajat      INT             NOT NULL,
    id_zona         INT             NOT NULL,
    id_sesizare     INT             NULL,
    tip_task        VARCHAR(15)     NOT NULL
                        CHECK (tip_task IN ('Reparatie', 'Mentenanta')),
    descriere       VARCHAR(1000)   NOT NULL,
    cost_estimat    DECIMAL(10,2)   CHECK (cost_estimat >= 0),
    data_creare     DATETIME        NOT NULL DEFAULT GETDATE(),
    deadline        DATE            NOT NULL,
    status          VARCHAR(15)     NOT NULL DEFAULT 'InProgress'
                        CHECK (status IN ('InProgress', 'Done', 'Anulat')),
    data_finalizare DATETIME        NULL,

    CONSTRAINT FK_Taskuri_Angajat FOREIGN KEY (id_angajat)
        REFERENCES Utilizatori(id_user),
    CONSTRAINT FK_Taskuri_Zona FOREIGN KEY (id_zona)
        REFERENCES Zone(id_zona),
    CONSTRAINT FK_Taskuri_Sesizare FOREIGN KEY (id_sesizare)
        REFERENCES Sesizari(id_sesizare)
        ON DELETE SET NULL
);
GO

ALTER TABLE InventarAngajat
    ADD CONSTRAINT FK_InvAngajat_Task FOREIGN KEY (id_task)
        REFERENCES Taskuri(id_task)
        ON DELETE SET NULL;
GO

-- Trigger: un angajat NU poate avea doua taskuri InProgress simultan
CREATE OR ALTER TRIGGER trg_UnTaskPerAngajat
ON Taskuri
AFTER INSERT, UPDATE
AS
BEGIN
    SET NOCOUNT ON;
    IF EXISTS (
        SELECT i.id_angajat
        FROM inserted i
        WHERE i.status = 'InProgress'
          AND EXISTS (
              SELECT 1 FROM Taskuri t
              WHERE t.id_angajat  = i.id_angajat
                AND t.status      = 'InProgress'
                AND t.id_task    <> i.id_task
          )
    )
    BEGIN
        RAISERROR('Angajatul are deja un task InProgress.', 16, 1);
        ROLLBACK TRANSACTION;
    END
END;
GO

-- ============================================================
-- 11. RAPOARTE
-- ============================================================
CREATE TABLE Rapoarte (
    id_raport       INT PRIMARY KEY IDENTITY(1,1),
    id_task         INT             NOT NULL,
    id_angajat      INT             NOT NULL,
    tip_raport      VARCHAR(15)     NOT NULL DEFAULT 'Status'
                        CHECK (tip_raport IN ('Finalizare', 'Status')),
    descriere       VARCHAR(2000)   NOT NULL,
    data_creare     DATETIME        NOT NULL DEFAULT GETDATE(),

    CONSTRAINT FK_Rapoarte_Task FOREIGN KEY (id_task)
        REFERENCES Taskuri(id_task),
    CONSTRAINT FK_Rapoarte_Angajat FOREIGN KEY (id_angajat)
        REFERENCES Utilizatori(id_user)
);
GO

-- ============================================================
-- 12. NOTIFICARI
-- ============================================================
CREATE TABLE Notificari (
    id_notif        INT PRIMARY KEY IDENTITY(1,1),
    id_task         INT             NOT NULL,
    id_expeditor    INT             NOT NULL,
    id_destinatar   INT             NOT NULL,
    mesaj           VARCHAR(500)    NOT NULL,
    citita          BIT             NOT NULL DEFAULT 0,
    data_creare     DATETIME        NOT NULL DEFAULT GETDATE(),
    data_citire     DATETIME        NULL,

    CONSTRAINT FK_Notificari_Task FOREIGN KEY (id_task)
        REFERENCES Taskuri(id_task),
    CONSTRAINT FK_Notificari_Expeditor FOREIGN KEY (id_expeditor)
        REFERENCES Utilizatori(id_user),
    CONSTRAINT FK_Notificari_Destinatar FOREIGN KEY (id_destinatar)
        REFERENCES Utilizatori(id_user)
);
GO

-- ============================================================
-- 13. EVENIMENTE
-- ============================================================
CREATE TABLE Evenimente (
    id_eveniment    INT PRIMARY KEY IDENTITY(1,1),
    id_zona         INT             NOT NULL,
    tip_eveniment   VARCHAR(20)     NOT NULL
                        CHECK (tip_eveniment IN (
                            'Film', 'Concert', 'FoodFestival', 'Altele'
                        )),
    denumire        VARCHAR(200)    NOT NULL,
    data_eveniment  DATE            NOT NULL,
    ora_start       TIME            NOT NULL,
    ora_sfarsit     TIME            NOT NULL,
    data_creare     DATETIME        NOT NULL DEFAULT GETDATE(),

    CONSTRAINT CHK_Ore_Eveniment CHECK (ora_sfarsit > ora_start),
    CONSTRAINT FK_Eveniment_Zona FOREIGN KEY (id_zona)
        REFERENCES Zone(id_zona)
);
GO

-- ============================================================
-- 14. CONTRACTE EVENIMENTE
-- FIX #2: coloana 'valoare' este NULL-able (codul nu o populeaza)
-- ============================================================
CREATE TABLE ContracteEvenimente (
    id_contract     INT PRIMARY KEY IDENTITY(1,1),
    id_firma        INT             NOT NULL,
    id_eveniment    INT             NOT NULL UNIQUE,
    valoare         DECIMAL(12,2)   NULL,      -- FIX #2: NULL allowed (codul insereaza fara valoare)
    data_semnare    DATE            NOT NULL DEFAULT GETDATE(),
    fisier_ref      VARCHAR(300),
    observatii      VARCHAR(500),
    data_creare     DATETIME        NOT NULL DEFAULT GETDATE(),

    CONSTRAINT FK_ContractEv_Firma FOREIGN KEY (id_firma)
        REFERENCES IFirma(id_firma),
    CONSTRAINT FK_ContractEv_Eveniment FOREIGN KEY (id_eveniment)
        REFERENCES Evenimente(id_eveniment)
        ON DELETE CASCADE
);
GO

-- Trigger: previne suprapunerea evenimentelor
CREATE OR ALTER TRIGGER trg_SuprapunereEvenimente
ON Evenimente
AFTER INSERT, UPDATE
AS
BEGIN
    SET NOCOUNT ON;
    IF EXISTS (
        SELECT 1
        FROM inserted i
        JOIN Evenimente e
          ON e.id_zona          = i.id_zona
         AND e.data_eveniment   = i.data_eveniment
         AND e.id_eveniment    <> i.id_eveniment
         AND e.ora_start        < i.ora_sfarsit
         AND e.ora_sfarsit      > i.ora_start
    )
    BEGIN
        RAISERROR('Exista deja un eveniment in aceasta zona la intervalul orar specificat.', 16, 1);
        ROLLBACK TRANSACTION;
    END
END;
GO

-- Trigger: previne conflict eveniment cu task reparatie activ
CREATE OR ALTER TRIGGER trg_ConflictEvenimentTask
ON Evenimente
AFTER INSERT, UPDATE
AS
BEGIN
    SET NOCOUNT ON;
    IF EXISTS (
        SELECT 1
        FROM inserted i
        JOIN Taskuri t
          ON t.id_zona  = i.id_zona
         AND t.deadline = i.data_eveniment
         AND t.status   = 'InProgress'
         AND t.tip_task = 'Reparatie'
    )
    BEGIN
        RAISERROR('Exista o lucrare de reparatie activa in aceasta zona in ziua evenimentului.', 16, 1);
        ROLLBACK TRANSACTION;
    END
END;
GO

-- ============================================================
-- 15. AUDIT LOG
-- FIX #3: id_user permite NULL pentru a suporta sesizarile de
--         la Guest (id_user = -1 in cod, dar -1 nu e valid ca FK)
--         Solutie: stocam NULL in loc de -1 pentru Guest
-- ============================================================
CREATE TABLE AuditLog (
    id_log          INT PRIMARY KEY IDENTITY(1,1),
    id_user         INT             NULL,       -- FIX #3: NULL pentru Guest
    actiune         VARCHAR(100)    NOT NULL,
    tabel_afectat   VARCHAR(50),
    id_inregistrare INT             NULL,
    detalii         VARCHAR(1000),
    data_actiune    DATETIME        NOT NULL DEFAULT GETDATE(),

    CONSTRAINT FK_AuditLog_User FOREIGN KEY (id_user)
        REFERENCES Utilizatori(id_user)
        ON DELETE SET NULL                      -- FIX #3
);
GO

-- ============================================================
-- INDECSI pentru performanta
-- ============================================================
CREATE INDEX IX_Zone_Parc           ON Zone(id_parc);
CREATE INDEX IX_Inventar_Zona       ON Inventar(id_zona);
CREATE INDEX IX_Inventar_Locatie    ON Inventar(locatie);
CREATE INDEX IX_Inventar_Junk       ON Inventar(in_junk);      -- FIX #1: index pe noua coloana
CREATE INDEX IX_Taskuri_Angajat     ON Taskuri(id_angajat);
CREATE INDEX IX_Taskuri_Status      ON Taskuri(status);
CREATE INDEX IX_Taskuri_Zona        ON Taskuri(id_zona);
CREATE INDEX IX_Sesizari_Status     ON Sesizari(status);
CREATE INDEX IX_Sesizari_Zona       ON Sesizari(id_zona);
CREATE INDEX IX_Notificari_Dest     ON Notificari(id_destinatar, citita);
CREATE INDEX IX_AuditLog_User       ON AuditLog(id_user);
CREATE INDEX IX_AuditLog_Data       ON AuditLog(data_actiune);
GO

-- ============================================================
-- DATE INITIALE (SEED DATA)
-- ============================================================

-- Categorii obiecte
INSERT INTO CategoriiObiect (tip_general, subtip, descriere) VALUES
    ('Utilitati',  'CosGunoi',          'Cos de gunoi stradal'),
    ('Utilitati',  'Toaleta',           'Toaleta publica'),
    ('Utilitati',  'Cismea',            'Cismea/fantana apa potabila'),
    ('Utilitati',  'Iluminat',          'Stalp de iluminat'),
    ('Decorative', 'Banca',             'Banca parc'),
    ('Decorative', 'Statui',            'Statuie/sculptura'),
    ('Decorative', 'MesaSah',           'Masa de sah exterior'),
    ('Naturale',   'Copac',             'Arbore ornamental sau de umbra'),
    ('Naturale',   'FloriRond',         'Rond de flori'),
    ('Naturale',   'Iarba',             'Suprafata inierbata'),
    ('Naturale',   'Nisip',             'Suprafata cu nisip'),
    ('Playground', 'Tobogan',           'Tobogan pentru copii'),
    ('Playground', 'Balansoar',         'Balansoar pentru copii'),
    ('Playground', 'Carusel',           'Carusel pentru copii'),
    ('Playground', 'Leagan',            'Leagan pentru copii'),
    ('Playground', 'BaraTractiuni',     'Bara pentru tractiuni'),
    ('Playground', 'Multifunctionale',  'Aparat fitness multifunctional'),
    ('Playground', 'EchipamentSport',   'Echipament zona sport');
GO

-- Parcuri
INSERT INTO Parcuri (nume, adresa, suprafata_mp, sector) VALUES
    ('Parcul Herastrau',    'Sos. Nordului, Sector 1',     187000, 1),
    ('Parcul Cismigiu',     'B-dul Regina Elisabeta',       17300, 5),
    ('Parcul Tineretului',  'Calea Vacaresti, Sector 4',    80000, 4),
    ('Parcul IOR',          'Str. Liviu Rebreanu, Sector 3',100000, 3),
    ('Parcul Floreasca',    'Str. Floreasca, Sector 2',     22000, 2);
GO

-- Zone pentru Parcul Herastrau (id_parc = 1)
INSERT INTO Zone (id_parc, nume_zona, tip_zona, suprafata_mp, descriere) VALUES
    (1, 'Lacul Herastrau',         'Lac',             60000, 'Lac principal'),
    (1, 'Aleea Principala',        'Alee',             5000, 'Aleea centrala'),
    (1, 'Loc de joaca Nord',       'LocJoaca',         2000, 'Zona joaca copii'),
    (1, 'Pista biciclete Est',     'PistaBicicleta',   3000, 'Pista ciclism'),
    (1, 'Zona verde Sud',          'ZonaVerde',       10000, 'Spatiu inierbat'),
    (1, 'Spatiu caini',            'SpatiuCaini',      1500, 'Zona animale'),
    (2, 'Lacul Cismigiu',          'Lac',              8000, 'Lacul central'),
    (2, 'Aleea Scriitorilor',      'Alee',             1200, 'Alee istorica'),
    (2, 'Loc de joaca Cismigiu',   'LocJoaca',          800, 'Zona copii'),
    (2, 'Zona verde Cismigiu',     'ZonaVerde',        4000, 'Peluze centrale');
GO

-- FIX #4: hash-ul este corect (SHA-256 al "admin123") — verificat
-- admin / admin123
INSERT INTO Utilizatori (username, parola_hash, rol, nume, prenume, email) VALUES
    ('admin',
     '240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9',
     'Admin', 'Popescu', 'Ion', 'admin@parcuribucuresti.ro');
GO

-- Firme
INSERT INTO IFirma (tip, nume, cui, adresa, telefon, email) VALUES
    ('Distribuitor', 'MobilierUrban SRL',   'RO12345678', 'Str. Industriei 5, Bucuresti',  '0721000001', 'contact@mobilierurban.ro'),
    ('Distribuitor', 'Verde&Curat SRL',     'RO87654321', 'Sos. Colentina 10, Bucuresti',  '0721000002', 'contact@verdecurat.ro'),
    ('Organizator',  'EventPark SRL',       'RO11223344', 'B-dul Unirii 15, Bucuresti',    '0721000003', 'contact@eventpark.ro'),
    ('Organizator',  'ConcertePlaiuri SRL', 'RO44332211', 'Calea Victoriei 30, Bucuresti', '0721000004', 'contact@concerteplaiuri.ro');
GO

PRINT 'Baza de date ParcuriBucuresti creata cu succes!';
PRINT 'Login implicit: username=admin, parola=admin123';
GO