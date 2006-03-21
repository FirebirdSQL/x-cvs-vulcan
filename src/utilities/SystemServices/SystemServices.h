/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by Paul Reeves for the Firebird Project
 *  Copyright (c) 2006 Paul Reeves
 *
 *  All Rights Reserved.
 */

#if _MSC_VER > 1000
//Note - pragma once is fine for MSVC and is also supported since gcc 3.4
//However it may be silently ignored by older preprocessors.
//As long as 
#pragma once
#endif

#include "fbdev.h"
#include "common.h"
#include "JString.h"
#include "Registry.h"
#include "SystemServicesException.h"
#include <Shlwapi.h>
#include "winsvc.h"


static const char* SYS_SERVICE_NAME				= "FirebirdServerVulcanInstance";
static const char* SYS_SERVICE_DISPLAY_NAME		= "Firebird Server - VulcanInstance";
static const char* SYS_SERVICE_DISPLAY_DESCR	= "Firebird Database Server - www.firebirdsql.org";
static const char* SYS_SERVICE_EXECUTABLE		= "bin\\fbserver";

//Service info strings
static const char* FB_SERVICE_RUNNING			= "Firebird Vulcan service is still running.";
static const char* FB_SERVICE_STATUS_UNKNOWN    = "Firebird Vulcan service status is unknown.";
static const char* FB_SERVICE_START_FAILED		= "Firebird Vulcan service failed to complete its start sequence.";
static const char* FB_SERVICE_STOP_FAILED		= "Firebird Vulcan service failed to complete its stop sequence.";
static const char* FB_SERVICE_ALREADY_DEFINED	= "Firebird Vulcan service already defined.";
static const char* FB_SERVICE_STATUS_RUNNING	= "Firebird Vulcan service running.";
static const char* FB_SERVICE_STATUS_NOT_STOPPED= "Firebird Vulcan service has not been stopped.";
static const char* FB_SERVICE_STATUS_STOPPED	= "Firebird Vulcan service stopped";
static const char* FB_SERVICE_STATUS_PAUSED		= "Firebird Vulcan service paused";
static const char* FB_SERVICE_STATUS_PENDING	= "Firebird Vulcan service is pending change.";
static const char* FB_SERVICE_STATUS_INSTALLED	= "Firebird Vulcan service is installed.";
static const char* FB_SERVICE_STATUS_NOT_INSTALLED = "Firebird Vulcan service is not installed.";
static const char* FB_NOT_INSTALLED				= "Firebird Vulcan is not installed correctly.\nPerhaps you need to run instreg?";
static const char* FB_NO_SERVICES				= "Services are not available on this plaform.";
static const char* FB_LOGON_SRVC_RIGHT_ALREADY_DEFINED= "Firebird Vulcan logon service right is already defined.";


static const int SVC_INSTALL	= 0x1;
static const int SVC_REMOVE		= 0x2;
static const int SVC_START		= 0x4;
static const int SVC_STOP		= 0x8;
static const int SVC_STATUS		= 0x10;
static const int SVC_MANUAL		= 0x20;
static const int SVC_PAUSE		= 0x40;
static const int SVC_DESKTOP	= 0x80;

struct Flags
	{
	int action;
	bool verbose;
	bool readOnly;
	const char *uname;
	const char *pword;
	};


class SystemServices
{
private:
	void init();
	void setRootPath();	

public:
	SystemServices();
	SystemServices(Flags argsflags);
	SystemServices(Flags argsflags, JString aServiceName, JString aDisplayName, JString aDisplayDescription);
	virtual ~SystemServices(void);

	// These public methods are intended to be called independently of each other.
	// ie, they handle all necessary opening and closing of service handles.
	// They also wrap all calls to the private methods within a try...catch block.
	void execute(void);				// Work out what to do from flags passed and do it.
	bool getFirebirdInstalled();	// True if firebird installed.
	bool getServiceInstalled();		// True if service installed.
	void status(void);				// Print service status.

	bool serviceSupportAvailable();	//Determine whether the host O/S supports services

	JString serviceName;
	JString displayName;
	JString displayDescription;
	JString userName;
	JString password;
	bool servicesAvailable;

private:
	Flags flags;
	DWORD desiredAccessRights;
	JString rootPath;

	// public methods always assume the serviceManager and serviceHandle handles 
	// are closed and open them as required. 
	// private methods should always assume they are open.
	SC_HANDLE serviceManager;
	SC_HANDLE serviceHandle; 

	SERVICE_STATUS serviceStatus;

	//All private methods know about throwing exceptions, but 
	//do not know how to catch them.
	void confirmServiceDeleted();

	void closeServiceHandle();
	void closeServiceManager();
	void openServiceHandle();
	void openServiceManager();

	void install(void);
	void pause(void);
	void printStatus(void);
	void query(void);
	void remove(void);
	void restart(void);
	void start(void);
	void stop(void);
};
