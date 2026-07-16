#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

struct sqlite3;

namespace chronochat {

struct CharacterInfo {
    std::string id;
    std::string name;
    std::string gender;
    int age = 0;
    std::string birthday;               // YYYY-MM-DD, defaults to creation date
    std::string mbti;
    std::string personalitySignature;
    std::string avatarPath;
    std::string storyText;              // immutable after creation
    std::string storyImages;
    std::string userDescription;        // character's view of the user (optional)
    std::string lastActiveDate;         // last conversation date, for memory compression trigger

    // Per-character API config (stored in DB, not in files; empty = use global)
    std::string textApiBaseUrl;
    std::string textApiKey;
    std::string textModel;
    std::string multimodalApiBaseUrl;
    std::string multimodalApiKey;
    std::string multimodalModel;

    int64_t createdAt = 0;
    int64_t updatedAt = 0;
};

class CharacterService {
public:
    static CharacterService& instance();

    bool initialize(const std::string& dataDir, const std::string& dbPath);

    std::vector<CharacterInfo> listCharacters() const;
    std::string createCharacter(const CharacterInfo& info);
    bool updateCharacter(const std::string& id, const CharacterInfo& info);
    bool deleteCharacter(const std::string& id);
    CharacterInfo getCharacter(const std::string& id) const;

    // File operations
    std::string getCharacterDir(const std::string& id) const;
    bool writeCharacterFile(const std::string& id, const std::string& filename, const std::string& content);
    std::string readCharacterFile(const std::string& id, const std::string& filename) const;

    std::string dataDir() const { return dataDir_; }

    // Memory compression: check & trigger on date change
    void updateLastActiveDate(const std::string& id, const std::string& date);
    std::string getLastActiveDate(const std::string& id) const;

private:
    CharacterService() = default;
    CharacterService(const CharacterService&) = delete;
    CharacterService& operator=(const CharacterService&) = delete;

    bool ensureTables();
    void migrateAddColumn(const std::string& colName, const std::string& colDef);

    std::string dataDir_;
    sqlite3* db_ = nullptr;
    mutable std::mutex mutex_;
};

} // namespace chronochat
