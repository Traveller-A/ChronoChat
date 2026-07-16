#include "admin_agent.h"
#include "../config/config_manager.h"
#include "ai_service.h"
#include <sstream>

namespace chronochat {

AdminAgent& AdminAgent::instance() {
    static AdminAgent agent;
    return agent;
}

void AdminAgent::decideNextSpeaker(
    const std::string& groupName,
    const std::string& memberList,
    const std::string& chatHistory,
    const std::string& adminInstructions,
    AdminCallback callback)
{
    // Use global API
    std::string apiUrl = ConfigManager::instance().getTextApiBaseUrl();
    std::string apiKey = ConfigManager::instance().getTextApiKey();
    std::string apiModel = ConfigManager::instance().getTextModel();

    if (apiUrl.empty() || apiKey.empty()) {
        callback(false, "wait", "", "No global API configured");
        return;
    }

    // Normalize URL
    while (!apiUrl.empty() && apiUrl.back() == '/') apiUrl.pop_back();
    if (apiUrl.length() < 3 || apiUrl.substr(apiUrl.length() - 3) != "/v1")
        apiUrl += "/v1";

    // Build system prompt
    std::ostringstream sysPrompt;
    sysPrompt << "You are the admin agent for a group chat named \"" << groupName << "\".\n"
              << "Group members: " << memberList << "\n"
              << "There is also a human user named '" << ConfigManager::instance().getUserName() << "' who can speak at any time.\n\n"
              << "Your job is to look at the recent conversation and decide which character should speak next.\n"
              << "This creates natural, flowing conversations between group members.\n\n"
              << "Decision rules:\n"
              << "- If a character was just asked a question, they should respond (action: \"speak\")\n"
              << "- If a topic is being discussed, pick a character who would have something interesting to say\n"
              << "- Rotate between members — don't let the same person speak twice in a row\n"
              << "- If no one obviously needs to speak, or the conversation naturally paused, use action: \"wait\"\n"
              << "- If the conversation seems to have concluded, use action: \"pause\"\n";

    if (!adminInstructions.empty()) {
        sysPrompt << "\n## Special Instructions from the User\n"
                  << adminInstructions << "\n"
                  << "Follow these instructions as additional scheduling rules.\n";
    }

    sysPrompt << "\nOutput ONLY a single JSON object, nothing else:\n"
              << "{\"action\":\"speak\"|\"wait\"|\"pause\", \"character\":\"CharacterName\", \"reason\":\"Brief reason in English\"}\n"
              << "The 'character' field is required when action is \"speak\", otherwise set it to empty string.";

    // Build user prompt: the chat history
    std::ostringstream userPrompt;
    userPrompt << "Recent chat history:\n"
               << (chatHistory.empty() ? "(No messages yet — the user just entered auto mode)" : chatHistory)
               << "\n\nDecide who should speak next.";

    // Call AI
    AIService::instance().chat(apiUrl, apiKey, apiModel, sysPrompt.str(), userPrompt.str(),
        [callback](bool success, const std::string& response) {
            if (!success) {
                callback(false, "wait", "", "Admin AI call failed: " + response);
                return;
            }

            // Parse JSON response
            Json::CharReaderBuilder reader;
            std::string errors;
            Json::Value root;
            std::istringstream stream(response);
            if (!Json::parseFromStream(reader, stream, &root, &errors)) {
                callback(false, "wait", "", "Failed to parse admin response: " + errors);
                return;
            }

            std::string action = root.get("action", "wait").asString();
            std::string character = root.get("character", "").asString();
            std::string reason = root.get("reason", "").asString();

            // Validate action
            if (action != "speak" && action != "wait" && action != "pause") {
                action = "wait";
            }

            callback(true, action, character, reason);
        });
}

} // namespace chronochat
