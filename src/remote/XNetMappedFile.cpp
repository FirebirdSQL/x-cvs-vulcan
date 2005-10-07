/*
 *	PROGRAM:	JRD Remote Interface/Server
 *      MODULE:         xnet.cpp
 *      DESCRIPTION:    Interprocess Server Communications module.
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2003.05.01 Victor Seryodkin, Dmitry Yemanov: Completed XNET implementation
 */


#ifdef _WIN32
#include <windows.h>
#endif /* WIN_NT */

#include <stdio.h>
#include <time.h>
//#include <errno.h>
#include "fbdev.h"
#include "common.h"
#include "XNetMappedFile.h"
#include "isc_proto.h"

#define XNET_MAPPED_FILE_NAME	"%s_MAP_%"ULONGFORMAT"_%"ULONGFORMAT
#define XNET_PREFIX				"FirebirdXNET"

XNetMappedFile::XNetMappedFile(ULONG map_number, time_t timestamp, int slots_per_map, int pages_per_slot)
{
	xpm_address = NULL;
	xpm_handle = 0;
	xpm_number = map_number;
	xpm_count = 0;
	xpm_timestamp = timestamp;
	slotsPerMap = slots_per_map;
	pagesPerSlot = pages_per_slot;
	
	/***
	if (!xnet_make_map(map_number, timestamp, &xpm_handle, &xpm_address))
		return NULL;
	***/
	
	for (int i = 0; i < slots_per_map; i++)
		xpm_ids[i] = XPM_FREE;

	xpm_flags = 0;

	//xpm_next = client_maps;
	//client_maps = xpm;
}

XNetMappedFile::~XNetMappedFile(void)
{
	close();
}

void XNetMappedFile::close(void)
{
	unmapFile(&xpm_address);
	closeFile(&xpm_handle);
}

bool XNetMappedFile::mapFile(bool createFlag)
{
#ifdef WIN_NT
	TEXT name_buffer[128];
	sprintf(name_buffer, XNET_MAPPED_FILE_NAME, XNET_PREFIX, xpm_number, (ULONG) xpm_timestamp);
	
	if (createFlag)
		xpm_handle = CreateFileMapping(INVALID_HANDLE_VALUE,
										ISC_get_security_desc(),
										PAGE_READWRITE,
										0L,
										XPS_MAPPED_SIZE(slotsPerMap, pagesPerSlot),
										name_buffer);
	else
		xpm_handle = OpenFileMapping(FILE_MAP_WRITE, FALSE, name_buffer);
		                              
	if (!xpm_handle || GetLastError() == ERROR_ALREADY_EXISTS)
		return FALSE;

	xpm_address = MapViewOfFile(xpm_handle, FILE_MAP_WRITE, 0, 0,
								 XPS_MAPPED_SIZE(slotsPerMap, pagesPerSlot));
								 
	if (!xpm_address)
		close();

#endif // WIN_NT

	return true;
}

void XNetMappedFile::unmapFile(void** handlePtr)
{
	if (*handlePtr)
		{
#ifdef WIN_NT
		UnmapViewOfFile(*handlePtr);
#endif
		*handlePtr = NULL;
		}
}

void XNetMappedFile::closeFile(HANDLE* handlePtr)
{
	if (*handlePtr)
		{
#ifdef WIN_NT
		CloseHandle(*handlePtr);
#endif
		*handlePtr = 0;
		}
}
