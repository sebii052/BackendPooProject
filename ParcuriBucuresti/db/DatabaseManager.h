#pragma once

// Previne conflictul windows.h (winsock v1) vs winsock2.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <functional>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#pragma comment(lib, "odbc32.lib")

namespace DbInternal {
    // std::string (UTF-8) -> std::wstring (UTF-16)
    inline std::wstring toWide(const std::string& s) {
        if (s.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0,
            s.c_str(), static_cast<int>(s.size()),
            nullptr, 0);
        std::wstring result(len, 0);
        MultiByteToWideChar(CP_UTF8, 0,
            s.c_str(), static_cast<int>(s.size()),
            &result[0], len);
        return result;
    }

    // std::wstring (UTF-16) -> std::string (UTF-8)
    // Folosit pentru datele citite din SQL Server — pastreaza diacriticele
    inline std::string fromWide(const wchar_t* ws) {
        if (!ws || ws[0] == L'\0') return "";
        int len = WideCharToMultiByte(CP_UTF8, 0,
            ws, -1,
            nullptr, 0,
            nullptr, nullptr);
        if (len <= 0) return "";
        std::string result(len - 1, 0); // len include null terminator
        WideCharToMultiByte(CP_UTF8, 0,
            ws, -1,
            &result[0], len,
            nullptr, nullptr);
        return result;
    }
}
#endif

// ============================================================
// CLASA DatabaseManager
// Singleton — o singura conexiune ODBC activa
// Compilat cu Character Set: Unicode (Visual Studio default)
// ============================================================
class DatabaseManager {
private:
    static DatabaseManager* s_instance;

#ifdef _WIN32
    SQLHENV m_env = SQL_NULL_HANDLE;
    SQLHDBC m_dbc = SQL_NULL_HANDLE;
#endif
    bool m_connected = false;

    DatabaseManager() = default;

#ifdef _WIN32
    std::string getOdbcError(SQLSMALLINT handleType,
        SQLHANDLE handle) const {
        SQLWCHAR    sqlState[6];
        SQLWCHAR    message[512];
        SQLINTEGER  nativeError;
        SQLSMALLINT msgLen;
        std::string result;

        if (SQLGetDiagRecW(handleType, handle, 1,
            sqlState, &nativeError,
            message, 512, &msgLen) == SQL_SUCCESS) {
            result = DbInternal::fromWide(sqlState) +
                ": " +
                DbInternal::fromWide(message);
        }
        return result;
    }
#endif

public:
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    static DatabaseManager& getInstance() {
        if (!s_instance)
            s_instance = new DatabaseManager();
        return *s_instance;
    }

    // --------------------------------------------------------
    // Conectare la SQL Server prin ODBC
    // --------------------------------------------------------
    bool conecteaza(const std::string& server = "localhost",
        const std::string& database = "ParcuriBucuresti",
        const std::string& username = "",
        const std::string& password = "") {
        if (m_connected) return true;

#ifdef _WIN32
        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_env);
        SQLSetEnvAttr(m_env, SQL_ATTR_ODBC_VERSION,
            reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        SQLAllocHandle(SQL_HANDLE_DBC, m_env, &m_dbc);

        std::string connStr;
        if (username.empty()) {
            connStr = "DRIVER={ODBC Driver 17 for SQL Server};"
                "SERVER=" + server + ";"
                "DATABASE=" + database + ";"
                "Trusted_Connection=Yes;"
                "TrustServerCertificate=Yes;";
        }
        else {
            connStr = "DRIVER={ODBC Driver 17 for SQL Server};"
                "SERVER=" + server + ";"
                "DATABASE=" + database + ";"
                "UID=" + username + ";"
                "PWD=" + password + ";"
                "TrustServerCertificate=Yes;";
        }

        std::wstring wConnStr = DbInternal::toWide(connStr);
        SQLWCHAR     outConnStr[1024];
        SQLSMALLINT  outLen;

        SQLRETURN ret = SQLDriverConnectW(
            m_dbc, nullptr,
            const_cast<SQLWCHAR*>(wConnStr.c_str()),
            SQL_NTS,
            outConnStr, 1024,
            &outLen, SQL_DRIVER_NOPROMPT);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::string err = getOdbcError(SQL_HANDLE_DBC, m_dbc);
            deconecteaza();
            throw std::runtime_error("Eroare conectare DB: " + err);
        }

        m_connected = true;
        return true;
#else
        (void)server; (void)database;
        (void)username; (void)password;
        throw std::runtime_error(
            "DatabaseManager suportat doar pe Windows.");
#endif
    }

    void deconecteaza() {
#ifdef _WIN32
        if (m_dbc != SQL_NULL_HANDLE) {
            SQLDisconnect(m_dbc);
            SQLFreeHandle(SQL_HANDLE_DBC, m_dbc);
            m_dbc = SQL_NULL_HANDLE;
        }
        if (m_env != SQL_NULL_HANDLE) {
            SQLFreeHandle(SQL_HANDLE_ENV, m_env);
            m_env = SQL_NULL_HANDLE;
        }
#endif
        m_connected = false;
    }

    bool esteConectat() const { return m_connected; }

    // --------------------------------------------------------
    // Executa query fara rezultat (INSERT, UPDATE, DELETE)
    // --------------------------------------------------------
    void executa(const std::string& query) {
        if (!m_connected)
            throw std::runtime_error(
                "Nu esti conectat la baza de date.");
#ifdef _WIN32
        std::wstring wQuery = DbInternal::toWide(query);
        SQLHSTMT stmt;
        SQLAllocHandle(SQL_HANDLE_STMT, m_dbc, &stmt);

        SQLRETURN ret = SQLExecDirectW(
            stmt,
            const_cast<SQLWCHAR*>(wQuery.c_str()),
            SQL_NTS);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::string err = getOdbcError(SQL_HANDLE_STMT, stmt);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            throw std::runtime_error(
                "Eroare SQL: " + err + "\nQuery: " + query);
        }
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
#else
        (void)query;
#endif
    }

    // --------------------------------------------------------
    // Executa query cu rezultate; apeleaza callback per rand
    // --------------------------------------------------------
#ifdef _WIN32
    void interogheaza(const std::string& query,
        std::function<void(SQLHSTMT)> callback) {
        if (!m_connected)
            throw std::runtime_error(
                "Nu esti conectat la baza de date.");

        std::wstring wQuery = DbInternal::toWide(query);
        SQLHSTMT stmt;
        SQLAllocHandle(SQL_HANDLE_STMT, m_dbc, &stmt);

        SQLRETURN ret = SQLExecDirectW(
            stmt,
            const_cast<SQLWCHAR*>(wQuery.c_str()),
            SQL_NTS);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::string err = getOdbcError(SQL_HANDLE_STMT, stmt);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            throw std::runtime_error("Eroare SQL: " + err);
        }

        while (SQLFetch(stmt) == SQL_SUCCESS)
            callback(stmt);

        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }

    // --------------------------------------------------------
    // Helpere: citire coloane din result set
    // --------------------------------------------------------
    static std::string getColumn(SQLHSTMT stmt,
        SQLUSMALLINT col) {
        SQLWCHAR buf[2048];  // suficient pentru texte lungi cu diacritice
        SQLLEN   indicator;
        buf[0] = L'\0';
        SQLGetData(stmt, col, SQL_C_WCHAR,
            buf, sizeof(buf), &indicator);
        if (indicator == SQL_NULL_DATA) return "";
        buf[2047] = L'\0';  // null terminator de siguranta
        return DbInternal::fromWide(buf);
    }

    static int getColumnInt(SQLHSTMT stmt,
        SQLUSMALLINT col) {
        // Citim ca WCHAR (la fel ca getColumn) pentru consistenta
        // Evita probleme de cursor state dupa mixed-type reads
        SQLWCHAR buf[32] = { 0 };
        SQLLEN   indicator = 0;
        SQLRETURN ret = SQLGetData(stmt, col, SQL_C_WCHAR,
            buf, sizeof(buf), &indicator);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return 0;
        if (indicator == SQL_NULL_DATA) return 0;
        buf[31] = L'\0';
        if (buf[0] == L'\0') return 0;
        // Convertire wchar -> int fara a trece prin std::string
        // (evita probleme cu caractere non-ASCII, desi int-urile sunt ASCII)
        int result = 0;
        bool negative = false;
        const wchar_t* p = buf;
        if (*p == L'-') { negative = true; p++; }
        while (*p >= L'0' && *p <= L'9') {
            result = result * 10 + (*p - L'0');
            p++;
        }
        return negative ? -result : result;
    }

    static double getColumnDouble(SQLHSTMT stmt,
        SQLUSMALLINT col) {
        SQLDOUBLE val = 0.0;
        SQLLEN    indicator;
        SQLGetData(stmt, col, SQL_C_DOUBLE,
            &val, sizeof(val), &indicator);
        if (indicator == SQL_NULL_DATA) return 0.0;
        return static_cast<double>(val);
    }
#endif // _WIN32

    ~DatabaseManager() { deconecteaza(); }
};

inline DatabaseManager* DatabaseManager::s_instance = nullptr;