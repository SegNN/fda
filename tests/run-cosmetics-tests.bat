@echo off
setlocal
cd /d "%~dp0\.."
where cl >nul 2>&1
if errorlevel 1 (
    echo Run this from an x64 Native Tools Command Prompt for Visual Studio.
    exit /b 1
)
if not exist test-build mkdir test-build
cl /nologo /std:c++17 /EHsc /O2 /utf-8 /Fetest-build\cosmetic-core-test.exe /Fotest-build\cosmetic_core_test.obj tests\cosmetic_core_test.cpp
if errorlevel 1 exit /b 1
test-build\cosmetic-core-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /utf-8 /Itests\winshim /Fotest-build\ /Fetest-build\cosmetic-adapter-test.exe tests\disabled_cosmetics_test.cpp src\skin_changer.cpp
if errorlevel 1 exit /b 1
test-build\cosmetic-adapter-test.exe
exit /b %errorlevel%
