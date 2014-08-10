@ECHO off
ECHO Copy HG to destination (without .hg subdir)
ECHO ------------------------------------------------------------------------------
ECHO.
CD /D %~dp0
SET /P dest=Enter destination [Default - C:\TEMP\scite-ru\]:
IF "%dest%"=="" SET dest=C:\TEMP\scite-ru\
ECHO \.hg\>exlist
ECHO exlist>>exlist
ECHO copy_hg.cmd>>exlist
XCOPY "%~dp0*.*" "%dest%" /S /H /K /EXCLUDE:exlist
DEL exlist
