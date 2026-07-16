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
    METHOD_LIST_END

    void getConfig(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void saveConfig(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void testTextApi(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void testMultimodalApi(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    // Mask API key: show first 4 and last 4 chars, middle replaced with ****
    static std::string maskKey(const std::string& key);
    // Normalize base URL: ensure it ends with /v1
    static std::string normalizeBaseUrl(const std::string& url);
};

} // namespace chronochat
