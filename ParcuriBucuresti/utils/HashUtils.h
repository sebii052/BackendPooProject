#pragma once
#include <string>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
  #include <windows.h>
  #include <wincrypt.h>
  #pragma comment(lib, "advapi32.lib")
#endif

// ============================================================
// CLASA HashUtils
// Utilitare pentru hashing parole (SHA-256)
// Parolele sunt stocate criptat in baza de date
// ============================================================
class HashUtils {
public:
    static std::string sha256(const std::string& input) {
#ifdef _WIN32
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        BYTE       rgbHash[32];
        DWORD      cbHash = 32;
        std::string result;

        if (!CryptAcquireContext(&hProv, nullptr, nullptr,
                                  PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            return "";

        if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            CryptReleaseContext(hProv, 0);
            return "";
        }

        if (!CryptHashData(hHash,
                           reinterpret_cast<const BYTE*>(input.c_str()),
                           static_cast<DWORD>(input.size()), 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }

        if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
            std::ostringstream oss;
            for (DWORD i = 0; i < cbHash; i++)
                oss << std::hex << std::setw(2)
                    << std::setfill('0')
                    << static_cast<int>(rgbHash[i]);
            result = oss.str();
        }

        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return result;
#else
        // Fallback non-Windows — inlocuieste cu OpenSSL in productie
        return input;
#endif
    }

    static bool verificaParola(const std::string& parola,
                                const std::string& hashStoccat) {
        return sha256(parola) == hashStoccat;
    }
};
