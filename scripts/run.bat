@echo off
setlocal

:: parse args
set NOBUILD=0
for %%A in (%*) do (
    if /i "%%A"=="nobuild" set NOBUILD=1
)

:: SET PATHS HERE!!
set ROOT=%~dp0..
set GODOT=%ROOT%\bin\godot\Godot_v4.6.1-stable_win64_console.exe
set PROJECT=%ROOT%\project

:: verify if godot exists
if not exist "%GODOT%" (
    echo ERROR: Not found godot executable in bin\godot.exe
    echo Please place the godot exe in: %ROOT%\bin\godot.exe
    pause
    exit /b 1
)

:: compile if its on building mode
if %NOBUILD%==0 (
    echo Compiling extension...
    cd /d "%ROOT%"
    scons platform=windows target=template_debug
    if %errorlevel% neq 0 (
        echo ERROR: Failed to compile
        pause
        exit /b 1
    )
)

:: open godot
echo Opening project in godot...
"%GODOT%" --path "%PROJECT%" --editor