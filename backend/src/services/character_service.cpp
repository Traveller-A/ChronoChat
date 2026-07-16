#include "character_service.h"
#include <sqlite3.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <filesystem>
#include <ctime>

namespace fs = std::filesystem;

namespace chronochat {

// ---- Helpers ----
static std::string generateHex(int len) {
    static const char hex[] = "0123456789abcdef";
    static std::mt19937_64 rng(std::random_device{}());
    static std::uniform_int_distribution<int> dist(0, 15);
    std::string s;
    s.reserve(len);
    for (int i = 0; i < len; ++i) s += hex[dist(rng)];
    return s;
}

static std::string makeId() {
    return generateHex(8) + "-" + generateHex(4) + "-" +
           generateHex(4) + "-" + generateHex(4) + "-" +
           generateHex(12);
}

static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

static std::string todayDate() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    return buf;
}

static const char* colText(sqlite3_stmt* stmt, int col, const char* defaultVal = "") {
    const unsigned char* txt = sqlite3_column_text(stmt, col);
    return txt ? reinterpret_cast<const char*>(txt) : defaultVal;
}

// ---- Singleton ----
CharacterService& CharacterService::instance() {
    static CharacterService svc;
    return svc;
}

// ---- Init ----
bool CharacterService::initialize(const std::string& dataDir, const std::string& dbPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    dataDir_ = dataDir;

    std::error_code ec;
    fs::create_directories(dataDir_ + "/characters", ec);
    fs::create_directories(dataDir_ + "/uploads", ec);

    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[CharacterService] Cannot open DB: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    return ensureTables();
}

void CharacterService::migrateAddColumn(const std::string& colName, const std::string& colDef) {
    // Check if column exists
    std::string checkSql = "SELECT " + colName + " FROM characters LIMIT 0;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, checkSql.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        sqlite3_finalize(stmt);
        return; // column exists
    }
    sqlite3_finalize(stmt);

    // Add column
    std::string sql = "ALTER TABLE characters ADD COLUMN " + colName + " " + colDef + ";";
    char* err = nullptr;
    sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (err) {
        std::cerr << "[CharacterService] Migration warning: " << err << std::endl;
        sqlite3_free(err);
    }
}

bool CharacterService::ensureTables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS characters (
            id              TEXT PRIMARY KEY NOT NULL,
            name            TEXT NOT NULL,
            gender          TEXT DEFAULT '',
            age             INTEGER DEFAULT 0,
            birthday        TEXT DEFAULT '',
            mbti            TEXT DEFAULT '',
            personality_signature TEXT DEFAULT '',
            avatar_path     TEXT DEFAULT '',
            story_text      TEXT DEFAULT '',
            story_images    TEXT DEFAULT '[]',
            user_description TEXT DEFAULT '',
            text_api_base_url      TEXT DEFAULT '',
            text_api_key           TEXT DEFAULT '',
            text_model             TEXT DEFAULT '',
            multimodal_api_base_url TEXT DEFAULT '',
            multimodal_api_key      TEXT DEFAULT '',
            multimodal_model        TEXT DEFAULT '',
            created_at      INTEGER NOT NULL,
            updated_at      INTEGER NOT NULL
        );
    )";
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "[CharacterService] Table error: " << err << std::endl;
        sqlite3_free(err);
        return false;
    }

    // Migrate: add columns that might not exist in older DBs
    migrateAddColumn("birthday", "TEXT DEFAULT ''");
    migrateAddColumn("user_description", "TEXT DEFAULT ''");
    migrateAddColumn("text_api_base_url", "TEXT DEFAULT ''");
    migrateAddColumn("text_api_key", "TEXT DEFAULT ''");
    migrateAddColumn("text_model", "TEXT DEFAULT ''");
    migrateAddColumn("multimodal_api_base_url", "TEXT DEFAULT ''");
    migrateAddColumn("multimodal_api_key", "TEXT DEFAULT ''");
    migrateAddColumn("multimodal_model", "TEXT DEFAULT ''");

    return true;
}

// ---- CRUD ----
static const char* kListQuery = R"(
    SELECT id, name, gender, age, birthday, mbti, personality_signature,
           avatar_path, story_text, story_images, user_description,
           text_api_base_url, text_api_key, text_model,
           multimodal_api_base_url, multimodal_api_key, multimodal_model,
           created_at, updated_at
    FROM characters ORDER BY updated_at DESC;
)";

static CharacterInfo rowToCharacter(sqlite3_stmt* stmt) {
    CharacterInfo c;
    c.id = colText(stmt, 0);
    c.name = colText(stmt, 1);
    c.gender = colText(stmt, 2);
    c.age = sqlite3_column_int(stmt, 3);
    c.birthday = colText(stmt, 4);
    c.mbti = colText(stmt, 5);
    c.personalitySignature = colText(stmt, 6);
    c.avatarPath = colText(stmt, 7);
    c.storyText = colText(stmt, 8);
    c.storyImages = colText(stmt, 9, "[]");
    c.userDescription = colText(stmt, 10);
    c.textApiBaseUrl = colText(stmt, 11);
    c.textApiKey = colText(stmt, 12);
    c.textModel = colText(stmt, 13);
    c.multimodalApiBaseUrl = colText(stmt, 14);
    c.multimodalApiKey = colText(stmt, 15);
    c.multimodalModel = colText(stmt, 16);
    c.createdAt = sqlite3_column_int64(stmt, 17);
    c.updatedAt = sqlite3_column_int64(stmt, 18);
    return c;
}

std::vector<CharacterInfo> CharacterService::listCharacters() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CharacterInfo> result;
    if (!db_) return result;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, kListQuery, -1, &stmt, nullptr) != SQLITE_OK) return result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result.push_back(rowToCharacter(stmt));
    }
    sqlite3_finalize(stmt);
    return result;
}

CharacterInfo CharacterService::getCharacter(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    CharacterInfo c;
    if (!db_) return c;

    const char* sql = R"(
        SELECT id, name, gender, age, birthday, mbti, personality_signature,
               avatar_path, story_text, story_images, user_description,
               text_api_base_url, text_api_key, text_model,
               multimodal_api_base_url, multimodal_api_key, multimodal_model,
               created_at, updated_at
        FROM characters WHERE id = ?;
    )";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return c;
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        c = rowToCharacter(stmt);
    }
    sqlite3_finalize(stmt);
    return c;
}

std::string CharacterService::createCharacter(const CharacterInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return "";

    std::string id = makeId();
    int64_t now = nowMs();
    std::string bday = info.birthday.empty() ? todayDate() : info.birthday;

    // Create character directory
    std::string charDir = dataDir_ + "/characters/" + id;
    std::error_code ec;
    fs::create_directories(charDir, ec);

    // Create empty markdown files
    std::ofstream(charDir + "/IDENTITY.md").close();
    std::ofstream(charDir + "/SOUL.md").close();
    std::ofstream(charDir + "/MEMORY.md").close();
    std::ofstream(charDir + "/USER.md").close();

    const char* sql = R"(
        INSERT INTO characters
        (id, name, gender, age, birthday, mbti, personality_signature,
         avatar_path, story_text, story_images, user_description,
         text_api_base_url, text_api_key, text_model,
         multimodal_api_base_url, multimodal_api_key, multimodal_model,
         created_at, updated_at)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);
    )";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return "";

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, info.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, info.gender.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, info.age);
    sqlite3_bind_text(stmt, 5, bday.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, info.mbti.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, info.personalitySignature.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, info.avatarPath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, info.storyText.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, info.storyImages.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, info.userDescription.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, info.textApiBaseUrl.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 13, info.textApiKey.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 14, info.textModel.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 15, info.multimodalApiBaseUrl.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 16, info.multimodalApiKey.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 17, info.multimodalModel.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 18, now);
    sqlite3_bind_int64(stmt, 19, now);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return "";
    }
    sqlite3_finalize(stmt);
    return id;
}

bool CharacterService::updateCharacter(const std::string& id, const CharacterInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return false;

    int64_t now = nowMs();
    const char* sql = R"(
        UPDATE characters SET
            name=?, gender=?, age=?, birthday=?, mbti=?, personality_signature=?,
            avatar_path=?, story_text=?, story_images=?, user_description=?,
            text_api_base_url=?, text_api_key=?, text_model=?,
            multimodal_api_base_url=?, multimodal_api_key=?, multimodal_model=?,
            updated_at=?
        WHERE id=?;
    )";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, info.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, info.gender.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, info.age);
    sqlite3_bind_text(stmt, 4, info.birthday.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, info.mbti.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, info.personalitySignature.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, info.avatarPath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, info.storyText.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, info.storyImages.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, info.userDescription.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, info.textApiBaseUrl.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, info.textApiKey.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 13, info.textModel.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 14, info.multimodalApiBaseUrl.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 15, info.multimodalApiKey.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 16, info.multimodalModel.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 17, now);
    sqlite3_bind_text(stmt, 18, id.c_str(), -1, SQLITE_STATIC);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool CharacterService::deleteCharacter(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return false;

    std::string charDir = dataDir_ + "/characters/" + id;
    std::error_code ec;
    fs::remove_all(charDir, ec);

    const char* sql = "DELETE FROM characters WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

// ---- File ops ----
std::string CharacterService::getCharacterDir(const std::string& id) const {
    return dataDir_ + "/characters/" + id;
}

bool CharacterService::writeCharacterFile(const std::string& id, const std::string& filename, const std::string& content) {
    std::string dir = getCharacterDir(id);
    std::error_code ec;
    fs::create_directories(dir, ec);
    std::ofstream out(dir + "/" + filename);
    if (!out) return false;
    out << content;
    return true;
}

std::string CharacterService::readCharacterFile(const std::string& id, const std::string& filename) const {
    std::string path = dataDir_ + "/characters/" + id + "/" + filename;
    std::ifstream in(path);
    if (!in) return "";
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

} // namespace chronochat
