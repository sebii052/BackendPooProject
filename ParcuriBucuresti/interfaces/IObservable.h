#pragma once
#include <string>

// ============================================================
// INTERFATA IObserver
// Orice entitate care poate fi notificata de un eveniment
// ============================================================
class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void onNotify(const std::string& eveniment,
                          const std::string& detalii) = 0;
};

// ============================================================
// INTERFATA IObservable
// Orice entitate care poate trimite notificari
// ============================================================
class IObservable {
public:
    virtual ~IObservable() = default;
    virtual void addObserver(IObserver* obs)                         = 0;
    virtual void removeObserver(IObserver* obs)                      = 0;
    virtual void notifyObservers(const std::string& eveniment,
                                 const std::string& detalii)         = 0;
};
