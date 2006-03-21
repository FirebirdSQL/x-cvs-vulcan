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
 *  The Original Code was created by Paul Reeves for the Firebird Project.
 *  Copyright (c) 2006 Paul Reeves
 *  All Rights Reserved.
 *
 *
 */

/* 
 * Notes (2006-02-28 PR)
 *
 * Status - installing and running as non-LocalSystem 
 *			account has not been tested and almost certainly
 *			won't work out of the box. The username needs to
 *			be prefixed with the computer name.
 *
 * To Do  - Service dependencies need to be added to install().
 *
 */


#include "SystemServices.h"


SystemServices::SystemServices()
{
	flags.action = 0;
	flags.verbose = false;
	flags.readOnly = false;
	init();
}


SystemServices::SystemServices(Flags argsflags) 
: flags(argsflags)
{
  init();
}


SystemServices::~SystemServices(void)
{
	closeServiceManager();
}


SystemServices::SystemServices(Flags argsflags, 
							   JString aServiceName, 
							   JString aDisplayName, 
							   JString aDisplayDescription)
: flags(argsflags), 
  serviceName(aServiceName),
  displayName(aDisplayName), 
  displayDescription(aDisplayDescription)
{
  init();
}


void SystemServices::init()
{
	serviceManager = NULL;
	serviceHandle = NULL; 
	desiredAccessRights = NULL;

	serviceSupportAvailable();
	setRootPath();

	if (serviceName.IsEmpty())
		serviceName = SYS_SERVICE_NAME;

	if (displayName.IsEmpty())
		displayName = SYS_SERVICE_DISPLAY_NAME;

	if (displayDescription.IsEmpty())
		displayDescription = SYS_SERVICE_DISPLAY_DESCR;

}

void SystemServices::setRootPath()
// Check if Firebird is installed by reading the registry
// If installed then set rootPath accordingly.
{
	rootPath = "";
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_ROOT_INSTANCES, 0, KEY_QUERY_VALUE, &hkey) 
		== ERROR_SUCCESS)
	{
		char root_path[MAX_PATH - 2];
		DWORD buffer_size = sizeof(root_path);
		if (RegQueryValueEx(hkey, FB_DEFAULT_INSTANCE, NULL, NULL, LPBYTE(root_path), &buffer_size)
			== ERROR_SUCCESS)
		{
			char full_path[MAX_PATH - 2];
			buffer_size = sizeof(full_path);
			buffer_size = GetLongPathName(root_path,full_path, buffer_size);
			PathAddBackslash(full_path);
			rootPath = full_path;
		}
		
		RegCloseKey(hkey);


	}
}


void SystemServices::execute()
	{

	if (flags.action == SVC_STATUS)
		{
		status();
		return;
		}

	try
		{
		openServiceManager();

		//Analyze flags and see what we need to do

		if (flags.action & SVC_STOP)
			stop();

		if (flags.action & SVC_REMOVE)
			remove();
	
		if (flags.action & SVC_INSTALL)
			install();

		if (flags.action & SVC_START)
			start();

		if (flags.action & SVC_PAUSE)
			pause();

		}
	catch( SystemServicesException* E )
		{
		printf(E->getText());
		delete E;
		}
	closeServiceManager();
	}


bool SystemServices::getServiceInstalled()
{
	bool result = false;

	try
		{
		openServiceManager();
		SC_HANDLE serviceHandle = OpenService (serviceManager, serviceName.getString(), GENERIC_READ );
		result = (bool) serviceHandle;
		if ( serviceHandle != NULL )
			{
			if (!CloseServiceHandle( serviceHandle ))
				throwSystemServicesExceptionCode(0);
			}
		}
	catch( SystemServicesException* E )
		{
		printf(E->getText());
		delete E;
		}
	return result;
}


bool SystemServices::getFirebirdInstalled()
{
	return (!rootPath.IsEmpty());
}


void SystemServices::confirmServiceDeleted()
{
	// If we can get a service handle then the service is not 
	// yet deleted, so keep testing.
	while (true)
	{
		Sleep(100);	

		SC_HANDLE serviceHandle = OpenService(serviceManager, serviceName, GENERIC_READ);
		if (!serviceHandle)
			break;
		else 
			CloseServiceHandle(serviceHandle);
	}
}


void SystemServices::install(void)
{
	if (rootPath.IsEmpty())
		throwSystemServicesExceptionText(FB_NOT_INSTALLED);

	JString fullPathToService = rootPath+SYS_SERVICE_EXECUTABLE+".exe -s";
	DWORD dwServiceType = SERVICE_WIN32_OWN_PROCESS;

	if (!userName.IsEmpty() && (flags.action & SVC_DESKTOP))
		dwServiceType |= SERVICE_INTERACTIVE_PROCESS;
	
	serviceHandle = CreateService(serviceManager,
							serviceName,
							displayName,
							SERVICE_ALL_ACCESS,
							dwServiceType,
							((flags.action & SVC_MANUAL) ? SERVICE_DEMAND_START : SERVICE_AUTO_START), 
							SERVICE_ERROR_NORMAL,
							fullPathToService.getString(), NULL, NULL, NULL,
							(!userName.IsEmpty() ? userName.getString() : NULL) ,
							(!password.IsEmpty() ? password.getString() : NULL) );


	if (serviceHandle == NULL)
	{
		DWORD dwError = GetLastError();
		if (dwError == ERROR_DUP_NAME || dwError == ERROR_SERVICE_EXISTS)
			throwSystemServicesExceptionText(FB_SERVICE_ALREADY_DEFINED);
		else
			throwSystemServicesExceptionCode(dwError);
	}

	// Now enter the description string into the service config, if this is
	// available on the current platform.
	HMODULE advapi32 = LoadLibrary("ADVAPI32.DLL");
	if (advapi32 != 0)
	{
		typedef BOOL __stdcall proto_config2(SC_HANDLE, DWORD, LPVOID);
		proto_config2* config2 =
			(proto_config2*)GetProcAddress(advapi32, "ChangeServiceConfig2A");
		if (config2 != 0)
		{
			(*config2)(serviceHandle, SERVICE_CONFIG_DESCRIPTION, &displayDescription);
		}
		FreeLibrary(advapi32);
	}

	closeServiceHandle();
	printf(FB_SERVICE_STATUS_INSTALLED);
	return ;
}


void SystemServices::pause(void)
{
	printf("Pausing service %s has not been implemented.\n",serviceName);
	return;
}


void SystemServices::printStatus(void)
{
// printStatus() is intended to be used to print status 
// to console _after_ a call to query() has been made.
// It is intended to be called from status()

	switch (serviceStatus.dwCurrentState)
	{
		case SERVICE_RUNNING : 
			{
			printf(FB_SERVICE_STATUS_RUNNING); 
			break;
			}
		case SERVICE_STOPPED : 
			{
			printf(FB_SERVICE_STATUS_STOPPED); 
			break;
			}
		case SERVICE_PAUSED : 
			{
			printf(FB_SERVICE_STATUS_PAUSED); 
			break;
			}
		case SERVICE_START_PENDING :
		case SERVICE_CONTINUE_PENDING :
		case SERVICE_PAUSE_PENDING :
		case SERVICE_STOP_PENDING : 
			{
			printf(FB_SERVICE_STATUS_PENDING); 
			break;
			}
		default :
			printf(FB_SERVICE_STATUS_UNKNOWN); 
	}
}


void SystemServices::query(void)
// query() is intended for general use by all methods. 
// It will error if no serviceHandle is available.
{
	if (!QueryServiceStatus(serviceHandle, &serviceStatus))
	{
		DWORD errnum = GetLastError();
		closeServiceHandle();
		closeServiceManager();
		throwSystemServicesExceptionCode(errnum);
	}
}


void SystemServices::remove(void)
{
	if ( flags.verbose)
		printf("Removing service %s...\n",serviceName);

	openServiceHandle();

	if (!QueryServiceStatus(serviceHandle, &serviceStatus))
		throwSystemServicesExceptionCode(0);

	//Oops - service hasn't stopped yet.
	if (serviceStatus.dwCurrentState > SERVICE_STOPPED)
		throwSystemServicesExceptionText(FB_SERVICE_RUNNING);

	if (!DeleteService(serviceHandle))
		throwSystemServicesExceptionCode(0);

	closeServiceHandle();

	confirmServiceDeleted();

	if ( flags.verbose)
		printf("Service %s removed.\n",serviceName);

	return;
}


void SystemServices::restart(void)
{
	if ( flags.verbose)
		printf("Restarting service...");

	stop();
	start();

	return;
}


void SystemServices::start(void)
{
	if ( flags.verbose)
		printf("Starting Service...\n");

	openServiceHandle();

	if ( !StartService(serviceHandle, 0, 0) )
		{
		DWORD dwError = GetLastError();
		closeServiceHandle();
		if (dwError != ERROR_SERVICE_ALREADY_RUNNING)
			throwSystemServicesExceptionCode(dwError);
		else
			return;
		}

	do
		query();
	while (serviceStatus.dwCurrentState == SERVICE_START_PENDING);

	closeServiceHandle();

	if (serviceStatus.dwCurrentState != SERVICE_RUNNING)
			throwSystemServicesExceptionText(FB_SERVICE_START_FAILED);

	return;
}


void SystemServices::status(void)
{
	try
		{
		openServiceManager();
		openServiceHandle();
		query();
		printStatus();
		closeServiceHandle();
		}
	catch( SystemServicesException* E )
		{
		printf(E->getText());
		delete E;
		}
	closeServiceManager();
}


void SystemServices::stop(void)
{
	if ( flags.verbose)
		printf("Stopping service %s...\n",serviceName);

	openServiceHandle();

	if ( !ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus) )
		{
		DWORD errnum = GetLastError();
		if (errnum == ERROR_SERVICE_NOT_ACTIVE)
			return;
		else
			throwSystemServicesExceptionCode(errnum);
		}

	do
		{
		query();
		Sleep(100);	
		}
	while 
		(serviceStatus.dwCurrentState == SERVICE_STOP_PENDING);


	if (serviceStatus.dwCurrentState != SERVICE_STOPPED)
		throwSystemServicesExceptionText(FB_SERVICE_STOP_FAILED);

	if ( flags.verbose)
		printf("Service %s stopped\n",serviceName);

	return;
}


void SystemServices::openServiceManager()
{
	if (!servicesAvailable)
		throwSystemServicesExceptionText(FB_NO_SERVICES);

	if (desiredAccessRights == 0)
		desiredAccessRights = GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE;

	if (serviceManager == NULL)
		{
		if (flags.verbose)
			printf("Attaching to Service Control Manager\n");

		serviceManager = OpenSCManager (NULL, SERVICES_ACTIVE_DATABASE, desiredAccessRights );

		if (serviceManager == NULL )
			//fall back to read only and allow querying of service status.
			{
			desiredAccessRights = GENERIC_READ;
			serviceManager = OpenSCManager (NULL, SERVICES_ACTIVE_DATABASE, desiredAccessRights );
			}
		}
	
	if (!serviceManager)
		throwSystemServicesExceptionCode(0);
}


void SystemServices::openServiceHandle()
{
	if (!getServiceInstalled())
		throwSystemServicesExceptionText(FB_SERVICE_STATUS_NOT_INSTALLED);

	serviceHandle = OpenService(serviceManager, serviceName, (desiredAccessRights==GENERIC_READ?GENERIC_READ:SERVICE_ALL_ACCESS));
    if (!serviceHandle)
		throwSystemServicesExceptionCode(0); 
}


void SystemServices::closeServiceManager()
// Closing service manager demands that we first close
// the service handle that the service manager holds open.
// The calling code (other methods in this class) should first
// have tried to close the service handle with closeServiceHandle()
// If that failed an error will have been thrown. Our task here
// is just to do final clean up.
{
	if (serviceManager)
		{
		//close service handle ignore any error
		if (serviceHandle)
			CloseServiceHandle(serviceHandle);

		//close manager and ignore error
		CloseServiceHandle(serviceManager);

		//Set null whether we succeed or not
		serviceManager = NULL;
		}
	return;
}

void SystemServices::closeServiceHandle()
{
	if (serviceHandle == NULL)
		return;

	if ( !CloseServiceHandle(serviceHandle) )
		throwSystemServicesExceptionCode(0);

	return;
}


bool SystemServices::serviceSupportAvailable()
{
	OSVERSIONINFO   OsVersionInfo;
	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OsVersionInfo);
	return (bool) (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
};

