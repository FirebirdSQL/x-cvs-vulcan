/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version WI-V6.2.972 Firebird 1.0.3 **********************/
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dba.epp
 *	DESCRIPTION:	Database analysis tool
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
 * 2001.08.07 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */


#include "fbdev.h"
#include "../jrd/common.h"
#include "../jrd/ib_stdio.h"
#include "../common/classes/alloc.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "../jrd/ibsetjmp.h"
#include "../jrd/jrd_time.h"
#include "../jrd/y_ref.h"
#include "../jrd/ibase.h"
#include "../jrd/ods.h"
#include "../jrd/btn.h"
#include "../jrd/license.h"
#include "../jrd/msg_encode.h"
#include "../jrd/gdsassert.h"
#ifndef SUPERSERVER
#include "../utilities/gstat/ppg_proto.h"
#endif
#include "../utilities/gstat/dbaswi.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_f_proto.h"
#include "../jrd/thd.h"
#include "../jrd/enc_proto.h"
#ifdef SUPERSERVER
#include "../utilities/common/cmd_util_proto.h"
#include "../jrd/thd_proto.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef WIN_NT
#include <io.h>
#include "../jrd/jrd_pwd.h"
#endif


/* For Netware the follow DB handle and isc_status is #defined to be a  */
/* local variable on the stack in main.  This is to avoid multiple      */
/* threading problems with module level statics.                        */
/*DATABASE DB = STATIC "yachts.lnk";*/
/**** GDS Preprocessor Definitions ****/
#ifndef _JRD_IBASE_H_
#include <ibase.h>
#endif

static const ISC_QUAD
   isc_blob_null = {0,0};	/* initializer for blobs */
static isc_db_handle
   DB = 0;		/* database handle */

static isc_tr_handle
   gds_trans = 0;		/* default transaction handle */
static long
   isc_status [20],	/* status vector */
   isc_status2 [20];	/* status vector */
static long
   isc_array_length, 	/* array return size */
   SQLCODE;		/* SQL status code */
static const char
   isc_tpb_0 [4] = {1,8,2,6};

static const short
   isc_1l = 190;
static const char
   isc_1 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 4,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 11, 'R','D','B','$','I','N','D','I','C','E','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_sort, 1, 
			blr_descending, 
			   blr_field, 0, 14, 'R','D','B','$','I','N','D','E','X','_','N','A','M','E', 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 14, 'R','D','B','$','I','N','D','E','X','_','N','A','M','E', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_field, 0, 12, 'R','D','B','$','I','N','D','E','X','_','I','D', 
			   blr_parameter, 1, 2,0, 
			blr_assignment, 
			   blr_field, 0, 18, 'R','D','B','$','I','N','D','E','X','_','I','N','A','C','T','I','V','E', 
			   blr_parameter, 1, 3,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_1 */

static const short
   isc_9l = 167;
static const char
   isc_9 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 3,0, 
	    blr_long, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_short, 0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 9, 'R','D','B','$','P','A','G','E','S', 0, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 15, 'R','D','B','$','R','E','L','A','T','I','O','N','_','I','D', 
			      blr_parameter, 0, 0,0, 
			   blr_eql, 
			      blr_field, 0, 17, 'R','D','B','$','P','A','G','E','_','S','E','Q','U','E','N','C','E', 
			      blr_literal, blr_long, 0, 0,0,0,0,
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 15, 'R','D','B','$','P','A','G','E','_','N','U','M','B','E','R', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_field, 0, 13, 'R','D','B','$','P','A','G','E','_','T','Y','P','E', 
			   blr_parameter, 1, 2,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_9 */

static const short
   isc_16l = 221;
static const char
   isc_16 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 8,0, 
	    blr_cstring, 32,0, 
	    blr_quad, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_cstring, 254,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 1, 
		  blr_relation, 13, 'R','D','B','$','R','E','L','A','T','I','O','N','S', 0, 
		  blr_sort, 1, 
		     blr_descending, 
			blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			blr_parameter, 0, 0,0, 
		     blr_assignment, 
			blr_field, 0, 12, 'R','D','B','$','V','I','E','W','_','B','L','R', 
			blr_parameter2, 0, 1,0, 6,0, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 2,0, 
		     blr_assignment, 
			blr_field, 0, 15, 'R','D','B','$','R','E','L','A','T','I','O','N','_','I','D', 
			blr_parameter, 0, 3,0, 
		     blr_assignment, 
			blr_field, 0, 17, 'R','D','B','$','E','X','T','E','R','N','A','L','_','F','I','L','E', 
			blr_parameter2, 0, 5,0, 4,0, 
		     blr_assignment, 
			blr_field, 0, 15, 'R','D','B','$','S','Y','S','T','E','M','_','F','L','A','G', 
			blr_parameter, 0, 7,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 2,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_16 */


#define gds_blob_null	isc_blob_null	/* compatibility symbols */
#define gds_status	isc_status
#define gds_status2	isc_status2
#define gds_array_length	isc_array_length
#define gds_count	isc_count
#define gds_slack	isc_slack
#define gds_utility	isc_utility	/* end of compatibility symbols */

#ifndef isc_version4
    Generate a compile-time error.
    Picking up a V3 include file after preprocessing with V4 GPRE.
#endif

/**** end of GPRE definitions ****/

#define DB          db_handle
#define isc_status  status_vector

#define ALLOC(size)	alloc ((size_t) size);
#define BUCKETS		5
#define WINDOW_SIZE	(1 << 17)

typedef struct dba_idx {
	struct dba_idx *idx_next;
	SSHORT idx_id;
	SSHORT idx_depth;
	SLONG idx_leaf_buckets;
	SLONG idx_total_duplicates;
	SLONG idx_max_duplicates;
	SLONG idx_nodes;
	SLONG idx_data_length;
	SLONG idx_fill_distribution[BUCKETS];
	SCHAR idx_name[32];
} *DBA_IDX;

typedef struct dba_rel {
	struct dba_rel *rel_next;
	struct dba_idx *rel_indexes;
	SLONG rel_index_root;
	SLONG rel_pointer_page;
	SLONG rel_slots;
	SLONG rel_data_pages;
	ULONG rel_records;
	ULONG rel_record_space;
	ULONG rel_versions;
	ULONG rel_version_space;
	ULONG rel_max_versions;
	SLONG rel_fill_distribution[BUCKETS];
	ULONG rel_total_space;
	SSHORT rel_id;
	SCHAR rel_name[32];
} *DBA_REL;

/* kidnapped from jrd/pio.h and abused */

typedef struct dba_fil {
	struct dba_fil *fil_next;	/* Next file in database */
	ULONG fil_min_page;			/* Minimum page number in file */
	ULONG fil_max_page;			/* Maximum page number in file */
	USHORT fil_fudge;			/* Fudge factor for page relocation */
#ifdef WIN_NT
	void *fil_desc;
#else
	int fil_desc;
#endif
	USHORT fil_length;			/* Length of expanded file name */
	SCHAR fil_string[1];		/* Expanded file name */
} *DBA_FIL;

static SCHAR *alloc(size_t);
static void analyze_data(DBA_REL, bool);
static bool analyze_data_page(DBA_REL, DPG, bool);
static ULONG analyze_fragments(DBA_REL, RHDF);
static ULONG analyze_versions(DBA_REL, RHDF);
static void analyze_index(DBA_REL, DBA_IDX);

#if (defined WIN_NT)
static void db_error(SLONG);
#else
static void db_error(int);
#endif

static DBA_FIL db_open(const char*, USHORT);
static PAG db_read(SLONG);
#ifdef SUPERSERVER
static void db_close(int);
#endif
static void move(SCHAR *, SCHAR *, SSHORT);
static void print_distribution(SCHAR *, SLONG *);
static void truncate_name(SCHAR *);
static void dba_error(USHORT, TEXT *, TEXT *, TEXT *, TEXT *, TEXT *);
static void dba_print(USHORT, TEXT *, TEXT *, TEXT *, TEXT *, TEXT *);

#ifndef INCLUDE_FB_BLK
#include "../include/fb_blk.h"
#endif

#include "../jrd/db_alias.h"
#include "../jrd/svc.h"
#include "../jrd/svc_proto.h"

#ifdef SUPERSERVER
#include <fcntl.h>
#if (defined WIN_NT)
#include <share.h>
#endif

#include "../jrd/jrd_pwd.h"
#include "../utilities/gstat/ppg_proto.h"

#define FPRINTF		SVC_fprintf

typedef struct open_files {
	int desc;
	struct open_files *open_files_next;
} open_files;

typedef struct dba_mem {
	char *memory;
	struct dba_mem *mem_next;
} dba_mem;
#endif

#ifndef FPRINTF
#define FPRINTF 	ib_fprintf
#endif

/* threading declarations for thread data */

typedef struct tdba {
	struct thdd tdba_thd_data;
	UCHAR *dba_env;
	DBA_FIL files;
	DBA_REL relations;
	SSHORT page_size;
	SLONG page_number;
	PAG buffer1;
	PAG buffer2;
	PAG global_buffer;
	int exit_code;
#ifdef SUPERSERVER
	SVC sw_outfile;
	dba_mem *head_of_mem_list;
	open_files *head_of_files_list;
	SVC dba_service_blk;
#else
	IB_FILE *sw_outfile;
#endif
	ISC_STATUS *dba_status;
	ISC_STATUS_ARRAY dba_status_vector;
} *TDBA;

#ifdef GET_THREAD_DATA
#undef GET_THREAD_DATA
#endif

#ifdef SUPERSERVER
#define GET_THREAD_DATA	        ((TDBA) THD_get_specific(THDD_TYPE_TDBA))
#define SET_THREAD_DATA         {  tddba = &thd_context;           \
                		   THD_put_specific ((THDD) tddba, THDD_TYPE_TDBA);\
				   tddba->tdba_thd_data.thdd_type = THDD_TYPE_TDBA; }
#define RESTORE_THREAD_DATA     THD_restore_specific(THDD_TYPE_TDBA)
#else
static struct tdba *gddba;

#define GET_THREAD_DATA	        (gddba)
#define SET_THREAD_DATA         gddba = tddba = &thd_context; \
				tddba->tdba_thd_data.thdd_type = THDD_TYPE_TDBA
#define RESTORE_THREAD_DATA
#endif

void inline dba_exit(int code, TDBA tddba)
{
	tddba->exit_code = code;
	Firebird::status_exception::raise(0);
}

#define GSTAT_MSG_FAC	21

#if defined (WIN95)
static bool fAnsiCP = false;
#define TRANSLATE_CP(a) if (!fAnsiCP) CharToOem(a, a)
#else
#define TRANSLATE_CP(a)
#endif




#ifdef SUPERSERVER
int main_gstat( SVC service)
#else

int CLIB_ROUTINE main(int argc, char **argv)
#endif
{
   struct {
          char  isc_5 [32];	/* RDB$INDEX_NAME */
          short isc_6;	/* isc_utility */
          short isc_7;	/* RDB$INDEX_ID */
          short isc_8;	/* RDB$INDEX_INACTIVE */
   } isc_4;
   struct {
          char  isc_3 [32];	/* RDB$RELATION_NAME */
   } isc_2;
   struct {
          long isc_13;	/* RDB$PAGE_NUMBER */
          short isc_14;	/* isc_utility */
          short isc_15;	/* RDB$PAGE_TYPE */
   } isc_12;
   struct {
          short isc_11;	/* RDB$RELATION_ID */
   } isc_10;
   struct {
          char  isc_18 [32];	/* RDB$RELATION_NAME */
          ISC_QUAD isc_19;	/* RDB$VIEW_BLR */
          short isc_20;	/* isc_utility */
          short isc_21;	/* RDB$RELATION_ID */
          short isc_22;	/* gds__null_flag */
          char  isc_23 [254];	/* RDB$EXTERNAL_FILE */
          short isc_24;	/* gds__null_flag */
          short isc_25;	/* RDB$SYSTEM_FLAG */
   } isc_17;
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *	Gather information from system relations to do analysis
 *	of a database.
 *
 **************************************/
	IN_SW_TAB in_sw_tab;
	DBA_REL relation;
	DBA_IDX index;
	HDR header;
	DBA_FIL current;
	SCHAR **end, *p, temp[1024], file_name[1024];
	double average;
	bool sw_system = false;
	bool sw_data = false;
	bool sw_index = false;
	bool sw_version = false;
	bool sw_header = false;
	bool sw_log = false;
	bool sw_record = false;
	bool sw_relation = false;
	UCHAR *vp, *vend;
	isc_db_handle db_handle = NULL;
#ifdef SUPERSERVER
	SVC sw_outfile;
	int argc;
	char **argv;
	open_files *open_file, *tmp1;
	dba_mem *alloced, *tmp2;
#else
	IB_FILE *sw_outfile;
#endif
	TEXT *q, *str, c;
	UCHAR *dpb;
	SSHORT dpb_length;
	UCHAR dpb_string[256];
	UCHAR buf[256];
	UCHAR pass_buff[128], user_buff[128], *password = pass_buff, *username =
		user_buff;
	struct tdba thd_context, *tddba;
	JMP_BUF env;
	isc_tr_handle transact1;
	isc_req_handle request1, request2, request3;
#if defined (WIN95) && !defined (SUPERSERVER)
	BOOL fAnsiCP;
#endif

	SET_THREAD_DATA;
	SVC_PUTSPECIFIC_DATA;
	memset(tddba, 0, sizeof(*tddba));
	tddba->dba_env = (UCHAR *) env;

#ifdef SUPERSERVER
/* Reinitialize static variables for multi-threading */
	argc = service->svc_argc;
	argv = service->svc_argv;
#endif

	ISC_STATUS* status_vector = NULL;

	try {

#ifdef SUPERSERVER
	sw_outfile = tddba->sw_outfile = service;
#else
	sw_outfile = tddba->sw_outfile = ib_stdout;
#endif

#if defined (WIN95) && !defined (SUPERSERVER)
	fAnsiCP = false;
#endif

/* Perform some special handling when run as an Interbase service.  The
   first switch can be "-svc" (lower case!) or it can be "-svc_re" followed
   by 3 file descriptors to use in re-directing ib_stdin, ib_stdout, and ib_stderr. */
	tddba->dba_status = tddba->dba_status_vector;
	status_vector = tddba->dba_status;

	bool called_as_service = false;

	if (argc > 1 && !strcmp(argv[1], "-svc")) {
		called_as_service = true;
		argv++;
		argc--;
	}
#ifdef SUPERSERVER
	else if (!strcmp(argv[1], "-svc_thd")) {
		called_as_service = true;
		tddba->dba_service_blk = service;
		tddba->dba_status = service->svc_status;
		argv++;
		argc--;
	}
#endif
	else if (argc > 4 && !strcmp(argv[1], "-svc_re")) {
		called_as_service = true;
		long redir_in = atol(argv[2]);
		long redir_out = atol(argv[3]);
		long redir_err = atol(argv[4]);
#ifdef WIN_NT
#if defined (WIN95) && !defined (SUPERSERVER)
		fAnsiCP = true;
#endif
		redir_in = _open_osfhandle(redir_in, 0);
		redir_out = _open_osfhandle(redir_out, 0);
		redir_err = _open_osfhandle(redir_err, 0);
#endif
		if (redir_in != 0)
			if (dup2((int) redir_in, 0))
				close((int) redir_in);
		if (redir_out != 1)
			if (dup2((int) redir_out, 1))
				close((int) redir_out);
		if (redir_err != 2)
			if (dup2((int) redir_err, 2))
				close((int) redir_err);
		argv += 4;
		argc -= 4;
	}
	char* name = NULL;

	MOVE_CLEAR(user_buff, sizeof(user_buff));
	MOVE_CLEAR(pass_buff, sizeof(pass_buff));

	for (end = argv + argc, ++argv; argv < end;) {
		str = *argv++;
		if (*str == '-') {
			if (!str[1])
				str = "-*NONE*";
			for (in_sw_tab = dba_in_sw_table; q = in_sw_tab->in_sw_name;
				 in_sw_tab++)
			{
				for (p = str + 1; c = *p++;)
					if (UPPER(c) != *q++)
						break;
				if (!c)
					break;
			}
			in_sw_tab->in_sw_state = TRUE;
			if (!in_sw_tab->in_sw) {
				dba_print(20, str + 1, 0, 0, 0, 0);	/* msg 20: unknown switch "%s" */
				dba_print(21, 0, 0, 0, 0, 0);	/* msg 21: Available switches: */
				for (in_sw_tab = dba_in_sw_table; in_sw_tab->in_sw;
					 in_sw_tab++)
					if (in_sw_tab->in_sw_msg)
						dba_print(in_sw_tab->in_sw_msg, 0, 0, 0, 0, 0);
#ifdef SUPERSERVER
				CMD_UTIL_put_svc_status(tddba->dba_service_blk->svc_status,
										GSTAT_MSG_FAC, 1,
										0, NULL,
										0, NULL, 0, NULL, 0, NULL, 0, NULL);
#endif
				dba_error(1, 0, 0, 0, 0, 0);	/* msg 1: found unknown switch */
			}
			else if (in_sw_tab->in_sw == IN_SW_DBA_USERNAME) {
				if (argv < end)
					strcpy((char*) username, *argv++);
			}
			else if (in_sw_tab->in_sw == IN_SW_DBA_PASSWORD) {
				if (argv < end)
					strcpy((char*) password, *argv++);
			}
			else if (in_sw_tab->in_sw == IN_SW_DBA_SYSTEM) {
				sw_system = true;
			}
			else if (in_sw_tab->in_sw == IN_SW_DBA_DATA) {
				sw_data = true;
			}
			else if (in_sw_tab->in_sw == IN_SW_DBA_INDEX) {
				sw_index = true;
			}
			else if (in_sw_tab->in_sw == IN_SW_DBA_VERSION) {
				sw_version = true;
			}
			else if (in_sw_tab->in_sw == IN_SW_DBA_HEADER) {
				sw_header = true;
			}
			else if (in_sw_tab->in_sw == IN_SW_DBA_LOG) {
				sw_log = true;
			}
			else if (in_sw_tab->in_sw == IN_SW_DBA_DATAIDX) {
				sw_index = sw_data = true;
			}
			else if (in_sw_tab->in_sw == IN_SW_DBA_RECORD) {
				sw_record = true;
			}
			else if (in_sw_tab->in_sw == IN_SW_DBA_RELATION) {
				sw_relation = true;
				while (argv < end && **argv != '-') {
					DBA_REL *next;

					if (strlen(*argv) > 31) {
						argv++;
						continue;
					}
					relation = (DBA_REL) ALLOC(sizeof(struct dba_rel));
					strcpy(relation->rel_name, *argv++);
					truncate_name(relation->rel_name);
					relation->rel_id = -1;
					for (next = &tddba->relations; *next;
						 next = &(*next)->rel_next);
					*next = relation;
				}
			}
		}
		else
			name = str;
	}

	if (sw_version)
		dba_print(5, GDS_VERSION, 0, 0, 0, 0);	/* msg 5: gstat version %s */

	if (!name) {
#ifdef SUPERSERVER
		CMD_UTIL_put_svc_status(tddba->dba_service_blk->svc_status,
								GSTAT_MSG_FAC, 2,
								0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);
#endif
		dba_error(2, 0, 0, 0, 0, 0);	/* msg 2: please retry, giving a database name */
	}

	if (!sw_data && !sw_index)
		sw_data = sw_index = true;

	if (sw_record && !sw_data)
		sw_data = true;

#if defined (WIN95) && !defined (SUPERSERVER)
	if (!fAnsiCP) {
		ULONG ulConsoleCP;

		ulConsoleCP = GetConsoleCP();
		if (ulConsoleCP == GetACP())
			fAnsiCP = true;
		else if (ulConsoleCP != GetOEMCP()) {
			FPRINTF(sw_outfile,
					"WARNING: The current codepage is not supported.  Any use of any\n"
					"         extended characters may result in incorrect file names.\n");
		}
	}
#endif

/* Open database and go to work */

	TEXT temp_buf[MAXPATHLEN];
	if (ResolveDatabaseAlias(name, temp_buf))
		name = temp_buf;

	current = db_open(name, strlen(name));
	tddba->page_size = sizeof(temp);
	tddba->global_buffer = (PAG) temp;
	tddba->page_number = -1;
	header = (HDR) db_read((SLONG) 0);

#ifdef SUPERSERVER
	SVC_STARTED(service);
#endif

/* ODS7 was not released as a shipping ODS and was replaced
 * by ODS 8 in version 4.0 of InterBase.  There will not be any
 * customer databases which have an ODS of 7 */

	if (header->hdr_ods_version > ODS_VERSION ||
		(header->hdr_ods_version < ODS_VERSION8 &&
		 header->hdr_ods_version != ODS_VERSION6)) {
#ifdef SUPERSERVER
		CMD_UTIL_put_svc_status(tddba->dba_service_blk->svc_status,
								GSTAT_MSG_FAC, 3,
								isc_arg_number, reinterpret_cast<void*>(ODS_VERSION),
								isc_arg_number, reinterpret_cast<void*>(header->hdr_ods_version),
								0, NULL, 0, NULL, 0, NULL);
#endif
		dba_error(3, (TEXT *) ODS_VERSION, (TEXT *)(ULONG) header->hdr_ods_version,
				  0, 0, 0);		/* msg 3: Wrong ODS version, expected %d, encountered %d? */
	}

#if defined (WIN95) && !defined (SUPERSERVER)
	if (!fAnsiCP)
		AnsiToOem(name, file_name);
	else
#endif
		strcpy(file_name, name);

	dba_print(6, file_name, 0, 0, 0, 0);	/* msg 6: \nDatabase \"%s\"\n */

	tddba->page_size = header->hdr_page_size;
	tddba->buffer1 = (PAG) ALLOC(tddba->page_size);
	tddba->buffer2 = (PAG) ALLOC(tddba->page_size);
	tddba->global_buffer = (PAG) ALLOC(tddba->page_size);
	tddba->page_number = -1;

/* gather continuation files */

	SLONG page = HEADER_PAGE;
	do {
		if (page != HEADER_PAGE)
			current = db_open(file_name, strlen(file_name));
		do {
			header = (HDR) db_read((SLONG) page);
			if (current != tddba->files)
				current->fil_fudge = 1;	/* ignore header page once read it */
			*file_name = '\0';
			for (vp = header->hdr_data, vend = vp + header->hdr_page_size;
				 vp < vend && *vp != HDR_end; vp += 2 + vp[1]) {
				if (*vp == HDR_file) {
					memcpy(file_name, vp + 2, vp[1]);
					*(file_name + vp[1]) = '\0';
#if defined (WIN95) && !defined (SUPERSERVER)
					if (!fAnsiCP)
						AnsiToOem(file_name, file_name);
#endif
				}
				if (*vp == HDR_last_page)
					memcpy(&current->fil_max_page, vp + 2,
						   sizeof(current->fil_max_page));
			}
		}
		while (page = header->hdr_next_page);
		page = current->fil_max_page + 1;	/* first page of next file */
	} while (*file_name);

/* Print header page */

	page = HEADER_PAGE;
	do {
		header = (HDR) db_read((SLONG) page);
		PPG_print_header(header, page, sw_outfile);
		page = header->hdr_next_page;
	} while (page);

	if (sw_header)
		dba_exit(FINI_OK, tddba);

/* print continuation file sequence */

	dba_print(7, 0, 0, 0, 0, 0);
	// msg 7: \n\nDatabase file sequence: 
	for (current = tddba->files; current->fil_next; current = current->fil_next) 
		dba_print(8, current->fil_string, current->fil_next->fil_string, 0, 0, 0);
		//* msg 8: File %s continues as file %s
	dba_print(9, current->fil_string,
		 (TEXT*)((current == tddba->files) ? "only" : "last"), 0, 0, 0);
		 // msg 9: File %s is the %s file\n

/* print log page */

	page = LOG_PAGE;
	do {
		const log_info_page* logp = (const log_info_page*) db_read((SLONG) page);
		PPG_print_log(logp, page, sw_outfile);
		page = logp->log_next_page;
	} while (page);

	if (sw_log)
		dba_exit(FINI_OK, tddba);

/* Check to make sure that the user accessing the database is either
 * SYSDBA or owner of the database */
	dpb = dpb_string;
	*dpb++ = isc_dpb_version1;
	*dpb++ = isc_dpb_gstat_attach;
	*dpb++ = 0;

	if (*username) {
		*dpb++ = isc_dpb_user_name;
		*dpb++ = strlen((char*) username);
		strcpy((char*) dpb, (char*) username);
		dpb += strlen((char*) username);
	}

	if (*password) {
		if (called_as_service)
			*dpb++ = isc_dpb_password_enc;
		else
			*dpb++ = isc_dpb_password;
		*dpb++ = strlen((char*) password);
		strcpy((char*) dpb, (char*) password);
		dpb += strlen((char*) password);
	}

	dpb_length = dpb - dpb_string;

	isc_attach_database(status_vector, 0, name, &DB, dpb_length,
						(char*) dpb_string);
	if (status_vector[1])
		dba_exit(FINI_ERROR, tddba);

	if (sw_version)
		isc_version(&DB, NULL, NULL);

	transact1 = 0;
	/*START_TRANSACTION transact1 READ_ONLY;*/
	{
	isc_start_transaction (isc_status, &transact1, (short) 1, &DB, (short) 4, isc_tpb_0);;
	/*ON_ERROR*/
	if (isc_status [1])
	   {
		dba_exit(FINI_ERROR, tddba);
	/*END_ERROR*/
	   }
	}

	request1 = NULL;
	request2 = NULL;
	request3 = NULL;

	/*FOR(TRANSACTION_HANDLE transact1 REQUEST_HANDLE request1)
		X IN RDB$RELATIONS SORTED BY DESC X.RDB$RELATION_NAME*/
	{
        if (!request1)
           isc_compile_request (isc_status, &DB, &request1, (short) sizeof (isc_16), (char ISC_FAR *) isc_16);
	if (request1)
           isc_start_request (isc_status, &request1, &transact1, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, &request1, (short) 0, (short) 304, &isc_17, (short) 0);
	   if (!isc_17.isc_20 || isc_status [1]) break;

		if (!sw_system && /*X.RDB$SYSTEM_FLAG*/
				  isc_17.isc_25) {
		  continue;
		}
		if (!/*X.RDB$VIEW_BLR.NULL*/
		     isc_17.isc_24 || !/*X.RDB$EXTERNAL_FILE.NULL*/
     isc_17.isc_22) {
			continue;
		}
		if (sw_relation)
		{
			truncate_name(/*X.RDB$RELATION_NAME*/
				      isc_17.isc_18);
			for (relation = tddba->relations; relation; relation = relation->rel_next)
                if (!(strcmp(relation->rel_name, /*X.RDB$RELATION_NAME*/
						 isc_17.isc_18))) {
					relation->rel_id = /*X.RDB$RELATION_ID*/
							   isc_17.isc_21;
					break;
                }
			if (!relation)
				continue;
		}
		else
		{
			relation = (DBA_REL) ALLOC(sizeof(struct dba_rel));
			relation->rel_next = tddba->relations;
			tddba->relations = relation;
			relation->rel_id = /*X.RDB$RELATION_ID*/
					   isc_17.isc_21;
			strcpy(relation->rel_name, /*X.RDB$RELATION_NAME*/
						   isc_17.isc_18);
			truncate_name(relation->rel_name);
		}

		/*FOR(TRANSACTION_HANDLE transact1 REQUEST_HANDLE request2)
			Y IN RDB$PAGES WITH Y.RDB$RELATION_ID EQ relation->rel_id AND
				Y.RDB$PAGE_SEQUENCE EQ 0*/
		{
                if (!request2)
                   isc_compile_request (isc_status, &DB, &request2, (short) sizeof (isc_9), (char ISC_FAR *) isc_9);
		isc_10.isc_11 = relation->rel_id;
		if (request2)
                   isc_start_and_send (isc_status, &request2, &transact1, (short) 0, (short) 2, &isc_10, (short) 0);
		if (!isc_status [1]) {
		while (1)
		   {
                   isc_receive (isc_status, &request2, (short) 1, (short) 8, &isc_12, (short) 0);
		   if (!isc_12.isc_14 || isc_status [1]) break; 

			if (/*Y.RDB$PAGE_TYPE*/
			    isc_12.isc_15 == pag_pointer) {
				relation->rel_pointer_page = /*Y.RDB$PAGE_NUMBER*/
							     isc_12.isc_13;
			}
            if (/*Y.RDB$PAGE_TYPE*/
		isc_12.isc_15 == pag_root) {
				relation->rel_index_root = /*Y.RDB$PAGE_NUMBER*/
							   isc_12.isc_13;
			}
		/*END_FOR;*/
		   }
		   };
		/*ON_ERROR*/
		if (isc_status [1])
		   {
			dba_exit(FINI_ERROR, tddba);
		/*END_ERROR*/
		   }
		}
		if (sw_index)
		/*FOR(TRANSACTION_HANDLE transact1 REQUEST_HANDLE request3)
			Y IN RDB$INDICES WITH Y.RDB$RELATION_NAME EQ relation->rel_name
				SORTED BY DESC Y.RDB$INDEX_NAME*/
		{
                if (!request3)
                   isc_compile_request (isc_status, &DB, &request3, (short) sizeof (isc_1), (char ISC_FAR *) isc_1);
		isc_vtov (relation->rel_name, isc_2.isc_3, 32);
		if (request3)
                   isc_start_and_send (isc_status, &request3, &transact1, (short) 0, (short) 32, &isc_2, (short) 0);
		if (!isc_status [1]) {
		while (1)
		   {
                   isc_receive (isc_status, &request3, (short) 1, (short) 38, &isc_4, (short) 0);
		   if (!isc_4.isc_6 || isc_status [1]) break; 
            if (/*Y.RDB$INDEX_INACTIVE*/
		isc_4.isc_8)
				  continue;
			index = (DBA_IDX) ALLOC(sizeof(struct dba_idx));
			index->idx_next = relation->rel_indexes;
			relation->rel_indexes = index;
			strcpy(index->idx_name, /*Y.RDB$INDEX_NAME*/
						isc_4.isc_5);
			truncate_name(index->idx_name);
			index->idx_id = /*Y.RDB$INDEX_ID*/
					isc_4.isc_7 - 1;
        /*END_FOR;*/
	   }
	   };
		/*ON_ERROR*/
		if (isc_status [1])
		   {
			dba_exit(FINI_ERROR, tddba);
		/*END_ERROR*/
		   }
		}
	/*END_FOR;*/
	   }
	   };
	/*ON_ERROR*/
	if (isc_status [1])
	   {
		dba_exit(FINI_ERROR, tddba);
	/*END_ERROR*/
	   }
	}

	if (request1) {
		isc_release_request(status_vector, &request1);
	}
	if (request2) {
		isc_release_request(status_vector, &request2);
	}
	if (request3) {
		isc_release_request(status_vector, &request3);
	}

	/*COMMIT transact1;*/
	{
	isc_commit_transaction (isc_status, &transact1);;
	/*ON_ERROR*/
	if (isc_status [1])
	   {
		dba_exit(FINI_ERROR, tddba);
	/*END_ERROR*/
	   }
	}
	// FINISH; error!
	/*FINISH*/
	{
	if (DB)
	   isc_detach_database (isc_status, &DB);;
	/*ON_ERROR*/
	if (isc_status [1])
	   {
		dba_exit(FINI_ERROR, tddba);
	/*END_ERROR*/
	   }
	}

	dba_print(10, 0, 0, 0, 0, 0);
	// msg 10: \nAnalyzing database pages ...\n 

	for (relation = tddba->relations; relation; relation = relation->rel_next)
	{
		if (relation->rel_id == -1) {
			continue;
		}
		if (sw_data) {
			analyze_data(relation, sw_record);
		}
		for (index = relation->rel_indexes; index; index = index->idx_next) {
			analyze_index(relation, index);
		}
	}

/* Print results */

	for (relation = tddba->relations; relation; relation = relation->rel_next)
	{
		if (relation->rel_id == -1) {
			continue;
		}
		FPRINTF(sw_outfile, "%s (%d)\n", relation->rel_name,
				relation->rel_id);
		if (sw_data)
		{
			dba_print(11, (TEXT *) relation->rel_pointer_page,
					  (TEXT *) relation->rel_index_root, 0, 0, 0);
			// msg 11: "    Primary pointer page: %ld, Index root page: %ld" 
			if (sw_record) {
				average = (relation->rel_records) ?
					(double) relation->rel_record_space /
					relation->rel_records : 0.0;
				sprintf((char*) buf, "%.2f", average);
				FPRINTF(sw_outfile,
						"    Average record length: %s, total records: %ld\n",
						buf, relation->rel_records);
				// dba_print(18, buf, relation->rel_records, 0, 0, 0);
				// msg 18: "    Average record length: %s, total records: %ld 
				average = (relation->rel_versions) ?
					(double) relation->rel_version_space /
					relation->rel_versions : 0.0;
				sprintf((char*) buf, "%.2f", average);
				FPRINTF(sw_outfile,
						"    Average version length: %s, total versions: %ld, max versions: %ld\n",
						buf, relation->rel_versions,
						relation->rel_max_versions);
				// dba_print(19, buf, relation->rel_versions, 
				//			 relation->rel_max_versions, 0, 0);
				// msg 19: " Average version length: %s, total versions: %ld, max versions: %ld 
			}

			average = (relation->rel_data_pages) ?
				(double) relation->rel_total_space * 100 /
				((double) relation->rel_data_pages *
				 (tddba->page_size - DPG_SIZE)) : 0;
			sprintf((char*) buf, "%.0f%%", average);
			dba_print(12, (TEXT*) relation->rel_data_pages, (TEXT*) relation->rel_slots,
					  (TEXT*) buf, 0, 0);	/* msg 12: "    Data pages: %ld, data page slots: %ld, average fill: %s */
			dba_print(13, 0, 0, 0, 0, 0);	/* msg 13: "    Fill distribution:" */
			print_distribution("\t", relation->rel_fill_distribution);
		}
		FPRINTF(sw_outfile, "\n");
		for (index = relation->rel_indexes; index; index = index->idx_next)
		{
			dba_print(14, index->idx_name, (TEXT *)(SLONG) index->idx_id, 0, 0, 0);
			// msg 14: "    Index %s (%d)" 
			dba_print(15, (TEXT *)(SLONG) index->idx_depth,
					  (TEXT *) index->idx_leaf_buckets,
					  (TEXT *) index->idx_nodes, 0, 0);
			// msg 15: \tDepth: %d, leaf buckets: %ld, nodes: %ld 
			average = (index->idx_nodes) ?
				index->idx_data_length / index->idx_nodes : 0;
			sprintf((char*) buf, "%.2f", average);
			dba_print(16, (TEXT*) buf, (TEXT*) index->idx_total_duplicates,
					  (TEXT*) index->idx_max_duplicates, 0, 0);
			// msg 16: \tAverage data length: %s, total dup: %ld, max dup: %ld"
			dba_print(17, 0, 0, 0, 0, 0);
			// msg 17: \tFill distribution:
			print_distribution("\t    ", index->idx_fill_distribution);
			FPRINTF(sw_outfile, "\n");
		}
	}

	dba_exit(FINI_OK, tddba);

	}	// try
	catch (const std::exception&)
	{
		/* free mem */

		if (status_vector[1])
		{
#ifdef SUPERSERVER
			ISC_STATUS* status = tddba->dba_service_blk->svc_status;
			if (status != status_vector)
			{
			    int i = 0, j;
				while (*status && (++i < ISC_STATUS_LENGTH)) {
					status++;
				}
				for (j = 0; status_vector[j] && (i < ISC_STATUS_LENGTH);
					 j++, i++)
				{
					*status++ = status_vector[j];
				}
			}
#endif
			ISC_STATUS* vector = status_vector;
			SCHAR s[1024];
			if (isc_interprete(s, &vector))
			{
				FPRINTF(tddba->sw_outfile, "%s\n", s);
				s[0] = '-';
				while (isc_interprete(s + 1, &vector)) {
					FPRINTF(tddba->sw_outfile, "%s\n", s);
				}
			}
		}

		/* if there still exists a database handle, disconnect from the
		 * server
		 */
		/*FINISH*/
		{
		if (DB)
		   isc_detach_database ((long*) 0L, &DB);
		};

#ifdef SUPERSERVER
		SVC_STARTED(service);
		alloced = tddba->head_of_mem_list;
		while (alloced != 0) {
			delete[] alloced->memory;
			alloced = alloced->mem_next;
		}

		/* close files */
		open_file = tddba->head_of_files_list;
		while (open_file) {
			db_close(open_file->desc);
			open_file = open_file->open_files_next;
		}

		/* free linked lists */
		while (tddba->head_of_files_list != 0) {
			tmp1 = tddba->head_of_files_list;
			tddba->head_of_files_list =
				tddba->head_of_files_list->open_files_next;
			delete tmp1;
		}

		while (tddba->head_of_mem_list != 0) {
			tmp2 = tddba->head_of_mem_list;
			tddba->head_of_mem_list = tddba->head_of_mem_list->mem_next;
			delete tmp2;
		}

		service->svc_handle = 0;
		if (service->svc_service->in_use != NULL)
			*(service->svc_service->in_use) = FALSE;

		/* Mark service thread as finished and cleanup memory being
		 * used by service in case if client detached from the service
		 */

		SVC_finish(service, SVC_finished);
#endif

		int exit_code = tddba->exit_code;
		RESTORE_THREAD_DATA;

#ifdef SUPERSERVER
		return exit_code;
#else
		exit(exit_code);
#endif
	}

	return 0;
}


static char *alloc(size_t size)
{
/**************************************
 *
 *	a l l o c
 *
 **************************************
 *
 * Functional description
 *	Allocate and zero a piece of memory.
 *
 **************************************/
	char *block, *p;

#ifdef SUPERSERVER

	TDBA tddba = GET_THREAD_DATA;

	block = p = FB_NEW(*getDefaultMemoryPool()) SCHAR[size];
	if (!p) {
		/* NOMEM: return error */
		dba_error(31, 0, 0, 0, 0, 0);
	}
/* note: shouldn't we check for the NULL?? */
	do
		*p++ = 0;
	while (--size);

	dba_mem* mem_list = FB_NEW(*getDefaultMemoryPool()) dba_mem;
	if (!mem_list) {
		/* NOMEM: return error */
		dba_error(31, 0, 0, 0, 0, 0);
	}
	mem_list->memory = block;
	mem_list->mem_next = 0;

	if (tddba->head_of_mem_list == 0)
		tddba->head_of_mem_list = mem_list;
	else {
		mem_list->mem_next = tddba->head_of_mem_list;
		tddba->head_of_mem_list = mem_list;
	}
#else
	block = p = (char*) gds__alloc(size);
	if (!p) {
		/* NOMEM: return error */
		dba_error(31, 0, 0, 0, 0, 0);
	}
	do
		*p++ = 0;
	while (--size);
#endif

	return block;
}


static void analyze_data( DBA_REL relation, bool sw_record)
{
/**************************************
 *
 *	a n a l y z e _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Analyze data pages associated with relation.
 *
 **************************************/
	PPG pointer_page;
	SLONG next_pp, *ptr, *end;
	TDBA tddba;

	tddba = GET_THREAD_DATA;

	pointer_page = (PPG) tddba->buffer1;

	for (next_pp = relation->rel_pointer_page; next_pp;
		 next_pp = pointer_page->ppg_next) {
		move((SCHAR*) db_read(next_pp), (SCHAR*) pointer_page, tddba->page_size);
		for (ptr = pointer_page->ppg_page, end =
			 ptr + pointer_page->ppg_count; ptr < end; ptr++) {
			++relation->rel_slots;
			if (*ptr) {
				++relation->rel_data_pages;
				if (!analyze_data_page(relation, (DPG) db_read(*ptr), sw_record))
					dba_print(18, (TEXT *) * ptr, 0, 0, 0, 0);
					// msg 18: "    Expected data on page %ld" */
			}
		}
	}
}


static bool analyze_data_page( DBA_REL relation, DPG page, bool sw_record)
{
/**************************************
 *
 *	a n a l y z e _ d a t a _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Analyze space utilization for data page.
 *
 **************************************/
	SSHORT bucket, space;
	RHDF header;
	TDBA tddba;
	struct dpg::dpg_repeat *tail, *end;

	tddba = GET_THREAD_DATA;

	if (page->dpg_header.pag_type != pag_data)
		return false;

	if (sw_record) {
		move((SCHAR*) page, (SCHAR*) tddba->buffer2, tddba->page_size);
		page = (DPG) tddba->buffer2;
	}

	space = page->dpg_count * sizeof(struct dpg::dpg_repeat);

	for (tail = page->dpg_rpt, end = tail + page->dpg_count; tail < end;
		 tail++)
		if (tail->dpg_offset && tail->dpg_length) {
			space += tail->dpg_length;
			if (sw_record) {
				header = (RHDF) ((SCHAR *) page + tail->dpg_offset);
				if (!
					(header->
					 rhdf_flags & (rhd_blob | rhd_chain | rhd_fragment))) {
					++relation->rel_records;
					relation->rel_record_space += tail->dpg_length;
					if (header->rhdf_flags & rhd_incomplete) {
						relation->rel_record_space -= RHDF_SIZE;
						relation->rel_record_space +=
							analyze_fragments(relation, header);
					}
					else
						relation->rel_record_space -= RHD_SIZE;

					if (header->rhdf_b_page)
						relation->rel_version_space +=
							analyze_versions(relation, header);
				}
			}
		}

	relation->rel_total_space += space;
	bucket = (space * BUCKETS) / (tddba->page_size - DPG_SIZE);

	if (bucket == BUCKETS)
		--bucket;

	++relation->rel_fill_distribution[bucket];

	return true;
}


static ULONG analyze_fragments( DBA_REL relation, RHDF header)
{
/**************************************
 *
 *	a n a l y z e _ f r a g m e n t s
 *
 **************************************
 *
 * Functional description
 *	Analyze space used by a record's fragments.
 *
 **************************************/
	ULONG space;
	SLONG f_page;
	USHORT f_line;
	TDBA tddba;
	DPG page;
	struct dpg::dpg_repeat *index;

	tddba = GET_THREAD_DATA;
	space = 0;

	while (header->rhdf_flags & rhd_incomplete) {
		f_page = header->rhdf_f_page;
		f_line = header->rhdf_f_line;
		page = (DPG) db_read(f_page);
		if (page->dpg_header.pag_type != pag_data ||
			page->dpg_relation != relation->rel_id ||
			page->dpg_count <= f_line) break;
		index = &page->dpg_rpt[f_line];
		if (!index->dpg_offset)
			break;
		space += index->dpg_length;
		space -= RHDF_SIZE;
		header = (RHDF) ((SCHAR *) page + index->dpg_offset);
	}

	return space;
}


static void analyze_index( DBA_REL relation, DBA_IDX index)
{
/**************************************
 *
 *	a n a l y z e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	TDBA tddba = GET_THREAD_DATA;

	IRT index_root = (IRT) db_read(relation->rel_index_root);

	SLONG page;
	if (index_root->irt_count <= index->idx_id ||
		!(page = index_root->irt_rpt[index->idx_id].irt_root)) 
	{
		return;
	}

	BTR bucket = (BTR) db_read(page);
	index->idx_depth = bucket->btr_level + 1;

	UCHAR *pointer;
	IndexNode node;
	while (bucket->btr_level) 
	{
		pointer = BTreeNode::getPointerFirstNode(bucket);
		BTreeNode::readNode(&node, pointer, bucket->btr_header.pag_flags, false);
		bucket = (BTR) db_read(node.pageNumber);
	}

	bool dup;
	bool firstLeafNode = true;
	SLONG number;
	SLONG duplicates = 0;
	// AB: In fact length for KEY should be MAX_KEY (1/4 of used page-size)
	// This value should be kept equal with size declared in btr.h
	//UCHAR key[4096];
	UCHAR* key = (UCHAR*) ALLOC(tddba->page_size / 4);
	UCHAR* p;
	UCHAR* q;
	UCHAR* firstNode;
	USHORT l, space, n, header;
	USHORT key_length = 0;
	while (true)
	{
		++index->idx_leaf_buckets;
		pointer = firstNode = BTreeNode::getPointerFirstNode(bucket);
		while (true) 
		{
			pointer = BTreeNode::readNode(&node, pointer, 
				bucket->btr_header.pag_flags, true);

			if (BTreeNode::isEndBucket(&node, true) || 
				BTreeNode::isEndLevel(&node, true)) 
			{
				break;
			}

			++index->idx_nodes;
			index->idx_data_length += node.length;
			l = node.length + node.prefix;
			if (node.nodePointer == firstNode) {
				dup = BTreeNode::keyEquality(key_length, key, &node);
			}
			else {
				dup = (!node.length) && (l == key_length);
			}
			if (firstLeafNode) {
				dup = false;
				firstLeafNode = false;
			}
			if (dup) {
				++index->idx_total_duplicates;
				++duplicates;
			}
			else {
				if (duplicates > index->idx_max_duplicates) {
					index->idx_max_duplicates = duplicates;
				}
				duplicates = 0;
			}

			key_length = l;
			l = node.length;
			if (l) {
				p = key + node.prefix;
				q = node.data;
				do {
					*p++ = *q++;
				} while (--l);
			}
		}

		if (duplicates > index->idx_max_duplicates) {
			index->idx_max_duplicates = duplicates;
		}

		header = (USHORT)(firstNode - (UCHAR*) bucket);
		space = bucket->btr_length - header;
		n = (space * BUCKETS) / (tddba->page_size - header);
		if (n == BUCKETS) {
			--n;
		}
		++index->idx_fill_distribution[n];

		if (BTreeNode::isEndLevel(&node, true)) {
			break;
		}
		number = page;
		page = bucket->btr_sibling;
		bucket = (BTR) db_read(page);
		if (bucket->btr_header.pag_type != pag_index) {
			dba_print(19, (TEXT *) page, (TEXT *) number, 0, 0, 0);
			// mag 19: "    Expected b-tree bucket on page %ld from %ld"
			break;
		}
	}
}


static ULONG analyze_versions( DBA_REL relation, RHDF header)
{
/**************************************
 *
 *	a n a l y z e _ v e r s i o n s
 *
 **************************************
 *
 * Functional description
 *	Analyze space used by a record's back versions.
 *
 **************************************/
	ULONG space, versions;
	SLONG b_page;
	USHORT b_line;
	TDBA tddba;
	DPG page;
	struct dpg::dpg_repeat *index;

	tddba = GET_THREAD_DATA;
	space = versions = 0;
	b_page = header->rhdf_b_page;
	b_line = header->rhdf_b_line;

	while (b_page) {
		page = (DPG) db_read(b_page);
		if (page->dpg_header.pag_type != pag_data ||
			page->dpg_relation != relation->rel_id ||
			page->dpg_count <= b_line) break;
		index = &page->dpg_rpt[b_line];
		if (!index->dpg_offset)
			break;
		space += index->dpg_length;
		++relation->rel_versions;
		++versions;
		header = (RHDF) ((SCHAR *) page + index->dpg_offset);
		b_page = header->rhdf_b_page;
		b_line = header->rhdf_b_line;
		if (header->rhdf_flags & rhd_incomplete) {
			space -= RHDF_SIZE;
			space += analyze_fragments(relation, header);
		}
		else
			space -= RHD_SIZE;
	}

	if (versions > relation->rel_max_versions)
		relation->rel_max_versions = versions;

	return space;
}


#ifdef WIN_NT
#ifdef SUPERSERVER
static void db_close( int file_desc)
{
/**************************************
 *
 *	d b _ c l o s e ( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *    Close an open file
 *
 **************************************/
	CloseHandle((HANDLE) file_desc);
}
#endif

static void db_error( SLONG status)
{
/**************************************
 *
 *	d b _ e r r o r		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	TEXT s[128];
	TDBA tddba;

	tddba = GET_THREAD_DATA;
	tddba->page_number = -1;

	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
					   NULL,
					   status,
					   GetUserDefaultLangID(),
					   s,
					   sizeof(s),
					   NULL))
			sprintf(s, "unknown Windows NT error %ld", status);

	FPRINTF(tddba->sw_outfile, "%s\n", s);
	dba_exit(FINI_ERROR, tddba);
}


// CVC: This function was using cast to char* for the first param always
// and the callers had to cast their char*'s to UCHAR*, too. Since the
// real parameter is char* and always the usage is char*, I changed signature.
static DBA_FIL db_open(const char* file_name, USHORT file_length)
{
/**************************************
 *
 *	d b _ o p e n		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Open a database file.
 *
 **************************************/

	TDBA tddba = GET_THREAD_DATA;
	
	DBA_FIL fil;

	if (tddba->files) {
		for (fil = tddba->files; fil->fil_next; fil = fil->fil_next);
		fil->fil_next =
			(DBA_FIL) ALLOC(sizeof(struct dba_fil) + strlen(file_name) + 1);
		fil->fil_next->fil_min_page = fil->fil_max_page + 1;
		fil = fil->fil_next;
	}
	else {						/* empty list */

		fil = tddba->files =
			(DBA_FIL) ALLOC(sizeof(struct dba_fil) + strlen(file_name) + 1);
		fil->fil_min_page = 0L;
	}

	fil->fil_next = NULL;
	strcpy(fil->fil_string, file_name);
	fil->fil_length = strlen(file_name);
	fil->fil_fudge = 0;
	fil->fil_max_page = 0L;

	fil->fil_desc = CreateFile(	reinterpret_cast<LPCTSTR>(file_name),
								GENERIC_READ,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL |
								FILE_FLAG_RANDOM_ACCESS,
								0);

	if (fil->fil_desc  == INVALID_HANDLE_VALUE)
	{
#ifdef SUPERSERVER
		CMD_UTIL_put_svc_status(tddba->dba_service_blk->svc_status,
								GSTAT_MSG_FAC, 29,
								isc_arg_string, file_name,
								0, NULL, 0, NULL, 0, NULL, 0, NULL);
		// msg 29: Can't open database file %s
#endif
		db_error(GetLastError());
	}

#ifdef SUPERSERVER
	open_files* file_list = FB_NEW(*getDefaultMemoryPool()) open_files;
	if (!file_list) {
		/* NOMEM: return error */
		dba_error(31, 0, 0, 0, 0, 0);
	}
	file_list->desc = reinterpret_cast<int>(fil->fil_desc);
	file_list->open_files_next = 0;

	if (tddba->head_of_files_list == 0)
		tddba->head_of_files_list = file_list;
	else {
		file_list->open_files_next = tddba->head_of_files_list;
		tddba->head_of_files_list = file_list;
	}
#endif

	return fil;
}


static PAG db_read( SLONG page_number)
{
/**************************************
 *
 *	d b _ r e a d		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Read a database page.
 *
 **************************************/
	SLONG actual_length;
	DBA_FIL fil;
	TDBA tddba;
	LARGE_INTEGER liOffset;

	tddba = GET_THREAD_DATA;

	if (tddba->page_number == page_number)
		return tddba->global_buffer;

	tddba->page_number = page_number;

	for (fil = tddba->files; page_number > (SLONG) fil->fil_max_page
							 && fil->fil_next;) 
		 fil = fil->fil_next;

	page_number -= fil->fil_min_page - fil->fil_fudge;
	liOffset.QuadPart =
		UInt32x32To64((DWORD) page_number, (DWORD) tddba->page_size);
	if (SetFilePointer
		(fil->fil_desc, (LONG) liOffset.LowPart, &liOffset.HighPart,
		 FILE_BEGIN) == (DWORD) -1) {
#ifdef SUPERSERVER
		CMD_UTIL_put_svc_status(tddba->dba_service_blk->svc_status,
								GSTAT_MSG_FAC, 30,
								0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);
		// msg 30: Can't read a database page 
#endif
		db_error(GetLastError());
	}

	if (!ReadFile(	fil->fil_desc,
					tddba->global_buffer,
					tddba->page_size,
					reinterpret_cast<LPDWORD>(&actual_length),
					NULL))
	{
#ifdef SUPERSERVER
		CMD_UTIL_put_svc_status(tddba->dba_service_blk->svc_status,
								GSTAT_MSG_FAC, 30,
								0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);
		// msg 30: Can't read a database page 
#endif
		db_error(GetLastError());
	}
	if (actual_length != tddba->page_size) {
#ifdef SUPERSERVER
		CMD_UTIL_put_svc_status(tddba->dba_service_blk->svc_status,
								GSTAT_MSG_FAC, 4,
								0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);
#endif
		dba_error(4, 0, 0, 0, 0, 0);
		// msg 4: Unexpected end of database file. 
	}

	return tddba->global_buffer;
}
#endif // ifdef WIN_NT


#ifndef WIN_NT
#ifdef SUPERSERVER
static void db_close( int file_desc)
{
/**************************************
 *
 *	d b _ c l o s e
 *
 **************************************
 *
 * Functional description
 *    Close an open file
 *
 **************************************/
	close(file_desc);
}
#endif

static void db_error( int status)
{
/**************************************
 *
 *	d b _ e r r o r
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	SCHAR *p;
	TDBA tddba;

	tddba = GET_THREAD_DATA;
	tddba->page_number = -1;

	/* FIXME: The strerror() function returns the appropriate description
	   string, or an unknown error message if the error code is unknown.
	   EKU: p cannot be NULL! */
#if 1
	FPRINTF(tddba->sw_outfile, "%s\n", strerror(status));
#else
	/* EKU: Old code */
#ifndef VMS
	FPRINTF(tddba->sw_outfile, "%s\n", strerror(status));
#else
	if ((p = strerror(status)) || (p = strerror(EVMSERR, status)))
		FPRINTF(tddba->sw_outfile, "%s\n", p);
	else
		FPRINTF(tddba->sw_outfile, "uninterpreted code %x\n", status);
#endif
#endif
	dba_exit(FINI_ERROR, tddba);
}


// CVC: This function was using cast to char* for the first param always
// and the callers had to cast their char*'s to UCHAR*, too. Since the
// real parameter is char* and always the usage is char*, I changed signature.
static DBA_FIL db_open(const char* file_name, USHORT file_length)
{
/**************************************
 *
 *	d b _ o p e n
 *
 **************************************
 *
 * Functional description
 *	Open a database file.
 *      Put the file on an ordered list.
 *
 **************************************/
	DBA_FIL fil;
	TDBA tddba;
#ifdef SUPERSERVER
	open_files *file_list;
#endif

	tddba = GET_THREAD_DATA;

	if (tddba->files) {
		for (fil = tddba->files; fil->fil_next; fil = fil->fil_next);
		fil->fil_next =
			(DBA_FIL) ALLOC(sizeof(struct dba_fil) + strlen(file_name) + 1);
		fil->fil_next->fil_min_page = fil->fil_max_page + 1;
		fil = fil->fil_next;
	}
	else {						/* empty list */

		fil = tddba->files =
			(DBA_FIL) ALLOC(sizeof(struct dba_fil) + strlen(file_name) + 1);
		fil->fil_min_page = 0L;
	}

	fil->fil_next = NULL;
	strcpy(fil->fil_string, file_name);
	fil->fil_length = strlen(file_name);
	fil->fil_fudge = 0;
	fil->fil_max_page = 0L;

	if ((fil->fil_desc = open(file_name, O_RDONLY)) == -1)
	{
#ifdef SUPERSERVER
		CMD_UTIL_put_svc_status(tddba->dba_service_blk->svc_status,
								GSTAT_MSG_FAC, 29,
								isc_arg_string, file_name,
								0, NULL, 0, NULL, 0, NULL, 0, NULL);
		// msg 29: Can't open database file %s 
#endif
		db_error(errno);
	}

#ifdef SUPERSERVER
	file_list = FB_NEW(*getDefaultMemoryPool()) open_files;
	if (!file_list) {
		/* NOMEM: return error */
		dba_error(31, 0, 0, 0, 0, 0);
	}
	file_list->desc = fil->fil_desc;
	file_list->open_files_next = 0;

	if (tddba->head_of_files_list == 0)
		tddba->head_of_files_list = file_list;
	else {
		file_list->open_files_next = tddba->head_of_files_list;
		tddba->head_of_files_list = file_list;
	}
#endif

	return fil;
}


static PAG db_read( SLONG page_number)
{
/**************************************
 *
 *	d b _ r e a d
 *
 **************************************
 *
 * Functional description
 *	Read a database page.
 *
 **************************************/
	SCHAR *p;
	SSHORT length, l;
	DBA_FIL fil;
	TDBA tddba;
	UINT64 offset;

	tddba = GET_THREAD_DATA;

	if (tddba->page_number == page_number)
		return tddba->global_buffer;

	tddba->page_number = page_number;

	for (fil = tddba->files; page_number > (SLONG) fil->fil_max_page
							 && fil->fil_next;) 
		fil = fil->fil_next;

	page_number -= fil->fil_min_page - fil->fil_fudge;
	offset = ((UINT64)page_number) * ((UINT64)tddba->page_size);
	if (lseek (fil->fil_desc, offset, 0) == -1) {
#ifdef SUPERSERVER
		CMD_UTIL_put_svc_status(tddba->dba_service_blk->svc_status,
								GSTAT_MSG_FAC, 30,
								0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);
		// msg 30: Can't read a database page 
#endif
		db_error(errno);
	}

	for (p = (SCHAR *) tddba->global_buffer, length = tddba->page_size;
		 length > 0;) {
		l = read(fil->fil_desc, p, length);
		if (l < 0) {
#ifdef SUPERSERVER
			CMD_UTIL_put_svc_status(tddba->dba_service_blk->svc_status,
									GSTAT_MSG_FAC, 30,
									0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);
			// msg 30: Can't read a database page 
#endif
			db_error(errno);
		}
		if (!l) {
#ifdef SUPERSERVER
			CMD_UTIL_put_svc_status(tddba->dba_service_blk->svc_status,
									GSTAT_MSG_FAC, 4,
									0, NULL,
									0, NULL, 0, NULL, 0, NULL, 0, NULL);
#endif
			dba_error(4, 0, 0, 0, 0, 0);
			// msg 4: Unexpected end of database file. 
		}
		p += l;
		length -= l;
	}

	return tddba->global_buffer;
}
#endif


static void dba_error(
					  USHORT errcode,
					  TEXT * arg1,
					  TEXT * arg2, TEXT * arg3, TEXT * arg4, TEXT * arg5)
{
/**************************************
 *
 *	d b a _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Format and print an error message, then punt.
 *
 **************************************/
	TDBA tddba;

	tddba = GET_THREAD_DATA;
	tddba->page_number = -1;

	dba_print(errcode, arg1, arg2, arg3, arg4, arg5);
	dba_exit(FINI_ERROR, tddba);
}


static void dba_print(
					  USHORT number,
					  TEXT * arg1,
					  TEXT * arg2, TEXT * arg3, TEXT * arg4, TEXT * arg5)
{
/**************************************
 *
 *	d b a _ p r i n t
 *
 **************************************
 *
 * Functional description
 *      Retrieve a message from the error file, format it, and print it.
 *
 **************************************/
	TEXT buffer[256];
	TDBA tddba;

	tddba = GET_THREAD_DATA;

	gds__msg_format(NULL, GSTAT_MSG_FAC, number, sizeof(buffer), buffer,
					arg1, arg2, arg3, arg4, arg5);
	TRANSLATE_CP(buffer);
	FPRINTF(tddba->sw_outfile, "%s\n", buffer);
}


static void move( SCHAR * from, SCHAR * to, SSHORT length)
{
/**************************************
 *
 *	m o v e
 *
 **************************************
 *
 * Functional description
 *	Move some stuff.
 *
 **************************************/

	memcpy(to, from, (int) length);
}


static void print_distribution( SCHAR * prefix, SLONG * vector)
{
/**************************************
 *
 *	p r i n t _ d i s t r i b u t i o n
 *
 **************************************
 *
 * Functional description
 *	Print distribution as percentages.
 *
 **************************************/
	SSHORT n;
	TDBA tddba;

	tddba = GET_THREAD_DATA;

	for (n = 0; n < BUCKETS; n++) {
		FPRINTF(tddba->sw_outfile, "%s%2d - %2d%% = %ld\n",
				prefix,
				n * 100 / BUCKETS, (n + 1) * 100 / BUCKETS - 1, vector[n]);
	}
}


static void truncate_name( SCHAR * string)
{
/**************************************
 *
 *	t r u n c a t e _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Zap trailing blanks.
 * CVC: But only "trailing", not embedded blanks.
 *
 **************************************/

	SCHAR* tail;

	for (tail = string - 1; *string; ++string)
    	if (*string != ' ')
			tail = string;
	*++tail = 0;
}

