cmake_minimum_required(VERSION 3.20)
project(ChronoChat CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Drogon 依赖 (通过 vcpkg 安装)
# ============================================================
find_package(Drogon REQUIRED)
find_package(Sqlite3 REQUIRED)

# ============================================================
# 源文件 - 按模块分组
# ============================================================
set(CONFIG_SOURCES
    src/config/config_manager.cpp
)

set(API_SOURCES
    src/api/hello_controller.cpp
    src/api/config_controller.cpp
    src/api/character_controller.cpp
    src/api/group_controller.cpp
)

set(WS_SOURCES
    src/ws/chat_websocket.cpp
)

set(SERVICE_SOURCES
    src/services/character_service.cpp
    src/services/memory_service.cpp
    src/services/ai_service.cpp
    src/services/group_service.cpp
)

set(MODEL_SOURCES
    src/models/character.cpp
)

set(AI_SOURCES
    src/ai/api_client.cpp
    src/ai/ai_service.cpp
    src/ai/admin_agent.cpp
)

set(UTIL_SOURCES
    src/utils/file_utils.cpp
)

# 汇总所有源文件
set(SOURCES
    src/main.cpp
    ${CONFIG_SOURCES}
    ${API_SOURCES}
    ${WS_SOURCES}
    ${SERVICE_SOURCES}
    ${MODEL_SOURCES}
    ${AI_SOURCES}
    ${UTIL_SOURCES}
)

# ============================================================
# 可执行文件
# ============================================================
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接 Drogon 及依赖库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Drogon::Drogon
    ${SQLITE3_LIBRARIES}
)

# MSVC: 防止链接器剥离静态注册的控制器符号
if(MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE
        "/OPT:NOREF"
    )
endif()

# 复制配置文件到构建目录
configure_file(
    ${CMAKE_SOURCE_DIR}/config.json
    ${CMAKE_BINARY_DIR}/config.json
    COPYONLY
)

# ============================================================
# 安装规则
# ============================================================
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES config.yml DESTINATION bin)
install(DIRECTORY data/ DESTINATION bin/data)
