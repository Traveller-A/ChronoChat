#pragma once

#include <string>
#include <mutex>

// Forward declarations
struct sqlite3;

namespace chronochat {

// Global config manager - thread-safe singleton, backed by SQLite
class ConfigManager {
public:
    static ConfigManager& instance();

    // Initialize database connection
    bool initialize(const std::string& dbPath);

    // Text API config
    std::string getTextApiBaseUrl() const;
    void setTextApiBaseUrl(const std::string& url);
    std::string getTextApiKey() const;
    void setTextApiKey(const std::string& key);
    std::string getTextModel() const;
    void setTextModel(const std::string& model);

    // Multimodal API config
    std::string getMultimodalApiBaseUrl() const;
    void setMultimodalApiBaseUrl(const std::string& url);
    std::string getMultimodalApiKey() const;
    void setMultimodalApiKey(const std::string& key);
    std::string getMultimodalModel() const;
    void setMultimodalModel(const std::string& model);

    // Persistence
    bool loadFromDb();
    bool saveToDb();

private:
    ConfigManager();
    ~ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Ensure the config table exists
    bool ensureTable();

    // Read a single config value from DB
    std::string getDbValue(const std::string& key, const std::string& defaultVal = "") const;
    // Write a single config value to DB (upsert)
    bool setDbValue(const std::string& key, const std::string& value);

    sqlite3* db_ = nullptr;
    mutable std::mutex mutex_;

    // In-memory cache
    std::string textApiBaseUrl_;
    std::string textApiKey_;
    std::string textModel_;
    std::string multimodalApiBaseUrl_;
    std::string multimodalApiKey_;
    std::string multimodalModel_;
};

} // namespace chronochat
