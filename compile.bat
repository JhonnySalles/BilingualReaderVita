@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo   BilingualReaderVita - Script de Compilacao
echo =======================================================
echo.

:: Obter o diretorio raiz do projeto sem barra final
set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

:: Criar a pasta de build caso nao exista
if not exist "%PROJECT_ROOT%\build" (
    echo [*] Criando diretorio build...
    mkdir "%PROJECT_ROOT%\build"
)

:: Criar a pasta apk caso nao exista
if not exist "%PROJECT_ROOT%\apk" (
    echo [*] Criando diretorio apk...
    mkdir "%PROJECT_ROOT%\apk"
)

:: Verificar se cmake e make estao disponiveis no PATH do Windows
where cmake >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [*] CMake detectado no ambiente Windows.
    echo [*] Compilando via Windows nativo...
    cd /d "%PROJECT_ROOT%\build"
    cmake ..
    if %ERRORLEVEL% NEQ 0 (
        echo [!] Erro ao executar o CMake.
        pause
        exit /b 1
    )
    make
    if %ERRORLEVEL% NEQ 0 (
        echo [!] Erro ao executar o Make.
        pause
        exit /b 1
    )
) else (
    echo [*] CMake nao encontrado diretamente no PATH do Windows.
    echo [*] Buscando instalacao do MSYS2...

    set "MSYS2_BASH="
    if exist "D:\msys64\usr\bin\bash.exe" set "MSYS2_BASH=D:\msys64\usr\bin\bash.exe"
    if not defined MSYS2_BASH if exist "C:\msys64\usr\bin\bash.exe" set "MSYS2_BASH=C:\msys64\usr\bin\bash.exe"
    if not defined MSYS2_BASH if exist "%MSYS2_PATH%\usr\bin\bash.exe" set "MSYS2_BASH=%MSYS2_PATH%\usr\bin\bash.exe"

    if defined MSYS2_BASH (
        echo [*] MSYS2 encontrado em: !MSYS2_BASH!
        echo [*] Executando compilacao atraves do MSYS2...
        echo.
        
        :: Converter caminho do Windows para formato UNIX (ex: F:\Projetos -> /f/Projetos)
        set "DRIVE_LETTER=%PROJECT_ROOT:~0,1%"
        set "REST_OF_PATH=%PROJECT_ROOT:~2%"
        set "REST_OF_PATH=!REST_OF_PATH:\=/!"
        set "UNIX_PROJECT_ROOT=/!DRIVE_LETTER!!REST_OF_PATH!"
        
        "!MSYS2_BASH!" -lc "cd '!UNIX_PROJECT_ROOT!' && bash compile.sh"
        if !ERRORLEVEL! NEQ 0 (
            echo.
            echo [!] Erro durante a compilacao no MSYS2.
            pause
            exit /b 1
        )
        goto :finalize
    ) else (
        echo [!] ERRO: Nem o CMake no PATH do Windows nem o MSYS2 foram encontrados.
        echo [!] Verifique se o MSYS2 esta em D:\msys64 ou C:\msys64.
        pause
        exit /b 1
    )
)

:: Copiar arquivo gerado para a pasta apk (caso compilado nativamente)
if exist "%PROJECT_ROOT%\build\BilingualReaderVita.vpk" (
    echo [*] Copiando BilingualReaderVita.vpk para a pasta apk...
    copy /y "%PROJECT_ROOT%\build\BilingualReaderVita.vpk" "%PROJECT_ROOT%\apk\" >nul
)

:finalize
echo.
echo =======================================================
if exist "%PROJECT_ROOT%\apk\BilingualReaderVita.vpk" (
    echo [OK] Compilacao concluida com sucesso!
    echo [OK] Arquivo gerado em: %PROJECT_ROOT%\apk\BilingualReaderVita.vpk
) else (
    echo [!] O arquivo BilingualReaderVita.vpk nao foi encontrado na pasta apk.
)
echo =======================================================
echo.
pause