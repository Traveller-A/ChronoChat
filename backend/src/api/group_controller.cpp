#include "group_controller.h"
#include "../services/group_service.h"
#include "../services/character_service.h"
#include "../config/config_manager.h"
#include "../ai/ai_service.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <regex>
#include <sstream>
#include <algorithm>

namespace chronochat {

static Json::Value groupToJson(const GroupInfo& g) {
    Json::Value j;
    j["id"]=g.id; j["name"]=g.name; j["description"]=g.description;
    j["avatar_path"]=g.avatarPath; j["mode"]=g.mode; j["status"]=g.status;
    j["members_json"]=g.membersJson;
    j["created_at"]=static_cast<Json::Int64>(g.createdAt);
    j["updated_at"]=static_cast<Json::Int64>(g.updatedAt);
    return j;
}
static void cors(const drogon::HttpResponsePtr& r) { r->addHeader("Access-Control-Allow-Origin", "*"); }
static Json::Value ok(const std::string& m="ok") { Json::Value j; j["code"]=0; j["message"]=m; return j; }
static Json::Value err(const std::string& m) { Json::Value j; j["code"]=1; j["message"]=m; return j; }

// ---- Helpers ----

// Normalize URL: ensure ends with /v1
static std::string normalizeUrl(const std::string& url) {
    if (url.empty()) return url;
    std::string result = url;
    while (!result.empty() && result.back() == '/') result.pop_back();
    if (result.length() < 3 || result.substr(result.length() - 3) != "/v1")
        result += "/v1";
    return result;
}

// Parse @mentions from a message. Returns vector of mentioned character names.
static std::vector<std::string> parseMentions(const std::string& message) {
    std::vector<std::string> mentions;
    std::regex mentionRegex("@([\\w\\u4e00-\\u9fff\\u3400-\\u4dbf]+)");
    std::sregex_iterator it(message.begin(), message.end(), mentionRegex);
    std::sregex_iterator end;
    for (; it != end; ++it) {
        std::string name = (*it)[1].str();
        // Deduplicate
        if (std::find(mentions.begin(), mentions.end(), name) == mentions.end()) {
            mentions.push_back(name);
        }
    }
    return mentions;
}

// Find a character by name among group members
static std::string findCharIdByName(const std::vector<std::string>& memberIds, const std::string& name) {
    for (const auto& cid : memberIds) {
        auto c = CharacterService::instance().getCharacter(cid);
        if (!c.id.empty() && c.name == name) {
            return c.id;
        }
    }
    return "";
}

void GroupController::list(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto groups = GroupService::instance().listGroups();
    Json::Value arr(Json::arrayValue);
    for (auto& g : groups) arr.append(groupToJson(g));
    auto r = ok(); r["data"]=arr;
    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
}

void GroupController::create(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto json = req->getJsonObject();
    if (!json) { auto hr = drogon::HttpResponse::newHttpJsonResponse(err("Invalid JSON")); cors(hr); cb(hr); return; }
    GroupInfo info;
    info.name = (*json).get("name","").asString();
    info.description = (*json).get("description","").asString();
    info.mode = (*json).get("mode","mention").asString();
    info.membersJson = (*json).get("members_json","[]").asString();
    if (info.name.empty()) { auto hr = drogon::HttpResponse::newHttpJsonResponse(err("Name required")); cors(hr); cb(hr); return; }
    std::string id = GroupService::instance().createGroup(info);
    if (id.empty()) { auto hr = drogon::HttpResponse::newHttpJsonResponse(err("Create failed")); cors(hr); cb(hr); return; }
    auto r = ok("Group created"); r["data"]=groupToJson(GroupService::instance().getGroup(id));
    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
}

void GroupController::getOne(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id) {
    auto g = GroupService::instance().getGroup(id);
    if (g.id.empty()) { auto hr = drogon::HttpResponse::newHttpJsonResponse(err("Not found")); cors(hr); cb(hr); return; }
    auto r = ok(); r["data"]=groupToJson(g);
    Json::Value memberDetails(Json::arrayValue);
    for (auto& cid : GroupService::instance().getMemberIds(id)) {
        auto c = CharacterService::instance().getCharacter(cid);
        if (!c.id.empty()) {
            Json::Value m;
            m["id"]=c.id;
            m["name"]=c.name;
            m["avatar_path"]=c.avatarPath;
            m["personality_signature"]=c.personalitySignature;
            memberDetails.append(m);
        }
    }
    r["data"]["member_details"] = memberDetails;
    r["data"]["member_count"] = static_cast<int>(memberDetails.size());
    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
}

void GroupController::update(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id) {
    auto json = req->getJsonObject();
    if (!json) { auto hr = drogon::HttpResponse::newHttpJsonResponse(err("Invalid JSON")); cors(hr); cb(hr); return; }
    auto g = GroupService::instance().getGroup(id);
    if (g.id.empty()) { auto hr = drogon::HttpResponse::newHttpJsonResponse(err("Not found")); cors(hr); cb(hr); return; }
    if (json->isMember("name")) g.name = (*json)["name"].asString();
    if (json->isMember("description")) g.description = (*json)["description"].asString();
    if (json->isMember("mode")) g.mode = (*json)["mode"].asString();
    if (json->isMember("status")) g.status = (*json)["status"].asString();
    if (json->isMember("avatar_path")) g.avatarPath = (*json)["avatar_path"].asString();
    if (json->isMember("members_json")) g.membersJson = (*json)["members_json"].asString();
    bool okb = GroupService::instance().updateGroup(id, g);
    auto r = okb ? ok("Updated") : err("Update failed");
    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
}

void GroupController::remove(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id) {
    bool okb = GroupService::instance().deleteGroup(id);
    auto r = okb ? ok("Deleted") : err("Delete failed");
    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
}

void GroupController::addMember(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id) {
    auto json = req->getJsonObject();
    std::string cid = json ? (*json).get("char_id","").asString() : "";
    if (cid.empty()) { auto hr = drogon::HttpResponse::newHttpJsonResponse(err("char_id required")); cors(hr); cb(hr); return; }
    bool okb = GroupService::instance().addMember(id, cid);
    auto r = okb ? ok("Member added") : err("Add failed");
    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
}

void GroupController::removeMember(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& gid, const std::string& cid) {
    bool okb = GroupService::instance().removeMember(gid, cid);
    auto r = okb ? ok("Member removed") : err("Remove failed");
    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
}

void GroupController::chat(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id) {
    auto json = req->getJsonObject();
    if (!json) { auto hr = drogon::HttpResponse::newHttpJsonResponse(err("Invalid JSON")); cors(hr); cb(hr); return; }

    std::string message = (*json).get("message","").asString();
    std::string senderId = (*json).get("sender_id","").asString();

    if (message.empty()) { auto hr = drogon::HttpResponse::newHttpJsonResponse(err("Message required")); cors(hr); cb(hr); return; }

    // Get group info
    auto g = GroupService::instance().getGroup(id);
    if (g.id.empty()) { auto hr = drogon::HttpResponse::newHttpJsonResponse(err("Group not found")); cors(hr); cb(hr); return; }

    auto memberIds = GroupService::instance().getMemberIds(id);

    // Determine sender display name
    std::string senderName = "Me";
    if (!senderId.empty()) {
        auto c = CharacterService::instance().getCharacter(senderId);
        if (!c.id.empty()) senderName = c.name;
    }

    // Append user message to chat log
    std::string logEntry = "[" + senderName + "]: " + message;
    GroupService::instance().appendChatLog(id, logEntry);

    // Check for @mentions (only process if message is from user, i.e. sender_id empty)
    if (senderId.empty()) {
        auto mentions = parseMentions(message);

        // Filter out mention targets that are not group members
        std::vector<std::string> validTargets;
        for (const auto& name : mentions) {
            std::string cid = findCharIdByName(memberIds, name);
            if (!cid.empty()) {
                validTargets.push_back(name);
            }
        }

        if (!validTargets.empty()) {
            // Process first mentioned character only (Phase 2 limitation)
            std::string targetName = validTargets[0];
            std::string targetId = findCharIdByName(memberIds, targetName);

            if (!targetId.empty()) {
                auto& charSvc = CharacterService::instance();
                auto c = charSvc.getCharacter(targetId);

                // API config: character-specific > global
                std::string apiUrl = c.textApiBaseUrl;
                std::string apiKey = c.textApiKey;
                std::string apiModel = c.textModel;
                if (apiUrl.empty() || apiKey.empty()) {
                    apiUrl = ConfigManager::instance().getTextApiBaseUrl();
                    apiKey = ConfigManager::instance().getTextApiKey();
                }
                if (apiModel.empty()) apiModel = ConfigManager::instance().getTextModel();

                if (apiUrl.empty() || apiKey.empty()) {
                    auto r = ok("Message sent (no API configured)");
                    r["data"]["reply"] = "";
                    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
                    return;
                }

                apiUrl = normalizeUrl(apiUrl);

                // Build system prompt for group chat context
                std::string identity = charSvc.readCharacterFile(targetId, "IDENTITY.md");
                std::string soul = charSvc.readCharacterFile(targetId, "SOUL.md");
                std::string memory = charSvc.readCharacterFile(targetId, "MEMORY.md");
                std::string userRel = charSvc.readCharacterFile(targetId, "USER.md");

                // Get recent chat history for context (last 30 lines)
                std::string recentChat = GroupService::instance().getRecentChatLog(id, 30);

                // Get member names for context
                std::ostringstream memberList;
                for (size_t i = 0; i < memberIds.size(); ++i) {
                    auto mc = charSvc.getCharacter(memberIds[i]);
                    if (!mc.id.empty()) {
                        if (i > 0) memberList << ", ";
                        memberList << mc.name;
                    }
                }

                std::ostringstream sysPrompt;
                sysPrompt << "You are roleplaying as " << targetName
                          << " in a group chat. The group members are: "
                          << memberList.str() << ".\n\n";

                if (!identity.empty())
                    sysPrompt << "## Your Identity\n" << identity << "\n\n";
                if (!soul.empty())
                    sysPrompt << "## Your Personality & Soul\n" << soul << "\n\n";
                if (!userRel.empty())
                    sysPrompt << "## Your Relationship with the User\n" << userRel << "\n\n";
                if (!memory.empty())
                    sysPrompt << "## Your Core Memories\n" << memory << "\n\n";

                sysPrompt << "## Recent Group Chat History\n"
                          << (recentChat.empty() ? "(No history yet)" : recentChat)
                          << "\n\n";

                sysPrompt << "Instructions:\n"
                          << "- You are in a group chat. The message above is directed at you (you were @mentioned).\n"
                          << "- Respond naturally as your character would in a group setting.\n"
                          << "- Keep your response concise (1-3 paragraphs).\n"
                          << "- You can refer to other group members by name if relevant.\n"
                          << "- Reference past events from your memories and chat history when relevant.\n"
                          << "- Stay in character at all times.\n"
                          << "- Do NOT include roleplay markers like *action* or [narrative].";

                // Capture vars for the callback
                std::string capturedName = targetName;
                std::string capturedId = targetId;

                AIService::instance().chat(apiUrl, apiKey, apiModel,
                    sysPrompt.str(), message,
                    [cb, id, capturedName, capturedId, message](bool success, const std::string& aiResponse) {
                        if (!success) {
                            auto r = ok("Message sent (AI failed)");
                            r["data"]["reply"] = "";
                            auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
                            return;
                        }

                        // Append AI response to chat log
                        std::string aiLog = "[" + capturedName + "]: " + aiResponse;
                        GroupService::instance().appendChatLog(id, aiLog);

                        auto r = ok("Message sent");
                        r["data"]["reply"] = aiResponse;
                        r["data"]["responder_name"] = capturedName;
                        r["data"]["responder_id"] = capturedId;
                        auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
                    });
                return; // Will respond via callback
            }
        }
    }

    // No @mention or not from user — just log and return success with no reply
    auto r = ok("Message sent");
    r["data"]["reply"] = "";
    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
}

void GroupController::history(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id) {
    std::string log = GroupService::instance().getChatLog(id);
    auto r = ok();
    r["data"]["history"] = log;
    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
}

} // namespace chronochat
