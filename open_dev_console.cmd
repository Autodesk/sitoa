@echo off

rem Windows helper script to set the environment and open a dev console by simply dubble-clicking the batch file.
rem Update the paths below to the locations that is correct for your system.

set "Path=C:\Users\lindg\DEV\Portable Python-2.7.17 x64\App\Python;%Path%"
set "Path=C:\Users\lindg\DEV\SolidAngle\Arnold-7.1.4.4-windows\bin;%Path%"
set "Path=%~dp0dist\Softimage_2015\Addons\SItoA\Application\Plugins\bin\nt-x86-64;%Path%" 

set "ARNOLD_PLUGIN_PATH=%~dp0dist\Softimage_2015\Addons\SItoA\Application\Plugins\bin\nt-x86-64"

set sitoa_dev_cmd="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 -vcvars_ver=14.0

WHERE wt.exe /Q
if %ERRORLEVEL% NEQ 0 (
    START "SItoA Dev Console" %sitoa_dev_cmd%
) else (
    wt.exe new-tab -d . --title "SItoA Dev Console" cmd /K %sitoa_dev_cmd%
)
