
::=============================================================================
::Note - if required, set VULCAN_SCRIPT_DEBUG manually before running 
::       this script. Then remove the @ symbol from the lines you wish  
::       to debug.
@if not defined VULCAN_SCRIPT_DEBUG (@echo off) else (@echo on)

@goto :MAIN
@goto :FINISH
@goto :EOF

::==============================================================================

::------------------------------------------------------------------------------
:SET_DEFAULTS
:: These vars will have been defined in really_make_all.bat
:: If this script is run stand-alone then we need to initialize them here. 
@if not defined VULCAN_ISS_DEBUG (set VULCAN_ISS_DEBUG=0)
@if not defined VULCAN_EXAMPLES (set VULCAN_EXAMPLES=0)
@if not defined VULCAN_BUILDCONFIG (set VULCAN_BUILDCONFIG=Release)
@if not defined VULCAN_SHIP_PDB (set VULCAN_SHIP_PDB=no_pdb)

:: Reset "make" vars to zero
set VULCAN_ZIP_PACK=0
set VULCAN_ISX_PACK=0
set VULCAN_EMB_PACK=0

:: Set our package number at 0 and increment every
:: time we rebuild in a single session
if not defined VULCAN_PACKAGE_NUMBER (
set VULCAN_PACKAGE_NUMBER=0
) else (
set /A VULCAN_PACKAGE_NUMBER+=1
)
@echo   Setting VULCAN_PACKAGE_NUMBER to %VULCAN_PACKAGE_NUMBER%

@goto :EOF
::end of SET_DEFAULTS
::------------------------------------------------------------------------------


::------------------------------------------------------------------------------
:SET_PARAMS
@echo off

:: See what we have on the command line
for %%v in ( %* )  do (
( if /I "%%v"=="DEBUG" (set VULCAN_BUILDCONFIG=Debug) )
  ( if /I "%%v"=="PDB" (set VULCAN_SHIP_PDB=ship_pdb) )
  ( if /I "%%v"=="ZIP" (set VULCAN_ZIP_PACK=1) )
  ( if /I "%%v"=="ISX" (set VULCAN_ISX_PACK=1) )
  ( if /I "%%v"=="EMB" (set VULCAN_EMB_PACK=1) )
  ( if /I "%%v"=="ALL" ( (set VULCAN_ZIP_PACK=1) & (set VULCAN_ISX_PACK=1) & (set VULCAN_EMB_PACK=1) ) )
)

:: Now check whether we are debugging the InnoSetup script
:: (Note - if you want to debug the ISS then you should set VULCAN_ISS_DEBUG manually) 
@if %VULCAN_ISS_DEBUG% equ 0 (@set ISS_BUILD_TYPE=iss_release) else (@set ISS_BUILD_TYPE=iss_debug)
@if %VULCAN_ISS_DEBUG% equ 0 (@set ISS_COMPRESS=compression) else (@set ISS_COMPRESS=nocompression)

@if %VULCAN_EXAMPLES% equ 0 (@set ISS_EXAMPLES=noexamples) else (@set ISS_EXAMPLES=examples)
::We don't have any examples yet, so lets comment this out for now
::@if %VULCAN_ISS_DEBUG% equ 1 (
::  @if %VULCAN_EXAMPLES% equ 0 (@set ISS_EXAMPLES=noexamples)
::)

::Are we doing a snapshot build? If so we always do less work.
if %VULCAN_SNAPSHOT% equ 1 (
  (set ISS_EXAMPLES=noexamples)
  (set VULCAN_ISX_PACK=0)
  (set VULCAN_EMB_PACK=0)
)
@goto :EOF
::End of SET_PARAMS
::------------------------------------------------------------------------------


::------------------------------------------------------------------------------
:CHECK_ENV
::========
@if not defined VULCAN_ROOT ( (pushd ..\..\..\..\vulcan) & (for /f %%a  in ('cd') do set VULCAN_ROOT=%%a) & (popd) )
@if defined VULCAN (
@if /I not "%VULCAN%"=="%VULCAN_ROOT%\install" (@set VULCAN_OLD=%VULCAN%) & (set VULCAN=%VULCAN_ROOT%\install)
)
set VULCAN_SCRIPT_DIR=%VULCAN_ROOT%\builds\win32
set VULCAN_MBC_DIR=%VULCAN_ROOT%\builds\MasterBuildConfig
set VULCAN_BUILD_DIR=%VULCAN_SCRIPT_DIR%\msvc%MSVC_VERSION%%VULCAN_CLIENTDIR%
set VULCAN_INSTALL_IMAGES=%VULCAN_ROOT%\builds\install_images
if not exist %VULCAN_INSTALL_IMAGES% (mkdir %VULCAN_INSTALL_IMAGES%)

::clear errorlevel
if %errorlevel% geq 1 (cd > nul)

:: Note: - version 8 (VS .Net 2005) not tested
if not defined MSVC_VERSION (
@devenv /? | find "Version 7" >nul 2>nul
@if not errorlevel 1 ((set MSVC_VERSION=7) & (set VS_VER=msvc7))

if not defined MSVC_VERSION (
@devenv /? | find "Version 8" >nul 2>nul
@if not errorlevel 1 ((set MSVC_VERSION=8) & (set VS_VER=msvc8))
)
if errorlevel 1 (call :ERROR "A working version of visual studio cannot be found on " "your current path. You need MS Visual Studio 7 or 8 to " "build an install kit.") & (goto :EOF)
)

@if %MSVC_VERSION% EQU 7 (set VULCAN_MSVCREDISTDIR=%FrameworkSDKDir%\bin)

@if %MSVC_VERSION% EQU 8 (
call :WARNING "Building install kits has not been tested with Visual Studio 2005" "Location of redistributable runtimes is unknown."
)
 
:: Determine Product Status
set VULCAN_PROD_STATUS=
@type %VULCAN_ROOT%\src\jrd\build_no.h | findstr /I UNSTABLE  > nul && (
set VULCAN_PROD_STATUS=DEV) || type %VULCAN_ROOT%\src\jrd\build_no.h | findstr /I DEVELOPMENT > nul && (
set VULCAN_PROD_STATUS=DEV) || type %VULCAN_ROOT%\src\jrd\build_no.h | findstr /I ALPHA > nul && (
set VULCAN_PROD_STATUS=DEV) || type %VULCAN_ROOT%\src\jrd\build_no.h | findstr /I BETA > nul && (
set VULCAN_PROD_STATUS=PROD) || type %VULCAN_ROOT%\src\jrd\build_no.h | findstr /I "Release Candidate" > nul && (
set VULCAN_PROD_STATUS=PROD) || type %VULCAN_ROOT%\src\jrd\build_no.h | findstr "RC" > nul && (
set VULCAN_PROD_STATUS=PROD) || type %VULCAN_ROOT%\src\jrd\build_no.h | findstr /I "Final" > nul && (
set VULCAN_PROD_STATUS=PROD)

::What do we do if we fall through? Throw an error and fix the problem, I guess. 
if not defined VULCAN_PROD_STATUS ((call :ERROR "Unable to determine Vulcan production status." "Subsequent script execution will be cancelled.") & (goto :EOF))

@if not DEFINED FB_EXTERNAL_DOCS (
@echo      The FB_EXTERNAL_DOCS environment var is not defined 
@echo      It should point to the directory containing the relevant release notes
@echo      in adobe pdf format.
@echo.
if "VULCAN_PROD_STATUS"=="PROD" ((call :ERROR "Subsequent script execution will be cancelled.") & (goto :EOF))
)

if %VULCAN_ZIP_PACK% EQU 1 (
  if not defined SEVENZIP (
    (call :ERROR "SEVENZIP environment variable is not defined.") & (goto :EOF)
  ) else (@echo     o Compression utility found.)
)

if %VULCAN_ISX_PACK% EQU 1 (
  if NOT DEFINED INNO_SETUP_PATH (
    (call :ERROR "INNO_SETUP_PATH variable not defined.") & (goto :EOF)
  ) else (@echo     o Inno Setup found at %INNO_SETUP_PATH%.)
)





goto :EOF
::End of CHECK_ENV 
::------------------------------------------------------------------------------


:SED_MAGIC
:: Do some sed magic to make sure that the final product
:: includes the version string in the filename.
:: If the Firebird Unix tools for Win32 aren't on
:: the path this will fail! Use of the cygwin tools has not
:: been tested and may produce unexpected results.
::========================================================
find "#define PRODUCT_VER_STRING" %VULCAN_ROOT%\src\jrd\build_no.h > %temp%.\b$1.txt
sed -n -e s/\"//g -e s/"#define PRODUCT_VER_STRING "//w%temp%.\b$2.txt %temp%.\b$1.txt
for /f "tokens=*" %%a in ('type %temp%.\b$2.txt') do set PRODUCT_VER_STRING=%%a

@echo s/define VersionString "1.0.0.0"/define VersionString "%PRODUCT_VER_STRING%"/ > %temp%.\b$3.txt
@echo s/define release/define %VULCAN_BUILDCONFIG%/ >> %temp%.\b$3.txt
@echo s/define msvc_version 7/define msvc_version %MSVC_VERSION%/ >> %temp%.\b$3.txt
@echo s/define no_pdb/define %VULCAN_SHIP_PDB%/ >> %temp%.\b$3.txt
@echo s/define package_number=\"0\"/define package_number=\"%VULCAN_PACKAGE_NUMBER%\"/ >> %temp%.\b$3.txt
@echo s/define iss_debug/define %ISS_BUILD_TYPE%/ >> %temp%.\b$3.txt
@echo s/define examples/define %ISS_EXAMPLES%/ >> %temp%.\b$3.txt
@echo s/define compression/define %ISS_COMPRESS%/ >> %temp%.\b$3.txt
@echo s/PRODUCT_VER_STRING/%PRODUCT_VER_STRING%/ >> %temp%.\b$3.txt
@echo s/define BaseVer "1_0"/define BaseVer "%PRODUCT_VER_STRING:~0,1%_%PRODUCT_VER_STRING:~2,1%"/ >> %temp%.\b$3.txt

sed -f  %temp%.\b$3.txt VulcanInstall.iss > VulcanInstall_%PRODUCT_VER_STRING%-%VULCAN_PACKAGE_NUMBER%.iss
del %temp%.\b$?.txt

::End of SED_MAGIC
::----------------
@goto :EOF


::------------------------------------------------------------------------------
:COPY_XTRA
::Copy stuff around so that %VULCAN% contains a complete image of the install kit.
::===================================

::MS BATCH FILE NOTE: copy does not set the errorlevel if it fails to copy a file.
::However, the cmd processor knows it has failed, hence the || branch execution.
if defined VULCAN_SCRIPT_DEBUG (@echo on)
set VULCAN_COPY_ERR=
for %%a in ( msvcp msvcr ) do (
copy "%VULCAN_MSVCREDISTDIR%\%%a%MSVC_VERSION%*.dll" %VULCAN%\bin\ > nul  || (set VULCAN_COPY_ERR=1)
if defined VULCAN_COPY_ERR ((call :ERROR "Copying %%a%MSVC_VERSION% from %VULCAN_MSVCREDISTDIR% failed" ) & (goto :EOF))
)

if defined VULCAN_SCRIPT_DEBUG (@echo off)

@implib.exe | findstr "Borland" > nul
@if errorlevel 0 (
  @echo   Generating firebird_bor.lib and fbdbc_bor.lib
  for %%v in (firebird fbdbc) do (
  (@implib %VULCAN%\lib\%%v_bor.lib %VULCAN%\bin\%%v.dll > nul)
  )
)

::Grab necessary header files
mkdir %VULCAN%\include 2> nul
for %%a in ( firebird.h ) do (
copy %VULCAN_ROOT%\src\include\%%a %VULCAN%\include > nul || (call :WARNING Copying copy %VULCAN_ROOT%\src\include\%%a failed.) 
)


if not defined UDF_STUFF_FIXED_FOR_VULCAN (goto :NEXT_BIT)
::This section is not ready yet
@echo   Copying udf library scripts...
for %%v in ( ib_udf.sql ib_udf2.sql ) do (
copy %VULCAN_ROOT%\src\extlib\%%v  %VULCAN%\UDF\%%v > nul || ((call :ERROR Copying %VULCAN_ROOT%\src\extlib\%%v failed.) & (goto :EOF))
)

::This section is not ready yet
for %%v in ( fbudf.sql fbudf.txt ) do (
copy %VULCAN_ROOT%\src\extlib\fbudf\%%v  %VULCAN%\UDF\%%v > nul || ((call :ERROR Copying %VULCAN_ROOT%\src\extlib\%%v failed.) & (goto :EOF))
)

:NEXT_BIT


@goto :EOF
::End of COPY_XTRA
::------------------------------------------------------------------------------


::------------------------------------------------------------------------------
:COPY_DOCS
::===================================
@echo.
@rmdir /S /Q %VULCAN%\doc 2>nul
@mkdir %VULCAN%\doc
@mkdir %VULCAN%\doc\licensing
@mkdir %VULCAN%\doc\engine
@mkdir %VULCAN%\doc\sql_extensions
@copy %VULCAN_ROOT%\doc\*.* %VULCAN%\doc\ > nul
@copy %VULCAN_ROOT%\doc\licensing\*.* %VULCAN%\doc\licensing\ > nul
@copy %VULCAN_ROOT%\doc\engine\*.* %VULCAN%\doc\engine\ > nul
@copy %VULCAN_ROOT%\doc\sql_extensions\*.* %VULCAN%\doc\sql_extensions\ > nul
@copy %VULCAN_ROOT%\ChangeLog %VULCAN%\doc\ChangeLog.txt >nul
@copy %VULCAN_SCRIPT_DIR%\install\installation_readme.txt %VULCAN%\doc\installation_readme.txt > nul
@copy %VULCAN_SCRIPT_DIR%\install\readme.txt %VULCAN%\ > nul
@ren "%VULCAN%\doc\WhatsNew" WhatsNew.txt
if exist "%VULCAN%\doc\WhatsNew"~ del "%VULCAN%\doc\WhatsNew~"


::Now remove stuff that is not relevant to an install image
for %%a in ( buildgen BuildSystem VulcanRules  ) do (
del %VULCAN%\doc\%%a.* 2> nul
del %VULCAN%\doc\%%a.*~ 2> nul
)

@echo     Copying pdf docs...
@for %%v in ( Firebird_Vulcan.ReleaseNotes.pdf ) do (
  @echo     ... %%v
  (@copy /Y %FB_EXTERNAL_DOCS%\%%v %VULCAN%\doc\%%v > nul) || (call :WARNING Copying %FB_EXTERNAL_DOCS%\%%v failed.)
  @cd > nul
)

@echo   Completed copying documentation.

@goto :EOF
::End of COPY_DOCS
::------------------------------------------------------------------------------

:TOUCH_ALL
::========
::Set file timestamp to something meaningful.
::While building and testing this feature might be annoying, so we don't do it.
::==========================================================
call :WARNING "%0 disabled."
goto :EOF
setlocal
set TIMESTRING=0%PRODUCT_VER_STRING:~0,1%:0%PRODUCT_VER_STRING:~2,1%:0%PRODUCT_VER_STRING:~4,1%
@if /I "%BUILDTYPE%"=="release" (
	(@echo Touching release build files with %TIMESTRING% timestamp) & (touch -s -D -t%TIMESTRING% %VULCAN%\*.*)
	(if %VULCAN_EMB_PACK% EQU 1 (@echo Touching release build files with %TIMESTRING% timestamp) & (touch -s -D -t%TIMESTRING% %VULCAN_BUILD_DIR%\emb_pack\*.*) )
	(if %VULCAN_ZIP_PACK% EQU 1 (@echo Touching release build files with %TIMESTRING% timestamp) & (touch -s -D -t%TIMESTRING% %VULCAN_BUILD_DIR%\zip_pack\*.*) )
)
endlocal
::End of TOUCH_ALL
::----------------
@goto :EOF


::------------------------------------------------------------------------------
:ZIP_GEN
:: Generate image to be zipped
::======

call :WARNING "%0 not yet implemented"

@goto :EOF
::End of ZIP_GEN
::------------------------------------------------------------------------------

::------------------------------------------------------------------------------
:EMB_GEN
:: Generate embedded image to be zipped
::======

call :WARNING "%0 not yet implemented"

@goto :EOF
::End of EMB_GEN
::------------------------------------------------------------------------------

::------------------------------------------------------------------------------
:ZIP_PACK
:: Zip the image 
::======

call :WARNING "%0 not yet implemented"

@goto :EOF
::End of ZIP_PACK
::------------------------------------------------------------------------------

::------------------------------------------------------------------------------
:EMB_PACK
:: Zip the embedded image 
::======

call :WARNING "%0 not yet implemented"

@goto :EOF
::End of EMB_PACK
::------------------------------------------------------------------------------

::------------------------------------------------------------------------------
:ISX_PACK
:: Run Innosetup and build the binary install kits. 
::===================================================
@echo.
"%INNO_SETUP_PATH%"\iscc %VULCAN_SCRIPT_DIR%\install\VulcanInstall_%PRODUCT_VER_STRING%-%VULCAN_PACKAGE_NUMBER%.iss
@echo.


@goto :EOF
::End of ISX_PACK
::------------------------------------------------------------------------------




::------------------------------------------------------------------------------
:ERROR
::====
@echo.
@echo   An  unrecoverable error occurred:
@echo.
for %%a in (%*) do (
@echo	     %%~a
shift
)
@echo.
EXIT /B 999

@goto :EOF
::End of ERROR
::------------------------------------------------------------------------------


::------------------------------------------------------------------------------
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
if "%VULCAN_PROD_STATUS%"=="PROD" (
@echo.
@echo   Production status is Final or Release Candidate
@echo   Error must be fixed before continuing
@echo.
cancel_script > nul 2>&1
) else (
@cd > nul
)


@goto :EOF
::End of WARNING
::------------------------------------------------------------------------------

::------------------------------------------------------------------------------
:HELP
::====
@echo.
@echo    The following parameters may be passed:
@echo.
@echo        ISX    Create installable binary kit
@echo.
@echo        ZIP    Create zip kit
@echo.
@echo        ALL    Create installable binary and a zip kit
@echo.
@echo      DEBUG    Create install kits from DEBUG target
@echo.
@echo        PDB    Include PDB files to aid debugging
@echo.
@echo    Running this script without parameters will prepare
@echo    the installation tree. (Copying extra files etc.)
@echo.

@goto :EOF
::End of HELP
::------------------------------------------------------------------------------


::------------------------------------------------------------------------------
:MAIN
::====
::Check if on-line help is required
for %%v in ( %1 %2 %3 %4 %5 %6 %7 %8 %9 )  do (
  ( @if /I "%%v"=="-h" (goto :HELP & goto :EOF) )
  ( @if /I "%%v"=="/h" (goto :HELP & goto :EOF) )
  ( @if /I "%%v"=="HELP" (goto :HELP & goto :EOF) )
)

(@echo. & @echo   Setting some defaults...) & (@call :SET_DEFAULTS ) || (goto :FINISH)
(@echo. & @echo   Reading command-line parameters...) & (@call :SET_PARAMS %* ) || (goto :FINISH)
(@echo. & @echo   Checking environment...) & (@call :CHECK_ENV %* ) || (goto :FINISH)
(@echo. & @echo   Setting version number...) & (@call :SED_MAGIC ) || (goto :FINISH)
(@echo. & @echo   Copying additional files needed for installation...) & (@call :COPY_XTRA )  || (goto :FINISH)
(@echo. & @echo   Copying documentation...) & (@call :COPY_DOCS )  || (goto :FINISH)

if %VULCAN_ZIP_PACK% EQU 1 (
(@echo. & @echo   Generating zip image...) & (@call :ZIP_GEN )  || (goto :FINISH)
(@echo. & @echo   Zipping image...) & (@call :ZIP_PACK )  || (goto :FINISH)
)

if %VULCAN_EMB_PACK% EQU 1 (
(@echo. & @echo   Generating embedded zip image...) & (@call :EMB_GEN )  || (goto :FINISH)
(@echo. & @echo   Zipping embedded image...) & (@call :EMB_PACK )  || (goto :FINISH)
)

if %VULCAN_ISX_PACK% EQU 1 (
(@echo. & @echo   Compiling binary installer...) & (@call :ISX_PACK )  || (goto :FINISH)
)

(@echo. & @echo   Finished executing MAIN without errors.)

@goto :FINISH
::------------------------------------------------------------------------------

::------------------------------------------------------------------------------
:FINISH
::=====
:: If an errorlevel has been set we need to clear it.
:: SET doesn't work (at least, not on W2K) so we use a non-destructive command
:: that we know will execute seccussefully.
@cd > nul
if defined VULCAN_SCRIPT_DEBUG (@echo Before cleanup...) & (@set | find "VULCAN") & (@echo.)
:: Do some cleanup here
if defined VULCAN_OLD ((set VULCAN=%VULCAN_OLD%) & (set VULCAN_OLD=))

set VULCAN_ROOT=
set VULCAN_SCRIPT_DIR=
set VULCAN_MBC_DIR=
set VULCAN_BUILD_DIR=
set VULCAN_INSTALL_IMAGES=

set VULCAN_MSVCREDISTDIR=
set VULCAN_ZIP_PACK=
set VULCAN_ISX_PACK=
set VULCAN_EMB_PACK=


set VULCAN_ISS_DEBUG=
set VULCAN_EXAMPLES=
set VULCAN_BUILDCONFIG=
set VULCAN_SHIP_PDB=
set VULCAN_PROD_STATUS=

set ISS_BUILD_TYPE=
set ISS_COMPRESS=
@set ISS_EXAMPLES=
if defined VULCAN_SCRIPT_DEBUG (@echo After cleanup...) & (@set | find "VULCAN") & (@echo.)
@goto :EOF
::------------------------------------------------------------------------------

