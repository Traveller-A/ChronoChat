# ChronoChat · 时空对话

> 一个以「时空」为意象的 AI 角色扮演聊天应用——与跨越时空的旅人对话，或举办一场时空茶话会。

ChronoChat 让你创建拥有独立身份、性格与记忆的虚拟角色，并通过 OpenAI 兼容的大模型 API 与他们进行即时聊天、书信往来，甚至把多个角色拉进同一个群聊让他们彼此互动。角色会随着对话积累记忆，在跨日时自动压缩过往对话为核心记忆，越聊越「懂你」。

---

## 一、项目简介

### 这是什么

ChronoChat 是一个前后端分离的 AI 角色扮演（Roleplay）聊天平台：

- **后端**：C++17 + [Drogon](https://github.com/drogonframework/drogon) 异步 Web 框架，SQLite 持久化，负责角色/群聊/配置的存储与 AI 调用编排。
- **前端**：Vue 3 + Vite + Element Plus，星空主题的沉浸式界面。

### 用来做什么

- **创建角色**：填写姓名、性别、年龄、生日、MBTI、背景故事等，由 AI 自动生成身份（IDENTITY）、灵魂性格（SOUL）、记忆（MEMORY）、对用户的认知（USER）四份档案。
- **一对一对话**：与角色进行**即时聊天**或**书信往来**两种模式，角色始终保持人设、引用过往记忆。
- **群聊茶话会**：把多个角色拉进一个群，支持 **@提及模式**（指定某角色接话）与**自动对话模式**（由「主持人 Agent」决定谁接着说，自动连续推进）。
- **多模态图片**：配置多模态 API 后，可在聊天中上传图片——视觉模型先将图片转为详细文字描述，再交给角色 AI 据此回复。
- **个人头像**：在设置页上传自己的头像，聊天时显示在你的消息气泡右侧。
- **记忆系统**：跨日对话时自动把昨日聊天与书信压缩合并进核心记忆，让角色记住长期关系与事件。
- **人格分析**：基于角色档案与对话历史，推测 MBTI、大五人格与说话风格。

---

## 二、功能特性

| 模块 | 能力 |
| --- | --- |
| 角色管理 | 创建 / 编辑 / 删除 / 导出 / 存为副本；头像上传；按最近活跃排序 |
| 角色档案 | AI 一键生成 IDENTITY / SOUL / MEMORY / USER 四份 Markdown 档案，可手动编辑 |
| 一对一聊天 | 即时模式（气泡）与书信模式（信纸卡片），独立历史、独立日志 |
| 记忆压缩 | 检测到日期变更时，将昨日 CHAT_LOG + LETTER_LOG 压缩并入 MEMORY.md |
| 群聊 | 多角色群组、@提及 / 自动两种对话模式、主持人 Agent 调度发言、群管理 |
| 多模态 | 上传图片 → 视觉模型生成描述 → 角色 AI 据描述回复；附图缩略图与预览 |
| 个性化 | 用户昵称、用户头像；每角色可单独配置文本/多模态 API（留空则用全局） |
| 人格分析 | MBTI / 大五人格 / 关键特质 / 说话风格推测 |
| API 配置 | 全局文本 API + 全局多模态 API（选填）；连接测试 |

---

## 三、技术栈

**后端**

- 语言：C++17
- Web 框架：Drogon（异步、高性能）
- 数据库：SQLite3（WAL 模式）
- JSON：jsoncpp
- 包管理：vcpkg
- 构建：CMake ≥ 3.20 + Visual Studio 2022（MSVC）

**前端**

- 框架：Vue 3（`<script setup>`）
- 构建：Vite 5
- 路由：Vue Router 4（Hash 模式）
- 状态：Pinia
- UI：Element Plus 2 + @element-plus/icons-vue（图标全局注册）
- HTTP：Axios
- 自动导入：unplugin-auto-import / unplugin-vue-components

**AI 接入**：任何 OpenAI 兼容的 `/v1/chat/completions` 接口（OpenAI、Azure OpenAI、OpenRouter、DeepSeek、Moonshot、Qwen-VL 等）。视觉调用使用 OpenAI `image_url` data-URI 标准。

---

## 四、项目架构

```
ChronoChat/
├── backend/                         # C++ 后端 (Drogon)
│   ├── CMakeLists.txt               # 构建配置（MSVC 加 /utf-8 以支持源码中文）
│   ├── config.json                  # Drogon 运行配置（端口 8080，body 上限 50MB）
│   ├── build_cmake.bat              # 一键构建脚本
│   ├── data/                        # 运行时数据库
│   │   └── chronochat.db            #   SQLite（system_config / characters / groups）
│   └── src/
│       ├── main.cpp                 # 入口：初始化 Config/Character/Group 服务并 run
│       ├── config/
│       │   └── config_manager.*     # 全局配置单例（文本/多模态 API、用户名、用户头像）
│       ├── api/                     # HTTP 控制器
│       │   ├── hello_controller.*   #   连通性测试 /api/hello
│       │   ├── config_controller.*  #   配置读写、API 测试、用户头像 CRUD
│       │   ├── character_controller.*#  角色 CRUD、档案生成、聊天、历史、人格分析、导出
│       │   └── group_controller.*   #   群聊 CRUD、成员、聊天、自动推进、模式切换
│       ├── services/                # 业务服务层
│       │   ├── character_service.*  #   角色表 CRUD + 角色文件读写 + 记忆压缩触发
│       │   └── group_service.*      #   群聊表 CRUD + 成员 + 聊天日志 + 主持人指令
│       ├── ai/                      # AI 调用封装
│       │   ├── ai_service.*         #   OpenAI 兼容 chat / chatVision（视觉）
│       │   └── admin_agent.*        #   群聊自动模式：决定下一个发言者
│       ├── models/ · ws/ · utils/   # 预留模块（部分为占位实现）
│       └── ...
├── frontend/                        # Vue 3 前端
│   ├── vite.config.js               # Vite 配置（/api 代理到 :8080）
│   └── src/
│       ├── main.js                  # 注册 Element Plus + 全部图标
│       ├── api/index.js             # Axios 封装与全部接口函数
│       ├── router/index.js          # 路由表
│       ├── views/                   # 页面
│       │   ├── HomePage.vue         #   首页（角色列表 / 群聊列表入口 + 后端状态）
│       │   ├── SettingsPage.vue     #   设置（API 配置 + 用户资料/头像）
│       │   ├── Character*.vue       #   角色列表/创建/编辑/信息/聊天
│       │   └── GroupChat*.vue       #   群聊列表/创建/聊天/管理
│       ├── components/StarField.vue #   星空背景
│       └── assets/global.css        #   全局主题变量与样式
├── data/                            # 运行时文件数据
│   ├── characters/<id>/             #   每个角色的档案与头像
│   ├── groups/<id>/chat.log         #   群聊日志
│   └── uploads/                     #   用户头像等上传文件
├── tools/                           # Python 辅助脚本（角色恢复等）
├── start.bat                        # Windows 一键启动（前后端）
└── README.md
```

### 数据模型

**SQLite（`backend/data/chronochat.db`）**

- `system_config`：键值表，存全局文本/多模态 API 配置、用户名、用户头像路径。
- `characters`：角色元数据（姓名、性别、年龄、生日、MBTI、签名、故事、各 API 配置、`last_active_date`、`last_chat_at` 等）。
- `groups`：群聊元数据（名称、描述、模式 `mention`/`auto`、状态、成员 JSON、主持人指令、时间戳）。

**文件系统（`data/`）**

每个角色一个目录 `data/characters/<id>/`，包含：

| 文件 | 作用 |
| --- | --- |
| `IDENTITY.md` | 身份：姓名、年龄、外貌、背景概述 |
| `SOUL.md` | 灵魂：性格特质、说话风格、价值观、行为习惯 |
| `MEMORY.md` | 核心记忆：由对话压缩而来的关键事实与关系发展 |
| `USER.md` | 角色对用户的认知与关系 |
| `CHAT_LOG.md` | 即时聊天历史（用于上下文，超长自动裁剪） |
| `LETTER_LOG.md` | 书信往来历史 |
| `avatar.jpg` | 角色头像 |

群聊日志存于 `data/groups/<id>/chat.log`；用户头像存于 `data/uploads/user_avatar.<ext>`。

### 请求流转（一对一聊天为例）

1. `POST /api/characters/{id}/chat` 收到消息（可附带 `images`）。
2. 若跨日：异步将昨日 CHAT_LOG + LETTER_LOG 压缩并入 MEMORY.md。
3. 解析多模态配置（角色专属 > 全局）；若带图且已配置，先调 `chatVision` 得到图片文字描述，与用户消息拼接为增强 prompt。
4. 由 IDENTITY/SOUL/USER/MEMORY/聊天历史 组装系统提示，调用文本模型。
5. 将用户消息与角色回复追加到对应日志，返回响应（含 `api_source`、`multimodal_available`、`user_avatar_set`）。

---

## 五、接口概览

所有接口前缀 `/api`，返回统一 `{ code, message, data }`（`code=0` 为成功）。

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/hello` | 后端连通性测试 |
| GET / POST | `/config` | 读取 / 保存全局配置 |
| POST | `/config/test-text` | 测试文本 API 连接 |
| POST | `/config/test-multimodal` | 测试多模态 API 连接 |
| GET / POST / DELETE | `/config/avatar` | 读取 / 上传(base64) / 删除 用户头像 |
| GET / POST | `/characters` | 角色列表 / 创建 |
| GET / PUT / DELETE | `/characters/{id}` | 详情 / 更新 / 删除 |
| GET | `/characters/{id}/avatar` | 角色头像图片 |
| POST | `/characters/{id}/test-text` `/test-multimodal` | 测试角色专属 API |
| POST | `/characters/{id}/generate` | AI 生成四份档案 |
| POST | `/characters/{id}/chat` | 发送消息（可带 `images`） |
| GET | `/characters/{id}/history` | 对话历史（`mode=chat`/`letter`） |
| POST | `/characters/{id}/analyze` | 人格分析 |
| GET | `/characters/{id}/export` · POST `/duplicate` | 导出 / 存为副本 |
| GET / POST | `/groups` | 群聊列表 / 创建 |
| GET / PUT / DELETE | `/groups/{id}` | 详情 / 更新 / 删除 |
| POST / DELETE | `/groups/{id}/members[/{cid}]` | 加 / 移除成员 |
| POST | `/groups/{id}/chat` | 群聊发言 |
| GET | `/groups/{id}/history` | 群聊历史 |
| POST | `/groups/{id}/auto-step` | 自动模式推进一步 |
| POST | `/groups/{id}/mode` | 切换 `mention` / `auto` |
| GET | `/groups/{id}/avatar` | 群聊头像 |

---

## 六、环境准备

### 前置条件

- **Node.js** ≥ 18
- **CMake** ≥ 3.20
- **Visual Studio 2022**（含 C++ 桌面开发）
- **vcpkg**（C++ 包管理器）
- **Python** ≥ 3.10（仅辅助脚本需要）

### 1. 安装 vcpkg

```bash
cd D:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
```

将 `D:\vcpkg` 加入 PATH，或构建时指定工具链文件路径。

> 若 GitHub 不稳定，可下载 ZIP 解压到 `D:\vcpkg`。

### 2. 安装 Drogon 依赖

```bash
vcpkg install drogon:x64-windows sqlite3:x64-windows jsoncpp:x64-windows
```

> 首次编译依赖需 10–30 分钟。

### 3. 构建后端

直接使用项目自带脚本（已配置好 MSVC 环境与 vcpkg 工具链）：

```bash
cd D:\WorkSpace\ZCode\260715\ChronoChat\backend
build_cmake.bat
```

或手动构建：

```bash
cd backend
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

构建成功后得到 `backend\build\Release\ChronoChat.exe`。

> **说明**：`CMakeLists.txt` 为 MSVC 启用了 `/utf-8`，源码中的中文字符串可正常编译。建议在「Developer Command Prompt for VS 2022」中构建。

### 4. 安装前端依赖

```bash
cd frontend
npm install
```

---

## 七、启动

### 方式一：一键启动（Windows）

双击项目根目录 `start.bat`：自动启动后端、前端开发服务器并打开浏览器。

### 方式二：分别启动

**终端 1 — 后端**

```bash
cd backend\build\Release
ChronoChat.exe
```

**终端 2 — 前端**

```bash
cd frontend
npm run dev
```

浏览器访问 http://localhost:5173 ，后端地址 http://localhost:8080 。

### 验证

- 首页底部应显示「后端服务已连接」。
- 访问 http://localhost:8080/api/hello 返回 JSON 即后端正常。

---

## 八、使用指南

1. **配置 API**：进入「设置」，填写文本处理 API（Base URL / Key / Model，如 `gpt-4o`、`deepseek-chat`），可点击「测试」验证。多模态 API 为选填，配置后才会开放图片上传。
2. **创建角色**：在「角色列表」新建角色，填写背景故事等，保存后点击「重新生成角色文件」让 AI 生成四份档案（也可在编辑页手动修改）。
3. **开始对话**：进入角色聊天，可切换「即时 / 书信」两种模式。即时模式支持回车发送。
4. **发送图片**：若已配置多模态 API，即时模式输入框左侧出现图片按钮，选图后随消息发送；角色会基于图片内容回复。
5. **个人头像**：在「设置 → 用户资料」上传头像，聊天时你的头像会显示在消息气泡右侧。
6. **群聊茶话会**：在「群聊列表」新建群组并添加成员。`@提及模式` 下 @某角色让其接话；`自动模式` 下角色们会在主持人 Agent 调度下自动轮流发言。
7. **角色专属 API**：在角色编辑页可为单个角色单独配置 API（留空则使用全局）。

---

## 九、注意事项

- **API 兼容性**：后端按 OpenAI 兼容协议调用 `/v1/chat/completions`，Base URL 会自动补全 `/v1`。视觉调用使用 `image_url` data-URI 格式，兼容多数 OpenAI 兼容网关。
- **请求体上限**：`config.json` 中 `client_max_body_size` 为 50MB，足以承载 base64 图片；前端对单图 >10MB 警告、>20MB 拒绝。
- **超时**：聊天接口（含视觉两步调用）前端超时设为 120s。
- **记忆压缩**：跨日首次对话时触发，压缩为异步执行；当日对话写入对应日志文件。
- **日志可读性**：带图消息在 `CHAT_LOG.md` 中仅记录文字 + 「（附图 N 张）」标记，不写入 base64。
- **编码**：后端源码含中文，依赖 MSVC `/utf-8` 标志（已在 `CMakeLists.txt` 配置）。
- **数据目录**：角色文件位于项目根 `data/`，元数据位于 `backend/data/chronochat.db`，二者由同一 SQLite 库关联，请勿随意移动。

---

## 十、许可

本项目为个人学习/创作用途。所用开源依赖（Drogon、Vue、Element Plus 等）请遵循其各自许可证。
