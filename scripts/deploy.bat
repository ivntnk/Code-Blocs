@echo off
setlocal
rem Cree Blocs\dist\Blocs a partir du .exe compile passe en argument.
rem Utilisation depuis une invite Qt MinGW : scripts\deploy.bat "chemin\vers\build\Blocs.exe"
set "EXE=%~1"
if "%EXE%"=="" goto usage
if not exist "%EXE%" (
    echo ERREUR : fichier introuvable : "%EXE%"
    goto usage
)
where windeployqt >nul 2>nul
if errorlevel 1 (
    echo ERREUR : windeployqt est introuvable.
    echo Lance ce script depuis l'invite Qt 6.12.0 MinGW 64-bit du menu Demarrer.
    exit /b 1
)
for %%F in ("%EXE%") do set "NAME=%%~nxF"
rem Le script est dans Blocs\scripts : remonter a Blocs, puis creer dist\Blocs.
for %%D in ("%~dp0..") do set "PROJECT=%%~fD"
set "DIST=%PROJECT%\dist\Blocs"
if exist "%DIST%" rmdir /s /q "%DIST%"
mkdir "%DIST%" || exit /b 1
copy "%EXE%" "%DIST%\" >nul || exit /b 1
windeployqt --release --no-translations "%DIST%\%NAME%"
if errorlevel 1 (
    echo ERREUR : windeployqt a echoue.
    exit /b 1
)
echo.
echo Deploiement termine : "%DIST%"
echo Lance "%DIST%\%NAME%" pour tester.
exit /b 0
:usage
echo Utilisation : %~nx0 "chemin\vers\build\Blocs.exe"
echo Exemple : %~nx0 "C:\chemin\du\projet\build\Desktop_Qt_6_12_0_MinGW_64_bit_Release\Blocs.exe"
exit /b 1
