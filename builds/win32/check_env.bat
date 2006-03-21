::
:: Verify the integrity of the Vulcan build environment
::

@echo off

:: See if we have a request for help on the command line
for %%v in ( %* )  do (
  ( @if /I "%%v"=="-h" (goto :HELP & goto :EOF) )
  ( @if /I "%%v"=="/h" (goto :HELP & goto :EOF) )
  ( @if /I "%%v"=="HELP" (goto :HELP & goto :EOF) )
)

:: Check we have at least one parameter passed to the script.
if "%1"=="" (goto :HELP & goto :EOF)

@if "%ISC_USER%"=="" (set ISC_USER=SYSDBA)
@if "%ISC_PASSWORD%"=="" (set ISC_PASSWORD=masterkey)

::Set VULCAN and VULCAN_ROOT
:: VULCAN_ROOT is the full path to the root of our source dir. Something like:
::   c:\sandbox\vulcan
:: VULCAN is always the install dir under VULCAN_ROOT
@pushd %*
@for /f "delims=" %%a in ('@cd') do (set VULCAN_ROOT=%%a)
set VULCAN=%VULCAN_ROOT%\install
set VULCAN_SCRIPT_DIR=%VULCAN_ROOT%\builds\win32
set VULCAN_MBC_DIR=%VULCAN_ROOT%\builds\MasterBuildConfig
popd

::=============
:CHECK_COMPILER
set errorlevel=
:: Attention - version 8 (VS .Net 2005) not tested
if not defined MSVC_VERSION (
@devenv /? | find "Version 7" >nul 2>nul
@if not errorlevel 1 ((set MSVC_VERSION=7) & (set VS_VER=msvc7))

if not defined MSVC_VERSION (
@devenv /? | find "Version 8" >nul 2>nul
@if not errorlevel 1 ((set MSVC_VERSION=8) & (set VS_VER=msvc8))
)
if errorlevel 1 (call :ERROR "A working version of visual studio cannot be found on " "your current path. You need MS Visual Studio 7 or 8 to " "build Firebird from these batch files.") & (goto :EOF)
)

set VULCAN_BUILD_DIR=%VULCAN_SCRIPT_DIR%\msvc%MSVC_VERSION%
mkdir %VULCAN_BUILD_DIR% 2>nul

::==========
:CHECK_UTILS
for  %%v in ( bison cp )  do (
@echo     Checking for %%v
@%%v --version > nul 2>&1
if errorlevel 9009 ((call :ERROR "Could not find %%v on your path." "If you need a copy try looking here:" "  http://gnuwin32.sourceforge.net/packages.html") & (goto :EOF) )
)


::============
:ENV_REPORT
::If we get this far we can signal success by setting a flag for later batch files.
set VULCAN_CHECK_ENV=1
@echo.
@echo     Environment Report
@echo     ------------------
@echo.
@echo     msvc_version=Visual Studio Version %MSVC_VERSION%
@echo     VULCAN_ROOT=%VULCAN_ROOT%
@echo     VULCAN=%VULCAN%
@echo     Located bison
@echo.
goto :EOF


::===========
:ERROR
@echo.
@echo    An  unrecoverable error occurred:
@echo.
for %%a in (%*) do (
@echo	     %%~a
shift
)
@echo.
EXIT /B 999
::cancel_script > nul 2>&1

::End of ERROR
::------------
@goto :EOF


:WARNING
::======
@echo.
@echo   **** WARNING - Execution of a non-critical component failed.
@echo.
for %%a in (%*) do (
@echo	     %%~a
shift
)
@echo.
@goto :EOF


::==============
:HELP
@echo.
@echo    This batch file takes one parameter - the path to the root
@echo    of the Vulcan source tree. It can be a path relative to
@echo    the script or a full path.
@echo.
@goto :EOF


