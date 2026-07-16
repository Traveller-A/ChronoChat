#include "group_controller.h"
#include "../services/group_service.h"
#include "../services/character_service.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>

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

    // Determine sender display name
    std::string senderName = "Me";
    if (!senderId.empty()) {
        auto c = CharacterService::instance().getCharacter(senderId);
        if (!c.id.empty()) {
            senderName = c.name;
        } else {
            senderName = "Unknown";
        }
    }

    // Append to chat log
    std::string logEntry = "[" + senderName + "]: " + message;
    GroupService::instance().appendChatLog(id, logEntry);

    // Phase 1: just log the message, no AI reply yet
    auto r = ok("Message sent");
    r["data"]["sender_name"] = senderName;
    r["data"]["reply"] = "";  // no AI reply in Phase 1
    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
}

void GroupController::history(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id) {
    std::string log = GroupService::instance().getChatLog(id);
    auto r = ok();
    r["data"]["history"] = log;
    auto hr = drogon::HttpResponse::newHttpJsonResponse(r); cors(hr); cb(hr);
}

} // namespace chronochat
