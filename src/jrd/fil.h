/*
 *	PROGRAM:	JRD temp file space list
 *	MODULE:		fil.h
 *	DESCRIPTION:	Lists of directories for temporary files or UDFs
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
 * 26-Sept-2001 Paul Beach - External File Directory Config. Parameter
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef JRD_FIL_H
#define JRD_FIL_H

#include "../jrd/thd.h"
#include "../jrd/SyncObject.h"

#ifdef UNIX
#define WORKDIR        "/tmp/"
#endif

#ifdef WIN_NT
#define WORKDIR        "/temp/"
#endif

#ifdef VMS
#define WORKDIR        "SYS$SCRATCH:"
#endif

#define	ALLROOM		-1UL		/* use all available space */

/* Defined the directory list structures. */

/* Temporary workfile directory list. */

struct DirectoryList {
	SLONG dls_header;
	DirectoryList* dls_next;
	ULONG dls_size;				/* Maximum size in the directory */
	ULONG dls_inuse;			/* Occupied space in the directory */
	TEXT dls_directory[2];		/* Directory name */
};  //*DLS;

typedef struct mdls {
	DirectoryList *mdls_dls;				/* Pointer to the directory list */
	//BOOLEAN mdls_mutex_init;
	SyncObject	syncObject;
	//MUTX_T mdls_mutex[1];		/* Mutex for directory list. Must be locked before list operations */
} MDLS;

/* external function directory list */

typedef struct fdls {
	fdls* fdls_next;
	TEXT fdls_directory[1];
} FDLS;

/* external file directory list */

typedef struct edls {
    edls* edls_next;
    TEXT edls_directory[1];
} EDLS;

#endif /* JRD_FIL_H */
