#include "ai_service.h"
#include <drogon/HttpClient.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <sstream>
#include <iostream>

namespace chronochat {

AIService& AIService::instance() {
    static AIService svc;
    return svc;
}

// ---- Shared HTTP send + response parse ----
void AIService::sendChatRequest(
    const std::string& baseUrl,
    const std::string& apiKey,
    const std::string& bodyStr,
    AICallback callback)
{
    auto httpReq = drogon::HttpRequest::newHttpRequest();
    httpReq->setMethod(drogon::Post);
    httpReq->setPath("/chat/completions");
    httpReq->setContentTypeString("application/json");
    httpReq->addHeader("Authorization", "Bearer " + apiKey);
    httpReq->setBody(bodyStr);

    auto client = drogon::HttpClient::newHttpClient(baseUrl);
    client->sendRequest(httpReq,
        [callback](drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            if (result != drogon::ReqResult::Ok || !response) {
                std::cerr << "[AIService] Request failed: "
                          << static_cast<int>(result) << std::endl;
                callback(false, "Network error");
                return;
            }

            int sc = response->getStatusCode();
            if (sc != 200) {
                std::cerr << "[AIService] API returned status " << sc
                          << ": " << response->getBody() << std::endl;
                callback(false, "API returned status " + std::to_string(sc));
                return;
            }

            // Parse response
            Json::CharReaderBuilder reader;
            std::string errors;
            Json::Value respJson;
            std::istringstream bodyStream(std::string(response->getBody()));
            if (!Json::parseFromStream(reader, bodyStream, &respJson, &errors)) {
                callback(false, "Failed to parse API response");
                return;
            }

            // Extract content from choices[0].message.content
            try {
                const Json::Value& contentVal = respJson["choices"][0]["message"]["content"];
                // Some vision models return content as an array of parts; join the text parts.
                if (contentVal.isString()) {
                    callback(true, contentVal.asString());
                } else if (contentVal.isArray()) {
                    std::string joined;
                    for (const auto& part : contentVal) {
                        if (part.isObject() && part.isMember("text"))
                            joined += part["text"].asString();
                        else if (part.isString())
                            joined += part.asString();
                    }
                    callback(true, joined);
                } else {
                    callback(false, "Unexpected API response structure");
                }
            } catch (...) {
                callback(false, "Unexpected API response structure");
            }
        });
}

void AIService::chat(
    const std::string& baseUrl,
    const std::string& apiKey,
    const std::string& model,
    const std::string& systemPrompt,
    const std::string& userPrompt,
    AICallback callback)
{
    // Build messages array
    Json::Value messages(Json::arrayValue);

    Json::Value sysMsg;
    sysMsg["role"] = "system";
    sysMsg["content"] = systemPrompt;
    messages.append(sysMsg);

    Json::Value userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userPrompt;
    messages.append(userMsg);

    // Build request body
    Json::Value body;
    body["model"] = model;
    body["messages"] = messages;
    body["temperature"] = 0.8;
    body["max_tokens"] = 4096;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string bodyStr = Json::writeString(writer, body);

    sendChatRequest(baseUrl, apiKey, bodyStr, std::move(callback));
}

void AIService::chatVision(
    const std::string& baseUrl,
    const std::string& apiKey,
    const std::string& model,
    const std::string& systemPrompt,
    const std::string& userPrompt,
    const std::vector<std::string>& imageDataUris,
    AICallback callback)
{
    // Build messages array
    Json::Value messages(Json::arrayValue);

    Json::Value sysMsg;
    sysMsg["role"] = "system";
    sysMsg["content"] = systemPrompt;
    messages.append(sysMsg);

    // User content is an array of text + image_url parts (OpenAI vision format)
    Json::Value userContent(Json::arrayValue);

    Json::Value textPart;
    textPart["type"] = "text";
    textPart["text"] = userPrompt;
    userContent.append(textPart);

    for (const auto& uri : imageDataUris) {
        if (uri.empty()) continue;
        Json::Value imgPart;
        imgPart["type"] = "image_url";
        Json::Value imgUrl;
        imgUrl["url"] = uri;
        imgPart["image_url"] = imgUrl;
        userContent.append(imgPart);
    }

    Json::Value userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userContent;
    messages.append(userMsg);

    // Build request body
    Json::Value body;
    body["model"] = model;
    body["messages"] = messages;
    body["temperature"] = 0.4;   // descriptions should be factual, not creative
    body["max_tokens"] = 2048;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string bodyStr = Json::writeString(writer, body);

    sendChatRequest(baseUrl, apiKey, bodyStr, std::move(callback));
}

} // namespace chronochat
