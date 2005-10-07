/*
 *	PROGRAM:	Interprocess Interface definitions
 *      MODULE:         XNetChannel.h
 *	DESCRIPTION:	Common descriptions
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

#ifndef XNET_MAPPED_FILE_H
#define XNET_MAPPED_FILE_H

#ifndef _WIN32
#ifndef HANDLE
#define HANDLE		int
#endif
#endif

/* mapped structure flags */

#define XPMF_SERVER_SHUTDOWN    1       /* server has shut down */
#define XPM_FREE 0                      /* xpm structure is free for use */
#define XPM_BUSY 1                      /* xpm structure is in use */

/* mapped file parameters */

#define XPS_MAPPED_PER_CLI(p)        ((ULONG)(p) * 1024L)
#define XPS_SLOT_OFFSET(pages,slot)  (XPS_MAPPED_PER_CLI(pages) * (ULONG)(slot))
#define XPS_MAPPED_SIZE(users,pages) ((ULONG)(users) * XPS_MAPPED_PER_CLI(pages))

#define XPS_USEFUL_SPACE(p)          (XPS_MAPPED_PER_CLI(p) - sizeof(struct xps))

#define XPS_DEF_NUM_CLI         10      /* default clients per mapped file */
#define XPS_DEF_PAGES_PER_CLI   8       /* default 1k pages space per client */

#define XPS_MIN_NUM_CLI         1       /* min clients per mapped file */
#define XPS_MIN_PAGES_PER_CLI   1       /* min 1k pages space per client */

#define XPS_MAX_NUM_CLI         64      /* max clients per mapped file */
#define XPS_MAX_PAGES_PER_CLI   16      /* max 1k pages space per client */


class XNetMappedFile
{
public:
	XNetMappedFile(ULONG map_number, time_t timestamp, int slots_per_map, int pages_per_slot);
	~XNetMappedFile(void);

    XNetMappedFile  *xpm_next;              /* pointer to next one */
    ULONG			xpm_count;              /* slots in use */
    ULONG			xpm_number;             /* mapped area number */
    HANDLE			xpm_handle;             /* handle of mapped memory */
    USHORT			xpm_flags;              /* flag word */
    void			*xpm_address;            /* address of mapped memory */
    UCHAR			xpm_ids[XPS_MAX_NUM_CLI]; /* ids */
    time_t			xpm_timestamp;          /* timestamp to avoid map name confilcts */
    int				slotsPerMap;
    int				pagesPerSlot;
    
	bool mapFile(bool createFlag);
	void close(void);
	static void unmapFile(void** handlePtr);
	static void closeFile(HANDLE* handlePtr);
};

#endif

