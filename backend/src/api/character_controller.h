#pragma once

#include <drogon/HttpController.h>

namespace chronochat {

class CharacterController : public drogon::HttpController<CharacterController> {
public:
    METHOD_LIST_BEGIN
    // CRUD
    ADD_METHOD_TO(CharacterController::list, "/api/characters", drogon::Get);
    ADD_METHOD_TO(CharacterController::create, "/api/characters", drogon::Post);
    ADD_METHOD_TO(CharacterController::getOne, "/api/characters/{1}", drogon::Get);
    ADD_METHOD_TO(CharacterController::update, "/api/characters/{1}", drogon::Put);
    ADD_METHOD_TO(CharacterController::remove, "/api/characters/{1}", drogon::Delete);
    ADD_METHOD_TO(CharacterController::serveAvatar, "/api/characters/{1}/avatar", drogon::Get);
    // Per-character API test
    ADD_METHOD_TO(CharacterController::testCharTextApi, "/api/characters/{1}/test-text", drogon::Post);
    ADD_METHOD_TO(CharacterController::testCharMultimodalApi, "/api/characters/{1}/test-multimodal", drogon::Post);
    // AI generation
    ADD_METHOD_TO(CharacterController::generateFiles, "/api/characters/{1}/generate", drogon::Post);
    // Chat
    ADD_METHOD_TO(CharacterController::chat, "/api/characters/{1}/chat", drogon::Post);
    ADD_METHOD_TO(CharacterController::chatHistory, "/api/characters/{1}/history", drogon::Get);
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void create(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getOne(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const std::string& id);
    void update(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const std::string& id);
    void remove(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const std::string& id);
    void serveAvatar(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const std::string& id);
    void testCharTextApi(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                         const std::string& id);
    void testCharMultimodalApi(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const std::string& id);
    void generateFiles(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const std::string& id);
    void chat(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& id);
    void chatHistory(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const std::string& id);

private:
    static Json::Value characterToJson(const struct CharacterInfo& c, bool includeApiKey = false);
    static std::string decodeBase64(const std::string& encoded);
    static std::string saveBase64Image(const std::string& base64Data,
                                       const std::string& dir,
                                       const std::string& prefix);
    static void doApiTest(const std::string& baseUrl, const std::string& apiKey,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

// Utility functions (also used by free functions in the .cpp)
std::string maskKey(const std::string& key);
std::string normalizeBaseUrl(const std::string& url);

} // namespace chronochat
