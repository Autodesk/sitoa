@echo off

rem Windows helper script to open a dev console by simply dubble-clicking the batch file.

set sitoa_dev_cmd="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 -vcvars_ver=14.0

WHERE wt.exe /Q
if %ERRORLEVEL% NEQ 0 (
    START "SItoA Dev Console" %sitoa_dev_cmd%
) else (
    wt.exe new-tab -d . --title "SItoA Dev Console" cmd /K %sitoa_dev_cmd%
)
