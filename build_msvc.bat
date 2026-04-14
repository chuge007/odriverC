@echo off
REM 设置 MSVC 环境
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"

REM 切换到项目目录
cd /d d:\dev\odriveCar

REM 编译
qmake
nmake debug

echo.
echo 编译完成！
pause
