REM Define Qt version once
set QT_VERSION=6.10.3
REM Same version with "_" instead of "." (e.g. 6_10_3), as used in the Qt Creator build folder name
set QT_VERSION_U=%QT_VERSION:.=_%

set DEPLOY_DIR=C:\Users\renoir\data\dev\deploy-gbp
set BUILD_DIR=C:\Users\renoir\data\dev\Qt\gbp\src\build\Desktop_Qt_%QT_VERSION_U%_MinGW_64_bit_Release

set PATH=%PATH%;C:\Qt\%QT_VERSION%\mingw_64\bin
windeployqt --verbose 1 --compiler-runtime --dir %DEPLOY_DIR% %BUILD_DIR%\graphical-budget-planner.exe
REM Since 1.8.2 the build produces graphical-budget-planner.exe : deliver it as gbp.exe on Windows
if exist %DEPLOY_DIR%\gbp.exe del %DEPLOY_DIR%\gbp.exe
ren %DEPLOY_DIR%\graphical-budget-planner.exe gbp.exe
