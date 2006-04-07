:: Clean tree

:: This script cleans stuff that can't be cleaned by MSVC such as the install
:: directory. It also runs MSVC and does a clean.

@echo off

:: See if we have a request for help on the command line
for %%v in ( %* )  do (
  ( @if /I "%%v"=="-h" (goto :HELP & goto :EOF) )
  ( @if /I "%%v"=="/h" (goto :HELP & goto :EOF) )
  ( @if /I "%%v"=="HELP" (goto :HELP & goto :EOF) )
)

goto :MAIN
goto :EOF


::========================
:CHECK_COMMANDLINE
:: See what we have on the command line
if "%1"=="" (call :HELP) & (goto :EOF)
set CLEAN_BUILD=0
set CLEAN_GEN=0

for %%v in ( %* )  do (
  ( if /I "%%v"=="ALL" ((set CLEAN_BUILD=1) & (set CLEAN_GEN=1)) )
  ( if /I "%%v"=="BUILD" ((set CLEAN_BUILD=1)) )
  ( if /I "%%v"=="GEN" ((set CLEAN_GEN=1)) )
)
goto :EOF


::========================
:CLEAN_BUILD
::for %%a in ( Release Debug ) do ( devenv %VULCAN_BUILD_DIR%\Vulcan\Vulcan.sln /clean %%a )
devenv %VULCAN_BUILD_DIR%\Vulcan\Vulcan.sln /clean %VULCAN_BUILDCONFIG%
@echo Now removing *.ilk files from %VULCAN%...
del /q %VULCAN%\bin\*.ilk
for %%a in ( "firebird.msg" "security.fdb" "vulcan.lck" ) do (del /q %VULCAN%\%%~a 2>nul )

::Do we want to removed all the old binaries too?
::del /q %VULCAN%\bin\*.*
::del /q %VULCAN%\help\*.*
::del /q %VULCAN%\databases\*.*
::del /q %VULCAN%\lib\*.*

goto :EOF


::========================
:CLEAN_GEN
@echo Removing gpre generated files...
del /F %VULCAN_ROOT%\src\burp\backup.cpp
del /F %VULCAN_ROOT%\src\burp\restore.cpp

del /F %VULCAN_ROOT%\src\dsql\array.cpp
del /F %VULCAN_ROOT%\src\dsql\blob.cpp
del %VULCAN_ROOT%\src\dsql\parse.cpp
del %VULCAN_ROOT%\src\dsql\parse.h

del /F %VULCAN_ROOT%\src\gpre\gpre_meta.cpp

del %VULCAN_ROOT%\src\gsec\security.cpp

del %VULCAN_ROOT%\src\gstat\dba.cpp

del %VULCAN_ROOT%\src\isql\extract.cpp
del %VULCAN_ROOT%\src\isql\isql.cpp
del %VULCAN_ROOT%\src\isql\show.cpp

del %VULCAN_ROOT%\src\jrd\dfw.cpp
del %VULCAN_ROOT%\src\jrd\dpm.cpp
del %VULCAN_ROOT%\src\jrd\dyn.cpp
del %VULCAN_ROOT%\src\jrd\dyn_def.cpp
del %VULCAN_ROOT%\src\jrd\dyn_del.cpp
del %VULCAN_ROOT%\src\jrd\dyn_mod.cpp
del %VULCAN_ROOT%\src\jrd\dyn_util.cpp
del %VULCAN_ROOT%\src\jrd\envelope.cpp
del %VULCAN_ROOT%\src\jrd\fun.cpp
del %VULCAN_ROOT%\src\jrd\grant.cpp
del %VULCAN_ROOT%\src\jrd\ini.cpp
del %VULCAN_ROOT%\src\jrd\met.cpp
del %VULCAN_ROOT%\src\jrd\pcmet.cpp
del %VULCAN_ROOT%\src\jrd\scl.cpp
del %VULCAN_ROOT%\src\jrd\stats.cpp

del %VULCAN_ROOT%\src\msgs\build_file.cpp

del %VULCAN_ROOT%\src\qli\help.cpp
del %VULCAN_ROOT%\src\qli\meta.cpp
del %VULCAN_ROOT%\src\qli\proc.cpp
del %VULCAN_ROOT%\src\qli\show.cpp

@echo Removing autoconfig.h...
del /F %VULCAN_ROOT%\src\include\gen\autoconfig.h


:: Do we want to do this?
::@echo Removing old build logs...
::del /s /q %VULCAN_BUILD_DIR%\buildlog.htm
::del /q %VULCAN_BUILD_DIR%\build_engine*.log

goto :EOF


:HELP
@echo.
@echo    %0 will clean the config defined in %%VULCAN_BUILDCONFIG%%
@echo.
@echo    The following parameters may be passed:
@echo.
@echo      BUILD    Run MSVC to clean build environment.
@echo.
@echo        GEN    Remove all other generated files .
@echo.
@echo        ALL    Do both.
@echo.
@echo.
goto :EOF


:ERROR
@echo.
@echo    An unrecoverable error occurred:
@echo.
for %%a in (%*) do (
@echo	     %%~a
shift
)
@echo.
EXIT /B 1
goto :EOF

::=============================================================================
:MAIN
setlocal

:: First check our environment
@if not defined VULCAN_CHECK_ENV ( @call check_env.bat "..\..\..\vulcan" ) || (@echo Error checking environment & @goto :EOF)

call :CHECK_COMMANDLINE %*
if %CLEAN_GEN% equ 1 (call :CLEAN_GEN)
if %CLEAN_BUILD% equ 1 (call :CLEAN_BUILD)
endlocal

goto :EOF


