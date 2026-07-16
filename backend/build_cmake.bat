@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d D:\WorkSpace\ZCode\260715\ChronoChat\backend\build
cmake .. -DCMAKE_TOOLCHAIN_FILE=D:/Software/vcpkg-master/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configure failed!
    exit /b 1
)
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    exit /b 1
)
echo [SUCCESS] ChronoChat backend built successfully!
dir Release\ChronoChat.exe
