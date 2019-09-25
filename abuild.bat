@echo off
set py=python

REM Use python launcher if it's found in PATH.
WHERE py >nul 2>nul
IF NOT ERRORLEVEL 1 (
    set "py=py -2"
)

@echo on

@REM invokes a local install of scons (forwarding all arguments)
@%py% contrib\scons\scons --site-dir=tools\site_scons %*

@set py=
