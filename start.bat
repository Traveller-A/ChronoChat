@echo off
title ChronoChat
set BACKEND_DIR=backend\build\Release
set FRONTEND_DIR=frontend

echo ========================================
echo   ChronoChat - Spacetime Dialogue Tool
echo ========================================
echo.

where node >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Node.js not found!
    pause
    exit /b 1
)

if not exist "%FRONTEND_DIR%\node_modules\" (
    echo [INFO] Installing frontend dependencies...
    cd %FRONTEND_DIR%
    call npm install
    cd ..
)

if exist "%BACKEND_DIR%\ChronoChat.exe" (
    echo [INFO] Starting backend server...
    start "Backend" cmd /c "cd /d %CD%\%BACKEND_DIR% && ChronoChat.exe"
    timeout /t 2 >nul
) else (
    echo [WARN] Backend not built.
)

echo [INFO] Starting frontend dev server...
cd %FRONTEND_DIR%
start "Frontend" cmd /c "npm run dev"
cd ..

echo.
echo ========================================
echo   Frontend: http://localhost:5173
echo   Backend:  http://localhost:8080
echo ========================================
echo.

timeout /t 3 >nul
start http://localhost:5173

pause
