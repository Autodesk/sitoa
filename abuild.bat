@echo off

REM Use python launcher if python isn't found in PATH.
WHERE python >nul 2>nul
IF NOT ERRORLEVEL 1 GOTO :usepython

WHERE py >nul 2>nul
IF NOT ERRORLEVEL 1 GOTO :usepy

:usepython
set py=python
GOTO :end

:usepy
set "py=py -2"
GOTO :end

:end
@echo on

@REM invokes a local install of scons (forwarding all arguments)
@%py% contrib\scons\scons --site-dir=tools\site_scons %*

@set py=
