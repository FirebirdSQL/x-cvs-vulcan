/*
 *	PROGRAM:	JRD Data Definition Utility
 *	MODULE:		dyn.h
 *	DESCRIPTION:	Dynamic data definition local data
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

#ifndef	JRD_DYN_H
#define JRD_DYN_H

const char* const ALL_PRIVILEGES = "SIUDR";
		/* all applicable grant/revoke privileges */
const char* const ALL_PROC_PRIVILEGES = "X";
		/* all applicable grant/revoke privileges for a procedure */
#define DYN_MSG_FAC		8
#define STUFF_COUNT		4
#define TEXT_BLOB_LENGTH	512


#define GET_STRING(from,to)	DYN_get_string (tdbb, (const UCHAR**)from, (TEXT*)to, sizeof (to), true)

#define GET_STRING_2(from,to)	DYN_get_string (tdbb, (const UCHAR**)from, (TEXT*)to, sizeof (to), false)


typedef struct gbl
{
	class Transaction* gbl_transaction;
} *GBL;

typedef struct dyn_fld {
	DSC dyn_dsc;
	BOOLEAN dyn_null_flag;
	USHORT dyn_dtype;
	USHORT dyn_precision;
	USHORT dyn_charlen;
	SSHORT dyn_collation;
	SSHORT dyn_charset;
	SqlIdentifier dyn_fld_source;
	SqlIdentifier dyn_rel_name;
	SqlIdentifier dyn_fld_name;
    USHORT dyn_charbytelen; /* Used to check modify operations on string types. */

} *DYN_FLD;

CLASS (OSRIException);
struct bid;

void	DYN_error(thread_db* tdbb, bool, USHORT, const TEXT*, const TEXT*, const TEXT*,
				const TEXT*, const TEXT*);
void	DYN_error_punt(thread_db* tdbb, bool, USHORT, const TEXT*, const TEXT*,
				const TEXT*, const TEXT*, const TEXT*);
void	DYN_error_punt(thread_db* tdbb, OSRIException*, USHORT, const TEXT*, const TEXT*,
				const TEXT*, const TEXT*, const TEXT*);
void	DYN_execute(thread_db* tdbb, GBL, const UCHAR**, const TEXT*, TEXT*, TEXT*, TEXT*, TEXT*);
SLONG	DYN_get_number(const UCHAR**);
USHORT	DYN_get_string(thread_db* tdbb, const UCHAR**, TEXT*, USHORT, bool);

// This function is not defined anywhere.
// void	DYN_get_string2(TEXT**, TEXT*, USHORT);

// This function doesn't need to be exported currently.
bool	DYN_is_it_sql_role(GBL, const TEXT*, TEXT*, thread_db*);
USHORT	DYN_put_blr_blob(thread_db* tdbb, GBL, const UCHAR**, bid*);

// This function is not defined anywhere.
//USHORT	DYN_put_blr_blob2(GBL, const UCHAR**, ISC_QUAD *);

USHORT	DYN_put_text_blob(thread_db* tdbb, GBL, const UCHAR**, bid*);
// This function is not defined anywhere.
//USHORT	DYN_put_text_blob2(GBL, const UCHAR**, ISC_QUAD *);

void	DYN_rundown_request(thread_db* tdbb, Request*, SSHORT);
USHORT	DYN_skip_attribute(const UCHAR**);

// This function is not defined anywhere.
//USHORT	DYN_skip_attribute2(const UCHAR**);

void	DYN_unsupported_verb(thread_db* tdbb);

#endif // JRD_DYN_H

