@echo off
chcp 65001 >nul
title ODrive 高级参数工具

echo ============================================================
echo   ODrive 高级参数工具 - 快速启动
echo ============================================================
echo.

REM 检查Python是否安装
python --version >nul 2>&1
if errorlevel 1 (
    echo ❌ 错误: 未找到Python
    echo.
    echo 请先安装Python: https://www.python.org/downloads/
    echo.
    pause
    exit /b 1
)

echo ✓ Python已安装
echo.

REM 检查odrive库是否安装
python -c "import odrive" >nul 2>&1
if errorlevel 1 (
    echo ⚠️  警告: 未安装odrive库
    echo.
    echo 正在安装odrive库...
    pip install odrive
    echo.
    if errorlevel 1 (
        echo ❌ 安装失败，请手动运行: pip install odrive
        pause
        exit /b 1
    )
    echo ✓ odrive库安装成功
    echo.
)

echo ✓ odrive库已安装
echo.

REM 询问使用哪个轴
echo 请选择要操作的轴:
echo   0 - axis0 (默认)
echo   1 - axis1
echo.
set /p AXIS_NUM="请输入轴号 (直接回车使用axis0): "

if "%AXIS_NUM%"=="" set AXIS_NUM=0

echo.
echo ============================================================
echo   正在启动工具 (axis%AXIS_NUM%)...
echo ============================================================
echo.

REM 运行Python脚本
python advanced_params_tool.py %AXIS_NUM%

echo.
echo ============================================================
echo   工具已退出
echo ============================================================
pause
