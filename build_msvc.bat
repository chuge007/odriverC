@echo off
setlocal

cd /d d:\dev\odriveCar

set "VCVARS="
for %%I in (
    "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) do (
    if not defined VCVARS if exist %%~I set "VCVARS=%%~I"
)

set "QMAKE=D:\qt\5.12.4\msvc2017_64\bin\qmake.exe"

if not defined VCVARS (
    echo [ERROR] vcvars64.bat not found in common Visual Studio locations.
    exit /b 1
)

if not exist "%QMAKE%" (
    echo [ERROR] qmake.exe not found:
    echo %QMAKE%
    exit /b 1
)

echo [INFO] Loading MSVC environment...
call "%VCVARS%"
if errorlevel 1 (
    echo [ERROR] Failed to load MSVC environment.
    exit /b 1
)

where nmake
if errorlevel 1 (
    echo [ERROR] nmake not found after loading MSVC environment.
    exit /b 1
)

echo [INFO] Generating Makefile with qmake...
"%QMAKE%" odriver.pro
if errorlevel 1 (
    echo [ERROR] qmake failed.
    exit /b 1
)

echo [INFO] Building debug target with nmake...
nmake debug
if errorlevel 1 (
    echo [ERROR] nmake build failed.
    exit /b 1
)

echo.
echo [INFO] Build completed successfully.
exit /b 0