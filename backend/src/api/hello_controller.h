#pragma once

#include <drogon/HttpController.h>

namespace chronochat {

/**
 * HelloController - 基础连通性测试接口
 * 用于验证前后端是否正常通信
 */
class HelloController : public drogon::HttpController<HelloController> {
public:
    METHOD_LIST_BEGIN
    // GET /api/hello  -> 返回欢迎信息
    ADD_METHOD_TO(HelloController::hello, "/api/hello", drogon::Get);
    // GET /api/ping   -> 返回 pong (用于健康检查)
    ADD_METHOD_TO(HelloController::ping, "/api/ping", drogon::Get);
    METHOD_LIST_END

    void hello(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void ping(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace chronochat
