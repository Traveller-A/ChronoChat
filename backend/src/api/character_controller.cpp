#include "character_controller.h"
#include "../services/character_service.h"
#include "../config/config_manager.h"
#include "../ai/ai_service.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpClient.h>
#include <json/json.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace chronochat {

// ---- Base64 helpers ----
static const std::string base64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string CharacterController::decodeBase64(const std::string& encoded) {
    std::string data = encoded;
    auto commaPos = data.find(',');
    if (commaPos != std::string::npos) data = data.substr(commaPos + 1);
    std::string out;
    int i = 0;
    unsigned char charArr4[4], charArr3[3];
    for (char c : data) {
        if (c == '=' || base64Chars.find(c) == std::string::npos) break;
        charArr4[i++] = static_cast<unsigned char>(base64Chars.find(c));
        if (i == 4) {
            charArr3[0] = (charArr4[0] << 2) + ((charArr4[1] & 0x30) >> 4);
            charArr3[1] = ((charArr4[1] & 0x0F) << 4) + ((charArr4[2] & 0x3C) >> 2);
            charArr3[2] = ((charArr4[2] & 0x03) << 6) + charArr4[3];
            for (i = 0; i < 3; i++) out += charArr3[i];
            i = 0;
        }
    }
    return out;
}

std::string CharacterController::saveBase64Image(
    const std::string& base64Data, const std::string& dir, const std::string& prefix)
{
    if (base64Data.empty()) return "";
    std::string ext = "png";
    if (base64Data.find("image/jpeg") != std::string::npos ||
        base64Data.find("image/jpg") != std::string::npos) ext = "jpg";
    else if (base64Data.find("image/gif") != std::string::npos) ext = "gif";
    else if (base64Data.find("image/webp") != std::string::npos) ext = "webp";

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    std::string filename = prefix + "." + ext;
    std::string path = dir + "/" + filename;
    std::string decoded = decodeBase64(base64Data);
    std::ofstream out(path, std::ios::binary);
    if (!out) return "";
    out.write(decoded.data(), decoded.size());
    return filename;
}

// ---- Key/URL utils (free functions) ----
std::string maskKey(const std::string& key) {
    if (key.empty()) return "";
    if (key.length() <= 8) return std::string(key.length(), '*');
    return key.substr(0, 4) + std::string(key.length() - 8, '*') + key.substr(key.length() - 4);
}

std::string normalizeBaseUrl(const std::string& url) {
    if (url.empty()) return url;
    std::string result = url;
    while (!result.empty() && result.back() == '/') result.pop_back();
    if (result.length() < 3 || result.substr(result.length() - 3) != "/v1")
        result += "/v1";
    return result;
}

// ---- JSON conversion ----
Json::Value CharacterController::characterToJson(const CharacterInfo& c, bool includeApiKey) {
    Json::Value j;
    j["id"] = c.id;
    j["name"] = c.name;
    j["gender"] = c.gender;
    j["age"] = c.age;
    j["birthday"] = c.birthday;
    j["mbti"] = c.mbti;
    j["personality_signature"] = c.personalitySignature;
    j["avatar_path"] = c.avatarPath;
    j["story_text"] = c.storyText;
    j["story_images"] = c.storyImages;
    j["user_description"] = c.userDescription;

    // Per-character API (key masked unless explicitly requested)
    j["text_api_base_url"] = c.textApiBaseUrl;
    j["text_api_key"] = includeApiKey ? c.textApiKey : maskKey(c.textApiKey);
    j["text_api_has_key"] = !c.textApiKey.empty();
    j["text_model"] = c.textModel;
    j["multimodal_api_base_url"] = c.multimodalApiBaseUrl;
    j["multimodal_api_key"] = includeApiKey ? c.multimodalApiKey : maskKey(c.multimodalApiKey);
    j["multimodal_api_has_key"] = !c.multimodalApiKey.empty();
    j["multimodal_model"] = c.multimodalModel;

    j["created_at"] = static_cast<Json::Int64>(c.createdAt);
    j["updated_at"] = static_cast<Json::Int64>(c.updatedAt);
    return j;
}

// ---- Populate CharacterInfo from JSON ----
static void populateFromJson(CharacterInfo& info, const Json::Value& json) {
    if (json.isMember("name")) info.name = json["name"].asString();
    if (json.isMember("gender")) info.gender = json["gender"].asString();
    if (json.isMember("age")) info.age = json["age"].asInt();
    if (json.isMember("birthday")) info.birthday = json["birthday"].asString();
    if (json.isMember("mbti")) info.mbti = json["mbti"].asString();
    if (json.isMember("personality_signature"))
        info.personalitySignature = json["personality_signature"].asString();
    if (json.isMember("avatar_path")) info.avatarPath = json["avatar_path"].asString();
    if (json.isMember("story_text")) info.storyText = json["story_text"].asString();
    if (json.isMember("story_images")) info.storyImages = json["story_images"].asString();
    if (json.isMember("user_description")) info.userDescription = json["user_description"].asString();

    // Per-character API — normalize URLs on save
    if (json.isMember("text_api_base_url"))
        info.textApiBaseUrl = normalizeBaseUrl(json["text_api_base_url"].asString());
    if (json.isMember("text_api_key")) {
        std::string k = json["text_api_key"].asString();
        if (!k.empty() && k.find("****") == std::string::npos) info.textApiKey = k;
    }
    if (json.isMember("text_model")) info.textModel = json["text_model"].asString();
    if (json.isMember("multimodal_api_base_url"))
        info.multimodalApiBaseUrl = normalizeBaseUrl(json["multimodal_api_base_url"].asString());
    if (json.isMember("multimodal_api_key")) {
        std::string k = json["multimodal_api_key"].asString();
        if (!k.empty() && k.find("****") == std::string::npos) info.multimodalApiKey = k;
    }
    if (json.isMember("multimodal_model")) info.multimodalModel = json["multimodal_model"].asString();
}

// ---- Endpoints ----

void CharacterController::list(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto chars = CharacterService::instance().listCharacters();
    Json::Value arr(Json::arrayValue);
    for (const auto& c : chars) arr.append(characterToJson(c));

    Json::Value resp;
    resp["code"] = 0; resp["message"] = "ok"; resp["data"] = arr;
    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

void CharacterController::create(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Invalid JSON";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    CharacterInfo info;
    populateFromJson(info, *json);

    if (info.name.empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Character name is required";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    auto& svc = CharacterService::instance();
    std::string id = svc.createCharacter(info);
    if (id.empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Failed to create character";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    // Handle avatar
    if (json->isMember("avatar_base64") && !(*json)["avatar_base64"].asString().empty()) {
        std::string avatarFile = saveBase64Image(
            (*json)["avatar_base64"].asString(), svc.getCharacterDir(id), "avatar");
        if (!avatarFile.empty()) {
            info.avatarPath = "characters/" + id + "/" + avatarFile;
            svc.updateCharacter(id, info);
        }
    }

    auto created = svc.getCharacter(id);
    Json::Value resp; resp["code"] = 0; resp["message"] = "Character created";
    resp["data"] = characterToJson(created);
    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

void CharacterController::getOne(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id)
{
    auto c = CharacterService::instance().getCharacter(id);
    if (c.id.empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Character not found";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    Json::Value data = characterToJson(c, true); // include full key for edit
    auto& svc = CharacterService::instance();
    data["identity_md"] = svc.readCharacterFile(id, "IDENTITY.md");
    data["soul_md"] = svc.readCharacterFile(id, "SOUL.md");
    data["memory_md"] = svc.readCharacterFile(id, "MEMORY.md");
    data["user_md"] = svc.readCharacterFile(id, "USER.md");

    Json::Value resp; resp["code"] = 0; resp["message"] = "ok"; resp["data"] = data;
    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

void CharacterController::update(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id)
{
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Invalid JSON";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    auto& svc = CharacterService::instance();
    auto existing = svc.getCharacter(id);
    if (existing.id.empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Character not found";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    // Merge only mutable fields
    if (json->isMember("age")) existing.age = (*json)["age"].asInt();
    if (json->isMember("birthday")) existing.birthday = (*json)["birthday"].asString();
    if (json->isMember("mbti")) existing.mbti = (*json)["mbti"].asString();
    if (json->isMember("personality_signature"))
        existing.personalitySignature = (*json)["personality_signature"].asString();
    if (json->isMember("user_description"))
        existing.userDescription = (*json)["user_description"].asString();

    // Per-char API
    if (json->isMember("text_api_base_url"))
        existing.textApiBaseUrl = normalizeBaseUrl((*json)["text_api_base_url"].asString());
    if (json->isMember("text_api_key")) {
        std::string k = (*json)["text_api_key"].asString();
        if (!k.empty() && k.find("****") == std::string::npos) existing.textApiKey = k;
    }
    if (json->isMember("text_model")) existing.textModel = (*json)["text_model"].asString();
    if (json->isMember("multimodal_api_base_url"))
        existing.multimodalApiBaseUrl = normalizeBaseUrl((*json)["multimodal_api_base_url"].asString());
    if (json->isMember("multimodal_api_key")) {
        std::string k = (*json)["multimodal_api_key"].asString();
        if (!k.empty() && k.find("****") == std::string::npos) existing.multimodalApiKey = k;
    }
    if (json->isMember("multimodal_model")) existing.multimodalModel = (*json)["multimodal_model"].asString();

    // Avatar
    if (json->isMember("avatar_base64") && !(*json)["avatar_base64"].asString().empty()) {
        std::string avatarFile = saveBase64Image(
            (*json)["avatar_base64"].asString(), svc.getCharacterDir(id), "avatar");
        if (!avatarFile.empty())
            existing.avatarPath = "characters/" + id + "/" + avatarFile;
    }

    // File updates
    if (json->isMember("identity_md"))
        svc.writeCharacterFile(id, "IDENTITY.md", (*json)["identity_md"].asString());
    if (json->isMember("soul_md"))
        svc.writeCharacterFile(id, "SOUL.md", (*json)["soul_md"].asString());
    if (json->isMember("memory_md"))
        svc.writeCharacterFile(id, "MEMORY.md", (*json)["memory_md"].asString());
    if (json->isMember("user_md"))
        svc.writeCharacterFile(id, "USER.md", (*json)["user_md"].asString());

    bool ok = svc.updateCharacter(id, existing);
    Json::Value resp; resp["code"] = ok ? 0 : 1;
    resp["message"] = ok ? "Character updated" : "Failed to update";
    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

void CharacterController::remove(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id)
{
    bool ok = CharacterService::instance().deleteCharacter(id);
    Json::Value resp; resp["code"] = ok ? 0 : 1;
    resp["message"] = ok ? "Character deleted" : "Failed to delete";
    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

void CharacterController::serveAvatar(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id)
{
    auto& svc = CharacterService::instance();
    auto c = svc.getCharacter(id);
    if (c.id.empty() || c.avatarPath.empty()) {
        auto httpResp = drogon::HttpResponse::newHttpResponse();
        httpResp->setStatusCode(drogon::k404NotFound);
        callback(httpResp); return;
    }
    std::string fullPath = svc.dataDir() + "/" + c.avatarPath;
    auto httpResp = drogon::HttpResponse::newFileResponse(fullPath);
    callback(httpResp);
}

// ---- Per-Character API test (shared logic) ----
void CharacterController::doApiTest(
    const std::string& baseUrl, const std::string& apiKey,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    if (baseUrl.empty() || apiKey.empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "API base URL or key is not configured";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*");
        callback(httpResp); return;
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
                int sc = response->getStatusCode();
                resp["code"] = (sc == 200 || sc == 401) ? 0 : 1;
                resp["message"] = sc == 200 ? "API connection successful"
                    : sc == 401 ? "API endpoint reachable, but API key may be invalid"
                    : "API returned status: " + std::to_string(sc);
            } else {
                resp["code"] = 1; resp["message"] = "Cannot connect to API endpoint";
            }
            auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
            httpResp->addHeader("Access-Control-Allow-Origin", "*");
            callback(httpResp);
        });
}

void CharacterController::testCharTextApi(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id)
{
    auto c = CharacterService::instance().getCharacter(id);
    if (c.id.empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Character not found";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    // Use character's own API if configured, otherwise global
    std::string url = c.textApiBaseUrl;
    std::string key = c.textApiKey;
    if (url.empty()) url = ConfigManager::instance().getTextApiBaseUrl();
    if (key.empty()) key = ConfigManager::instance().getTextApiKey();

    doApiTest(url, key, std::move(callback));
}

void CharacterController::testCharMultimodalApi(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id)
{
    auto c = CharacterService::instance().getCharacter(id);
    if (c.id.empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Character not found";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    std::string url = c.multimodalApiBaseUrl;
    std::string key = c.multimodalApiKey;
    if (url.empty()) url = ConfigManager::instance().getMultimodalApiBaseUrl();
    if (key.empty()) key = ConfigManager::instance().getMultimodalApiKey();

    doApiTest(url, key, std::move(callback));
}

void CharacterController::generateFiles(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id)
{
    auto& svc = CharacterService::instance();
    auto c = svc.getCharacter(id);
    if (c.id.empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Character not found";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    // Determine API config: character-specific first, then global
    std::string apiUrl = c.textApiBaseUrl;
    std::string apiKey = c.textApiKey;
    std::string apiModel = c.textModel;
    if (apiUrl.empty()) apiUrl = ConfigManager::instance().getTextApiBaseUrl();
    if (apiKey.empty()) apiKey = ConfigManager::instance().getTextApiKey();
    if (apiModel.empty()) apiModel = ConfigManager::instance().getTextModel();

    if (apiUrl.empty() || apiKey.empty()) {
        Json::Value resp; resp["code"] = 1;
        resp["message"] = "API not configured. Please set up API in Settings first.";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    // Build system prompt
    std::string systemPrompt = R"(You are a creative character designer. Based on the information provided, you must generate FOUR markdown files for a virtual character. Output them in the following format EXACTLY:

===IDENTITY===
[Basic identity: name, age, gender, appearance, background summary]
===SOUL===
[Personality traits, speaking style, values, emotional patterns, MBTI analysis, behavioral quirks]
===MEMORY===
[Key life events based on the background story, ordered chronologically as bullet points]
===USER===
[The character's relationship with and view of the user, based on the provided user description]

Rules:
1. Write in Chinese if the input is Chinese, otherwise in English.
2. Each section should be substantial (200-500 words).
3. Be creative and vivid — make the character feel alive.
4. Do NOT include the section markers (===XXX===) in the section content itself.
5. Keep strictly to the format: ===SECTION_NAME=== followed by the content, then the next section.)";

    // Build user prompt with character info
    std::ostringstream userPrompt;
    userPrompt << "Create a character with the following information:\n\n";
    userPrompt << "Name: " << c.name << "\n";
    if (!c.gender.empty()) userPrompt << "Gender: " << c.gender << "\n";
    if (c.age > 0) userPrompt << "Age: " << c.age << "\n";
    if (!c.mbti.empty()) userPrompt << "MBTI: " << c.mbti << "\n";
    if (!c.personalitySignature.empty())
        userPrompt << "Personality Signature: " << c.personalitySignature << "\n";
    if (!c.birthday.empty())
        userPrompt << "Birthday: " << c.birthday << "\n";

    if (!c.storyText.empty()) {
        userPrompt << "\n--- Background Story ---\n" << c.storyText << "\n";
    }
    if (!c.userDescription.empty()) {
        userPrompt << "\n--- How the character views the user ---\n" << c.userDescription << "\n";
    }

    userPrompt << "\nPlease generate the four markdown files now.";

    // Call AI
    AIService::instance().chat(apiUrl, apiKey, apiModel, systemPrompt, userPrompt.str(),
        [callback, id](bool success, const std::string& response) {
            if (!success) {
                Json::Value resp; resp["code"] = 1; resp["message"] = "AI generation failed: " + response;
                auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
                httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
            }

            // Parse sections
            auto& svc = CharacterService::instance();
            auto extractSection = [&](const std::string& marker) -> std::string {
                auto start = response.find("===" + marker + "===");
                if (start == std::string::npos) return "";
                start += marker.length() + 6; // "===MARKER===" length
                // Skip newline
                if (start < response.length() && response[start] == '\n') start++;
                if (start < response.length() && response[start] == '\r') start++;
                // Find next section or end
                auto end = response.find("===", start);
                std::string content = (end == std::string::npos)
                    ? response.substr(start) : response.substr(start, end - start);
                // Trim trailing whitespace
                while (!content.empty() && (content.back() == '\n' || content.back() == '\r'))
                    content.pop_back();
                return content;
            };

            std::string identity = extractSection("IDENTITY");
            std::string soul = extractSection("SOUL");
            std::string memory = extractSection("MEMORY");
            std::string user = extractSection("USER");

            // Write to files
            if (!identity.empty()) svc.writeCharacterFile(id, "IDENTITY.md", identity);
            if (!soul.empty()) svc.writeCharacterFile(id, "SOUL.md", soul);
            if (!memory.empty()) svc.writeCharacterFile(id, "MEMORY.md", memory);
            if (!user.empty()) svc.writeCharacterFile(id, "USER.md", user);

            Json::Value data;
            data["identity_generated"] = !identity.empty();
            data["soul_generated"] = !soul.empty();
            data["memory_generated"] = !memory.empty();
            data["user_generated"] = !user.empty();

            Json::Value resp; resp["code"] = 0; resp["message"] = "Character files generated";
            resp["data"] = data;
            auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
            httpResp->addHeader("Access-Control-Allow-Origin", "*");
            callback(httpResp);
        });
}

void CharacterController::chat(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("message")) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Message is required";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }
    std::string userMessage = (*json)["message"].asString();
    if (userMessage.empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Message is empty";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    auto& svc = CharacterService::instance();
    auto c = svc.getCharacter(id);
    if (c.id.empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Character not found";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    // API config: character-specific > global
    std::string apiUrl = c.textApiBaseUrl;
    std::string apiKey = c.textApiKey;
    std::string apiModel = c.textModel;
    std::string apiSource = "global";
    if (!apiUrl.empty() && !apiKey.empty()) {
        apiSource = "character";
    } else {
        apiUrl = ConfigManager::instance().getTextApiBaseUrl();
        apiKey = ConfigManager::instance().getTextApiKey();
    }
    if (apiModel.empty()) apiModel = ConfigManager::instance().getTextModel();

    if (apiUrl.empty() || apiKey.empty()) {
        Json::Value resp; resp["code"] = 1;
        resp["message"] = "API not configured. Please configure API in Settings.";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    // Load character files for context
    std::string identity = svc.readCharacterFile(id, "IDENTITY.md");
    std::string soul = svc.readCharacterFile(id, "SOUL.md");
    std::string memory = svc.readCharacterFile(id, "MEMORY.md");
    std::string userRel = svc.readCharacterFile(id, "USER.md");

    // Build system prompt from character identity + soul
    std::string systemPrompt = "You are roleplaying as the following character. "
        "Respond naturally in character. Stay true to your personality and memories.\n\n";
    if (!identity.empty()) systemPrompt += "## Identity\n" + identity + "\n\n";
    if (!soul.empty()) systemPrompt += "## Personality & Soul\n" + soul + "\n\n";
    if (!userRel.empty()) systemPrompt += "## Your Relationship with the User\n" + userRel + "\n\n";
    systemPrompt += "## Recent Memories\n" + memory + "\n\n";
    systemPrompt += "Instructions:\n"
        "- Respond as the character would, using their speaking style.\n"
        "- Reference past events from memories when relevant.\n"
        "- Keep responses concise (1-3 paragraphs) unless the user asks for more.\n"
        "- If you don't know something, make reasonable inferences based on your personality.\n"
        "- Stay in character at all times.";

    // Call AI
    AIService::instance().chat(apiUrl, apiKey, apiModel, systemPrompt, userMessage,
        [callback, id, userMessage, apiSource](bool success, const std::string& aiResponse) {
            if (!success) {
                Json::Value resp; resp["code"] = 1;
                resp["message"] = "AI response failed: " + aiResponse;
                auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
                httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
            }

            // Append to MEMORY.md
            auto& svc = CharacterService::instance();
            std::string memory = svc.readCharacterFile(id, "MEMORY.md");
            memory += "\n\n---\n**User**: " + userMessage + "\n**Character**: " + aiResponse + "\n";
            // Keep memory from growing too large (keep last ~20 exchanges)
            auto lines = std::count(memory.begin(), memory.end(), '\n');
            if (lines > 500) {
                // Trim oldest entries
                auto pos = memory.find("---\n", memory.find("---\n") + 10);
                if (pos != std::string::npos && pos < memory.length() / 3)
                    memory = memory.substr(pos);
            }
            svc.writeCharacterFile(id, "MEMORY.md", memory);

            Json::Value data;
            data["message"] = aiResponse;
            data["api_source"] = apiSource;

            Json::Value resp; resp["code"] = 0; resp["message"] = "ok"; resp["data"] = data;
            auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
            httpResp->addHeader("Access-Control-Allow-Origin", "*");
            callback(httpResp);
        });
}

void CharacterController::chatHistory(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id)
{
    auto& svc = CharacterService::instance();
    auto c = svc.getCharacter(id);
    if (c.id.empty()) {
        Json::Value resp; resp["code"] = 1; resp["message"] = "Character not found";
        auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
        httpResp->addHeader("Access-Control-Allow-Origin", "*"); callback(httpResp); return;
    }

    std::string memory = svc.readCharacterFile(id, "MEMORY.md");

    Json::Value data;
    data["memory"] = memory;

    // Check API source
    bool hasCharApi = !c.textApiBaseUrl.empty() && !c.textApiKey.empty();
    data["api_source"] = hasCharApi ? "character" : "global";

    Json::Value resp; resp["code"] = 0; resp["message"] = "ok"; resp["data"] = data;
    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

} // namespace chronochat
