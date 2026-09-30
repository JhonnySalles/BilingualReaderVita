@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo   Rebuild MuPDF for PS Vita (MSYS2)
echo =======================================================
echo.

set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

set "MSYS2_BASH="
if exist "D:\msys64\usr\bin\bash.exe" set "MSYS2_BASH=D:\msys64\usr\bin\bash.exe"
if not defined MSYS2_BASH if exist "C:\msys64\usr\bin\bash.exe" set "MSYS2_BASH=C:\msys64\usr\bin\bash.exe"
if not defined MSYS2_BASH if exist "%MSYS2_PATH%\usr\bin\bash.exe" set "MSYS2_BASH=%MSYS2_PATH%\usr\bin\bash.exe"

if defined MSYS2_BASH (
    echo [*] MSYS2 encontrado em: !MSYS2_BASH!
    echo [*] Executando build_mupdf_vita.sh...
    echo.
    
    set "DRIVE_LETTER=%PROJECT_ROOT:~0,1%"
    set "REST_OF_PATH=%PROJECT_ROOT:~2%"
    set "REST_OF_PATH=!REST_OF_PATH:\=/!"
    set "UNIX_PROJECT_ROOT=/!DRIVE_LETTER!!REST_OF_PATH!"
    
    "!MSYS2_BASH!" -lc "cd '!UNIX_PROJECT_ROOT!' && bash build_mupdf_vita.sh"
    if !ERRORLEVEL! NEQ 0 (
        echo.
        echo [!] Erro durante a compilacao do MuPDF no MSYS2.
        pause
        exit /b 1
    )
    echo.
    echo [OK] MuPDF recompilado com sucesso!
) else (
    echo [!] ERRO: MSYS2 nao encontrado.
    pause
    exit /b 1
)

pause
