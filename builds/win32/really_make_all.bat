::
:: Do everything necessary to build Vulcan
::
@echo off

:: Know our own name
@title=Building Firebird Vulcan

:: See if we have a request for help on the command line
for %%v in ( %* )  do (
  ( @if /I "%%v"=="-h" (goto :HELP & goto :EOF) )
  ( @if /I "%%v"=="/h" (goto :HELP & goto :EOF) )
  ( @if /I "%%v"=="HELP" (goto :HELP & goto :EOF) )
)

goto :MAIN
goto :EOF

::===============
:: Set defaults
:SET_DEFAULTS
set VULCAN_CLEAN=0
set VULCAN_CLEANONLY=0
set VULCAN_ENGINE=1
set VULCAN_EXAMPLES=0
set VULCAN_INSTALL_KITS=0
set VULCAN_ISS_DEBUG=0
set VULCAN_DEBUG=0
set VULCAN_SNAPSHOT=0
set VULCAN_REBUILD=0
set VULCAN_BUILDCONFIG=Release
set VULCAN_AUTO_BUILD=1
set VULCAN_PREPAREGUI=0
set VULCAN_START_TIME=0
set VULCAN_END_TIME=0
set VULCAN_BAT_DEBUG=

goto :EOF

::==========================================
:CHECK_COMMANDLINE
:: lets see what we have on the command line
for %%v in ( %* )  do (
  ( @if /I "%%v"=="-B" (@set VULCAN_BAT_DEBUG=1) )
  ( @if /I "%%v"=="BAT_DEBUG" (@set VULCAN_BAT_DEBUG=1) )
  ( @if /I "%%v"=="-C" (@set VULCAN_CLEAN=1) )
  ( @if /I "%%v"=="CLEAN" (@set VULCAN_CLEAN=1) )
  ( @if /I "%%v"=="-CO" ((@set VULCAN_CLEANONLY=1) & (@set VULCAN_CLEAN=1) ))
  ( @if /I "%%v"=="CLEANONLY" ((@set VULCAN_CLEANONLY=1) & (@set VULCAN_CLEAN=1) ))
  ( @if /I "%%v"=="-D" ( (@set VULCAN_DEBUG=1) & (set VULCAN_BUILDCONFIG=Debug) & (set DEBUG=1) ) )
  ( @if /I "%%v"=="DEBUG" ( (@set VULCAN_DEBUG=1) & (set VULCAN_BUILDCONFIG=Debug) & (set DEBUG=1) ) )
  ( @if /I "%%v"=="-E" (@set VULCAN_EXAMPLES=1) )
  ( @if /I "%%v"=="EXAMPLES" (@set VULCAN_EXAMPLES=1) )
  ( @if /I "%%v"=="-I" (@set VULCAN_INSTALL_KITS=1) )
  ( @if /I "%%v"=="INSTALL" (@set VULCAN_INSTALL_KITS=1) )
  ( @if /I "%%v"=="-ID" (@set VULCAN_ISS_DEBUG=1) )
  ( @if /I "%%v"=="ISS_DEBUG" (@set VULCAN_ISS_DEBUG=1) )
  ( @if /I "%%v"=="-N" (@set VULCAN_ENGINE=0) )
  ( @if /I "%%v"=="NOENGINE" (@set VULCAN_ENGINE=0) )
  ( @if /I "%%v"=="-P" (@set VULCAN_AUTO_BUILD=0) & (set VULCAN_PREPAREGUI=1))
  ( @if /I "%%v"=="PREPAREGUI" (@set VULCAN_AUTO_BUILD=0) & (set VULCAN_PREPAREGUI=1))
  ( @if /I "%%v"=="-R" (@set VULCAN_REBUILD=1) )
  ( @if /I "%%v"=="REBUILD" (@set VULCAN_REBUILD=1) )
  ( @if /I "%%v"=="-S" (@set VULCAN_SNAPSHOT=1) )
  ( @if /I "%%v"=="SNAPSHOT" (@set VULCAN_SNAPSHOT=1) )
)

:: Now resolve ambiguities
@if %VULCAN_PREPAREGUI% equ 1 (set VULCAN_INSTALL_KITS=0)


@echo.
@echo  The following components for Firebird Vulcan will be run:
@echo.
@if %VULCAN_CLEAN% equ 1      (@echo    Cleaning previous build) else (@echo    NOT Cleaning previous build)
@if %VULCAN_CLEANONLY% equ 1  (@echo    and then terminating. Any other parameters will be ignored.)
@if %VULCAN_ENGINE% equ 1     (
  (@if  %VULCAN_AUTO_BUILD% equ 1 ((@echo    Building engine)
  (@if %VULCAN_REBUILD% equ 1      (@echo      and rebuilding all object files))
) else (@echo    Preparing GUI) )
) else (@echo    NOT building engine)
::@if %VULCAN_EXAMPLES% equ 1   (@echo    Building examples kit) else (@echo    NOT building examples kit)
@if %VULCAN_INSTALL_KITS% equ 1       (@echo    Building install kits) else (@echo    NOT building install kits)
)
@if %VULCAN_ISS_DEBUG% equ 1  (@if %VULCAN_INSTALL_KITS% equ 1 (@echo      innosetup kit will be built in debug mode))
@if %VULCAN_DEBUG% equ 1      (@echo    Compiling with DEBUG flag) else (@echo. )
@echo.
@echo.

goto :EOF

::================================
:CHECK_ENV
@echo   Checking environment...
call check_env.bat "..\..\..\vulcan"
if ERRORLEVEL 999 (
  (call :ERROR "Running check_env.bat failed") & (goto :EOF))
)

goto :EOF

::====================================
:BUILD_VSRELO
@echo   Building VSRelo...
call build_vsrelo.bat BUILD
if ERRORLEVEL 1 (
  (call :ERROR "Building VSRelo failed") & (goto :EOF))
)
goto :EOF


::=======================================
:RUN_VSRELO
@echo   Running VSRelo...
call build_vsrelo.bat RUN
if ERRORLEVEL 999 (
  (call :ERROR "Running VSRelo failed") & (goto :EOF))
)
goto :EOF


::===========
:BUILD_VULCAN
pushd %VULCAN_SCRIPT_DIR%
@if %VULCAN_ENGINE% equ 1 ((@echo. & @echo    Opening Visual Studio...) & (@call :BUILD_ENGINE))

goto :EOF


::===========
:BUILD_ENGINE
setlocal
set PATH=%VULCAN%\bin;%PATH%
set BUILD=/build
@if %VULCAN_REBUILD% equ 1 (set BUILD=/rebuild)
if not defined VULCAN_BAT_DEBUG (
if %VULCAN_AUTO_BUILD% equ 1 (
devenv %VULCAN_BUILD_DIR%\Vulcan\Vulcan.sln /useenv %BUILD% %VULCAN_BUILDCONFIG%
) else (
devenv %VULCAN_BUILD_DIR%\Vulcan\Vulcan.sln /useenv
)
)

if errorlevel 1 (call :ERROR "Building %VULCAN_BUILDCONFIG% failed" & goto :EOF)
endlocal
@goto :EOF


:BUILD_INSTALL_KITS
::=================
@echo.
@echo Building install kits
@echo CWD is %CD%
@dir /og /p
::@call %VULCAN_ROOT\builds\win32\install\BuildInstallKits.bat
@echo.
@goto :EOF


::==========
:GET_TIME
if defined VULCAN_BAT_DEBUG (
if %VULCAN_END_TIME% neq 0 (
@echo VULCAN_END_TIME is %VULCAN_END_TIME% and VULCAN_START_TIME is %VULCAN_START_TIME%
)
)
set HOURSECS=
set MINSECS=
set SECS=
set SECSTOTAL=

for /F " tokens=5-8* delims=:. " %%i IN ('echo.^|time') DO (
set /a HOURSECS = %%i * 60 * 60
set /a MINSECS = %%j * 60
set SECS=%%k
)

set /a SECSTOTAL = %HOURSECS% + %MINSECS% + %SECS%
if NOT "%VULCAN_START_TIME%"=="0" (if "%VULCAN_END_TIME%"=="0" (set VULCAN_END_TIME=%SECSTOTAL%))
if "%VULCAN_START_TIME%"=="0" (set VULCAN_START_TIME=%SECSTOTAL%)

@goto :EOF


::=========
:PRINT_TIME
setlocal
set /a TOTALSECS = %VULCAN_END_TIME%-%VULCAN_START_TIME%
set /a HOURS = ( %TOTALSECS% / 60 / 60)
set /a HOURSECS = %HOURS% * 60 * 60
set /a MINS = ( (%TOTALSECS% - %HOURSECS%) / 60 )
set /a MINSECS = %MINS% * 60
set /a SECS = ( %TOTALSECS% - %HOURSECS% - %MINSECS% )
if defined VULCAN_BAT_DEBUG (
@echo VULCAN_END_TIME is %VULCAN_END_TIME% and  VULCAN_START_TIME is %VULCAN_START_TIME%
@echo TOTALSECS is %TOTALSECS% and SECS is %SECS%
@echo HOURS is %HOURS% and HOURSECS is %HOURSECS%
@echo MINS is %MINS% and MINSECS is %MINSECS%
)
@echo Build time was %HOURS%:%MINS%:%SECS%
endlocal
goto :EOF


::===========
:ERROR
@echo.
@echo  ERROR:
@echo.
for %%a in (%*) do (
@echo	     %%~a
shift
)
@echo.
EXIT /B 1

::End of ERROR
::------------
@goto :FINISH


::==============
:HELP
@echo.
@echo    The following parameters may be passed:
@echo.
@echo      CLEAN      Run clean_vulcan.bat
@echo.
@echo      CLEANONLY  Run clean_vulcan.bat and exit
@echo.
@echo      NOENGINE   Don't build the engine
@echo.
@echo      REBUILD    Recompile object files
@echo.
@echo      SNAPSHOT   Do a snapshot build
@echo.
@echo      PREPAREGUI Prepare build environment and open Visual Studio
@echo                 Don't start build.
@echo.
::@echo      EXAMPLES   Run make_examples.bat  (Not Yet Implemented)
::@echo.
@echo      INSTALL    Run BuildInstallKits.bat
@echo.
@echo      DEBUG      Do a debug build
@echo.
@echo      BAT_DEBUG  Echo some extra info from this build script.
@echo.
@echo      ISS_DEBUG  Create a debug version of the Innosetup script
@echo.
@echo    If no parameters are passed then a full build will occur.
@echo.
@echo    Do not pass DEBUG unless you really want to do a debug build.
@echo    It is not necessary to create a debug build in order to debug the
@echo    server.
@echo.
@echo    ISS_DEBUG is used for testing/debugging the InnoSetup script.
@echo    It will NOT create an installable binary.
@echo.
@goto :FINISH


::=============
:MAIN
call :SET_DEFAULTS
call :GET_TIME
call :CHECK_COMMANDLINE %*
call :CHECK_ENV  || (@echo Error checking environment & @goto :FINISH)
@if %VULCAN_CLEAN% equ 1   ((@echo Cleaning previous build...) & (@call clean_vulcan.bat %VULCAN_DEBUG% BUILD GEN ))
if %VULCAN_CLEANONLY% equ 1 (@goto :FINISH)
call :BUILD_VSRELO || (@echo Error building vsrelo & @goto :FINISH)
call :RUN_VSRELO
::|| (@echo errorlevel is %ERRORLEVEL% & @echo Error running vsrelo & @goto :EOF)
if %VULCAN_ENGINE% equ 1 (call :BUILD_VULCAN || (@echo Error building vulcan & @goto :FINISH))
call :GET_TIME
call :PRINT_TIME
if %VULCAN_INSTALL_KITS% equ 1 (call :BUILD_INSTALL_KITS || (@echo Error building install kits & @goto :FINISH))
goto :FINISH
goto :EOF


:FINISH
::Clear variables
@set VULCAN_CLEAN=
@set VULCAN_CLEANONLY=
@set VULCAN_ENGINE=
@set VULCAN_EXAMPLES=
@set VULCAN_INSTALL_KITS=
@set VULCAN_ISS_DEBUG=
@set VULCAN_DEBUG=
@set VULCAN_REBUILD=
@set VULCAN_START_TIME=
@set VULCAN_END_TIME=
@set VULCAN_SNAPSHOT=
@set VULCAN_BUILDCONFIG=
@set VULCAN_AUTO_BUILD=
@set VULCAN_PREPAREGUI=
@set VULCAN_BAT_DEBUG=


@popd
goto :EOF



