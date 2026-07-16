#include "config_manager.h"
#include <sqlite3.h>
#include <iostream>

namespace chronochat {

ConfigManager& ConfigManager::instance() {
    static ConfigManager mgr;
    return mgr;
}

ConfigManager::ConfigManager() = default;

ConfigManager::~ConfigManager() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool ConfigManager::initialize(const std::string& dbPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_) return true;

    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[ConfigManager] Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    // Enable WAL mode for better concurrent access
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);

    if (!ensureTable()) return false;
    if (!loadFromDb()) return false;

    return true;
}

bool ConfigManager::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS system_config (
            key   TEXT PRIMARY KEY NOT NULL,
            value TEXT NOT NULL DEFAULT ''
        );
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[ConfigManager] Create table error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::string ConfigManager::getDbValue(const std::string& key, const std::string& defaultVal) const {
    if (!db_) return defaultVal;

    const char* sql = "SELECT value FROM system_config WHERE key = ?;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return defaultVal;

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    std::string result = defaultVal;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (val) result = val;
    }
    sqlite3_finalize(stmt);
    return result;
}

bool ConfigManager::setDbValue(const std::string& key, const std::string& value) {
    if (!db_) return false;

    const char* sql = R"(
        INSERT INTO system_config (key, value) VALUES (?, ?)
        ON CONFLICT(key) DO UPDATE SET value = excluded.value;
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool ConfigManager::loadFromDb() {
    if (!db_) return false;

    textApiBaseUrl_       = getDbValue("text_api_base_url");
    textApiKey_           = getDbValue("text_api_key");
    textModel_            = getDbValue("text_model");
    multimodalApiBaseUrl_ = getDbValue("multimodal_api_base_url");
    multimodalApiKey_     = getDbValue("multimodal_api_key");
    multimodalModel_      = getDbValue("multimodal_model");

    std::cout << "[ConfigManager] Config loaded from database" << std::endl;
    return true;
}

bool ConfigManager::saveToDb() {
    if (!db_) return false;

    bool ok = true;
    ok &= setDbValue("text_api_base_url",       textApiBaseUrl_);
    ok &= setDbValue("text_api_key",             textApiKey_);
    ok &= setDbValue("text_model",               textModel_);
    ok &= setDbValue("multimodal_api_base_url",  multimodalApiBaseUrl_);
    ok &= setDbValue("multimodal_api_key",       multimodalApiKey_);
    ok &= setDbValue("multimodal_model",         multimodalModel_);

    return ok;
}

// ---- Getters / Setters ----

std::string ConfigManager::getTextApiBaseUrl() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return textApiBaseUrl_;
}
void ConfigManager::setTextApiBaseUrl(const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);
    textApiBaseUrl_ = url;
}

std::string ConfigManager::getTextApiKey() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return textApiKey_;
}
void ConfigManager::setTextApiKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    textApiKey_ = key;
}

std::string ConfigManager::getTextModel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return textModel_;
}
void ConfigManager::setTextModel(const std::string& model) {
    std::lock_guard<std::mutex> lock(mutex_);
    textModel_ = model;
}

std::string ConfigManager::getMultimodalApiBaseUrl() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return multimodalApiBaseUrl_;
}
void ConfigManager::setMultimodalApiBaseUrl(const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);
    multimodalApiBaseUrl_ = url;
}

std::string ConfigManager::getMultimodalApiKey() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return multimodalApiKey_;
}
void ConfigManager::setMultimodalApiKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    multimodalApiKey_ = key;
}

std::string ConfigManager::getMultimodalModel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return multimodalModel_;
}
void ConfigManager::setMultimodalModel(const std::string& model) {
    std::lock_guard<std::mutex> lock(mutex_);
    multimodalModel_ = model;
}

} // namespace chronochat
