/*
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
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2002 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "fbdev.h"

#include "../../common/config/config_file.h"
#include "../jrd/os/fbsyslog.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <fstream>
#include <iostream>

// Invalid or missing CONF_FILE may lead to severe errors
// in applications. That's why for regular SERVER builds
// it's better to exit with appropriate diags rather continue
// with missing / wrong configuration.
#if (! defined(BOOT_BUILD)) && (! defined(EMBEDDED)) && (! defined(SUPERCLIENT))
#define EXIT_ON_NO_CONF
#define INFORM_ON_NO_CONF
#else
#undef EXIT_ON_NO_CONF
#undef INFORM_ON_NO_CONF
#endif

typedef Firebird::string string;

/******************************************************************************
 *
 *	Allow case-insensitive comparison
 */

bool ConfigFile::key_compare::operator()(const string& x, const string& y) const
{
	return Firebird::PathName(x) < Firebird::PathName(y);
}

/******************************************************************************
 *
 *	Strip leading spaces
 */

void ConfigFile::stripLeadingWhiteSpace(string& s)
{
	if (!s.size())
	{
		return;
	}

	const string::size_type startPos = s.find_first_not_of(" \t\r");
	if (startPos == string::npos)
	{
		s.erase();	// nothing but air
	}
	else if (startPos)
	{
		s = s.substr(startPos);
	}
}

/******************************************************************************
 *
 *	Strip trailing spaces
 */

void ConfigFile::stripTrailingWhiteSpace(string& s)
{
	if (!s.size())
	{
		return;
	}

	string::size_type endPos = s.find_last_not_of(" \t\r");
	if (endPos != string::npos)
	{
		// Note that endPos is the index to the last non-ws char
		// why we have to inc. it
		++endPos;
		s = s.substr(0, endPos);
	}
}

/******************************************************************************
 *
 *	Strip any comments
 */

void ConfigFile::stripComments(string& s)
{
	// Note that this is only a hack. It won't work in case inputLine
	// contains hash-marks embedded in quotes! Not that I know if we
	// should care about that case.
	const string::size_type commentPos = s.find('#');
	if (commentPos != string::npos)
	{
		s = s.substr(0, commentPos);
	}
}

/******************************************************************************
 *
 *	Check whether the given key exists or not
 */

bool ConfigFile::doesKeyExist(const string& key)
{
    checkLoadConfig();

    string data = getString(key);

    return !data.empty();
}

/******************************************************************************
 *
 *	Return string value corresponding the given key
 */

string ConfigFile::getString(const string& key)
{
    checkLoadConfig();

    mymap_t::iterator lookup;

    lookup = parameters.find(key);

    if (lookup != parameters.end())
    {
    	return lookup->second;
    }

    return string();
}

/******************************************************************************
 *
 *	Parse key
 */

string ConfigFile::parseKeyFrom(const string& inputLine, string::size_type& endPos)
{
    endPos = inputLine.find_first_of("=\t");
    if (endPos == string::npos)
    {
        return inputLine;
    }

    return inputLine.substr(0, endPos);
}

/******************************************************************************
 *
 *	Parse value
 */

string ConfigFile::parseValueFrom(string inputLine, string::size_type initialPos)
{
    if (initialPos == string::npos)
    {
        return string();
    }

    // skip leading white spaces
    unsigned int startPos = inputLine.find_first_not_of("= \t", initialPos);
    if (startPos == string::npos)
    {
        return string();
    }

    stripTrailingWhiteSpace(inputLine);
    return inputLine.substr(startPos);
}

/******************************************************************************
 *
 *	Load file, if necessary
 */

void ConfigFile::checkLoadConfig()
{
	if (!isLoadedFlg)
	{
    	loadConfig();
	}
}

/******************************************************************************
 *
 *	Load file immediately
 */

void ConfigFile::loadConfig()
{
	isLoadedFlg = true;

	parameters.clear();

    std::ifstream configFileStream(configFile.c_str());
	
#ifdef EXIT_ON_NO_CONF
	int BadLinesCount = 0;
#endif
    if (!configFileStream)
    {
        // config file does not exist
#ifdef INFORM_ON_NO_CONF
		string Msg = "Missing configuration file: " + configFile;
#ifdef EXIT_ON_NO_CONF
		if (fExitOnError)
			Msg += ", exiting";
#endif
		Firebird::Syslog::Record(fExitOnError ? 
				Firebird::Syslog::Error :
				Firebird::Syslog::Warning, Msg);
#ifdef EXIT_ON_NO_CONF
		if (fExitOnError)
			exit(1);
#endif
#endif //INFORM_ON_NO_CONF
		return;
    }
    string inputLine;

    while (!configFileStream.eof())
    {
		std::getline(configFileStream, inputLine);

		stripComments(inputLine);
		stripLeadingWhiteSpace(inputLine);
		
		if (!inputLine.size())
		{
			continue;	// comment-line or empty line
		}

        if (inputLine.find('=') == string::npos)
        {
            string Msg = configFile + ": illegal line \"" +
				inputLine + "\"";
			Firebird::Syslog::Record(fExitOnError ? 
					Firebird::Syslog::Error :
					Firebird::Syslog::Warning, Msg);
#ifdef EXIT_ON_NO_CONF
			BadLinesCount++;
#endif
            continue;
        }

        string::size_type endPos;

        string key   = parseKeyFrom(inputLine, endPos);
		stripTrailingWhiteSpace(key);
		// TODO: here we must check for correct parameter spelling !
        string value = parseValueFrom(inputLine, endPos);

		// parameters.insert(pair<string, string>(key, value));
		// Just to display yet another template function
        parameters.insert(std::make_pair(key, value));
    }
#ifdef EXIT_ON_NO_CONF
	if (BadLinesCount && fExitOnError) {
		exit(1);
	}
#endif
}
