#pragma once

#include <drogon/WebSocketController.h>

namespace chronochat {

// ChatWebSocket - real-time chat communication (Phase 4 implementation)
class ChatWebSocket : public drogon::WebSocketController<ChatWebSocket> {
public:
    void handleNewMessage(const drogon::WebSocketConnectionPtr& wsConnPtr,
                          std::string&& message,
                          const drogon::WebSocketMessageType& type) override;
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& wsConnPtr) override;
    void handleNewConnection(const drogon::HttpRequestPtr& req,
                             const drogon::WebSocketConnectionPtr& wsConnPtr) override;

    WS_PATH_LIST_BEGIN
    // Paths registered in Phase 4
    WS_PATH_LIST_END
};

} // namespace chronochat
