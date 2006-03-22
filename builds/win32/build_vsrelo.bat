::Build VSRelo
::=============

@echo off

:: See if we have a request for help on the command line
for %%v in ( %* )  do (
  ( @if /I "%%v"=="-h" (goto :HELP & goto :EOF) )
  ( @if /I "%%v"=="/h" (goto :HELP & goto :EOF) )
  ( @if /I "%%v"=="HELP" (goto :HELP & goto :EOF) )
)

@goto :MAIN
@goto :EOF

:BUILD_VSRELO
@if defined VULCAN_REBUILD (set BUILD=/rebuild) else (set BUILD=/build)
@if not defined VULCAN_BUILDCONFIG (set VULCAN_BUILDCONFIG=Release)
@if not exist %VULCAN_SCRIPT_DIR%\vsrelo.exe (
pushd %VULCAN_MBC_DIR%\VSRelo
if not exist %VULCAN_ROOT%\src\include\gen\autoconfig.h (cp %VULCAN_ROOT%\src\include\gen\autoconfig_msvc.h %VULCAN_ROOT%\src\include\gen\autoconfig.h)
devenv VSRelo.vcproj /useenv %BUILD% %VULCAN_BUILDCONFIG%  /OUT build_vsrelo.log
if errorlevel 1 (popd & call :ERROR "VSRelo %VULCAN_BUILDCONFIG% build failed" & goto :EOF)
popd
)
goto :EOF

::=============================================================================
:RUN_VSRELO
@echo Generating project files in %VULCAN_BUILD_DIR%
mkdir %VULCAN_BUILD_DIR% 2>nul
@%VULCAN_SCRIPT_DIR%\vsrelo %VULCAN_MBC_DIR% -s -o %VULCAN_BUILD_DIR% -c

:SED_MASSAGE
::Here we need to search for lines with bison and replace them with the original
::eg this:
::  CommandLine="..\..\MasterBuildConfig\jrd\bison -y -l -d -b dsql $(InputDir)$(InputName).y
::must be replaced with this:
::  CommandLine="bison -y -l -d -b dsql $(InputDir)$(InputName).y
ren %VULCAN_BUILD_DIR%\jrd\jrd.vcproj jrd.temp1
::sed s/CommandLine=\".*MasterBuildConfig\\jrd\\bison/CommandLine=\"bison/ %VULCAN_BUILD_DIR%\jrd\jrd.temp0 > %VULCAN_BUILD_DIR%\jrd\jrd.temp1
::sed s/ImportLibrary=\".*MasterBuildConfig\\jrd/ImportLibrary=\"\./p %VULCAN_BUILD_DIR%\jrd\jrd.temp1 > %VULCAN_BUILD_DIR%\jrd\jrd.vcproj
sed s/ImportLibrary=\'.*MasterBuildConfig\\jrd/ImportLibrary=\'\./p %VULCAN_BUILD_DIR%\jrd\jrd.temp1 > %VULCAN_BUILD_DIR%\jrd\jrd.vcproj

del %VULCAN_BUILD_DIR%\jrd\jrd.temp*


:COPY_SOLUTIONS
@mkdir %VULCAN_BUILD_DIR%\Vulcan 2>nul || (set ERRORLEVEL=0)
::See COPY_CONF for an example of dynamically copying templates.
for %%v in ( DevTools Vulcan ) do (
@copy %VULCAN_MBC_DIR%\Vulcan\%%v.sln.template %VULCAN_BUILD_DIR%\Vulcan\%%v.sln > nul
)

:COPY_FILES
::
:: Note - here we use cp instead of copy. This is because we
:: need to touch the datestamp. Copy preserves the datestamp.
::======================
:: copy autoconfig.h
cp %VULCAN_ROOT%\src\include\gen\autoconfig_msvc.h %VULCAN_ROOT%\src\include\gen\autoconfig.h

@echo Copy gpre generated boot files...
pushd %VULCAN_ROOT%\src
for %%a in ( burp\backup  burp\restore  dsql\array  dsql\blob  gpre\gpre_meta  ) do ( cp %%a.boot %%a.cpp )
popd

call :COPY_CONF_FILES %VULCAN_MBC_DIR%  %VULCAN_BUILD_DIR%
@goto :EOF


:COPY_CONF_FILES
:: copy the .conf files from source to dest
:: Note - this routine uses a hard coded list of known directories.
:: We MIGHT want to change this in future to use something like this:
::   pushd %VULCAN_BUILD_DIR%
::   for /r %%a in (.) do ((
::       (cd /d %%a)
::       for /f "tokens=*" %%b in ('cd') do (
:: 	    dir /b %%b\*.conf 2>nul && ( @echo copying %VULCAN_BUILD_DIR%\%%~nxb\*.conf & @echo to %VULCAN_MBC_DIR%\%%~nxb\*.conf & copy %VULCAN_BUILD_DIR%\%%~nxb\*.conf %VULCAN_MBC_DIR%\%%~nxb\*.conf )
::   )))
::   popd

setlocal
set SOURCE=%1
set DEST=%2
@Echo Copying conf files to %DEST% ...
@copy %SOURCE%\*.conf %DEST% > nul
set FILECOUNT=1
for %%a in (alice burp config Databases gateway gpre gsec gstat FbDbc isql jrd msgs qli remote SecurityDatabase server Services test ThreadTest why Workbench XLoad) do (
if not exist %DEST%\%%a (mkdir %DEST%\%%a)
@copy %SOURCE%\%%a\*.conf %DEST%\%%a > nul
set /A FILECOUNT+=1
)
echo Copied %FILECOUNT% .conf files.
endlocal
@goto :EOF


::===========
:MAPBACK
:: Resync previously relocated files back to MasterBuildConfig
::
@echo Mapping project files from %VULCAN_BUILD_DIR%
@echo to %VULCAN_MBC_DIR%

::First, relocate the vcproj files
if not exist %VULCAN_BUILD_DIR% (mkdir %VULCAN_BUILD_DIR% 2>nul)
@%VULCAN_SCRIPT_DIR%\vsrelo %VULCAN_BUILD_DIR% -s -o %VULCAN_MBC_DIR% -c

@echo Copying %VULCAN_BUILD_DIR%\Vulcan\*.sln to %VULCAN_MBC_DIR%\Vulcan\*.sln.template
@copy %VULCAN_BUILD_DIR%\Vulcan\*.sln %VULCAN_MBC_DIR%\Vulcan\*.sln.template

::and finally call a small subroutine to copy our known list of directories that hold .conf files.
call :COPY_CONF_FILES %VULCAN_BUILD_DIR% %VULCAN_MBC_DIR%

@goto :EOF



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


::==============
:HELP
@echo.
@echo    This batch file requires ONE of the following parameters.
@echo.
@echo      BUILD   Build VSRelo
@echo.
@echo      RUN     Run VSRelo
@echo.
@echo      MAPBACK Convert relocated files back to MasterBuildConfig
@echo.
@echo.
@goto :EOF


::==============
:MAIN

:: First check our environment
@if not defined VULCAN_CHECK_ENV ( @call check_env.bat "..\vulcan" )

:: See what else we have on the command line
for %%v in ( %* )  do (
  ( @if /I "%%v"=="-B" (call :BUILD_VSRELO & goto :EOF) )
  ( @if /I "%%v"=="BUILD" (call :BUILD_VSRELO & goto :EOF) )
  ( @if /I "%%v"=="-R" (call :RUN_VSRELO & goto :EOF) )
  ( @if /I "%%v"=="RUN" (call :RUN_VSRELO & goto :EOF) )
  ( @if /I "%%v"=="-M" (call :MAPBACK & goto :EOF) )
  ( @if /I "%%v"=="MAPBACK" (call :MAPBACK & goto :EOF) )
)

::If no params then do everything
::call :BUILD_VSRELO
::call :RUN_VSRELO

goto :EOF


