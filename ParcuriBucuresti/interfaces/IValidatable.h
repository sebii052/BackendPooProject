#pragma once
#include <string>

// ============================================================
// INTERFATA IValidatable
// Orice entitate care isi poate valida propria stare
// ============================================================
class IValidatable {
public:
    virtual ~IValidatable() = default;
    virtual bool        isValid()             const = 0;
    virtual std::string getValidationError()  const = 0;
};
