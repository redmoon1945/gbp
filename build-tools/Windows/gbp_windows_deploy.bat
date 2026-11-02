@echo off
REM Variables set here (PATH included) stay local to this script
setlocal
REM Usage : gbp_windows_deploy.bat x.y.z  (e.g. gbp_windows_deploy.bat 1.9.0)
REM Empties DEPLOY_DIR, deploys GBP in DEPLOY_DIR\gbp-x.y.z, then zips that folder as
REM gbp-x.y.z-windows-x86_64.zip in the directory of this script (the zip contains the gbp-x.y.z
REM folder). DEPLOY_DIR is a working directory : it is deleted once the zip is created.

REM The version is mandatory, in the form x.y.z
set VERSION=%~1
if "%VERSION%"=="" (
    echo ERROR : version missing. Usage : gbp_windows_deploy.bat x.y.z  e.g. gbp_windows_deploy.bat 1.9.0
    exit /b 1
)
echo %VERSION%| findstr /r /x "[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*" >nul || (
    echo ERROR : invalid version "%VERSION%". Expected x.y.z  e.g. 1.9.0
    exit /b 1
)

REM Define Qt version once
set QT_VERSION=6.11.3
REM Same version with "_" instead of "." (e.g. 6_11_3), as used in the Qt Creator build folder name
set QT_VERSION_U=%QT_VERSION:.=_%

set DEPLOY_DIR=C:\Users\renoir\data\dev\deploy-gbp
set BUILD_DIR=C:\Users\renoir\data\dev\Qt\gbp\src\build\Desktop_Qt_%QT_VERSION_U%_MinGW_64_bit_Release
set APP_DIR=%DEPLOY_DIR%\gbp-%VERSION%
REM The zip goes in the directory of this script (%~dp0 ends with a "\")
set ZIP_FILE=%~dp0gbp-%VERSION%-windows-x86_64.zip

REM Start from an empty DEPLOY_DIR, so that no file of a previous deployment ends up in the zip
if exist "%DEPLOY_DIR%" rmdir /s /q "%DEPLOY_DIR%"
if exist "%DEPLOY_DIR%" (
    echo ERROR : cannot empty %DEPLOY_DIR% ^(a file is in use ?^)
    exit /b 1
)
mkdir "%APP_DIR%"

set "PATH=%PATH%;C:\Qt\%QT_VERSION%\mingw_64\bin"
windeployqt --verbose 1 --compiler-runtime --dir "%APP_DIR%" "%BUILD_DIR%\graphical-budget-planner.exe"
if errorlevel 1 (
    echo ERROR : windeployqt failed
    exit /b 1
)
REM Since 1.8.2 the build produces graphical-budget-planner.exe : deliver it as gbp.exe on Windows.
REM windeployqt --dir does not copy the executable itself : copy it here, renamed.
copy /Y "%BUILD_DIR%\graphical-budget-planner.exe" "%APP_DIR%\gbp.exe"
if errorlevel 1 (
    echo ERROR : cannot copy graphical-budget-planner.exe
    exit /b 1
)
REM GBP translations (gbp_*.qm), loaded from the executable's folder : not copied by windeployqt
copy /Y "%BUILD_DIR%\gbp_*.qm" "%APP_DIR%"
if errorlevel 1 (
    echo ERROR : cannot copy the translation files gbp_*.qm
    exit /b 1
)

REM Zip the gbp-x.y.z folder itself, not only its content (replaces a previous zip of the same
REM version)
powershell -NoProfile -Command "Compress-Archive -Path '%APP_DIR%' -DestinationPath '%ZIP_FILE%' -Force"
if errorlevel 1 (
    echo ERROR : cannot create %ZIP_FILE%
    exit /b 1
)

REM The working directory is no longer needed : remove it
rmdir /s /q "%DEPLOY_DIR%"
if exist "%DEPLOY_DIR%" echo WARNING : cannot delete %DEPLOY_DIR% ^(a file is in use ?^)
echo Created %ZIP_FILE%
