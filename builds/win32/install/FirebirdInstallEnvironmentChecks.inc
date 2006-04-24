(*  Initial Developer's Public License.
  The contents of this file are subject to the  Initial Developer's Public
  License Version 1.0 (the "License"). You may not use this file except
  in compliance with the License. You may obtain a copy of the License at
    http://www.ibphoenix.com?a=ibphoenix&page=ibp_idpl
  Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
  for the specific language governing rights and limitations under the
  License.

  The Original Code is copyright 2001-2003 Paul Reeves for IBPhoenix.

  The Initial Developer of the Original Code is Paul Reeves for IBPhoenix.

  All Rights Reserved.

  This file contains a whole bunch of functions that
  check the registry and see what versions of firebird or interbase
  are already installed.

  This stuff _is_ firebird/interbase specific and some of it is entirely dependant
  upon parts of the InnoSetup install script.

  Here is a partial list of functions available in this script:

    
    procedure SetupSharedFilesArray;
    procedure GetSharedLibCountBeforeCopy;
    procedure CheckSharedLibCountAtEnd;

    function GetFirebirdDir: string;
    function GetInterBaseDir: string;
    function TestForWorkingInstalls: boolean;

    function FirebirdOneRunning: boolean;

    function InstallCPLApplet: boolean;
    function ShowInstallCPLAppletTask: boolean;
    function CopyGds32: boolean;
    function ShowCopyGds32Task: boolean;
    function CopyFbClientLib: boolean;
    function ShowCopyFbClientLibTask: boolean;


 *)



