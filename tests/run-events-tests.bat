@echo off
setlocal
cd /d "%~dp0\.."
where cl >nul 2>&1
if errorlevel 1 (
    echo Run from an x64 Native Tools Command Prompt for Visual Studio.
    exit /b 1
)
if not exist test-build mkdir test-build
for %%T in (map_event_tracker damage_estimate) do (
    cl /nologo /std:c++17 /EHsc /O2 /utf-8 /Fetest-build\%%T.exe /Fotest-build\%%T.obj tests\%%T_test.cpp
    if errorlevel 1 exit /b 1
    test-build\%%T.exe
    if errorlevel 1 exit /b 1
)
echo PASS portable event and damage logic. This does not test live Dota.
exit /b 0
