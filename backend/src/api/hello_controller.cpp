#include "hello_controller.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>

namespace chronochat {

void HelloController::hello(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    Json::Value resp;
    resp["code"] = 0;
    resp["message"] = "Hello from ChronoChat Backend!";
    resp["data"]["name"] = "ChronoChat";
    resp["data"]["version"] = "0.0.1";
    resp["data"]["description"] = "时空对话工具 - 后端服务运行正常";

    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

void HelloController::ping(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    Json::Value resp;
    resp["code"] = 0;
    resp["message"] = "pong";
    resp["data"]["timestamp"] = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    auto httpResp = drogon::HttpResponse::newHttpJsonResponse(resp);
    httpResp->addHeader("Access-Control-Allow-Origin", "*");
    callback(httpResp);
}

} // namespace chronochat
