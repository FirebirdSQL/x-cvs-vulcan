/*
 *      PROGRAM:        Firebird Y-valve
 *      MODULE:         alt2.c
 *      DESCRIPTION:    Alternative entrypoints
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
 * 21-December-2003	James A. Starkey, cloned from jrd/alt.c
 *
 */

#include "fbdev.h"
#include "common.h"
#include "ibase.h"
#include "../jrd/gds_proto.h"

extern "C" {

void API_ROUTINE isc_vtov(const char * string1, char * string2, SSHORT length)
{
	gds__vtov(string1, string2, length);
}

ISC_LONG API_ROUTINE isc_ftof(const char * string1,
						   USHORT length1, char * string2, USHORT length2)
{
	return gds__ftof(string1, length1, string2, length2);
}

BOOLEAN	WHY_set_shutdown (BOOLEAN flag)
{
	return false;
}

SLONG API_ROUTINE isc_sqlcode(const ISC_STATUS * status_vector)
{
	return gds__sqlcode(status_vector);
}


} // extern "C"
