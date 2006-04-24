;  Initial Developer's Public License.
;  The contents of this file are subject to the  Initial Developer's Public
;  License Version 1.0 (the "License"). You may not use this file except
;  in compliance with the License. You may obtain a copy of the License at
;    http://www.ibphoenix.com?a=ibphoenix&page=ibp_idpl
;  Software distributed under the License is distributed on an "AS IS" basis,
;  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
;  for the specific language governing rights and limitations under the
;  License.
;
;  The Original Code is copyright 2001-2006 Paul Reeves.
;
;  The Initial Developer of the Original Code is Paul Reeves.
;
;  All Rights Reserved.
;
;   Contributor(s):
;     Tilo Muetze, Theo ? and Michael Rimov for improved detection
;     of an existing install directory.
;     Simon Carter for the WinSock2 detection.
;     Philippe Makowski for internationalization and french translation
;
;
;
;   This script has been tested against Inno Setup v5.1.6 and should
;   work with all subsequent releases of InnoSetup v5.
;   It will not work with Inno Setup 4.n.
;
;
;   Note: Entries marked ;** are provisionally disabled. It is anticipated
;   that they will be available for a subsequent release.
;




;-------Innosetup script debug flags
;A dynamically generated sed script sets the appropriate define
;See BuildExecutableInstall.bat for more details.

;This define is not used in practice, but is retained for documentation
;purposes. If set to iss_release it implies that the defines for files,
;examples and compression are set.
;#define iss_release
#define iss_debug

#define core_file_set
#define examples

;Compression is turned off when debugging as it effectively doubles compile time.
#ifdef iss_release
#define compression
#endif

;-------end of Innosetup script debug flags section

;-------Start of Innosetup script
#define msvc_version 7
#define FirebirdURL "http://www.firebirdsql.org"
#define BaseVer "1_0"
#define VersionString "1.0.0.0"
#define release
#define no_pdb
#define i18n
#define ProductNameQualifier "Vulcan"
#define sharedfile_install_flags "sharedfile uninsnosharedfileprompt ignoreversion"

;------If necessary we can turn off i18n by uncommenting this undefine
#undef  i18n

;------And make sure that examples are turned off for the time being
#undef examples

;Some strings to distinguish the name of final executable
#ifdef ship_pdb
#define pdb_str="_pdb"
#else
#define pdb_str=""
#endif
#ifdef debug
#define debug_str="_debug"
#else
#define debug_str=""
#endif

#define package_number="0"


#define LEGACY_DETECTION_CODE
[Setup]
AppName={cm:MyAppName,{#ProductNameQualifier},{#VersionString}}
;The following entry is important - all ISS install packages should
;duplicate this. See the InnoSetup help for details.
AppID=FBDBServer_{#BaseVer}
AppVerName=Firebird {#ProductNameQualifier} {#VersionString}
AppPublisher=Firebird Project
AppPublisherURL={#FirebirdURL}
AppSupportURL={#FirebirdURL}
AppUpdatesURL={#FirebirdURL}
DefaultDirName={code:ChooseInstallDir|{pf}\Firebird\Firebird{#ProductNameQualifier}_{#BaseVer}}
DefaultGroupName=Firebird{#ProductNameQualifier}_{#BaseVer}
AllowNoIcons=true
SourceDir=..\..\..\
#ifdef iss_release
LicenseFile=doc\licensing\IPLicense.txt
#endif
AlwaysShowComponentsList=true
WizardImageFile=builds\win32\install\vulcan_install_logo1.bmp
PrivilegesRequired=admin
UninstallDisplayIcon={app}\bin\fbserver.exe
OutputDir=builds\install_images
OutputBaseFilename=Firebird{#ProductNameQualifier}-{#VersionString}-{#package_number}-Win32{#debug_str}{#pdb_str}

#ifdef compression
Compression=lzma
SolidCompression=true
#else
Compression=none
SolidCompression=false
#endif

ShowTasksTreeLines=false
LanguageDetectionMethod=uilanguage

[Languages]
Name: en; MessagesFile: compiler:Default.isl; InfoBeforeFile: builds\win32\install\installation_readme.txt; InfoAfterFile: builds\win32\install\readme.txt;
#ifdef i18n
Name: ba; MessagesFile: compiler:Languages\Bosnian.isl; InfoBeforeFile: builds\install\arch-specific\win32\ba\Instalacija_ProcitajMe.txt; InfoAfterFile: builds\install\arch-specific\win32\ba\ProcitajMe.txt;
Name: fr; MessagesFile: compiler:Languages\French.isl; InfoBeforeFile: builds\install\arch-specific\win32\fr\installation_lisezmoi.txt; InfoAfterFile: builds\install\arch-specific\win32\fr\lisezmoi.txt;
Name: de; MessagesFile: compiler:Languages\German.isl; InfoBeforeFile: builds\install\arch-specific\win32\de\installation_liesmich.txt; InfoAfterFile: builds\install\arch-specific\win32\de\liesmich.txt;
Name: es; MessagesFile: compiler:Languages\Spanish.isl; InfoBeforeFile: builds\install\arch-specific\win32\es\leame_instalacion.txt; InfoAfterFile: builds\install\arch-specific\win32\es\leame.txt;
Name: hu; MessagesFile: compiler:Languages\Hungarian.isl; InfoBeforeFile: builds\install\arch-specific\win32\installation_readme.txt; InfoAfterFile: builds\install\arch-specific\win32\readme.txt;
Name: it; MessagesFile: compiler:Languages\Italian.isl; InfoBeforeFile: builds\install\arch-specific\win32\it\leggimi_installazione.txt; InfoAfterFile: builds\install\arch-specific\win32\it\leggimi.txt
Name: pl; MessagesFile: compiler:Languages\Polish.isl; InfoBeforeFile: builds\install\arch-specific\win32\pl\instalacja_czytajto.txt; InfoAfterFile: builds\install\arch-specific\win32\pl\czytajto.txt;
Name: pt; MessagesFile: compiler:Languages\PortugueseStd.isl; InfoBeforeFile: builds\install\arch-specific\win32\pt\instalacao_leia-me.txt; InfoAfterFile: builds\install\arch-specific\win32\pt\leia-me.txt
Name: si; MessagesFile: compiler:Languages\Slovenian.isl; InfoBeforeFile: builds\install\arch-specific\win32\si\instalacija_precitajMe.txt; InfoAfterFile: builds\install\arch-specific\win32\readme.txt;
#endif

[Messages]
en.BeveledLabel=English
#ifdef i18n
ba.BeveledLabel=Bosanski
fr.BeveledLabel=Français
de.BeveledLabel=Deutsch
es.BeveledLabel=Espagnol
hu.BeveledLabel=Magyar
it.BeveledLabel=Italiano
pl.BeveledLabel=Polski
pt.BeveledLabel=Português
si.BeveledLabel=Slovenski
#endif


[CustomMessages]
#include "custom_messages.inc"
#ifdef i18n
#include "ba\custom_messages_ba.inc"
#include "fr\custom_messages_fr.inc"
#include "de\custom_messages_de.inc"
#include "es\custom_messages_es.inc"
#include "hu\custom_messages_hu.inc"
#include "it\custom_messages_it.inc"
#include "pl\custom_messages_pl.inc"
#include "pt\custom_messages_pt.inc"
#include "si\custom_messages_si.inc"
#endif

#ifdef iss_debug
; By default, the languages available at runtime depend on the user's
; code page. A user with the Western European code page set will not
; even see that we support installation with the Slovenian language
; for example.
; It can be useful when debugging to force the display of all available
; languages by setting LanguageCodePage to 0. Of course, if the langauge
; is not supported by the user's current code page it be unusable.
[LangOptions]
LanguageCodePage=0
#endif

[Types]
Name: ServerInstall; Description: {cm:ServerInstall}
Name: DeveloperInstall; Description: {cm:DeveloperInstall}
Name: ClientInstall; Description: {cm:ClientInstall}
Name: CustomInstall; Description: {cm:CustomInstall}; Flags: iscustom;
[Components]
Name: ServerComponent; Description: {cm:ServerComponent}; Types: ServerInstall;
Name: DevAdminComponent; Description: {cm:DevAdminComponent}; Types: ServerInstall DeveloperInstall;
Name: ClientComponent; Description: {cm:ClientComponent}; Types: ServerInstall DeveloperInstall ClientInstall CustomInstall; Flags: fixed disablenouninstallwarning;
[Tasks]
;Server tasks
Name: UseApplicationTask; Description: {cm:UseApplicationTaskMsg}; GroupDescription: {cm:TaskGroupDescription}; Components: ServerComponent; MinVersion: 4,4; Flags: exclusive; Check: ConfigureFirebird;
Name: UseServiceTask; Description: {cm:UseServiceTask}; GroupDescription: {cm:TaskGroupDescription}; Components: ServerComponent; MinVersion: 0,4; Flags: exclusive; Check: ConfigureFirebird;
Name: AutoStartTask; Description: {cm:AutoStartTask,{#ProductNameQualifier}}; Components: ServerComponent; MinVersion: 4,4; Check: ConfigureFirebird;
;Copying of client libs to <sys>
;**Name: CopyFbClientToSysTask; Description: {cm:CopyFbClientToSysTask,,{#ProductNameQualifier}}; Components: ClientComponent; MinVersion: 4,4; Flags: Unchecked; Check: ShowCopyFbClientLibTask;
;**Name: CopyFbClientAsGds32Task; Description: {cm:CopyFbClientAsGds32Task}; Components: ClientComponent; MinVersion: 4,4; Check: ShowCopyGds32Task;

#ifdef CPLAPPLET_AVAILABLE
;Allow user to not install cpl applet
;Name: InstallCPLAppletTask; Description: {cm:InstallCPLAppletTask}; Components: SuperServerComponent; MinVersion: 4.0,4.0; Check: ShowInstallCPLAppletTask;
#endif

[Run]
;Always register Firebird Vulcan
Filename: {app}\bin\instreg.exe; Parameters: "install "; StatusMsg: {cm:instreg}; MinVersion: 4.0,4.0; Components: ClientComponent; Flags: runminimized

;**Filename: {app}\bin\instclient.exe; Parameters: "install fbclient"; StatusMsg: {cm:instclientCopyFbClient}; MinVersion: 4.0,4.0; Components: ClientComponent; Flags: runminimized; Check: CopyFBClientLib;
;**Filename: {app}\bin\instclient.exe; Parameters: "install gds32"; StatusMsg: {cm:instclientGenGds32}; MinVersion: 4.0,4.0; Components: ClientComponent; Flags: runminimized; Check: CopyGds32


;If on NT/Win2k etc and 'Install and start service' requested
Filename: {app}\bin\instsvc.exe; Parameters: "install {code:ServiceStartFlags|""""} "; StatusMsg: {cm:instsvcSetup}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized; Tasks: UseServiceTask; Check: ConfigureFirebird;
Filename: {app}\bin\instsvc.exe; Description: {cm:instsvcStartQuestion}; Parameters: start; StatusMsg: {cm:instsvcStartMsg}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized postinstall; Tasks: UseServiceTask; Check: StartEngine

;If 'start as application' requested
Filename: {code:StartApp|{app}\bin\fbserver.exe}; Description: {cm:instappStartQuestion}; Parameters: -a; StatusMsg: {cm:instappStartMsg}; MinVersion: 0,4.0; Components: ServerComponent; Flags: nowait postinstall; Tasks: UseApplicationTask; Check: StartEngine


[Registry]
;If user has chosen to start as App they may well want to start automatically. That is handled by a function below.
;Unless we set a marker here the uninstall will leave some annoying debris.
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: Firebird; ValueData: ; Flags: uninsdeletevalue; Tasks: UseApplicationTask; Check: ConfigureFirebird;

;This doesn't seem to get cleared automatically by instreg on uninstall, so lets make sure of it
Root: HKLM; Subkey: "SOFTWARE\Firebird Project"; Flags: uninsdeletekeyifempty; Components: ClientComponent DevAdminComponent ServerComponent

;Clean up Invalid registry entries from previous installs.
Root: HKLM; Subkey: "SOFTWARE\FirebirdSQL"; ValueType: none; Flags: deletekey;

;User _may_ be installing over an existing 1.5 install, and it may have been set to run as application on startup
;so we had better delete this entry unless they have chosen to autostart as application
; - except that this seems to be broken. Bah!
;Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; Valuetype: none; ValueName: 'Firebird'; ValueData: ''; flags: deletevalue; Check: IsNotAutoStartApp;

[Icons]
Name: {group}\Firebird {#ProductNameQualifier} Server; Filename: {app}\bin\fb_server.exe; Parameters: -a; Flags: runminimized; MinVersion: 4.0,4.0;  IconIndex: 0; Components: ServerComponent; Comment: Run Firebird {#ProductNameQualifier} server
Name: {group}\Firebird {#ProductNameQualifier} ISQL Tool; Filename: {app}\bin\isql.exe; WorkingDir: {app}; MinVersion: 4.0,4.0;  Comment: {cm:RunISQL}
;Always install the original english version
Name: {group}\Firebird {#ProductNameQualifier} {#VersionString} Readme; Filename: {app}\readme.txt; MinVersion: 4.0,4.0;
#ifdef i18n
;And install translated readme.txt if non-default language is chosen.
Name: {group}\{cm:IconReadme,{#ProductNameQualifier,#VersionString}}; Filename: {app}\{cm:ReadMeFile}; MinVersion: 4.0,4.0; Components: DevAdminComponent; Check: NonDefaultLanguage;
#endif
Name: {group}\Uninstall Firebird; Filename: {uninstallexe}; Comment: Uninstall Firebird

[Files]
#ifdef core_file_set
Source: install\doc\licensing\IPLicense.txt; DestDir: {app}\doc; Components: ClientComponent; Flags: sharedfile ignoreversion;
Source: install\doc\licensing\IDPLicense.txt; DestDir: {app}\doc; Components: ClientComponent; Flags: sharedfile ignoreversion
;Always install the original english version
Source: install\readme.txt; DestDir: {app}; Components: DevAdminComponent; Flags: ignoreversion;

#ifdef i18n
;Translated files
Source: builds\win32\install\ba\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: ba;
Source: builds\win32\install\fr\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: fr;
Source: builds\win32\install\de\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: de;
Source: builds\win32\install\es\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: es;
Source: builds\win32\install\hu\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: hu;
Source: builds\win32\install\it\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: it;
Source: builds\win32\install\pl\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: pl;
Source: builds\win32\install\pt\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: pt;
Source: builds\win32\install\si\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: si;
#endif ;ifdef i18n

Source: install\security.fdb;         DestDir: {app}; DestName: security.fdb.default; Components: ServerComponent; check: ConfFileExists(ExpandConstant('{app}\security.fdb'));
Source: install\security.fdb;         DestDir: {app}; DestName: security.fdb; Components: ServerComponent; Flags: uninsneveruninstall onlyifdoesntexist; check: NoConfFileExists(ExpandConstant('{app}\security.fdb'));
Source: install\firebird.msg;         DestDir: {app}; Components: ClientComponent; Flags: sharedfile uninsnosharedfileprompt ignoreversion
Source: install\firebird.log;         DestDir: {app}; Components: ServerComponent; Flags: uninsneveruninstall skipifsourcedoesntexist external dontcopy
Source: install\master.conf;          DestDir: {app}; DestName: master.conf.default; Components: ClientComponent; check: ConfFileExists(ExpandConstant('{app}\master.conf'));
Source: install\master.conf;          DestDir: {app}; DestName: master.conf; Components: ClientComponent; Flags: uninsneveruninstall; check: NoConfFileExists(ExpandConstant('{app}\master.conf'));
Source: install\server.conf;          DestDir: {app}; DestName: server.conf.default; Components: ServerComponent; check: ConfFileExists(ExpandConstant('{app}\server.conf'));
Source: install\server.conf;          DestDir: {app}; DestName: server.conf; Components: ServerComponent; Flags: uninsneveruninstall; check: NoConfFileExists(ExpandConstant('{app}\server.conf'));
Source: install\databases.conf;       DestDir: {app}; DestName: databases.conf.default; Components: ClientComponent; check: ConfFileExists(ExpandConstant('{app}\databases.conf'));
Source: install\databases.conf;       DestDir: {app}; DestName: databases.conf; Components: ClientComponent; Flags: uninsneveruninstall; check: NoConfFileExists(ExpandConstant('{app}\databases.conf'));
Source: install\client.conf;          DestDir: {app}; DestName: client.conf.default; Components: ClientComponent; check: ConfFileExists(ExpandConstant('{app}\client.conf'));
Source: install\client.conf;          DestDir: {app}; DestName: client.conf; Components: ClientComponent; Flags: uninsneveruninstall; check: NoConfFileExists(ExpandConstant('{app}\client.conf'));

Source: install\bin\engine11.dll;     DestDir: {app}\bin; Components: ServerComponent;    Flags: {#sharedfile_install_flags};
Source: install\bin\fbconfig.exe;     DestDir: {app}\bin; Components: DevAdminComponent;  Flags: {#sharedfile_install_flags};
Source: install\bin\fbdbc.dll;        DestDir: {app}\bin; Components: ClientComponent;    Flags: {#sharedfile_install_flags};
Source: install\bin\fbserver.exe;     DestDir: {app}\bin; Components: ServerComponent;    Flags: {#sharedfile_install_flags};
Source: install\bin\firebird.dll;     DestDir: {app}\bin; Components: ClientComponent;    Flags: {#sharedfile_install_flags};
Source: install\bin\gateway.dll;      DestDir: {app}\bin; Components: ServerComponent;    Flags: {#sharedfile_install_flags};
Source: install\bin\gbak.exe;         DestDir: {app}\bin; Components: DevAdminComponent;  Flags: {#sharedfile_install_flags};
Source: install\bin\gfix.exe;         DestDir: {app}\bin; Components: DevAdminComponent;  Flags: {#sharedfile_install_flags};
Source: install\bin\gpre.exe;         DestDir: {app}\bin; Components: DevAdminComponent;  Flags: {#sharedfile_install_flags};
Source: install\bin\gsec.exe;         DestDir: {app}\bin; Components: DevAdminComponent;  Flags: {#sharedfile_install_flags};
Source: install\bin\gstat.exe;        DestDir: {app}\bin; Components: DevAdminComponent;  Flags: {#sharedfile_install_flags};
Source: install\bin\ib_util.dll;      DestDir: {app}\bin; Components: ServerComponent;    Flags: {#sharedfile_install_flags};
Source: install\bin\instreg.exe;      DestDir: {app}\bin; Components: ServerComponent;    Flags: {#sharedfile_install_flags};
Source: install\bin\instsvc.exe;      DestDir: {app}\bin; Components: ServerComponent;    Flags: {#sharedfile_install_flags};
Source: install\bin\isql.exe;         DestDir: {app}\bin; Components: DevAdminComponent;  Flags: {#sharedfile_install_flags};
Source: install\bin\fb_lock_print.exe;DestDir: {app}\bin; Components: ServerComponent;    Flags: {#sharedfile_install_flags};
Source: install\bin\qli.exe;          DestDir: {app}\bin; Components: DevAdminComponent;  Flags: {#sharedfile_install_flags};
Source: install\bin\remote.dll;       DestDir: {app}\bin; Components: ServerComponent;    Flags: {#sharedfile_install_flags};
Source: install\bin\services.dll;     DestDir: {app}\bin; Components: ServerComponent;    Flags: {#sharedfile_install_flags};

; NOTE: These dll's MUST never be sourced from the local system32 directory.
; Deploy libraries from %FrameworkSDKDir% if compiling with Visual Studio 7.
; The BuildExecutableInstall.bat will attempt to locate them and place them in install\bin\
Source: install\bin\msvcr{#msvc_version}?.dll; DestDir: {app}\bin; Components: ClientComponent;
Source: install\bin\msvcr{#msvc_version}?.dll; DestDir: {sys}; Components: ClientComponent; Flags: sharedfile uninsneveruninstall;
Source: install\bin\msvcp{#msvc_version}?.dll; DestDir: {app}\bin; Components: ClientComponent;
Source: install\bin\msvcp{#msvc_version}?.dll; DestDir: {sys}; Components: ClientComponent; Flags: sharedfile uninsneveruninstall;

;Docs
Source: doc\building_and_installation\win32_installation_scripted.txt; DestDir: {app}\doc; DestName: installation_scripted.txt; Components: DevAdminComponent; Flags: skipifsourcedoesntexist  ignoreversion
Source: doc\engine\*.*; DestDir: {app}\doc\engine; Components: DevAdminComponent; Flags: skipifsourcedoesntexist  ignoreversion
Source: doc\sql_extensions\*.*; DestDir: {app}\doc\sql_extensions; Components: DevAdminComponent; Flags: skipifsourcedoesntexist ignoreversion

;Other stuff
Source: install\help\*.*; DestDir: {app}\help; Components: DevAdminComponent;
Source: install\include\*.*; DestDir: {app}\include; Components: DevAdminComponent;
Source: install\intl\fbintl.dll; DestDir: {app}\intl; Components: ServerComponent; Flags: {#sharedfile_install_flags};
Source: install\lib\*.lib; DestDir: {app}\lib; Components: DevAdminComponent; Flags: ignoreversion;
Source: install\UDF\ib_udf.dll;       DestDir: {app}\UDF; Components: ServerComponent;    Flags: {#sharedfile_install_flags};
;**Source: output\UDF\fbudf.dll; DestDir: {app}\UDF; Components: ServerComponent; Flags: sharedfile ignoreversion;
;**Source: install\UDF\*.sql; DestDir: {app}\UDF; Components: ServerComponent; Flags: ignoreversion;
;**Source: install\misc\*.*; DestDir: {app}\misc; Components: ServerComponent; Flags: ignoreversion;
;**Source: install\misc\upgrade\security\*.*; DestDir: {app}\misc\upgrade\security; Components: ServerComponent; Flags: ignoreversion;
;**Source: install\misc\upgrade\ib_udf\*.*; DestDir: {app}\misc\upgrade\ib_udf; Components: ServerComponent; Flags: ignoreversion;

#ifdef CPLAPPLET_AVAILABLE
;Note - Win9x requires 8.3 filenames for the uninsrestartdelete option to work
Source: output\system32\Firebird2Control.cpl; DestDir: {sys}; Components: SuperServerComponent; MinVersion: 0,4.0; Flags: sharedfile ignoreversion promptifolder restartreplace uninsrestartdelete; Check: InstallCPLApplet
Source: output\system32\Firebird2Control.cpl; DestDir: {sys}; Destname: FIREBI~1.CPL; Components: SuperServerComponent; MinVersion: 4.0,0; Flags: sharedfile ignoreversion promptifolder restartreplace uninsrestartdelete; Check: InstallCPLApplet
#endif

#endif ;ifdef core_file_set

#ifdef examples
Source: install\examples\*.*; DestDir: {app}\examples; Components: DevAdminComponent;  Flags: ignoreversion;
Source: install\examples\api\*.*; DestDir: {app}\examples\api; Components: DevAdminComponent;  Flags: ignoreversion;
Source: install\examples\build_win32\*.*; DestDir: {app}\examples\build_win32; Components: DevAdminComponent;  Flags: ignoreversion;
Source: install\examples\dyn\*.*; DestDir: {app}\examples\dyn; Components: DevAdminComponent;  Flags: ignoreversion;
Source: install\examples\empbuild\*.*; DestDir: {app}\examples\empbuild; Components: DevAdminComponent;  Flags: ignoreversion;
Source: install\examples\include\*.*; DestDir: {app}\examples\include; Components: DevAdminComponent;  Flags: ignoreversion;
Source: install\examples\stat\*.*; DestDir: {app}\examples\stat; Components: DevAdminComponent;  Flags: ignoreversion;
Source: install\examples\udf\*.*; DestDir: {app}\examples\udf; Components: DevAdminComponent;  Flags: ignoreversion;
#endif

#ifdef ship_pdb
Source: install\bin\fbdbc.pdb; DestDir: {app}\bin; Components: ClientComponent;
Source: install\bin\fbserver.pdb; DestDir: {app}\bin; Components: ServerComponent;
Source: install\bin\firebird.pdb; DestDir: {app}\bin; Components: ClientComponent;
Source: install\bin\gateway.pdb; DestDir: {app}\bin; Components: ClientComponent;
Source: install\bin\remote.pdb; DestDir: {app}\bin; Components: ClientComponent;
Source: install\bin\services.pdb; DestDir: {app}\bin; Components: ServerComponent;
#endif

[UninstallRun]
Filename: {app}\bin\instsvc.exe; Parameters: " stop"; StatusMsg: {cm:instsvcStopMsg}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized; Tasks: UseServiceTask; RunOnceId: StopService
Filename: {app}\bin\instsvc.exe; Parameters: " remove"; StatusMsg: {cm:instsvcRemove}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized; Tasks: UseServiceTask; RunOnceId: RemoveService
;**Filename: {app}\bin\instclient.exe; Parameters: " remove gds32"; StatusMsg: {cm:instclientDecLibCountGds32}; MinVersion: 4.0,4.0; Flags: runminimized;
;**Filename: {app}\bin\instclient.exe; Parameters: " remove fbclient"; StatusMsg: {cm:instclientDecLibCountFbClient}; MinVersion: 4.0,4.0; Flags: runminimized;
Filename: {app}\bin\instreg.exe; Parameters: " remove"; StatusMsg: {cm:instreg}; MinVersion: 4.0,4.0; Flags: runminimized; RunOnceId: RemoveRegistryEntry

[UninstallDelete]
Type: files; Name: {app}\*.lck
Type: files; Name: {app}\*.evn


[_ISTool]
EnableISX=true

[Code]
program Setup;

// Some global variables are also in FirebirdInstallEnvironmentChecks.inc
// This is not ideal, but then this scripting environment is not ideal, either.
// The basic point of the include files is to isolate chunks of code that are
// a) Form a module or have common functionality
// b) Debugged.
// This hopefully keeps the main script simpler to follow.

Var
  InstallRootDir: String;
  ForceInstall: Boolean;        // If /force set on command-line we install _and_
                                 // configure. Default is to install and configure only if
                                 // no other working installation is found (unless we are installing
                                 // over the same version)

  NoCPL: Boolean;               // pass /nocpl on command-line.
  NoLegacyClient: Boolean;      // pass /nogds32 on command line.
  CopyFbClient: Boolean;     // pass /copyfbclient on command line.

#include "FirebirdInstallSupportFunctions.inc"

//=============== Start of FirebirdInstallEnvironmentChecks.inc ======================

#ifdef LEGACY_DETECTION_CODE
//Registry keys for Firebird and InterBase
Const
  //All InterBase and Firebird 1.0.n except IB5.n
  IBRegKey            = 'SOFTWARE\Borland\InterBase\CurrentVersion';
  //IB5.n
  IB5RegKey           = 'SOFTWARE\InterBase Corp\InterBase\CurrentVersion';
  //Fb15 RC
  FB15RCKey           = 'SOFTWARE\FirebirdSQL\Firebird\CurrentVersion';
  FB15RCKeyRoot       = 'SOFTWARE\FirebirdSQL';

  //All IB, Fb 1.0  and Fb 1.5RC's use RootDirectory entry
  LegacyRegPathEntry  = 'RootDirectory';

  //Firebird 1.5 and beyond
  FB2RegKey       = 'SOFTWARE\Firebird Project\Firebird Server\Instances';

  FBRegPathEntry  = 'DefaultInstance';    //Stores path to root

  IB4MessageFile  = 'interbas.msg';
  IBMessageFile   = 'interbase.msg';      //IB5, IB6, IB7 and Fb 1.0
  FBMessageFile   = 'firebird.msg';       //Fb2 codebase

  IBDesc = 'InterBase %s ';
  FBDesc = 'Firebird %s ';



Const
  //Install Types
  NotInstalled          = 0;
  ClientInstall         = 1;
  AdminInstall          = 2;
  SuperServerInstall    = 4;
  ClassicServerInstall  = 8;
  BrokenInstall         = 32;   //version or component mismatch found, so mark broken

  //Possible product installs
  IB4Install    = 0;
  IB5Install    = 1;
  IB6Install    = 2;
  IB65Install   = 3;
  IB7Install    = 4;
  FB1Install    = 5;
  FB15RCInstall = 6;
  FB15Install   = 7;
  FB2Install    = 8;            //All Fb 1.6 and beyond
  MaxProdInstalled = FB2Install;

  //ProductsInstalled
  IB4     = $0001;
  IB5     = $0002;
  IB6     = $0004;
  IB65    = $0008;
  IB7     = $0010;
  FB1     = $0020;
  FB15RC  = $0040;
  FB15    = $0080;
  FB2     = $0100;

  //  Likely gds32 version strings for installed versions of Firebird or InterBase are:
  //  [6,0,n,n]     InterBase 6.0
  //  [6,2,0,nnn]   Firebird 1.0.0
  //  [6,2,2,nnn]   Firebird 1.0.2
  //  [6,2,3,nnn]   Firebird 1.0.3
  //  [6,5,n,n]     InterBase 6.5
  //  [6,3,0,nnnn]  Firebird 1.5.0
  //  [6,3,0,nnnn]  Firebird 2.0.0.10516
  //  [7,n,n,n]     InterBase 7


Const
  Install   = 1;
  Configure = 2;

Var
  ProductsInstalled: Integer;
  ProductsInstalledCount: Integer;
  InstallAndConfigure: Integer;

Type
  TProduct = record
    ProductID:      Integer;
    Description:    String;
    RegKey:         String;
    RegEntry:       String;
    RegVersion:     String;
    MessageFile:    String;
    Path:           String;
    ClientVersion:  String;
    GBAKVersion:    String;
    ServerVersion:  String;
    InstallType:    Integer;
    ActualVersion:  String;
    FirebirdVersion:String;
  end;


Var
  ProductsInstalledArray: Array of TProduct;

procedure InitExistingInstallRecords;
var
  product: Integer;
begin
  SetArrayLength(ProductsInstalledArray,MaxProdInstalled + 1);
  for product := 0 to MaxProdInstalled do begin

     ProductsInstalledArray[product].ProductID := product;

    case product of

      IB4Install: begin
        ProductsInstalledArray[product].Description := IBDesc;
        ProductsInstalledArray[product].RegKey := IBRegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IB4MessageFile;
      end;

      IB5Install: begin
        ProductsInstalledArray[product].Description := IBDesc;
        ProductsInstalledArray[product].RegKey := IB5RegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IBMessageFile;
      end;

      IB6Install: begin
        ProductsInstalledArray[product].Description := IBDesc;
        ProductsInstalledArray[product].RegKey := IBRegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IBMessageFile;
      end;

      IB65Install: begin
        ProductsInstalledArray[product].Description := IBDesc;
        ProductsInstalledArray[product].RegKey := IBRegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IBMessageFile;
      end;

      IB7Install: begin
        ProductsInstalledArray[product].Description := IBDesc;
        ProductsInstalledArray[product].RegKey := IBRegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IBMessageFile;
      end;

      FB1Install: begin
        ProductsInstalledArray[product].Description := FBDesc;
        ProductsInstalledArray[product].RegKey := IBRegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IBMessageFile;
      end;

      FB15RCInstall: begin
        ProductsInstalledArray[product].Description := FBDesc;
        ProductsInstalledArray[product].RegKey := FB15RCKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := FBMessageFile;
      end;

      FB15Install: begin
        ProductsInstalledArray[product].Description := FBDesc;
        ProductsInstalledArray[product].RegKey := FB2RegKey;
        ProductsInstalledArray[product].RegEntry := FBRegPathEntry;
        ProductsInstalledArray[product].MessageFile := FBMessageFile;
      end;

      FB2Install: begin
        ProductsInstalledArray[product].Description := FBDesc;
        ProductsInstalledArray[product].RegKey := FB2RegKey;
        ProductsInstalledArray[product].RegEntry := FBRegPathEntry;
        ProductsInstalledArray[product].MessageFile := FBMessageFile;
      end;

    end; //case

    ProductsInstalledArray[product].Path := GetRegistryEntry(
        ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);

    ProductsInstalledArray[product].RegVersion := GetRegistryEntry(
        ProductsInstalledArray[product].RegKey, 'Version');

  end;  //for
end; //function


Procedure AnalyzeEnvironment;
var
  product: Integer;
  gds32VersionString: String;
  VerInt: Array of Integer;
  BoolOne, BoolTwo, BoolEval: Boolean;
  EvalOne, EvalTwo: Integer;

  dbg_ProductPath, dbg_ClientVersion, dbg_GBAKVersion, dbg_Server: String;
  dbg_InstallType : Integer;
  eval_bool: boolean;


begin

  ProductsInstalled := 0;
  ProductsInstalledCount := 0;

  //Test for gds32 version in <sys>
  if FileExists(GetSysPath+'\gds32.dll') then begin
    gds32VersionString := GetInstalledVersion(GetSysPath+'\gds32.dll',VerInt);
  end;

  for product := 0 to MaxProdInstalled do begin

    // Check if working client already installed.
    ///////////////////////
      dbg_ProductPath := ProductsInstalledArray[product].Path;
      dbg_ClientVersion := ProductsInstalledArray[product].ClientVersion
      dbg_GBAKVersion := ProductsInstalledArray[product].GBAKVersion;
      dbg_Server := ProductsInstalledArray[product].ServerVersion;
      dbg_InstallType := ProductsInstalledArray[product].InstallType;

    if FileExists(ProductsInstalledArray[product].Path + '\bin\fbclient.dll') then
      ProductsInstalledArray[product].ClientVersion := GetInstalledVersion(
        ProductsInstalledArray[product].Path + '\bin\fbclient.dll',VerInt)
      else
        ProductsInstalledArray[product].ClientVersion := gds32VersionString;

    If (ProductsInstalledArray[product].Path<>'') AND (ProductsInstalledArray[product].ClientVersion <> '') AND
      (FileExists(ProductsInstalledArray[product].Path+'\'+ProductsInstalledArray[product].MessageFile)) then
        ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + ClientInstall
    else
      //The minimum requirements for a working client don't exist, so ignore this product.
      Continue;

    // Client version found, so see what else is there
    ///////////
    if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin

      GetVersionNumbersString( ProductsInstalledArray[product].Path+'\bin\gbak.exe',
          ProductsInstalledArray[product].GBAKVersion);
      If ProductsInstalledArray[product].GBAKVersion <> '' then begin
        ProductsInstalledArray[product].ActualVersion := ProductsInstalledArray[product].GBAKVersion;
        ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + AdminInstall;
      end;

      if FileExists(ProductsInstalledArray[product].Path+'\bin\fb_inet_server.exe') then begin
        GetVersionNumbersString( ProductsInstalledArray[product].Path+'\bin\fb_inet_server.exe',
          ProductsInstalledArray[product].ServerVersion);
        If ProductsInstalledArray[product].ServerVersion <> '' then begin
          ProductsInstalledArray[product].ActualVersion := ProductsInstalledArray[product].ServerVersion;
          ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + ClassicServerInstall;
        end;
      end;

      if FileExists(ProductsInstalledArray[product].Path+'\bin\fbserver.exe') then begin
        GetVersionNumbersString( ProductsInstalledArray[product].Path+'\bin\fbserver.exe',
          ProductsInstalledArray[product].ServerVersion);
        If ProductsInstalledArray[product].ServerVersion <> '' then begin
          ProductsInstalledArray[product].ActualVersion := ProductsInstalledArray[product].ServerVersion;
          ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + SuperServerInstall;
        end;
      end;

      if FileExists(ProductsInstalledArray[product].Path+'\bin\ibserver.exe') then begin
        GetVersionNumbersString( ProductsInstalledArray[product].Path+'\bin\ibserver.exe',
          ProductsInstalledArray[product].ServerVersion);
        If ProductsInstalledArray[product].ServerVersion <> '' then begin
          ProductsInstalledArray[product].ActualVersion := ProductsInstalledArray[product].ServerVersion;
          ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + SuperServerInstall;
        end;
      end;

      if (ProductsInstalledArray[product].InstallType <> NotInstalled) then begin
        //Check that we haven't already flagged the install as broken.
        // AND ((ProductsInstalledArray[product].InstallType AND BrokenInstall)<>BrokenInstall))
          //Now test that the version strings match!
          if (CompareStr(ProductsInstalledArray[product].ClientVersion, ProductsInstalledArray[product].GBAKVersion)<> 0) then
            ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + BrokenInstall
          else
            if (CompareStr(ProductsInstalledArray[product].ClientVersion, ProductsInstalledArray[product].ServerVersion )<> 0) then
              ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + BrokenInstall;
      end;


      //Now, resolve version numbers.
      ///////////////////////////////

      case product of
        IB4Install: begin
          //check to see if the client library matches the server version installed.
          if CompareVersion(ProductsInstalledArray[product].ActualVersion, '4.0.0.0',1) <> 0 then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall)= ClientInstall) then begin
              //Although, if we get here, we must have an install, because the message file is unique to 4.n
              ProductsInstalled := ProductsInstalled + IB4;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end;
        end;

        IB5Install: begin
          //check to see if the client library matches the server version installed.
          if CompareVersion(ProductsInstalledArray[product].ActualVersion, '5.0.0.0',1) <> 0 then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            //Again, if we get here we must have an install, because the registry key is unique to 5.n
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall)= ClientInstall) then begin
              ProductsInstalled := ProductsInstalled + IB5;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end
        end;

        IB6Install: begin
          //If we get here we have ambiguity with other versions of InterBase and Firebird
          if ( pos('InterBase',ProductsInstalledArray[product].RegVersion) > 0 ) then begin
            if CompareVersion(ProductsInstalledArray[product].ClientVersion, '6.0.0.0',2) <> 0 then
              ProductsInstalledArray[product].InstallType := NotInstalled
            else
              if ((ProductsInstalledArray[product].InstallType AND ClientInstall)= ClientInstall)  then begin
                ProductsInstalled := ProductsInstalled + IB6;
                ProductsInstalledCount := ProductsInstalledCount + 1;
              end;
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

        IB65Install: begin
          //If we get here we have ambiguity with other versions of InterBase and Firebird
          if ( pos('InterBase',ProductsInstalledArray[product].RegVersion) > 0 ) then begin
            if CompareVersion(ProductsInstalledArray[product].ClientVersion, '6.5.0.0',2) <> 0 then
              ProductsInstalledArray[product].InstallType := NotInstalled
            else
              if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin
                ProductsInstalled := ProductsInstalled + IB65;
                ProductsInstalledCount := ProductsInstalledCount + 1;
                end
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

        IB7Install: begin
          //If we get here we have ambiguity with other versions of InterBase and Firebird
          if ( pos('InterBase',ProductsInstalledArray[product].RegVersion) > 0 ) then begin
            if CompareVersion(ProductsInstalledArray[product].ClientVersion, '7.0.0.0',1) <> 0 then
              ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin
              ProductsInstalled := ProductsInstalled + IB7;
              ProductsInstalledCount := ProductsInstalledCount + 1;
              end;
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

        FB1Install: begin
          if ( pos('Firebird',ProductsInstalledArray[product].RegVersion) > 0 ) then begin
            if CompareVersion(ProductsInstalledArray[product].ClientVersion, '6.2.0.0',2) <> 0 then
              ProductsInstalledArray[product].InstallType := NotInstalled
            else
              if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin
                ProductsInstalled := ProductsInstalled + FB1;
                ProductsInstalledCount := ProductsInstalledCount + 1;
                ProductsInstalledArray[product].ActualVersion := ConvertIBVerStrToFbVerStr(ProductsInstalledArray[product].ActualVersion);
                end;
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

        FB15RCInstall: begin
          if CompareVersion(ProductsInstalledArray[product].ClientVersion, '1.5.0.0',2) <> 0 then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin
              ProductsInstalled := ProductsInstalled + FB15RC;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end;
        end;

        FB15Install: begin
          if CompareVersion(ProductsInstalledArray[product].ClientVersion, '1.5.0.0',2) <> 0 then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin
              ProductsInstalled := ProductsInstalled + FB15;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end;
        end;

        FB2Install: begin
          if (CompareVersion(ProductsInstalledArray[product].ClientVersion, '2.0.0.0',2) <> 0) then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall)  then begin
              ProductsInstalled := ProductsInstalled + FB2;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end;
        end;

      end;//case


    end; //if ((ProductsInstalledArray[product].InstallType AND ClientInstall)= ClientInstall) then begin
  end; //for
end;

function ConfigureFirebird: boolean;
begin
  result := (InstallAndConfigure AND Configure) = Configure;
end;

/////=======================================
//end of LEGACY_DETECION_CODE
#endif

Var
  InterBaseVer: Array of Integer;
  FirebirdVer: Array of Integer;


Type
  TSharedFileArrayRecord = record
    Filename: String;
    Count: Integer;
  end;

var
  SharedFileArray: Array of TSharedFileArrayRecord;



procedure SetupSharedFilesArray;
//All shared files go in this list. Use
// find /n "sharedfile" FirebirdInstall_15.iss
//to list them in the order they appear in the setup script
begin
SetArrayLength(SharedFileArray,24);

SharedFileArray[0].Filename := ExpandConstant('{app}')+'\IPLicense.txt';
SharedFileArray[1].Filename := ExpandConstant('{app}')+'\IDPLicense.txt';
SharedFileArray[2].Filename := ExpandConstant('{app}')+'\firebird.msg';
SharedFileArray[3].Filename := ExpandConstant('{app}')+'\bin\gbak.exe';
SharedFileArray[4].Filename := ExpandConstant('{app}')+'\bin\gfix.exe';
SharedFileArray[5].Filename := ExpandConstant('{app}')+'\bin\gsec.exe';
SharedFileArray[6].Filename := ExpandConstant('{app}')+'\bin\gsplit.exe';
SharedFileArray[7].Filename := ExpandConstant('{app}')+'\bin\gstat.exe';
SharedFileArray[8].Filename := ExpandConstant('{app}')+'\bin\fbguard.exe';
SharedFileArray[9].Filename := ExpandConstant('{app}')+'\bin\fb_lock_print.exe';
SharedFileArray[10].Filename := ExpandConstant('{app}')+'\bin\fbserver.exe';
SharedFileArray[11].Filename := ExpandConstant('{app}')+'\bin\ib_util.dll';
SharedFileArray[12].Filename := ExpandConstant('{app}')+'\bin\instclient.exe';
SharedFileArray[13].Filename := ExpandConstant('{app}')+'\bin\instreg.exe';
SharedFileArray[14].Filename := ExpandConstant('{app}')+'\bin\instsvc.exe';

SharedFileArray[17].Filename := ExpandConstant('{app}')+'\bin\fbclient.dll';
SharedFileArray[18].Filename := ExpandConstant('{app}')+'\bin\msvcrt.dll';
SharedFileArray[19].Filename := ExpandConstant('{app}')+'\bin\msvcp{#msvc_version}0.dll';

SharedFileArray[20].Filename := ExpandConstant('{app}')+'\bin\fbintl.dll';

SharedFileArray[21].Filename := ExpandConstant('{app}')+'\UDF\ib_udf.dll';
SharedFileArray[22].Filename := ExpandConstant('{app}')+'\UDF\fbudf.dll';


if UsingWinNT then
  SharedFileArray[23].Filename := ExpandConstant('{sys}')+'\Firebird2Control.cpl'
else
  SharedFileArray[23].Filename := ExpandConstant('{sys}')+'\FIREBI~1.CPL';



end;



procedure GetSharedLibCountBeforeCopy;
var
  dw: Cardinal;
  i:  Integer;
begin
  for i:= 0 to GetArrayLength(SharedFileArray)-1 do begin
    if RegQueryDWordValue(HKEY_LOCAL_MACHINE,
          'SOFTWARE\Microsoft\Windows\CurrentVersion\SharedDLLs',SharedFileArray[i].filename, dw) then
      SharedFileArray[i].Count := dw
    else
      SharedFileArray[i].Count := 0;
  end;
end;


procedure CheckSharedLibCountAtEnd;
// If a shared file exists on disk (from a manual install perhaps?) then
// the Installer will set the SharedFile count to 2 even if no registry
// entry exists. Is it a bug, an anomaly or a WAD?
// Is it InnoSetup or the O/S?
// Anyway, let's work around it, otherwise the files will appear 'sticky'
// after an uninstall.

var
  dw: cardinal;
  i: Integer;

begin
  for i:= 0 to GetArrayLength(SharedFileArray)-1 do begin
    if RegQueryDWordValue(HKEY_LOCAL_MACHINE,
      'SOFTWARE\Microsoft\Windows\CurrentVersion\SharedDLLs',SharedFileArray[i].Filename, dw) then begin
        if (( dw - SharedFileArray[i].Count ) > 1 ) then begin
          dw := SharedFileArray[i].Count + 1 ;
          RegWriteDWordValue(HKEY_LOCAL_MACHINE,
              'SOFTWARE\Microsoft\Windows\CurrentVersion\SharedDLLs',SharedFileArray[i].Filename, dw);
      end;
    end;
  end;
end;


///===================================================

function GetFirebirdDir: string;
//Check if Firebird installed, get version info to global var and return root dir
var
	FirebirdDir: String;
begin
  FirebirdDir := '';
	FirebirdVer := [0,0,0,0];
  RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SOFTWARE\Firebird Project\Firebird Server\Instances','DefaultInstance', FirebirdDir);
  //If nothing returned then check for the registry entry used during beta/RC phase
  if (FirebirdDir='') then
    RegQueryStringValue(HKEY_LOCAL_MACHINE,
      'SOFTWARE\FirebirdSQL\Firebird\CurrentVersion','RootDirectory', FirebirdDir);
  if (FirebirdDir<>'') then
    GetInstalledVersion(FirebirdDir+'\bin\gbak.exe', FirebirdVer);

  Result := FirebirdDir;
end;



function GetInterBaseDir: string;
//Check if InterBase installed, get version info to global var and return root dir
var
  InterBaseDir: String;
begin
  InterBaseDir := '';
	InterBaseVer   := [0,0,0,0];
  RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SOFTWARE\Borland\InterBase\CurrentVersion','RootDirectory', InterBaseDir);
  if ( InterBaseDir <> '' ) then
    GetInstalledVersion(InterBaseDir+'\bin\gbak.exe',InterBaseVer);

  Result := InterBaseDir;
end;


function FirebirdOneRunning: boolean;
var
  i: Integer;
begin
  result := false;

  //Look for a running copy of InterBase or Firebird 1.0.
  i:=0;
  i:=FindWindowByClassName('IB_Server') ;
  if ( i<>0 ) then
    result := true;
end;

//Detect any 1.5 or 2.0 server running with default ClassName or mutex
function FirebirdDefaultServerRunning: boolean;
var
  Handle: Integer;
  mutex_found: boolean;
begin
  result := False;
  //Look for a running version of Firebird 1.5 or later
  Handle := FindWindowByClassName('FB_Disabled');
  if ( Handle = 0 ) then
    Handle := FindWindowByClassName('FB_Server');
  if ( Handle = 0 ) then
    Handle := FindWindowByClassName('FB_Guard');

  if (Handle > 0) then
     result := True
  else begin
    mutex_found := CheckForMutexes('FirebirdGuardianMutex,FirebirdServerMutex');
    if mutex_found then
      result := true;
  end;

end;

#ifdef CPLAPPLET_AVAILABLE
function InstallCPLApplet: boolean;
begin
  result := False;
  if ( (ConfigureFirebird) AND (not NoCPL) ) then
     result := IsTaskSelected('InstallCPLAppletTask') ;
end;


function ShowInstallCPLAppletTask: boolean;
begin
  //If NOCPL is on the command line then don't offer the task in UI mode.
  result := ((not NoCPL) and ConfigureFirebird);
end;
#endif

#ifdef LEGACY_GDS32
function CopyGds32: boolean;
begin
  //Note that we invert the value of NOLEGACYCLIENT so we provide the
  //correct answer to the question 'Do we copy GDS32 to <sys>' which is
  //the default behaviour.
  result := False;
  if ConfigureFirebird then begin
    //If one of these is false then either the commandline switch was passed
    //or the user unchecked the Copy client as GDS32 box
    result := ( (not NoLegacyClient) AND (IsTaskSelected('CopyFbClientAsGds32Task') );
  end;
end;


function ShowCopyGds32Task: boolean;
begin
  //If NOGDS32 is on the command line then don't offer the task in UI mode.
  result := ((not NoLegacyClient) and ConfigureFirebird);
end;
#endif

#ifdef LEGACY_FBCLIENT
function CopyFbClientLib: boolean;
begin
//Note that the default for this is the opposite to CopyGds32.
  result := ( (CopyFbClient) OR (ShouldProcessEntry('ClientComponent', 'CopyFbClientToSysTask')= srYes) );
end;


function ShowCopyFbClientLibTask: boolean;
//See note for ShowCopyGds32Task.
begin
  result := False;
  if ConfigureFirebird then
    result := ((not CopyFbClient) and ConfigureFirebird);
end;
#endif

//=============== End of FirebirdInstallEnvironmentChecks.inc ======================


function SummarizeInstalledProducts: String;
var
  InstallSummaryArray: TArrayofString;
  product: Integer;
  i: Integer;
  StatusDescription: String;
  InstallSummary: String;
  prodstr: String;
begin

//do nothing gracefully if we are called by accident.
if ProductsInstalledCount = 0 then
  exit;

SetArrayLength(InstallSummaryArray,ProductsInstalledCount);
for product := 0 to MaxProdInstalled -1 do begin
  if (ProductsInstalledArray[product].InstallType <> NotInstalled) then begin
      InstallSummaryArray[i] := Format(ProductsInstalledArray[product].Description , [ProductsInstalledArray[product].ActualVersion]);

    if (ProductsInstalledArray[product].ServerVersion <> '') then begin
      if ((ProductsInstalledArray[product].InstallType AND ClassicServerInstall) = ClassicServerInstall) then
        InstallSummaryArray[i] := InstallSummaryArray[i] + ' '+ExpandConstant('{cm:ClassicServerInstall}')
      else
        InstallSummaryArray[i] := InstallSummaryArray[i] + ' '+ExpandConstant('{cm:SuperServerInstall}')
      end
    else begin
      if (ProductsInstalledArray[product].GBAKVersion <> '') then
        InstallSummaryArray[i] := InstallSummaryArray[i] + ' '+ExpandConstant('{cm:DeveloperInstall}')
      else
        InstallSummaryArray[i] := InstallSummaryArray[i] + ' '+ExpandConstant('{cm:ClientInstall}')
    end;

    if ((ProductsInstalledArray[product].InstallType AND BrokenInstall) = BrokenInstall) then
      InstallSummaryArray[i] := InstallSummaryArray[i]
      + #13 + ExpandConstant('{cm:PreviousInstallBroken}')
    else
      InstallSummaryArray[i] := InstallSummaryArray[i]
      + #13 + ExpandConstant('{cm:PreviousInstallGood}')
    ;

    i:= i+1;
  end;
end;

for i:=0 to ProductsInstalledCount-1 do
  InstallSummary := InstallSummary + InstallSummaryArray[i] + #13;

//If FB2 is installed
If ((ProductsInstalled AND FB2) = FB2) then
      InstallSummary := InstallSummary
      +#13 + ExpandConstant('{cm:InstallSummarySuffix1,{#ProductNameQualifier}}')
      +#13 + ExpandConstant('{cm:InstallSummarySuffix2,{#ProductNameQualifier}}')
      +#13 + ExpandConstant('{cm:InstallSummarySuffix3}')
      +#13 + ExpandConstant('{cm:InstallSummarySuffix4}')
      +#13
;

if ProductsInstalledCount = 1 then
  StatusDescription := Format(ExpandConstant('{cm:InstalledProducts}'), [IntToStr(ProductsInstalledCount), ExpandConstant('{cm:InstalledProdCountSingular}')])
else
  StatusDescription := Format(ExpandConstant('{cm:InstalledProducts}'), [IntToStr(ProductsInstalledCount), ExpandConstant('{cm:InstalledProdCountPlural}')]);

  Result := StatusDescription
    +#13
    +#13 + InstallSummary
    +#13 + ExpandConstant('{cm:InstallSummaryCancel1,{#ProductNameQualifier}}')
    +#13 + ExpandConstant('{cm:InstallSummaryCancel2}')
    +#13
    +#13 + ExpandConstant('{cm:InstallSummaryCancel3}')
    +#13
end;

function AnalysisAssessment: boolean;
var
  MsgText: String;
  MsgResult: Integer;
begin
  result := false;

  //We've got all this information. What do we do with it?

  if ProductsInstalledCount = 0 then begin
      result := true;
      exit;
  end;

  //If Fb15 RC or Fb15 is installed then we can install over it.
  //unless we find the server running.
  if (ProductsInstalledCount = 1) AND
    (((ProductsInstalled AND FB15) = FB15) OR
     ((ProductsInstalled AND FB15RC) = FB15RC)) then
    if ( FirebirdDefaultServerRunning ) then begin
      result := false;
      MsgBox( #13+ExpandConstant('{cm:FbRunning1,1.5}')
      +#13
      +#13+ExpandConstant('{cm:FbRunning2}')
      +#13+ExpandConstant('{cm:FbRunning3}')
      +#13, mbError, MB_OK);
      exit;
      end
    else begin
      result := true;
      exit;
    end
  ;

  //If Fb2.0 is installed then we can install over it.
  //unless we find the server running.
  if (ProductsInstalledCount = 1) AND
    ((ProductsInstalled AND FB2) = FB2) then
    if ( FirebirdDefaultServerRunning ) then begin
      result := false;
      MsgBox( #13+ExpandConstant('{cm:FbRunning1,2.0}')
      +#13
      +#13+ExpandConstant('{cm:FbRunning2}')
      +#13+ExpandConstant('{cm:FbRunning3}')
      +#13, mbError, MB_OK);
      exit;
      end
    else begin
      result := true;
      exit;
    end
  ;

  if ForceInstall then begin
    result := true;
    exit;
  end;

  //Otherwise, show user the analysis report.
  MsgText := SummarizeInstalledProducts;
  MsgResult := MsgBox(MsgText,mbConfirmation,MB_YESNO);
  if (MsgResult = IDNO ) then
    result := true;
    //but we don't configure
    if ((InstallAndConfigure AND Configure) = Configure) then
      InstallAndConfigure := InstallAndConfigure - Configure;
end;


function InitializeSetup(): Boolean;
var
  i: Integer;
  CommandLine: String;
begin

  result := true;

  if not CheckWinsock2 then begin
    result := False;
    exit;
  end

  CommandLine:=GetCmdTail;
  if pos('FORCE',Uppercase(CommandLine))>0 then
    ForceInstall:=True;

  if pos('NOCPL', Uppercase(CommandLine))>0 then
    NoCPL := True;

  if pos('NOGDS32', Uppercase(CommandLine))>0 then
    NoLegacyClient := True;

  if pos('COPYFBCLIENT', Uppercase(CommandLine))>0 then
    CopyFbClient := True;

  //By default we want to install and confugure,
  //unless subsequent analysis suggests otherwise.
  InstallAndConfigure := Install + Configure;

  InstallRootDir := '';

  InitExistingInstallRecords;
  AnalyzeEnvironment;
  result := AnalysisAssessment;
  if result then
    //There is a possibility that all our efforts to detect an
    //install were in vain and a server _is_ running...
    if ( FirebirdDefaultServerRunning ) then begin
      result := false;
      MsgBox( #13+ExpandConstant('{cm:FbRunning1, }')
      +#13
      +#13+ExpandConstant('{cm:FbRunning2}')
      +#13+ExpandConstant('{cm:FbRunning3}')
      +#13, mbError, MB_OK);
      exit;
    end;
end;


procedure DeInitializeSetup();
var
  ErrCode: Integer;
begin
  // Did the install fail because winsock 2 was not installed?
  if Winsock2Failure then
    // Ask user if they want to visit the Winsock2 update web page.
  	if MsgBox(ExpandConstant('{cm:Winsock2Web1}')+#13#13+ExpandConstant('{cm:Winsock2Web2}'), mbInformation, MB_YESNO) = idYes then
  	  // User wants to visit the web page
      ShellExec(sMSWinsock2Update, '','', '', SW_SHOWNORMAL, ewNoWait, ErrCode);
end;


//This function tries to find an existing install of Firebird Vulcan
//If it succeeds it suggests that directory for the install
//Otherwise it suggests the default for Fb
function ChooseInstallDir(Default: String): String;
var
	InterBaseRootDir,
	FirebirdRootDir: String;
begin
  //The installer likes to call this FOUR times, which makes debugging a pain,
  //so let's test for a previous call.
  if (InstallRootDir = '') then begin

    // Try to find the value of "RootDirectory" in the Firebird
    // registry settings. This is either where Fb 1.0 exists or Fb 1.5
    InterBaseRootDir := GetInterBaseDir;
    FirebirdRootDir := GetFirebirdDir;

    if (FirebirdRootDir <> '') and ( FirebirdRootDir = InterBaseRootDir ) then  //Fb 1.0 must be installed so don't overwrite it.
      InstallRootDir := Default;

    if (( InstallRootDir = '' ) and
        ( FirebirdRootDir = Default )) then // Fb 1.5 is already installed,
      InstallRootDir := Default;             // so we offer to install over it

    if (( InstallRootDir = '') and
        ( FirebirdVer[0] = 1 ) and ( FirebirdVer[1] = 5 ) ) then   // Firebird 1.5 is installed
      InstallRootDir := FirebirdRootDir;                            // but the user has changed the default

    // if we haven't found anything then try the FIREBIRD env var
    // User may have preferred location for Firebird, but has possibly
    // uninstalled previous version
    if (InstallRootDir = '') then
      InstallRootDir:=getenv('VULCAN');

    //if no existing locations found make sure we default to the default.
    if (InstallRootDir = '') then
      InstallRootDir := Default;

  end; // if InstallRootDir = '' then begin

  Result := ExpandConstant(InstallRootDir);

end;


function ServiceStartFlags(Default: String): String;
var
  ServerType: String;
  SvcParams: String;
begin
  servertype := '';
  SvcParams := '';

  if IsTaskSelected('AutoStartTask') then
    SvcParams := ' -auto '
  else
    SvcParams := ' -demand ';

  if IsTaskSelected('UseGuardianTask') then
    SvcParams := SvcParams + ' -guardian'
  else
    SvcParams := SvcParams;

  Result := SvcParams;
end;

function StartApp(Default: String): String;
begin
  if IsTaskSelected('UseGuardianTask') then
    Result := GetAppPath+'\bin\fbguard.exe'
  else
    Result := GetAppPath+'\bin\fbserver.exe';
end;

function IsNotAutoStartApp: boolean;
//Support function to help remove unwanted registry entry.
begin
  result := true;
  if ( IsTaskSelected('AutoStartTask') and
     IsTaskSelected('UseApplicationTask') ) then
  result := false;
end;

function NonDefaultLanguage: boolean;
//return true if language other than default is chosen
begin
  result := (ActiveLanguage <> 'en');
end;


procedure CurPageChanged(CurPage: Integer);
begin
  case CurPage of
    wpInfoBefore:   WizardForm.INFOBEFOREMEMO.font.name:='Courier New';
    wpInfoAfter:    WizardForm.INFOAFTERMEMO.font.name:='Courier New';
    wpSelectTasks:  WizardForm.TASKSLIST.height := WizardForm.TASKSLIST.height+30;
    wpFinished:     ; // Create some links to Firebird and IBP here?.
  end;
end;


procedure CurStepChanged(CurStep: TSetupStep);
var
  AppStr: String;
  ReadMeFileStr: String;
begin
   case CurStep of
    ssInstall: begin
              SetupSharedFilesArray;
              GetSharedLibCountBeforeCopy;
      end;

    ssDone: begin
      //If user has chosen to install an app and run it automatically set up the registry accordingly
      //so that the server or guardian starts evertime they login.
      if (IsTaskSelected('AutoStartTask') and
              IsTaskSelected('UseApplicationTask')) then begin
        AppStr := StartApp('')+' -a';
        RegWriteStringValue (HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Run', 'Firebird', AppStr);
      end;
      
      
      //Reset shared library count if necessary
      CheckSharedLibCountAtEnd;

      //Move lang specific readme from doc dir to root of install.
      if NonDefaultLanguage then begin
        ReadMeFileStr := ExpandConstant('{cm:ReadMeFile}');
        if FileCopy(GetAppPath+'\doc\'+ReadMeFileStr, GetAppPath+'\'+ReadMeFileStr, false) then
          DeleteFile(GetAppPath+'\doc\'+ReadMeFileStr);
      end;
      
    end;
  end;
end;

function StartEngine: boolean;
begin
  if ConfigureFirebird then
    result := not FirebirdOneRunning;
end;

// InnoSetup has a Check Parameter that allows installation if the function returns true.
// For Firebird Vulcan .conf files we want to do two things:
//   o if a .conf already exists then install it as .conf.default
//   o if a .conf does not exist then install it as .conf
//
// This double test is also needed because the uninstallation rules are different for each file.
// We never uninstall a .conf because it may have user specific changes to it.
// We always uninstall .conf.default files.

function ConfFileExists(Default: String) : boolean;
begin
  Result := fileexists(Default);
end;

function NoConfFileExists(Default: String) : boolean;
begin
  Result := not fileexists(Default);
end;



begin
end.



