#pragma once
#include <drogon/HttpController.h>

namespace chronochat {

class GroupController : public drogon::HttpController<GroupController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(GroupController::list, "/api/groups", drogon::Get);
    ADD_METHOD_TO(GroupController::create, "/api/groups", drogon::Post);
    ADD_METHOD_TO(GroupController::getOne, "/api/groups/{1}", drogon::Get);
    ADD_METHOD_TO(GroupController::update, "/api/groups/{1}", drogon::Put);
    ADD_METHOD_TO(GroupController::remove, "/api/groups/{1}", drogon::Delete);
    ADD_METHOD_TO(GroupController::addMember, "/api/groups/{1}/members", drogon::Post);
    ADD_METHOD_TO(GroupController::removeMember, "/api/groups/{1}/members/{2}", drogon::Delete);
    ADD_METHOD_TO(GroupController::chat, "/api/groups/{1}/chat", drogon::Post);
    ADD_METHOD_TO(GroupController::history, "/api/groups/{1}/history", drogon::Get);
    ADD_METHOD_TO(GroupController::autoStep, "/api/groups/{1}/auto-step", drogon::Post);
    ADD_METHOD_TO(GroupController::setMode, "/api/groups/{1}/mode", drogon::Post);
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void create(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void getOne(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id);
    void update(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id);
    void remove(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id);
    void addMember(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id);
    void removeMember(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& gid, const std::string& cid);
    void chat(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id);
    void history(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id);
    void autoStep(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id);
    void setMode(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::string& id);
};

} // namespace chronochat
