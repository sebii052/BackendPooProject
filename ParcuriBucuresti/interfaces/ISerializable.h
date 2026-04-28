#pragma once
#include <string>

// ============================================================
// INTERFATA ISerializable
// Orice entitate care poate fi convertita in string
// pentru logging, debug sau export
// ============================================================
class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual std::string toString() const = 0;
};
