@echo off
cd /d "%~dp0"
set VCVARS=
for %%V in (18 17 16) do if not defined VCVARS if exist "%ProgramFiles%\Microsoft Visual Studio\%%V\Community\VC\Auxiliary\Build\vcvars64.bat" set VCVARS=%ProgramFiles%\Microsoft Visual Studio\%%V\Community\VC\Auxiliary\Build\vcvars64.bat
if not defined VCVARS if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" set VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat
if not defined VCVARS (
    echo [-] vcvars64.bat not found
    exit /b 1
)
call "%VCVARS%" >nul 2>&1
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W3 /D_CRT_SECURE_NO_WARNINGS inj.cpp /Fe:..\build\inj.exe /link /SUBSYSTEM:CONSOLE shell32.lib
if errorlevel 1 (
    echo [] build failed
    exit /b 1
)
echo [+] build\inj.exe
