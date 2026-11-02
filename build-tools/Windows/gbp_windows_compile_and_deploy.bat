@echo off
REM Variables set here (PATH included) stay local to this script
setlocal
REM Usage : gbp_windows_compile_and_deploy.bat x.y.z  (e.g. gbp_windows_compile_and_deploy.bat 1.9.0)
REM Compiles GBP with gbp_windows_compile.bat, then, only if the compilation succeeded, deploys
REM and zips it with gbp_windows_deploy.bat. Both scripts must be in the same directory as this one.
REM   x.y.z : the gbp version, passed to gbp_windows_deploy.bat (mandatory)

REM Check the version now (same rule as gbp_windows_deploy.bat), rather than after a long compilation
if "%~1"=="" (
    echo ERROR : version missing. Usage : gbp_windows_compile_and_deploy.bat x.y.z  e.g. 1.9.0
    exit /b 1
)
echo %~1| findstr /r /x "[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*" >nul || (
    echo ERROR : invalid version "%~1". Expected x.y.z  e.g. 1.9.0
    exit /b 1
)

REM %~dp0 is the directory of this script : the other scripts are found from any current dir
call "%~dp0gbp_windows_compile.bat"
if errorlevel 1 (
    echo ERROR : compilation failed, nothing deployed
    exit /b 1
)

call "%~dp0gbp_windows_deploy.bat" %1
if errorlevel 1 (
    echo ERROR : deployment failed
    exit /b 1
)
echo Release %~1 done
