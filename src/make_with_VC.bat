@echo off
REM for make debug version use: 
REM >make_with_VC.bat DEBUG

setlocal
set VC9=%ProgramFiles%\Microsoft Visual Studio 9.0
set vc10=%ProgramFiles%\Microsoft Visual Studio 10.0
set Tools=Common7\Tools
set path=%vc10%\%Tools%;%VC9%\%Tools%;%path%
call vsvars32.bat

if "%1"=="DEBUG" set parameter1=DEBUG=1

cd scintilla\win32
nmake %parameter1% -f scintilla.mak
if errorlevel 1 goto :end

cd ..\..\scite\win32
nmake %parameter1% -f scite.mak
if errorlevel 1 goto :end

copy /Y ..\bin\SciTE.exe ..\..\..\pack\
copy /Y ..\bin\SciLexer.dll ..\..\..\pack\

:end
cd ..\..
