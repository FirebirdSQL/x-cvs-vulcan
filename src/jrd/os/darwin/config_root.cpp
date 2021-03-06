/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		config_root.cpp
 *	DESCRIPTION:	Configuration manager (platform specific - linux/posix)
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by John Bellardo
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2003 John Bellardo <bellardo at cs.ucsd.edu>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#include "fbdev.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "fb_types.h"
#include "fb_string.h"

#include "../jrd/os/config_root.h"
#include "../jrd/os/path_utils.h"
#include "../jrd/file_params.h"

#include <CoreServices.framework/Frameworks/CarbonCore.framework/Headers/MacTypes.h>
#include <CoreFoundation/CFBundle.h>
#include <CoreFoundation/CFURL.h>

typedef Firebird::string string;

static const char *CONFIG_FILE = "firebird.conf";

ConfigRoot::ConfigRoot()
{
	CFBundleRef fbFramework;
	CFURLRef	msgFileUrl;
	CFStringRef	msgFilePath;
	char file_buff[MAXPATHLEN];

	// Check the environment variable
	const char* envPath = getenv("FIREBIRD");
	if (envPath != NULL && strcmp("", envPath))
	{
		root_dir = envPath;
		return;
	}

	// Attempt to locate the Firebird.framework bundle
	if ((fbFramework = CFBundleGetBundleWithIdentifier(
			CFSTR(DARWIN_FRAMEWORK_ID)) ))
	if ((msgFileUrl = CFBundleCopyResourceURL( fbFramework,
			CFSTR(DARWIN_GEN_DIR), NULL, NULL)))
	if ((msgFilePath = CFURLCopyFileSystemPath(msgFileUrl,
			kCFURLPOSIXPathStyle)))
	if ((CFStringGetCString(msgFilePath, file_buff, MAXPATHLEN,
			kCFStringEncodingMacRoman )) )
	{
		root_dir = file_buff;
		return;
	}

	// As a last resort get it from the default install directory
	// this doesn't make much sense because the framework method should
	// never fail, but what the heck.
	root_dir = FB_PREFIX;
}

const char *ConfigRoot::getRootDirectory() const
{
	return root_dir.c_str();
}

const char *ConfigRoot::getConfigFile() const
{
	static string file = root_dir + string(CONFIG_FILE);
	return file.c_str();
}
