# ============================================================
# ChronoChat 开发环境搭建指南
# ============================================================

## 前置条件

- **Node.js** >= 18 (已安装: ✅)
- **CMake** >= 3.20 (已安装: ✅)
- **Visual Studio 2022** (已安装: ✅ Community 版)
- **vcpkg** (C++ 包管理器) - 需要手动安装
- **Python** >= 3.10 (已安装: ✅)

---

## 一、安装 vcpkg（C++ 包管理器）

### 步骤 1: 克隆 vcpkg

打开 **Git Bash** 或 **命令提示符**，执行：

```bash
cd D:\
git clone https://github.com/microsoft/vcpkg.git
```

> ⚠️ 如果 GitHub 连接不稳定，可以下载 ZIP 包：
> 1. 浏览器打开 https://github.com/microsoft/vcpkg/archive/refs/heads/master.zip
> 2. 下载后解压到 `D:\vcpkg`

### 步骤 2: 安装 vcpkg

```bash
cd D:\vcpkg
bootstrap-vcpkg.bat
```

### 步骤 3: 设置环境变量

将 `D:\vcpkg` 添加到系统 PATH 环境变量中，或者每次构建时指定路径。

### 步骤 4: 集成到 Visual Studio（可选）

```bash
vcpkg integrate install
```

---

## 二、安装 Drogon 依赖

```bash
vcpkg install drogon:x64-windows sqlite3:x64-windows jsoncpp:x64-windows
```

> ⚠️ 这一步会从网络下载和编译，可能需要 10-30 分钟，请耐心等待。

---

## 三、构建后端

### 使用 CMake 构建

```bash
cd D:\WorkSpace\ZCode\260715\ChronoChat\backend
mkdir build
cd build

cmake .. -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

构建成功后，`backend\build\Release\ChronoChat.exe` 即为可执行文件。

> 💡 **提示**: 使用 Visual Studio Developer Command Prompt 运行以上命令可以获得最佳兼容性。
> 打开方式：开始菜单 -> Visual Studio 2022 -> Developer Command Prompt for VS 2022

---

## 四、安装前端依赖

```bash
cd D:\WorkSpace\ZCode\260715\ChronoChat\frontend
npm install
```

---

## 五、启动项目

### 方式一：使用启动脚本

双击项目根目录下的 `start.bat`

### 方式二：分别启动

**终端 1 - 后端:**
```bash
cd backend\build\Release
ChronoChat.exe
```

**终端 2 - 前端:**
```bash
cd frontend
npm run dev
```

### 方式三：仅前端预览（后端未构建时）

```bash
cd frontend
npm run dev
```
浏览器访问 http://localhost:5173

---

## 六、验证

1. 前端启动后，浏览器打开 http://localhost:5173
2. 首页底部应显示后端连接状态
3. 后端启动后，访问 http://localhost:8080/api/hello 应返回 JSON

---

## 项目结构

```
ChronoChat/
├── backend/                     # C++ 后端 (Drogon)
│   ├── CMakeLists.txt           # CMake 构建配置
│   ├── config.yml               # 后端配置文件
│   ├── src/
│   │   ├── main.cpp             # 入口
│   │   ├── config/              # 配置管理模块
│   │   ├── api/                 # HTTP API 控制器
│   │   ├── ws/                  # WebSocket 控制器
│   │   ├── services/            # 业务逻辑服务层
│   │   ├── models/              # 数据模型
│   │   ├── ai/                  # AI 调用封装
│   │   └── utils/               # 工具函数
│   └── data/                    # 运行时数据
├── frontend/                    # Vue 3 前端
│   ├── src/
│   │   ├── views/               # 页面组件
│   │   ├── components/          # 通用组件
│   │   ├── router/              # 路由
│   │   ├── stores/              # Pinia 状态管理
│   │   ├── api/                 # API 调用封装
│   │   └── assets/              # 静态资源
│   └── ...
├── tools/                       # Python 工具脚本
├── start.bat                    # Windows 启动脚本
└── README.md
```
