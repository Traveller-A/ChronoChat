#include "config_controller.h"
#include "../config/config_manager.h"
#include "../services/character_service.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpClient.h>
#include <json/json.h>
#include <filesystem>
#include <fstream>

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

    std::string userAvatarPath = cfg.getUserAvatarPath();
    data["user_avatar_set"] = !userAvatarPath.empty();
    data["user_avatar_url"] = "/api/config/avatar";

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

// ---- Clear all API configurations (global + per-character) ----

void ConfigController::clearApi(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto& cfg = ConfigManager::instance();
    // Wipe global text + multimodal API config (URL / key / model)
    cfg.setTextApiBaseUrl("");
    cfg.setTextApiKey("");
    cfg.setTextModel("");
    cfg.setMultimodalApiBaseUrl("");
    cfg.setMultimodalApiKey("");
    cfg.setMultimodalModel("");
    bool globalOk = cfg.saveToDb();

    // Wipe every character's per-character API config
    bool charOk = CharacterService::instance().clearAllCharacterApi();

    Json::Value data;
    data["global_cleared"] = globalOk;
    data["characters_cleared"] = charOk;

    Json::Value resp;
    resp["code"] = (globalOk && charOk) ? 0 : 1;
    resp["message"] = (globalOk && charOk)
        ? "All API configurations cleared"
        : "Partial failure while clearing API configurations";
    resp["data"] = data;

    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

// ---- User avatar ----

static const std::string b64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string ConfigController::decodeBase64(const std::string& encoded) {
    std::string data = encoded;
    auto commaPos = data.find(',');
    if (commaPos != std::string::npos) data = data.substr(commaPos + 1);
    std::string out;
    int i = 0;
    unsigned char arr4[4], arr3[3];
    for (char c : data) {
        if (c == '=' || b64Chars.find(c) == std::string::npos) break;
        arr4[i++] = static_cast<unsigned char>(b64Chars.find(c));
        if (i == 4) {
            arr3[0] = (arr4[0] << 2) + ((arr4[1] & 0x30) >> 4);
            arr3[1] = ((arr4[1] & 0x0F) << 4) + ((arr4[2] & 0x3C) >> 2);
            arr3[2] = ((arr4[2] & 0x03) << 6) + arr4[3];
            for (i = 0; i < 3; i++) out += arr3[i];
            i = 0;
        }
    }
    return out;
}

std::string ConfigController::saveBase64Avatar(
    const std::string& base64Data, const std::string& dir)
{
    if (base64Data.empty()) return "";
    std::string ext = "png";
    if (base64Data.find("image/jpeg") != std::string::npos ||
        base64Data.find("image/jpg") != std::string::npos) ext = "jpg";
    else if (base64Data.find("image/gif") != std::string::npos) ext = "gif";
    else if (base64Data.find("image/webp") != std::string::npos) ext = "webp";

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    std::string filename = "user_avatar." + ext;
    std::string path = dir + "/" + filename;
    std::string decoded = decodeBase64(base64Data);
    std::ofstream out(path, std::ios::binary);
    if (!out) return "";
    out.write(decoded.data(), decoded.size());
    return filename;
}

void ConfigController::getAvatar(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto& cfg = ConfigManager::instance();
    std::string relPath = cfg.getUserAvatarPath();
    if (relPath.empty()) {
        auto httpResp = drogon::HttpResponse::newHttpResponse();
        httpResp->setStatusCode(drogon::k404NotFound);
        callback(httpResp);
        return;
    }
    std::string fullPath = CharacterService::instance().dataDir() + "/" + relPath;
    auto httpResp = drogon::HttpResponse::newFileResponse(fullPath);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    httpResp->addHeader("Cache-Control", "no-cache, must-revalidate");
    callback(httpResp);
}

void ConfigController::uploadAvatar(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("avatar_base64") ||
        (*json)["avatar_base64"].asString().empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "avatar_base64 is required";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*");
        callback(httpResp);
        return;
    }

    std::string base64 = (*json)["avatar_base64"].asString();
    std::string dir = CharacterService::instance().dataDir() + "/uploads";
    std::string filename = saveBase64Avatar(base64, dir);
    if (filename.empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Failed to save avatar";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*");
        callback(httpResp);
        return;
    }

    // Remove the previous avatar file if its extension differs
    auto& cfg = ConfigManager::instance();
    std::string oldRel = cfg.getUserAvatarPath();
    if (!oldRel.empty() && oldRel != "uploads/" + filename) {
        std::error_code ec;
        std::filesystem::remove(CharacterService::instance().dataDir() + "/" + oldRel, ec);
    }

    cfg.setUserAvatarPath("uploads/" + filename);
    cfg.saveToDb();

    Json::Value data;
    data["user_avatar_set"] = true;
    data["user_avatar_url"] = "/api/config/avatar";
    Json::Value resp; resp["code"] = 0; resp["message"] = "Avatar updated"; resp["data"] = data;
    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

void ConfigController::deleteAvatar(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto& cfg = ConfigManager::instance();
    std::string relPath = cfg.getUserAvatarPath();
    if (!relPath.empty()) {
        std::error_code ec;
        std::filesystem::remove(CharacterService::instance().dataDir() + "/" + relPath, ec);
        cfg.setUserAvatarPath("");
        cfg.saveToDb();
    }

    Json::Value data;
    data["user_avatar_set"] = false;
    Json::Value resp; resp["code"] = 0; resp["message"] = "Avatar removed"; resp["data"] = data;
    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

} // namespace chronochat
