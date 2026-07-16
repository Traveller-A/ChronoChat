#include "group_service.h"
#include <sqlite3.h>
#include <json/json.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <algorithm>

namespace fs = std::filesystem;

namespace chronochat {

static std::string makeId() {
    static const char hex[] = "0123456789abcdef";
    static std::mt19937_64 rng(std::random_device{}());
    static std::uniform_int_distribution<int> dist(0, 15);
    auto gen = [](int n){ std::string s; for(int i=0;i<n;++i) s+=hex[dist(rng)]; return s; };
    return gen(8)+"-"+gen(4)+"-"+gen(4)+"-"+gen(4)+"-"+gen(12);
}
static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
static const char* colText(sqlite3_stmt* stmt, int col, const char* d = "") {
    const unsigned char* t = sqlite3_column_text(stmt, col);
    return t ? reinterpret_cast<const char*>(t) : d;
}

GroupService& GroupService::instance() { static GroupService s; return s; }

bool GroupService::initialize(const std::string& dataDir, const std::string& dbPath) {
    dataDir_ = dataDir;
    fs::create_directories(dataDir_ + "/groups", std::error_code{});
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) return false;
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    return ensureTables();
}

bool GroupService::ensureTables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS groups (
            id           TEXT PRIMARY KEY NOT NULL,
            name         TEXT NOT NULL,
            description  TEXT DEFAULT '',
            avatar_path  TEXT DEFAULT '',
            mode         TEXT DEFAULT 'mention',
            status       TEXT DEFAULT 'open',
            members_json TEXT DEFAULT '[]',
            created_at   INTEGER NOT NULL,
            updated_at   INTEGER NOT NULL
        );
    )";
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) { sqlite3_free(err); return false; }

    // Migrate: add columns that might not exist in older DBs
    auto addCol = [this](const char* col, const char* def) {
        std::string check = "SELECT " + std::string(col) + " FROM groups LIMIT 0;";
        sqlite3_stmt* st = nullptr;
        if (sqlite3_prepare_v2(db_, check.c_str(), -1, &st, nullptr) == SQLITE_OK) { sqlite3_finalize(st); return; }
        sqlite3_finalize(st);
        std::string alt = "ALTER TABLE groups ADD COLUMN " + std::string(col) + " " + def + ";";
        sqlite3_exec(db_, alt.c_str(), nullptr, nullptr, nullptr);
    };
    addCol("description", "TEXT DEFAULT ''");
    addCol("mode", "TEXT DEFAULT 'mention'");

    return true;
}

std::vector<GroupInfo> GroupService::listGroups() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<GroupInfo> result;
    if (!db_) return result;
    const char* sql = "SELECT id,name,description,avatar_path,mode,status,members_json,created_at,updated_at FROM groups ORDER BY updated_at DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GroupInfo g;
        g.id=colText(stmt,0); g.name=colText(stmt,1); g.description=colText(stmt,2);
        g.avatarPath=colText(stmt,3); g.mode=colText(stmt,4,"mention"); g.status=colText(stmt,5,"open");
        g.membersJson=colText(stmt,6,"[]");
        g.createdAt=sqlite3_column_int64(stmt,7); g.updatedAt=sqlite3_column_int64(stmt,8);
        result.push_back(g);
    }
    sqlite3_finalize(stmt);
    return result;
}

GroupInfo GroupService::getGroup(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    GroupInfo g;
    if (!db_) return g;
    const char* sql = "SELECT id,name,description,avatar_path,mode,status,members_json,created_at,updated_at FROM groups WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return g;
    sqlite3_bind_text(stmt,1,id.c_str(),-1,SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        g.id=colText(stmt,0); g.name=colText(stmt,1); g.description=colText(stmt,2);
        g.avatarPath=colText(stmt,3); g.mode=colText(stmt,4,"mention"); g.status=colText(stmt,5,"open");
        g.membersJson=colText(stmt,6,"[]");
        g.createdAt=sqlite3_column_int64(stmt,7); g.updatedAt=sqlite3_column_int64(stmt,8);
    }
    sqlite3_finalize(stmt);
    return g;
}

std::string GroupService::createGroup(const GroupInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return "";
    std::string id = makeId();
    int64_t now = nowMs();
    fs::create_directories(dataDir_ + "/groups/" + id, std::error_code{});
    const char* sql = R"(
        INSERT INTO groups(id,name,description,avatar_path,mode,status,members_json,created_at,updated_at)
        VALUES(?,?,?,?,?,?,?,?,?);
    )";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return "";
    sqlite3_bind_text(stmt,1,id.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,2,info.name.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,3,info.description.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,4,info.avatarPath.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,5,info.mode.empty()?"mention":info.mode.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,6,info.status.empty()?"open":info.status.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,7,info.membersJson.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_int64(stmt,8,now);
    sqlite3_bind_int64(stmt,9,now);
    if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); return ""; }
    sqlite3_finalize(stmt);
    return id;
}

bool GroupService::updateGroup(const std::string& id, const GroupInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return false;
    int64_t now = nowMs();
    const char* sql = R"(
        UPDATE groups SET name=?,description=?,avatar_path=?,mode=?,status=?,members_json=?,updated_at=? WHERE id=?;
    )";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt,1,info.name.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,2,info.description.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,3,info.avatarPath.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,4,info.mode.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,5,info.status.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,6,info.membersJson.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_int64(stmt,7,now);
    sqlite3_bind_text(stmt,8,id.c_str(),-1,SQLITE_STATIC);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool GroupService::deleteGroup(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    fs::remove_all(dataDir_ + "/groups/" + id, std::error_code{});
    if (!db_) return false;
    const char* sql = "DELETE FROM groups WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt,1,id.c_str(),-1,SQLITE_STATIC);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<std::string> GroupService::getMemberIds(const std::string& groupId) const {
    auto g = getGroup(groupId);
    std::vector<std::string> ids;
    Json::CharReaderBuilder builder;
    std::string errors;
    Json::Value root;
    std::istringstream stream(g.membersJson);
    if (Json::parseFromStream(builder, stream, &root, &errors) && root.isArray()) {
        for (const auto& v : root) ids.push_back(v.asString());
    }
    return ids;
}

bool GroupService::addMember(const std::string& groupId, const std::string& charId) {
    auto ids = getMemberIds(groupId);
    if (std::find(ids.begin(), ids.end(), charId) != ids.end()) return false;
    ids.push_back(charId);
    Json::Value arr(Json::arrayValue);
    for (auto& id : ids) arr.append(id);
    Json::StreamWriterBuilder w; w["indentation"] = "";
    auto g = getGroup(groupId);
    g.membersJson = Json::writeString(w, arr);
    return updateGroup(groupId, g);
}

bool GroupService::removeMember(const std::string& groupId, const std::string& charId) {
    auto ids = getMemberIds(groupId);
    ids.erase(std::remove(ids.begin(), ids.end(), charId), ids.end());
    Json::Value arr(Json::arrayValue);
    for (auto& id : ids) arr.append(id);
    Json::StreamWriterBuilder w; w["indentation"] = "";
    auto g = getGroup(groupId);
    g.membersJson = Json::writeString(w, arr);
    return updateGroup(groupId, g);
}

std::string GroupService::getChatLog(const std::string& groupId) const {
    std::string path = dataDir_ + "/groups/" + groupId + "/chat.log";
    std::ifstream in(path);
    if (!in) return "";
    std::ostringstream oss; oss << in.rdbuf(); return oss.str();
}

void GroupService::appendChatLog(const std::string& groupId, const std::string& entry) {
    std::string path = dataDir_ + "/groups/" + groupId + "/chat.log";
    std::ofstream out(path, std::ios::app);
    out << entry << "\n";
}

std::string GroupService::getGroupDir(const std::string& id) const {
    return dataDir_ + "/groups/" + id;
}

} // namespace chronochat
