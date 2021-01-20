@echo off

rem Windows helper script to open a dev console by simply dubble-clicking the batch file.

START "SItoA Dev Console" "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 -vcvars_ver=14.0
