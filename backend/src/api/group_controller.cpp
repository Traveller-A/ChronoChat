#include "group_controller.h"
#include "../services/group_service.h"
#include "../services/character_service.h"
#include "../config/config_manager.h"
#include "../ai/ai_service.h"
#include "../ai/admin_agent.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <sstream>
#include <algorithm>
#include <cctype>

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

// Parse @mentions from a message using manual string scanning (avoids MSVC regex Unicode issues).
// Returns vector of mentioned names (deduplicated).
static std::vector<std::string> parseMentions(const std::string& message) {
    std::vector<std::string> mentions;
    size_t i = 0;
    while (i < message.length()) {
        // Find '@'
        size_t atPos = message.find('@', i);
        if (atPos == std::string::npos) break;

        // Extract name: start from char after @, go until whitespace or end
        size_t nameStart = atPos + 1;
        if (nameStart >= message.length()) break;

        size_t nameEnd = nameStart;
        while (nameEnd < message.length() && !std::isspace(static_cast<unsigned char>(message[nameEnd]))) {
            ++nameEnd;
        }

        std::string name = message.substr(nameStart, nameEnd - nameStart);
        if (!name.empty()) {
            // Deduplicate
            if (std::find(mentions.begin(), mentions.end(), name) == mentions.end()) {
                mentions.push_back(name);
            }
        }
        i = nameEnd;
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

// Replace "[CharacterName]: " with "[你]: " in chat history so the AI
// recognizes its own past messages and won't repeat them.
static std::string replaceOwnName(const std::string& history, const std::string& characterName) {
    std::string marker = "[" + characterName + "]: ";
    std::string replacement = "[你]: ";
    std::string result = history;
    size_t pos = 0;
    while ((pos = result.find(marker, pos)) != std::string::npos) {
        result.replace(pos, marker.length(), replacement);
        pos += replacement.length();
    }
    return result;
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

    // Determine sender display name (use "Me" as placeholder; replaced with userName in prompts)
    std::string senderName = "Me";
    if (!senderId.empty()) {
        auto c = CharacterService::instance().getCharacter(senderId);
        if (!c.id.empty()) senderName = c.name;
    }

    // Append user/character message to chat log
    std::string logEntry = "[" + senderName + "]: " + message;
    GroupService::instance().appendChatLog(id, logEntry);

    // Check for @mentions (only process if message is from user, i.e. sender_id empty)
    if (senderId.empty()) {
        // Check for @admin / @管理员 — store as admin instructions
        bool isAdminCmd = false;
        {
            auto mentions = parseMentions(message);
            for (const auto& name : mentions) {
                if (name == "admin" || name == "\xE7\xAE\xA1\xE7\x90\x86\xE5\x91\x98") {
                    isAdminCmd = true;
                    break;
                }
            }
        }
        if (isAdminCmd) {
            // Store the message as admin instructions
            std::string existing = GroupService::instance().getAdminInstructions(id);
            std::string newInstructions = existing.empty()
                ? message
                : existing + "\n" + message;
            GroupService::instance().setAdminInstructions(id, newInstructions);

            auto r = ok("Admin instruction saved");
            r["data"]["reply"] = "Got it! I'll follow your scheduling instructions.";
            r["data"]["responder_name"] = "Admin";
            r["data"]["responder_id"] = "";
            // Also log the admin response
            GroupService::instance().appendChatLog(id, "[Admin]: Got it! I'll follow your scheduling instructions.");
            auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
            return;
        }

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
                // Replace own name so AI won't repeat past messages
                recentChat = replaceOwnName(recentChat, targetName);
                // Replace "Me" with configured user name so AI addresses user correctly
                std::string userName = ConfigManager::instance().getUserName();
                {
                    std::string meMarker = "[Me]: ";
                    std::string userMarker = "[" + userName + "]: ";
                    size_t pos = 0;
                    while ((pos = recentChat.find(meMarker, pos)) != std::string::npos) {
                        recentChat.replace(pos, meMarker.length(), userMarker);
                        pos += userMarker.length();
                    }
                }

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
                sysPrompt << "=== Group Chat Context ===\n"
                          << "You are " << targetName << ", and you are currently in a group chat.\n"
                          << "Group members: " << memberList.str() << "\n"
                          << "The user '" << userName << "' is also in this group.\n"
                          << "When someone sends a message starting with @" << targetName
                          << ", they are speaking directly to you.\n\n";

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
                          << "- You are a member of this group chat, alongside: "
                          << memberList.str() << ".\n"
                          << "- The human user is named '" << userName << "'. Address them as " << userName << ".\n"
                          << "- The user @" << targetName << " to get your attention. The message above is directed at you.\n"
                          << "- Respond naturally as your character would in a group setting — you're speaking to everyone in the chat.\n"
                          << "- Keep your response brief (1-3 sentences). This is a fast-paced group chat, not a letter.\n"
                          << "- You can address other group members by name if relevant to the conversation.\n"
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

void GroupController::autoStep(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id) {
    auto g = GroupService::instance().getGroup(id);
    if (g.id.empty()) { auto hr = drogon::HttpResponse::newHttpJsonResponse(err("Group not found")); cors(hr); cb(hr); return; }

    auto memberIds = GroupService::instance().getMemberIds(id);
    if (memberIds.empty()) {
        auto r = ok("No members"); r["data"]["action"] = "pause";
        auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr); return;
    }

    // Build member name list
    auto& charSvc = CharacterService::instance();
    std::ostringstream memberList;
    for (size_t i = 0; i < memberIds.size(); ++i) {
        auto mc = charSvc.getCharacter(memberIds[i]);
        if (!mc.id.empty()) {
            if (i > 0) memberList << ", ";
            memberList << mc.name;
        }
    }

    // Get recent chat history and admin instructions
    std::string recentChat = GroupService::instance().getRecentChatLog(id, 30);
    std::string adminInstructions = GroupService::instance().getAdminInstructions(id);

    // Call AdminAgent to decide next speaker
    AdminAgent::instance().decideNextSpeaker(
        g.name, memberList.str(), recentChat, adminInstructions,
        [cb, id, memberIds](bool success, const std::string& action,
                            const std::string& character, const std::string& reason) {
            if (!success || action != "speak") {
                auto r = ok("Admin decision");
                r["data"]["action"] = action;
                r["data"]["reason"] = reason;
                auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
                return;
            }

            // Find the character by name
            auto& charSvc = CharacterService::instance();
            std::string targetId;
            for (const auto& cid : memberIds) {
                auto c = charSvc.getCharacter(cid);
                if (!c.id.empty() && c.name == character) {
                    targetId = cid;
                    break;
                }
            }

            if (targetId.empty()) {
                auto r = ok("Character not found");
                r["data"]["action"] = "wait";
                r["data"]["reason"] = "Character '" + character + "' not found in group";
                auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
                return;
            }

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
                auto r = ok("No API configured");
                r["data"]["action"] = "wait";
                auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
                return;
            }

            // Normalize URL
            while (!apiUrl.empty() && apiUrl.back() == '/') apiUrl.pop_back();
            if (apiUrl.length() < 3 || apiUrl.substr(apiUrl.length() - 3) != "/v1")
                apiUrl += "/v1";

            // Build system prompt for group chat context
            std::string identity = charSvc.readCharacterFile(targetId, "IDENTITY.md");
            std::string soul = charSvc.readCharacterFile(targetId, "SOUL.md");
            std::string memory = charSvc.readCharacterFile(targetId, "MEMORY.md");

            // Get member names
            std::ostringstream mList;
            for (size_t i = 0; i < memberIds.size(); ++i) {
                auto mc = charSvc.getCharacter(memberIds[i]);
                if (!mc.id.empty()) {
                    if (i > 0) mList << ", ";
                    mList << mc.name;
                }
            }

            // Get recent chat for character context
            std::string groupChat = GroupService::instance().getRecentChatLog(id, 30);
            // Replace own name so AI won't repeat past messages
            groupChat = replaceOwnName(groupChat, character);
            // Replace "Me" with configured user name
            std::string userName = ConfigManager::instance().getUserName();
            {
                std::string meMarker = "[Me]: ";
                std::string userMarker = "[" + userName + "]: ";
                size_t pos = 0;
                while ((pos = groupChat.find(meMarker, pos)) != std::string::npos) {
                    groupChat.replace(pos, meMarker.length(), userMarker);
                    pos += userMarker.length();
                }
            }

            std::ostringstream sysPrompt;
            sysPrompt << "=== Group Chat Context ===\n"
                      << "You are " << character << ", and you are currently in a group chat.\n"
                      << "Group members: " << mList.str() << "\n"
                      << "The user '" << userName << "' is also in this group.\n"
                      << "You have been chosen to speak next by the conversation moderator.\n\n";

            if (!identity.empty())
                sysPrompt << "## Your Identity\n" << identity << "\n\n";
            if (!soul.empty())
                sysPrompt << "## Your Personality & Soul\n" << soul << "\n\n";
            if (!memory.empty())
                sysPrompt << "## Your Core Memories\n" << memory << "\n\n";

            sysPrompt << "## Recent Group Chat History\n"
                      << (groupChat.empty() ? "(No history yet)" : groupChat)
                      << "\n\n";

            sysPrompt << "Instructions:\n"
                      << "- You are a member of this group chat, alongside: " << mList.str() << ".\n"
                      << "- The human user is named '" << userName << "'. Address them as " << userName << ".\n"
                      << "- You've been selected to speak. Respond naturally in character.\n"
                      << "- Keep your response brief (1-3 sentences). This is a fast-paced group chat.\n"
                      << "- You can address other members by name if relevant.\n"
                      << "- Reference past events from memories and chat history when relevant.\n"
                      << "- Stay in character at all times.\n"
                      << "- Do NOT include roleplay markers like *action* or [narrative].";

            // Build a user message that explains the context
            std::string contextMsg = "The group has been chatting. Now it's your turn to speak. "
                                     "Respond naturally as " + character + " in the group chat.";

            std::string capturedName = character;
            std::string capturedId = targetId;
            std::string capturedReason = reason;

            AIService::instance().chat(apiUrl, apiKey, apiModel,
                sysPrompt.str(), contextMsg,
                [cb, id, capturedName, capturedId, capturedReason](bool aiOk, const std::string& aiResponse) {
                    if (!aiOk) {
                        auto r = ok("AI failed");
                        r["data"]["action"] = "wait";
                        auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
                        return;
                    }

                    // Append AI response to chat log
                    std::string logEntry = "[" + capturedName + "]: " + aiResponse;
                    GroupService::instance().appendChatLog(id, logEntry);

                    auto r = ok("Auto step");
                    r["data"]["action"] = "speak";
                    r["data"]["character"] = capturedName;
                    r["data"]["character_id"] = capturedId;
                    r["data"]["reply"] = aiResponse;
                    r["data"]["reason"] = capturedReason;
                    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
                });
        });
}

void GroupController::setMode(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id) {
    auto json = req->getJsonObject();
    if (!json) { auto hr = drogon::HttpResponse::newHttpJsonResponse(err("Invalid JSON")); cors(hr); cb(hr); return; }

    std::string mode = (*json).get("mode", "").asString();
    if (mode != "mention" && mode != "auto") {
        auto hr = drogon::HttpResponse::newHttpJsonResponse(err("Invalid mode. Use 'mention' or 'auto'")); cors(hr); cb(hr);
        return;
    }

    bool okb = GroupService::instance().updateMode(id, mode);
    auto r = okb ? ok("Mode updated") : err("Update failed");
    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
}

} // namespace chronochat
