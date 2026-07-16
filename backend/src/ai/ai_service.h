#pragma once

#include <string>
#include <functional>
#include <json/json.h>

namespace drogon {
    class HttpResponse;
    enum class ReqResult;
}

namespace chronochat {

// Callback type for async AI responses
using AICallback = std::function<void(bool success, const std::string& responseText)>;

// OpenAI-compatible chat completions API client
class AIService {
public:
    static AIService& instance();

    // Send a chat completion request
    // baseUrl: should end with /v1 (e.g. https://api.openai.com/v1)
    // apiKey: bearer token
    // model: model name (e.g. gpt-4o, deepseek-chat)
    // systemPrompt: system message
    // userPrompt: user message
    // callback: called with (success, content)
    void chat(const std::string& baseUrl,
              const std::string& apiKey,
              const std::string& model,
              const std::string& systemPrompt,
              const std::string& userPrompt,
              AICallback callback);

private:
    AIService() = default;
};

} // namespace chronochat
