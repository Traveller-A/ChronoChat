#include "config_controller.h"
#include "../config/config_manager.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpClient.h>
#include <json/json.h>

namespace chronochat {

std::string ConfigController::maskKey(const std::string& key) {
    if (key.empty()) return "";
    if (key.length() <= 8) {
        return std::string(key.length(), '*');
    }
    return key.substr(0, 4) + std::string(key.length() - 8, '*') + key.substr(key.length() - 4);
}

std::string ConfigController::normalizeBaseUrl(const std::string& url) {
    if (url.empty()) return url;
    std::string result = url;
    // Trim trailing slash
    while (!result.empty() && result.back() == '/') {
        result.pop_back();
    }
    // Auto-append /v1 if not present (OpenAI-compatible API convention)
    if (result.length() < 3 || result.substr(result.length() - 3) != "/v1") {
        result += "/v1";
    }
    return result;
}

void ConfigController::getConfig(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto& cfg = ConfigManager::instance();

    std::string textKey = cfg.getTextApiKey();
    std::string multiKey = cfg.getMultimodalApiKey();

    Json::Value data;
    data["text_api_base_url"] = cfg.getTextApiBaseUrl();
    data["text_api_key"] = maskKey(textKey);
    data["text_api_has_key"] = !textKey.empty();
    data["text_model"] = cfg.getTextModel();

    data["multimodal_api_base_url"] = cfg.getMultimodalApiBaseUrl();
    data["multimodal_api_key"] = maskKey(multiKey);
    data["multimodal_api_has_key"] = !multiKey.empty();
    data["multimodal_model"] = cfg.getMultimodalModel();

    data["user_name"] = cfg.getUserName();

    Json::Value resp;
    resp["code"] = 0;
    resp["message"] = "ok";
    resp["data"] = data;

    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

void ConfigController::saveConfig(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value resp;
        resp["code"] = 1;
        resp["message"] = "Invalid JSON body";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*");
        callback(httpResp);
        return;
    }

    auto& cfg = ConfigManager::instance();

    // Text API Base URL — normalize on save
    if (json->isMember("text_api_base_url"))
        cfg.setTextApiBaseUrl(normalizeBaseUrl((*json)["text_api_base_url"].asString()));

    // Text API Key - only update if a new key is provided
    if (json->isMember("text_api_key")) {
        std::string newKey = (*json)["text_api_key"].asString();
        if (!newKey.empty() && newKey.find("****") == std::string::npos) {
            cfg.setTextApiKey(newKey);
        }
    }

    if (json->isMember("text_model"))
        cfg.setTextModel((*json)["text_model"].asString());

    // Multimodal API Base URL — normalize on save
    if (json->isMember("multimodal_api_base_url"))
        cfg.setMultimodalApiBaseUrl(normalizeBaseUrl((*json)["multimodal_api_base_url"].asString()));

    // Multimodal API Key
    if (json->isMember("multimodal_api_key")) {
        std::string newKey = (*json)["multimodal_api_key"].asString();
        if (!newKey.empty() && newKey.find("****") == std::string::npos) {
            cfg.setMultimodalApiKey(newKey);
        }
    }

    if (json->isMember("multimodal_model"))
        cfg.setMultimodalModel((*json)["multimodal_model"].asString());

    if (json->isMember("user_name"))
        cfg.setUserName((*json)["user_name"].asString());

    bool saved = cfg.saveToDb();

    Json::Value resp;
    resp["code"] = saved ? 0 : 1;
    resp["message"] = saved ? "Configuration saved" : "Failed to save configuration";

    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

void ConfigController::testTextApi(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto& cfg = ConfigManager::instance();
    std::string baseUrl = cfg.getTextApiBaseUrl();
    std::string apiKey = cfg.getTextApiKey();

    if (baseUrl.empty() || apiKey.empty()) {
        Json::Value resp;
        resp["code"] = 1;
        resp["message"] = "Text API base URL or key is not configured";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*");
        callback(httpResp);
        return;
    }

    auto client = drogon::HttpClient::newHttpClient(baseUrl);
    auto testReq = drogon::HttpRequest::newHttpRequest();
    testReq->setMethod(drogon::Get);
    testReq->setPath("/models");
    testReq->addHeader("Authorization", "Bearer " + apiKey);

    client->sendRequest(testReq,
        [callback](drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            Json::Value resp;
            if (result == drogon::ReqResult::Ok && response) {
                int statusCode = response->getStatusCode();
                resp["code"] = (statusCode == 200 || statusCode == 401) ? 0 : 1;
                resp["message"] = statusCode == 200
                    ? "Text API connection successful"
                    : statusCode == 401
                        ? "Text API endpoint reachable, but API key may be invalid"
                        : "Text API returned status: " + std::to_string(statusCode);
            } else {
                resp["code"] = 1;
                resp["message"] = "Cannot connect to text API endpoint";
            }
            auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
            httpResp->addHeader("Access-Control-Allow-Origin", "*");
            callback(httpResp);
        });
}

void ConfigController::testMultimodalApi(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto& cfg = ConfigManager::instance();
    std::string baseUrl = cfg.getMultimodalApiBaseUrl();
    std::string apiKey = cfg.getMultimodalApiKey();

    if (baseUrl.empty() || apiKey.empty()) {
        Json::Value resp;
        resp["code"] = 1;
        resp["message"] = "Multimodal API base URL or key is not configured";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*");
        callback(httpResp);
        return;
    }

    auto client = drogon::HttpClient::newHttpClient(baseUrl);
    auto testReq = drogon::HttpRequest::newHttpRequest();
    testReq->setMethod(drogon::Get);
    testReq->setPath("/models");
    testReq->addHeader("Authorization", "Bearer " + apiKey);

    client->sendRequest(testReq,
        [callback](drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            Json::Value resp;
            if (result == drogon::ReqResult::Ok && response) {
                int statusCode = response->getStatusCode();
                resp["code"] = (statusCode == 200 || statusCode == 401) ? 0 : 1;
                resp["message"] = statusCode == 200
                    ? "Multimodal API connection successful"
                    : statusCode == 401
                        ? "Multimodal API endpoint reachable, but API key may be invalid"
                        : "Multimodal API returned status: " + std::to_string(statusCode);
            } else {
                resp["code"] = 1;
                resp["message"] = "Cannot connect to multimodal API endpoint";
            }
            auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
            httpResp->addHeader("Access-Control-Allow-Origin", "*");
            callback(httpResp);
        });
}

} // namespace chronochat
