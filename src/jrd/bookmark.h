/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		bookmark.h
 *	DESCRIPTION:	Prototype header file for bookmark.cpp
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
 */

#ifndef JRD_BOOKMARK_H
#define JRD_BOOKMARK_H

struct tdbb;

Bookmark*	BKM_allocate(tdbb*, RecordSource*, USHORT);
Bookmark*	BKM_lookup(tdbb*, jrd_nod*);
void	BKM_release(tdbb*, jrd_nod*);

#endif /* JRD_BOOKMARK_H */
