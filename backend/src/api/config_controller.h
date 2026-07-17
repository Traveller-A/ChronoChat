#pragma once

#include <drogon/HttpController.h>

namespace chronochat {

// ConfigController - API configuration management
class ConfigController : public drogon::HttpController<ConfigController> {
public:
    METHOD_LIST_BEGIN
    // GET  /api/config         -> get current config
    ADD_METHOD_TO(ConfigController::getConfig, "/api/config", drogon::Get);
    // POST /api/config         -> save config
    ADD_METHOD_TO(ConfigController::saveConfig, "/api/config", drogon::Post);
    // POST /api/config/test-text  -> test text API connection
    ADD_METHOD_TO(ConfigController::testTextApi, "/api/config/test-text", drogon::Post);
    // POST /api/config/test-multimodal -> test multimodal API connection
    ADD_METHOD_TO(ConfigController::testMultimodalApi, "/api/config/test-multimodal", drogon::Post);
    // POST /api/config/clear-api -> clear ALL API config (global + every character's)
    ADD_METHOD_TO(ConfigController::clearApi, "/api/config/clear-api", drogon::Post);
    // GET    /api/config/avatar -> serve the current user avatar image
    ADD_METHOD_TO(ConfigController::getAvatar, "/api/config/avatar", drogon::Get);
    // POST   /api/config/avatar -> upload (base64) a new user avatar
    ADD_METHOD_TO(ConfigController::uploadAvatar, "/api/config/avatar", drogon::Post);
    // DELETE /api/config/avatar -> remove the user avatar
    ADD_METHOD_TO(ConfigController::deleteAvatar, "/api/config/avatar", drogon::Delete);
    METHOD_LIST_END

    void getConfig(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void saveConfig(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void testTextApi(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void testMultimodalApi(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void clearApi(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getAvatar(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void uploadAvatar(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void deleteAvatar(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    // Mask API key: show first 4 and last 4 chars, middle replaced with ****
    static std::string maskKey(const std::string& key);
    // Normalize base URL: ensure it ends with /v1
    static std::string normalizeBaseUrl(const std::string& url);
    // Decode a (possibly data-URI prefixed) base64 string to raw bytes
    static std::string decodeBase64(const std::string& encoded);
    // Save a base64 (data-URI) image to <dir>/user_avatar.<ext>; returns the
    // filename (e.g. "user_avatar.png") or "" on failure.
    static std::string saveBase64Avatar(const std::string& base64Data,
                                        const std::string& dir);
};

} // namespace chronochat
