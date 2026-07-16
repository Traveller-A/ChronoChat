#pragma once

#include <string>
#include <functional>
#include <json/json.h>

namespace chronochat {

// Callback type for AdminAgent
using AdminCallback = std::function<void(bool success, const std::string& action,
                                         const std::string& character, const std::string& reason)>;

// AdminAgent: decides which character should speak next in auto-dialogue mode.
// Uses the global API configuration.
class AdminAgent {
public:
    static AdminAgent& instance();

    // Decide the next speaker in a group chat.
    // groupName: display name of the group
    // memberList: comma-separated list of member names
    // chatHistory: recent chat log entries
    // adminInstructions: user-provided scheduling rules (may be empty)
    // callback: called with (success, action, character_name, reason)
    void decideNextSpeaker(const std::string& groupName,
                           const std::string& memberList,
                           const std::string& chatHistory,
                           const std::string& adminInstructions,
                           AdminCallback callback);

private:
    AdminAgent() = default;
};

} // namespace chronochat
