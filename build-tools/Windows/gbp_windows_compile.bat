@echo off
REM Variables set here (PATH included) stay local to this script
setlocal
REM Usage : gbp_windows_compile.bat  (no argument)
REM Compiles GBP in "Release" without Qt Creator, with the CMake, Ninja and MinGW installed with
REM Qt. The result goes in the same build directory as Qt Creator, the one used by gbp_windows_deploy.bat.
REM It is always a full rebuild : the build directory is deleted first.

if not "%~1"=="" (
    echo ERROR : unknown argument "%~1". Usage : gbp_windows_compile.bat  ^(no argument^)
    exit /b 1
)

REM Define Qt version once
set QT_VERSION=6.11.3
REM Same version with "_" instead of "." (e.g. 6_11_3), as used in the Qt Creator build folder name
set QT_VERSION_U=%QT_VERSION:.=_%

set QT_DIR=C:\Qt\%QT_VERSION%\mingw_64
set QT_TOOLS=C:\Qt\Tools
set SRC_DIR=C:\Users\renoir\data\dev\Qt\gbp\src
set BUILD_DIR=%SRC_DIR%\build\Desktop_Qt_%QT_VERSION_U%_MinGW_64_bit_Release

REM The MinGW folder name contains its version (e.g. mingw1310_64) : take the one installed
set MINGW_DIR=
for /d %%D in ("%QT_TOOLS%\mingw*_64") do set MINGW_DIR=%%D
if not defined MINGW_DIR (
    echo ERROR : no MinGW found in %QT_TOOLS% ^(install it with the Qt installer^)
    exit /b 1
)
if not exist "%QT_DIR%\bin\qmake.exe" (
    echo ERROR : Qt %QT_VERSION% for MinGW not found in %QT_DIR%
    exit /b 1
)

REM Qt, MinGW, CMake and Ninja must be found first in the PATH
set "PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%QT_TOOLS%\CMake_64\bin;%QT_TOOLS%\Ninja;%PATH%"
where cmake >nul 2>&1 || (
    echo ERROR : cmake not found ^(install CMake with the Qt installer^)
    exit /b 1
)
where ninja >nul 2>&1 || (
    echo ERROR : ninja not found ^(install Ninja with the Qt installer^)
    exit /b 1
)

REM Always a full rebuild, from an empty build directory (also avoids conflicts with a build
REM directory configured differently by Qt Creator)
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if exist "%BUILD_DIR%" (
    echo ERROR : cannot delete %BUILD_DIR% ^(a file is in use ?^)
    exit /b 1
)

REM CMake prefers "/" in the paths it stores
set QT_DIR_F=%QT_DIR:\=/%
set MINGW_DIR_F=%MINGW_DIR:\=/%

REM Configure, then build with all the processor cores
cmake -S "%SRC_DIR%" -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_DIR_F%" -DCMAKE_CXX_COMPILER="%MINGW_DIR_F%/bin/g++.exe"
if errorlevel 1 (
    echo ERROR : CMake configuration failed
    exit /b 1
)
cmake --build "%BUILD_DIR%" --parallel
if errorlevel 1 (
    echo ERROR : build failed
    exit /b 1
)
echo Built %BUILD_DIR%\graphical-budget-planner.exe
