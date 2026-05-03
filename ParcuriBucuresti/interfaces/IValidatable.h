#pragma once
#include <string>

// ============================================================
// INTERFATA IValidatable
// Orice entitate care isi poate valida propria stare
// ============================================================
class IValidatabil {
public:
    virtual ~IValidatabil() = default;
    virtual bool        isValid()             const = 0;
    virtual std::string getValidationError()  const = 0;
};
