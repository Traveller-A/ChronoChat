#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

struct sqlite3;

namespace chronochat {

struct GroupInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string avatarPath;
    std::string mode;          // "mention" or "auto"
    std::string status;        // "open", "paused", "closed"
    std::string membersJson;   // JSON array of character IDs
    int64_t createdAt = 0;
    int64_t updatedAt = 0;
};

class GroupService {
public:
    static GroupService& instance();
    bool initialize(const std::string& dataDir, const std::string& dbPath);

    std::vector<GroupInfo> listGroups() const;
    std::string createGroup(const GroupInfo& info);
    bool updateGroup(const std::string& id, const GroupInfo& info);
    bool deleteGroup(const std::string& id);
    GroupInfo getGroup(const std::string& id) const;

    // Members
    std::vector<std::string> getMemberIds(const std::string& groupId) const;
    bool addMember(const std::string& groupId, const std::string& charId);
    bool removeMember(const std::string& groupId, const std::string& charId);

    // Chat log
    std::string getChatLog(const std::string& groupId) const;
    std::string getRecentChatLog(const std::string& groupId, int maxLines) const;
    void appendChatLog(const std::string& groupId, const std::string& entry);

    // List ordering: bump last chat timestamp so the group list shows
    // most recently chatted groups first.
    void touchLastChatAt(const std::string& groupId);

    // Admin instructions (for auto-dialogue mode)
    std::string getAdminInstructions(const std::string& groupId) const;
    void setAdminInstructions(const std::string& groupId, const std::string& instructions);

    // Update group mode only
    bool updateMode(const std::string& groupId, const std::string& mode);

    std::string dataDir() const { return dataDir_; }
    std::string getGroupDir(const std::string& id) const;

private:
    GroupService() = default;
    bool ensureTables();
    // One-time backfill: derive last_chat_at from existing chat.log files for
    // rows that predate the column. Called from ensureTables (unlocked).
    void backfillLastChatAt();
    std::string dataDir_;
    sqlite3* db_ = nullptr;
    mutable std::mutex mutex_;
};

} // namespace chronochat
