/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Service.cpp
 *	DESCRIPTION:	Service manager functions
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "EPSON" define
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "IMP" port
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DELTA" port
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "IMP" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#include <stdarg.h>
#include <string.h>

#include "fbdev.h"
#include "../jrd/common.h"
#include "../jrd/ib_stdio.h"
#include "../jrd/jrd_time.h"
#include "../jrd/file_params.h"
//#include "../jrd/jrd.h"
#include "../jrd/svc.h"
//#include "../jrd/jrd_pwd.h"
#include "../alice/aliceswi.h"
#include "../burp/burpswi.h"
#include "../jrd/ibase.h"
#include "gen/iberror.h"
#include "../jrd/license.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/svc_proto.h"
#include "../jrd/thd_proto.h"
#include "../jrd/utl_proto.h"
//#include "../jrd/enc_proto.h"
#include "../utilities/gsec/gsecswi.h"
#include "../utilities/gstat/dbaswi.h"
#include "PBGen.h"
#include "InfoGen.h"
#include "Mutex.h"
#include "OSRIException.h"
#include "Sync.h"
#include "gdsassert.h"
#include "EngineInfoGen.h"
#include "ConfObject.h"
#include "../jrd/Stack.h"

#ifdef SERVER_SHUTDOWN
#include "../jrd/jrd_proto.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_VFORK_H
#include <vfork.h>
#endif

#ifdef sparc
#ifdef SOLARIS
#include <fcntl.h>
#endif
#endif

#ifdef SCO_UNIX
#include <fcntl.h>
#endif

/* This is defined in JRD/SCL.H, but including it causes
 * a linker warning.
 */
#define SYSDBA_USER_NAME	"SYSDBA"
#define SVC_user_dba		2
#define SVC_user_any		1
#define SVC_user_none		0

#if !defined(WIN_NT)
#  include <signal.h>
#  ifndef VMS
#    include <sys/param.h>
#    include <sys/stat.h>
#  else
#    include <stat.h>
#  endif
#  include <errno.h>
#else
#  include <windows.h>
#  include <io.h> // _open, _get_osfhandle
#  include <stdlib.h>
#  include <fcntl.h>
#  include <sys/stat.h>
#  define SYS_ERR		isc_arg_win32
#ifndef strcasecmp
#define strcasecmp		stricmp
#define strncasecmp		strnicmp
#endif
#endif


#ifndef SYS_ERR
#define SYS_ERR		isc_arg_unix
#endif

#ifdef VMS
#define waitpid(x,y,z)	wait (y)
#endif

#define statistics	stat

#define GET_LINE	1
#define GET_EOF		2
#define GET_BINARY	4

#define	SVC_TRMNTR	'\377'

#ifdef HAVE_GETTIMEOFDAY
#ifndef GETTIMEOFDAY
#define GETTIMEOFDAY(tv)	gettimeofday (tv, 0);
#endif
#endif

/* This checks if the service has forked a process.  If not,
   it will post the isc_svcnoexe error. */

static inline void is_service_running(const Service* service)
{
	if (!(service->svc_flags & SVC_forked)) 
		{
		THREAD_ENTER;
		SVC_post (isc_svcnoexe, isc_arg_string, service->svc_service->serv_name, 0); 
		}
}

static inline void need_admin_privs(ISC_STATUS** status, const char* message)
{
	ISC_STATUS* stat = *status;
	*stat++ = isc_insufficient_svc_privileges;
	*stat++ = isc_arg_string;
	*stat++ = (ISC_STATUS) message; 
	*stat++ = isc_arg_end;
	*status = stat;
}

bool ck_space_for_numeric(UCHAR*& info, const UCHAR* const end)
{
	if ((info + 1 + sizeof (ULONG)) > end)
		{
		if (info < end)
			*info++ = isc_info_truncated;
			
		THREAD_ENTER;
		return false;
		}
		
	return true;
}


/* Option block for service parameter block */

typedef struct spb {
	TEXT *spb_sys_user_name;
	TEXT *spb_user_name;
	TEXT *spb_password;
	TEXT *spb_password_enc;
	TEXT *spb_command_line;
	USHORT spb_version;
} SPB;

static JString conv_switches(USHORT, USHORT, const UCHAR*);
static const TEXT* find_switch(int, IN_SW_TAB);
static USHORT process_switches(USHORT, const UCHAR*, TEXT*);
static void get_options(const UCHAR*, USHORT, TEXT*, SPB*);
static TEXT* get_string_parameter(const UCHAR**, TEXT**);
#ifndef SUPERSERVER
static void io_error(const TEXT *, SLONG, const TEXT *, ISC_STATUS, BOOLEAN);
static void service_close(Service*);
#endif
static BOOLEAN get_action_svc_bitmask(const UCHAR**, const IN_SW_TAB, TEXT**, USHORT*,
									  USHORT*);
static void get_action_svc_string(const UCHAR**, TEXT**, USHORT*, USHORT*);
static void get_action_svc_data(const UCHAR**, TEXT**, USHORT*, USHORT*);
static BOOLEAN get_action_svc_parameter(const UCHAR**, IN_SW_TAB, TEXT**, USHORT*, USHORT*);

#ifdef SUPERSERVER
static UCHAR service_dequeue_byte(Service*);
static void service_enqueue_byte(UCHAR, Service*);
static USHORT service_add_one(USHORT i);
static USHORT service_empty(Service* service);
static USHORT service_full(Service* service);
static void service_fork(void (*)(), Service*);
#else
static void service_fork(TEXT *, Service*);
#endif

static void service_get(Service*, UCHAR*, USHORT, USHORT, USHORT, USHORT *);
static void service_put(Service*, const UCHAR*, USHORT);

#if !defined(WIN_NT) && !defined(SUPERSERVER)
static void timeout_handler(void *service);
#endif

#if defined(WIN_NT) && !defined(SUPERSERVER)
static USHORT service_read(Service*, UCHAR *, USHORT, USHORT);
#endif

#ifdef DEBUG
int test_thread(Service*);
void test_cmd(USHORT, SCHAR *, TEXT **);
#define TEST_THREAD test_thread
#define TEST_CMD test_cmd
#else
#define TEST_THREAD NULL
#define TEST_CMD NULL
#endif

#ifdef SERVER_SHUTDOWN
static shutdown_fct_t shutdown_fct = 0;
static ULONG shutdown_param = 0L;
#endif

#define SPB_SEC_USERNAME	"isc_spb_sec_username"

//static MUTX_T svc_mutex[1], thd_mutex[1];
static Mutex	thd_mutex;
static Mutex	svc_mutex;
static BOOLEAN svc_initialized = FALSE, thd_initialized = FALSE;

/* Service Functions */
#ifdef SUPERSERVER
#include "../burp/burp_proto.h"
#include "../alice/alice_proto.h"
#include "../utilities/gsec/gsec_proto.h"

int main_lock_print();
//int main_gstat(Service* service);
THREAD_ENTRY_DECLARE main_gstat(THREAD_ENTRY_PARAM arg);

#define MAIN_GBAK		BURP_main
#define MAIN_GFIX		ALICE_main
#define MAIN_WAL_PRINT	main_wal_print
#define MAIN_LOCK_PRINT	main_lock_print
#define MAIN_GSTAT		main_gstat
#define MAIN_GSEC		GSEC_main
#else
#define MAIN_GBAK		NULL
#define MAIN_GFIX		NULL
#define MAIN_WAL_PRINT	NULL
#define MAIN_LOCK_PRINT	NULL
#define MAIN_GSTAT		NULL
#define MAIN_GSEC		NULL
#endif

void SVC_STATUS_ARG(ISC_STATUS*& status, USHORT type, const void* value)
{
	if (value)
		switch (type)
			{
			case isc_arg_number:
				*status++ = type;
				*status++ = reinterpret_cast<ISC_STATUS>(value);
				break;
				
			case isc_arg_string:
				*status++ = type;
				*status++ = (ISC_STATUS)
				static_cast<const char*>(value);
				break;
				
			default:
				break;
			}
}

/* Entries which have a NULL serv_executable field will not fork
   a process on the server, but will establish a valid connection
   which can be used for isc_info_svc calls.

  The format of the structure is:

  isc_action_svc call,
  old service name (for compatibility),
  old cmd-line switches (for compatibility),
  executable to fork (for compatibility),
  thread to execute,
  in use flag (for compatibility)
*/

static const serv services[] =
{

	{ isc_action_max, "print_cache", "-Service", "bin/fb_cache_print", NULL, 0 },
	{ isc_action_max, "print_locks", "-Service", "bin/fb_lock_print", NULL, 0 },
	{ isc_action_max, "start_cache", "-Service", "bin/fb_cache_manager", NULL, 0 },
	{ isc_action_max, "analyze_database", "-Service", "bin/gstat", NULL, 0 },
	{ isc_action_max, "backup", "-Service -b", "bin/gbak",	MAIN_GBAK, 0 },
	{ isc_action_max, "create", "-Service -c", "bin/gbak",	MAIN_GBAK, 0 },
	{ isc_action_max, "restore", "-Service -r", "bin/gbak",	MAIN_GBAK, 0 },
	{ isc_action_max, "gdef", "-Service", "bin/gdef", NULL, 0 },
	{ isc_action_max, "gsec", "-Service", "bin/gsec", NULL, 0 },
	{ isc_action_max, "disable_journal", "-Service -disable", "bin/gjrn", NULL, 0 },
	{ isc_action_max, "dump_journal", "-Service -online_dump", "bin/gjrn", NULL, 0 },
	{ isc_action_max, "enable_journal", "-Service -enable", "bin/gjrn", NULL, 0 },
	{ isc_action_max, "monitor_journal", "-Service -console", "bin/gjrn", NULL, 0 },
	{ isc_action_max, "query_server", NULL, NULL, NULL, 0 },
	{ isc_action_max, "start_journal", "-Service -server", "bin/gjrn", NULL, 0 },
	{ isc_action_max, "stop_cache", "-Service -shut -cache", "bin/gfix", NULL, 0 },
	{ isc_action_max, "stop_journal", "-Service -console", "bin/gjrn", NULL, 0 },
	{ isc_action_max, "anonymous", NULL, NULL, NULL, 0 },

/* NEW VERSION 2 calls, the name field MUST be different from those names above
 */
	{ isc_action_max, "service_mgr", NULL, NULL, NULL, 0 },
	{ isc_action_svc_backup, "Backup Database", NULL, "bin/gbak",	MAIN_GBAK, 0 },
	{ isc_action_svc_restore, "Restore Database", NULL, "bin/gbak",	MAIN_GBAK, 0 },
	{ isc_action_svc_repair, "Repair Database", NULL, "bin/gfix",	MAIN_GFIX, 0 },
	{ isc_action_svc_add_user, "Add User", NULL, "bin/gsec",	MAIN_GSEC, 0 },
	{ isc_action_svc_delete_user, "Delete User", NULL, "bin/gsec",	MAIN_GSEC, 0 },
	{ isc_action_svc_modify_user, "Modify User", NULL, "bin/gsec",	MAIN_GSEC, 0 },
	{ isc_action_svc_display_user, "Display User", NULL, "bin/gsec",	MAIN_GSEC, 0 },
	{ isc_action_svc_properties, "Database Properties", NULL, "bin/gfix",	MAIN_GFIX, 0 },
	{ isc_action_svc_lock_stats, "Lock Stats", NULL, "bin/fb_lock_print",	TEST_THREAD, 0 },
	{ isc_action_svc_db_stats, "Database Stats", NULL, "bin/gstat",	MAIN_GSTAT, 0 },
	{ isc_action_svc_get_ib_log, "Get Log File", NULL, NULL,	SVC_read_ib_log, 0 },
/* actions with no names are undocumented */
	{ isc_action_svc_set_config, NULL, NULL, NULL,	TEST_THREAD, 0 },
	{ isc_action_svc_default_config, NULL, NULL, NULL,	TEST_THREAD, 0 },
	{ isc_action_svc_set_env, NULL, NULL, NULL,	TEST_THREAD, 0 },
	{ isc_action_svc_set_env_lock, NULL, NULL, NULL,	TEST_THREAD, 0 },
	{ isc_action_svc_set_env_msg, NULL, NULL, NULL,	TEST_THREAD, 0 },
	{ 0, NULL, NULL, NULL, NULL, 0 }
};

/* The SERVER_CAPABILITIES_FLAG is used to mark architectural
** differences across servers.  This allows applications like server
** manager to disable features as necessary.
*/

#ifdef SUPERSERVER
#define SERVER_CAPABILITIES     	REMOTE_HOP_SUPPORT | \
                                    MULTI_CLIENT_SUPPORT | \
			                		SERVER_CONFIG_SUPPORT
#endif /* SUPERSERVER */


#ifndef SERVER_CAPABILITIES
#  define SERVER_CAPABILITIES_FLAG	REMOTE_HOP_SUPPORT | NO_SERVER_SHUTDOWN_SUPPORT
#else

#  ifdef WIN_NT
#    define SERVER_CAPABILITIES_FLAG    SERVER_CAPABILITIES | QUOTED_FILENAME_SUPPORT
#  else

#    define SERVER_CAPABILITIES_FLAG    SERVER_CAPABILITIES | NO_SERVER_SHUTDOWN_SUPPORT
#  endif /* WIN_NT */

#endif /* SERVER_CAPABILITIES */


Service* SVC_attach(ConfObject* configuration, 
					const TEXT*		service_name,
					USHORT			spb_length,
					const UCHAR*	spb)
{
/**************************************
 *
 *	S V C _ a t t a c h
 *
 **************************************
 *
 * Functional description
 *	Connect to a Firebird service.
 *
 **************************************/
	TEXT misc_buf[512];
#ifndef SUPERSERVER
	TEXT service_path[MAXPATHLEN];
#endif
	SPB options;
	//TEXT name[129];
	//int id = 0;
	//int group;
	//int node_id;

	/* If the service name begins with a slash, ignore it. */

	int service_length = (int) strlen(service_name);
	
	if (*service_name == '/' || *service_name == '\\') 
		{
		service_name++;
		
		if (service_length)
			service_length--;
		}
		
	if (service_length) 
		{
		strncpy(misc_buf, service_name, (int) service_length);
		misc_buf[service_length] = 0;
		}
	else 
		strcpy(misc_buf, service_name);

	/* Find the service by looking for an exact match. */
	
	const struct serv* serv;
	
	for (serv = services; serv->serv_name; serv++)
		if (!strcmp(misc_buf, serv->serv_name))
			break;

	if (!serv->serv_name)
		SVC_post(isc_service_att_err, isc_arg_gds, isc_svcnotdef,
				 isc_arg_string, misc_buf, 0);

	//GET_THREAD_DATA;

	/* If anything goes wrong, we want to be able to free any memory
	   that may have been allocated. */

	UCHAR* spb_buf = 0;
	TEXT* switches = 0;
	TEXT* misc = 0;
	Service* service = NULL;

	try 
		{
		/* Insert internal switch SERVICE_THD_PARAM in the service parameter block. */

		const UCHAR* p = spb;
		const UCHAR* const end = spb + spb_length;
		bool cmd_line_found = false;
		
		while (!cmd_line_found && p < end) 
			{
			switch (*p++) 
				{
				case isc_spb_version1:
				case isc_spb_version:
					p++;
					break;

				case isc_spb_sys_user_name:
				case isc_spb_user_name:
				case isc_spb_password:
				case isc_spb_password_enc:
					{
					USHORT length = *p++;
					p += length;
					}
					break;

				case isc_spb_command_line:
					cmd_line_found = true;
					break;
				}
			}

		/* dimitr: it looks that we shouldn't execute the below code
				if the first switch of the command line is "-svc_re",
				but I couldn't find where such a switch is specified
				by any of the client tools, so it seems that in fact
				it's not used at all. Hence I ignore this situation. */
				   
		if (cmd_line_found && p++ < end) 
			{
			USHORT ignored_length = 0;
			
			if (!strncmp((const char*) p, "-Service ", 5))
				ignored_length = 5;
			else if (!strncmp((const char*)p, "-svc_thd ", 9))
				ignored_length = 9;
				
			USHORT param_length = sizeof(SERVICE_THD_PARAM) - 1;
			USHORT spb_buf_length = spb_length + param_length - ignored_length + 1;
			UCHAR* q = spb_buf = new UCHAR [spb_buf_length + 1];
			memcpy(q, spb, p - spb);
			q += p - spb - 1;
			*q++ += param_length - ignored_length + 1;
			memcpy(q, SERVICE_THD_PARAM, param_length);
			q += param_length;
			*q++ = ' ';
			p += ignored_length;
			memcpy(q, p, end - p);
			spb = spb_buf;
			spb_length = spb_buf_length;
			}

		/* Process the service parameter block. */

		if (spb_length > sizeof(misc_buf))
			misc = new TEXT [spb_length];
		else 
			misc = misc_buf;

		get_options(spb, spb_length, misc, &options);

		/* Perhaps checkout the user in the security database. */
		
		USHORT user_flag;
		
		if (!strcmp(serv->serv_name, "anonymous")) 
			user_flag = SVC_user_none;
		else 
			{
			if (!options.spb_user_name)
				// user name and password are required while
				// attaching to the services manager
				SVC_post(isc_service_att_err, isc_arg_gds, isc_svcnouser, 0);

			/***				
			if (options.spb_user_name || id == -1)
				SecurityDatabase::verifyUser(name, options.spb_user_name,
											options.spb_password, options.spb_password_enc,
											&id, &group, &node_id);
			***/
			PBGen gen(isc_dpb_version1);
			gen.putParameter(isc_dpb_user_name, options.spb_user_name);
			
			if (options.spb_password)
				gen.putParameter(isc_dpb_password, options.spb_password);
				
			if (options.spb_password_enc)
				gen.putParameter(isc_dpb_password_enc, options.spb_password_enc);
				
			ISC_STATUS statusVector[ISC_STATUS_LENGTH];
			isc_db_handle dbHandle = 0;
			JString databaseName = configuration->getValue("SecurityDatabase", "security.fdb");

			try
				{			
				if (isc_attach_database(statusVector, 0, databaseName, &dbHandle, gen.getLength(), (char*) gen.buffer))
					throw OSRIException(statusVector);
				}
			catch (...)
				{
				if (dbHandle)
					isc_detach_database(statusVector, &dbHandle);
				throw;
				}

			// AB: detach immediately, for the moment we don't need it anymore.
			if (dbHandle)
				isc_detach_database(statusVector, &dbHandle);

			/* Check that the validated user has the authority to access this service */

			if (strcasecmp(options.spb_user_name, SYSDBA_USER_NAME))
				user_flag = SVC_user_any;
			else 
				user_flag = SVC_user_dba | SVC_user_any;
			}

		/* Allocate a buffer for the service switches, if any.  Then move them in. */

		int len = ((serv->serv_std_switches) ? (int) strlen(serv->serv_std_switches) : 0) +
			((options.spb_command_line) ? (int) strlen(options.spb_command_line) : 0) +
			1;

		if (len > 1)
			switches = new TEXT [len];

		if (switches)
			*switches = 0;
			
		if (serv->serv_std_switches)
			strcpy(switches, serv->serv_std_switches);
			
		if (options.spb_command_line)
			{
			if (serv->serv_std_switches)
				strcat(switches, " ");
			
			strcat(switches, options.spb_command_line);
			}

		/* Services operate outside of the context of databases.  Therefore
		   we cannot use the JRD allocator. */

		service = new Service(configuration);
		//service = FB_NEW(*getDefaultMemoryPool()) Service;
		//memset((void *) service, 0, sizeof(struct Service));
		//service->svc_status = new ISC_STATUS [ISC_STATUS_LENGTH];
		//memset(service->svc_status, 0, ISC_STATUS_LENGTH * sizeof(ISC_STATUS));

		//service->blk_type = type_svc;
		//service->blk_pool_id = 0;
		//service->blk_length = 0;
		service->svc_service = serv;
		//service->svc_flags = serv->serv_executable ? SVC_forked : 0;
		service->svc_flags = (serv->serv_executable ? SVC_forked : 0) | (switches ? SVC_cmd_line : 0);
		service->svc_switches = switches;
		service->svc_handle = 0;
		service->svc_user_flag = user_flag;
		service->svc_do_shutdown = FALSE;
		
#ifdef SUPERSERVER
		service->svc_stdout_head = 1;
		service->svc_stdout_tail = SVC_STDOUT_BUFFER_SIZE;
		service->svc_stdout = NULL;
		service->svc_argv = NULL;
#endif
	
		service->svc_spb_version = options.spb_version;
		
		if (options.spb_user_name)
			service->userName = options.spb_user_name;
			//strcpy(service->svc_username, options.spb_user_name);

		/* The password will be issued to the service threads on NT since
		 * there is no OS authentication.  If the password is not yet
		 * encrypted, then encrypt it before saving it (since there is no
		 * decrypt function).
		 */
		
		if (options.spb_password)
			service->setPassword(options.spb_password);
			/***
			{
			UCHAR* s = reinterpret_cast<UCHAR*>(service->svc_enc_password);
			UCHAR* p = (UCHAR *) ENC_crypt(options.spb_password, PASSWORD_SALT) + 2;
			int l = (int) strlen((char *) p);
			MOVE_FASTER(p, s, l);
			service->svc_enc_password[l] = 0;
			}
			***/

		if (options.spb_password_enc)
			service->setEncryptedPassword(options.spb_password_enc);
			//strcpy(service->svc_enc_password, options.spb_password_enc);

	/* If an executable is defined for the service, try to fork a new process.
	* Only do this if we are working with a version 1 service */

#ifndef SUPERSERVER
		if (serv->serv_executable && options.spb_version == isc_spb_version1)
#else
		if (serv->serv_thd && options.spb_version == isc_spb_version1)
#endif
		{
#ifndef SUPERSERVER
			gds__prefix(service_path, serv->serv_executable);
			service_fork(service_path, service);
#else
			/* if service is single threaded, only call if not currently running */
			
			if (serv->in_use == NULL) 
				service_fork(reinterpret_cast < void (*)() > (serv->serv_thd), service);
			else if (!*(serv->in_use)) 
				{
				*(serv->in_use) = TRUE;
				service_fork(reinterpret_cast < void (*)() > (serv->serv_thd), service);
				}
			else 
				SVC_post(isc_service_att_err, isc_arg_gds,
						isc_svc_in_use, isc_arg_string, serv->serv_name, 0);
#endif
		}

		delete [] spb_buf;

		if (misc != misc_buf) 
			delete [] misc;
		}	// try
	catch (...)
		 {
		delete[] spb_buf;
		delete[] switches;
		delete service;
		
		if (misc && misc != misc_buf) 
			delete[] misc;
			
		throw;
		}

	return service;
}


#if defined(SUPERSERVER) && defined(SERVER_SHUTDOWN)
static int shutdown_thread(void *arg) {
/**************************************
 *
 *	s h u t d o w n _ t h r e a d
 *
 **************************************
 *
 * Functional description
 *	Shutdown SuperServer. If hangs, server 
 *  is forcely & dirty closed after timeout.
 *
 **************************************/

	THREAD_ENTER;
	//JRD_shutdown_all();
	
	if (fb_shutdown_connections (fb_shutdown_immediate, 500))
		fb_shutdown_connections (fb_shutdown_panic, 2000);
		
	*reinterpret_cast<int*>(arg) = 1;
	return 0;
}
#endif // defined(SUPERSERVER) && defined(SERVER_SHUTDOWN)


void SVC_detach(Service* service)
{
/**************************************
 *
 *	S V C _ d e t a c h
 *
 **************************************
 *
 * Functional description
 *	Close down a service.
 *
 **************************************/

#ifdef SERVER_SHUTDOWN
	if (service->svc_do_shutdown) 
		{
#ifdef SUPERSERVER
		int flShutdownComplete = 0;
		THD_start_thread(shutdown_thread, &flShutdownComplete, THREAD_medium, 0, 0);
		THREAD_EXIT;
		int timeout = 10;	// seconds
		
		while (timeout--) 
			{
			if (flShutdownComplete)
				break;
			THREAD_SLEEP(1 * 1000);
			}
		if (!flShutdownComplete) 
			gds__log("Forced server shutdown - not all databases closed");
			
#else // SUPERSERVER

		if (fb_shutdown_connections (fb_shutdown_immediate, 500))
			fb_shutdown_connections (fb_shutdown_panic, 2000);
			
#endif // SUPERSERVER

		if (shutdown_fct)
			(shutdown_fct) (shutdown_param);
		else
			exit(0);

		/* The shutdown operation is complete, just wait to die */
		while (1)
			;
		}
#endif /* SERVER_SHUTDOWN */

#ifdef SUPERSERVER

	/* Mark service as detached. */
	/* If service thread is finished, cleanup memory being used by service. */

	SVC_finish(service, SVC_detached);

#else

	/* Go ahead and cleanup memory being used by service */
	
	SVC_cleanup(service);

#endif
}


void SVC_post(ISC_STATUS status, ...)
{
/********************************************
 *
 *   S V C _ p o s t
 *
 ********************************************
 *
 * Functional Description:
 *     Uses ERR_string to save string data for the
 *     status vector
 ********************************************/
 
	va_list		args;
	va_start	(args, status);
	throw OSRIException (args, status);
}


#ifdef SERVER_SHUTDOWN
void SVC_shutdown_init(shutdown_fct_t fptr,
			ULONG param)
{
/**************************************
 *
 *	S V C _ s h u t d o w n _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Set the global function pointer to the shutdown function.
 *
 **************************************/

	shutdown_fct = fptr;
	shutdown_param = param;
}
#endif // SERVER_SHUTDOWN


#ifdef SUPERSERVER
void SVC_fprintf(Service* service, const SCHAR* format, ...)
{
/**************************************
 *
 *	S V C _ f p r i n t f
 *
 **************************************/
/* Ensure that service is not detached. */
	if (!(service->svc_flags & SVC_detached)) {
		va_list arglist;
		char buf[1000];
		ULONG i = 0;

		va_start(arglist, format);
		// CVC: No bounds checking on "buf"; we assume everything fits in it.
		// Where is that limit documented??? I changed to vsnprintf
		//n = vsprintf(buf, format, arglist);
		int n = VSNPRINTF(buf, sizeof(buf), format, arglist);
		va_end(arglist);
		// Nothing is said if this answer generated by service_enqueue_bytes
		// should be null terminated always. Given the old code that called
		// vsprintf, it's clear that '\0' was't sent, so in the new code,
		// n is adjusted in case of insufficient buffer to behave the same.
		// Changed ULONG to int, since this is the official type of the
		// printf family and makes negative test natural.
		// We could send a truncated answer in this case, since vsnprintf
		// returns negative when buffer is not enough. Since the old code
		// didn't send anything after an error, enable this to test if sending
		// a partial result would crash the receiver. Failure may happen before
		// reaching end of buffer for other reasons, so we might transfer trash.
		//if (n < 0)
		//{
		//    n = sizeof(buf) - 1;
		//    buf[n] = 0;
		//}
		while (n > 0 && !(service->svc_flags & SVC_detached)) {
			service_enqueue_byte(buf[i], service);
			n--;
			i++;
		}
	}
}


void SVC_putc(Service* service, const UCHAR ch)
{
/**************************************
 *
 *	S V C _ p u t c
 *
 **************************************/
 
	/* Ensure that service is not detached. */
	
	if (!(service->svc_flags & SVC_detached))
		service_enqueue_byte(ch, service);
}

//____________________________________________________________
//
//	Routine which is used by the service for calling back when there is output.
//
// CVC: Now we are using a SLONG to hold a pointer to the Service!

int SVC_output(Service* output_data, const UCHAR* output_buf)
{
	SVC_fprintf(output_data, "%s", output_buf);

	return 0;
}

#endif /*SUPERSERVER*/

ISC_STATUS SVC_query2(Service* service,
					  thread_db* tdbb,
					  USHORT send_item_length,
					  const UCHAR* send_items,
					  USHORT recv_item_length,
					  const UCHAR* recv_items,
					  USHORT buffer_length,
					  UCHAR* info)
{
/**************************************
 *
 *	S V C _ q u e r y 2
 *
 **************************************
 *
 * Functional description
 *	Provide information on service object
 *
 **************************************/
	UCHAR	item;
	TEXT	buffer[MAXPATHLEN];
	UCHAR	infoBuffer[MAXPATHLEN];
	USHORT	l, length, version, get_flags;

	THREAD_EXIT;

	// Setup the status vector
	
	//ISC_STATUS *status = tdbb->tdbb_status_vector;
	//*status++ = isc_arg_gds;

	// Process the send portion of the query first

	USHORT timeout = 0;
	const UCHAR* items = send_items;
	const UCHAR* const end_items = items + send_item_length;
	
	while (items < end_items && *items != isc_info_end)
		switch ((item = *items++))
			{
			case isc_info_end:
				break;

			default:
				if (items + 2 <= end_items) 
					{
					l = (USHORT) gds__vax_integer(reinterpret_cast<const UCHAR*>(items), 2);
					items += 2;
					
					if (items + l <= end_items) 
						switch (item) 
							{
							case isc_info_svc_line:
								service_put(service, items, l);
								break;
								
							case isc_info_svc_message:
								service_put(service, items - 3, l + 3);
								break;
								
							case isc_info_svc_timeout:
								timeout = (USHORT) gds__vax_integer(reinterpret_cast<const UCHAR*>(items), l);
								break;
								
							case isc_info_svc_version:
								version = (USHORT) gds__vax_integer(reinterpret_cast<const UCHAR*>(items), l);
								break;
							}

					items += l;
					}
				else
					items += 2;
				break;
			}

	// Process the receive portion of the query now


	items = recv_items;
	const UCHAR* const end_items2 = items + recv_item_length;
	
	// Build request info specific for engine
	UCHAR requestBuffer[MAXPATHLEN];
	EngineInfoGen requestEngineInfo(requestBuffer, sizeof(requestBuffer));

	while (items < end_items2 && *items != isc_info_end)
		{
		switch ((item = *items++))
			{
			case isc_info_svc_svr_db_info:
				requestEngineInfo.putItem(isc_info_engine_req_database_paths);
				requestEngineInfo.putItem(isc_info_engine_req_num_attachments);
				requestEngineInfo.putItem(isc_info_engine_req_num_databases);
				break;

			case isc_info_svc_server_version:
				requestEngineInfo.putItem(isc_info_engine_req_engine_version);
				break;
			}
		}

	// Get engine information if we have an engine info request
	int nrOfAttachments = 0;
	JString engineVersion = "";
	Stack databasePaths;
	try
		{
		if (requestEngineInfo.size() >= 0)
			{
			requestEngineInfo.putItem(isc_info_end);

			ISC_STATUS statusVector[ISC_STATUS_LENGTH];
			UCHAR infoBuffer[32000];
			if (fb_engine_info(statusVector, NULL, sizeof(requestBuffer), requestBuffer,
					sizeof(infoBuffer), infoBuffer))
				{
				throw OSRIException(statusVector);
				}

			EngineInfoReader readInfo(infoBuffer, sizeof(infoBuffer));

			while (!readInfo.eof())
				{
				UCHAR engineItem = readInfo.getItem();
				if (engineItem == isc_info_end)
					break;

				switch (engineItem)
					{
					case isc_info_engine_engine_version:
						engineVersion = readInfo.getValueString();
						break;

					case isc_info_engine_databases:
						engineItem = readInfo.getItem();
						if (engineItem != isc_info_engine_list_begin)
							{
							readInfo.skipItem();
							break;
							}

						while (!readInfo.eof() && (engineItem != isc_info_engine_list_end))
							{
							engineItem = readInfo.getItem();
							switch (engineItem)
								{
								case isc_info_engine_db_attachments:
									nrOfAttachments += readInfo.getValueInt();
									readInfo.skipItem();
									break;

								case isc_info_engine_db_path:
									if (readInfo.getItem() == isc_info_engine_value)
										{
										int strLength = readInfo.getShort();
										JString* value = new JString;
										value->setString((char*) readInfo.getBuffer(strLength), strLength);
										databasePaths.push(value);
										}
									break;

								default:
									readInfo.skipItem();
									break;
								}
							}
						break;

					default:
						readInfo.skipItem();
						break;
					}
				}
			}

		//const UCHAR* const end = info + buffer_length;
		InfoGen infoGen(info, buffer_length);
		items = recv_items;
		bool infoPutError = false;
		//const end_items2 = items + recv_item_length;
		
		while (items < end_items2 && *items != isc_info_end)
			{

			if (infoPutError)
				break;
			/*
			if we attached to the "anonymous" service we allow only following queries:

			isc_info_svc_get_config     - called from within remote/ibconfig.cpp
			isc_info_svc_dump_pool_info - called from within utilities/print_pool.cpp
			isc_info_svc_user_dbpath    - called from within utilities/security.cpp
			*/
			 
			if (service->svc_user_flag == SVC_user_none)
				switch (*items)
					{
					case isc_info_svc_get_config:
					case isc_info_svc_dump_pool_info:
					case isc_info_svc_user_dbpath:
						break;
						
					default:
						SVC_post(isc_bad_spb_form, 0);
						break;
					}

			switch ((item = *items++))
				{
				case isc_info_end:
					break;

//#ifdef SERVER_SHUTDOWN
				case isc_info_svc_svr_db_info:
					{
					/*
					UCHAR dbbuf[1024];
					USHORT num_dbs = 0, num = 0;
					USHORT num_att = 0;
					UCHAR *ptr = NULL;
					UCHAR *ptr2 = NULL;

					*info++ = item;
					*/

					infoGen.putItem(item);

					infoGen.putItem(isc_spb_num_att);
					infoGen.putInt(nrOfAttachments);

					infoGen.putItem(isc_spb_num_db);
					infoGen.putInt(databasePaths.count());

					while (!databasePaths.isEmpty())
						{
						JString* databasePath = (JString*) databasePaths.pop();
						if (!infoGen.put(isc_spb_dbname, databasePath->length(), (UCHAR*) databasePath->getString())) 
							{
							infoPutError = true;
							}											
						delete databasePath;

						if (infoPutError)
							break;
						}

					//ptr = JRD_num_attachments(reinterpret_cast<char*>(dbbuf),
					//						sizeof(dbbuf), JRD_info_dbnames,
					//						&num_att, &num_dbs);
											
					/* Move the number of attachments into the info buffer */
					
					/*if (!ck_space_for_numeric(info, end))
						return 0;				
						
					*info++ = isc_spb_num_att;
					ADD_SPB_NUMERIC(info, num_att);
					*/
	//				infoGen.putItem(isc_spb_num_att);
	//				infoGen.putInt(num_att);

					/* Move the number of databases in use into the info buffer */
					
					/*if (!ck_space_for_numeric(info, end))
						return 0;
						
					*info++ = isc_spb_num_db;
					ADD_SPB_NUMERIC(info, num_dbs);
					*/
	//				infoGen.putItem(isc_spb_num_db);
	//				infoGen.putInt(num_dbs);

					/* Move db names into the info buffer */
					
					/*
					if (ptr2 = ptr) 
						{
						num = (USHORT) isc_vax_integer((const char*) ptr2, sizeof(USHORT));
						fb_assert(num == num_dbs);
						ptr2 += sizeof(USHORT);
						
						for (; num; num--) 
							{
							length = (USHORT) isc_vax_integer((const char*) ptr2, sizeof(USHORT));
							ptr2 += sizeof(USHORT);
							
							if (! infoGen.put(isc_spb_dbname, length, ptr2)) 
								{
								THREAD_ENTER;
								return 0;
								}
								
							ptr2 += length;
							}

						//if (ptr != reinterpret_cast < TEXT * >(dbbuf))
						if (ptr != dbbuf)
							gds__free(ptr);	// memory has been allocated by JRD_num_attachments()
						}
					*/

					/*
					if (info < end)
						*info++ = isc_info_flag_end;
					*/
					if (!infoPutError)
						infoGen.putItem(isc_info_flag_end);
					}

					break;

#ifdef SERVER_SHUTDOWN

				case isc_info_svc_svr_online:
					*info++ = item;
					
					if (service->svc_user_flag & SVC_user_dba) 
						{
						service->svc_do_shutdown = FALSE;
						WHY_set_shutdown(FALSE);
						}
					else
						need_admin_privs(&status, "isc_info_svc_svr_online");
					break;

				case isc_info_svc_svr_offline:
					*info++ = item;
					
					if (service->svc_user_flag & SVC_user_dba) 
						{
						service->svc_do_shutdown = TRUE;
						WHY_set_shutdown(TRUE);
						}
					else
						need_admin_privs(&status, "isc_info_svc_svr_offline");
						
					break;
#endif /* SERVER_SHUTDOWN */

				/* The following 3 service commands (or items) stuff the response
				buffer 'info' with values of environment variable INTERBASE,
				INTERBASE_LOCK or INTERBASE_MSG. If the environment variable
				is not set then default value is returned.
				*/
					 
				case isc_info_svc_get_env:
				case isc_info_svc_get_env_lock:
				case isc_info_svc_get_env_msg:
					switch (item) 
						{
						case isc_info_svc_get_env:
							gds__prefix(buffer, "");
							break;
							
						case isc_info_svc_get_env_lock:
							gds__prefix_lock(buffer, "");
							break;
							
						case isc_info_svc_get_env_msg:
							gds__prefix_msg(buffer, "");
						}

					/* Note: it is safe to use strlen to get a length of "buffer"
					because gds_prefix[_lock|_msg] return a zero-terminated
					string
					*/
					 
					//info = INF_put_item(item, buffer, info, end);
					infoGen.putString(item, buffer);
					break;

#ifdef SUPERSERVER
				case isc_info_svc_dump_pool_info:
					{
					char fname[MAXPATHLEN];
					int length = isc_vax_integer((char*) items, sizeof(USHORT));
					
					if (length >= sizeof(fname))
						length = sizeof(fname) - 1; // truncation
						
					items += sizeof(USHORT);
					strncpy(fname, (const char*) items, length);
					fname[length] = 0;
					//JRD_print_all_counters(fname);		// It's a no-op anyway (jas)
					break;
					}
#endif
		/*
				case isc_info_svc_get_config:
					// TODO: iterate through all integer-based config values
					//		 and return them to the client
					break;

				case isc_info_svc_default_config:
					*info++ = item;
					if (service->svc_user_flag & SVC_user_dba) {
						THREAD_ENTER;
						// TODO: reset the config values to defaults
						THREAD_EXIT;
					}
					else
						need_admin_privs(&status, "isc_info_svc_default_config");
					break;

				case isc_info_svc_set_config:
					*info++ = item;
					if (service->svc_user_flag & SVC_user_dba) {
						THREAD_ENTER;
						// TODO: set the config values
						THREAD_EXIT;
					}
					else {
						need_admin_privs(&status, "isc_info_svc_set_config");
					}
					break;
		*/
				case isc_info_svc_version:
					/* The version of the service manager */
					/***
					if (!ck_space_for_numeric(info, end))
						return 0;
					*info++ = item;
					ADD_SPB_NUMERIC(info, SERVICE_VERSION);
					***/
					infoGen.putInt(item, SERVICE_VERSION);
					break;

				case isc_info_svc_capabilities:
					/* bitmask defining any specific architectural differences */
					/***
					if (!ck_space_for_numeric(info, end))
						return 0;
					*info++ = item;
					ADD_SPB_NUMERIC(info, SERVER_CAPABILITIES_FLAG);
					***/
					infoGen.putInt(item, SERVER_CAPABILITIES_FLAG);
					break;

				case isc_info_svc_running:
					/* Returns the status of the flag SVC_thd_running */
					/***
					if (!ck_space_for_numeric(info, end))
						return 0;
					*info++ = item;
					***/
					
					if (service->svc_flags & SVC_thd_running)
						//ADD_SPB_NUMERIC(info, TRUE)
						infoGen.putInt(item, TRUE);
					else
						//ADD_SPB_NUMERIC(info, FALSE)
						infoGen.putInt(item, FALSE);

					break;

				case isc_info_svc_server_version:
					/* The version of the server engine */
					//info = INF_put_item(item, GDS_VERSION, info, end);
					infoGen.putString(item, engineVersion.getString());
					break;

				case isc_info_svc_implementation:
					/* The server implementation - e.g. Interbase/sun4 */
					isc_format_implementation(IMPLEMENTATION, sizeof(buffer), buffer, 0, 0, NULL);
					//info = INF_put_item(item, buffer, info, end);
					infoGen.putString(item, buffer);
					break;


				case isc_info_svc_user_dbpath:
					/* The path to the user security database (security.fdb) */
					//SecurityDatabase::getPath(buffer);
					infoGen.putString(item, service->getSecurityDatabase());
					break;

				case isc_info_svc_response:
					service_put(service, &item, 1);
					service_get(service, &item, 1, GET_BINARY, 0, &length);
					service_get(service, infoBuffer, 2, GET_BINARY, 0, &length);
					l = (USHORT) gds__vax_integer(infoBuffer, 2);
					service->responseBuffer.resize(l);
					service_get(service, service->responseBuffer.space, l, GET_BINARY, 0, &length);
					service->responseLength = length;
					//info = INF_put_item(item, length, info + 3, info, end);
					l = infoGen.maxRemaining();
					
					if (length < l)
						{
						infoGen.put(item, length, service->responseBuffer.space);
						service->responseOffset = length;
						}
					else
						{
						infoGen.put(item, l, service->responseBuffer.space);
						service->responseOffset = l;
						infoGen.forceTruncation();
						}
					
					break;

				case isc_info_svc_response_more:
					length = service->responseLength - service->responseOffset;
					l = infoGen.maxRemaining();
					
					if (length < l)
						{
						infoGen.put(item, length, service->responseBuffer.space + service->responseOffset);
						service->responseOffset = 0;
						service->responseLength = 0;
						}
					else
						{
						infoGen.put(item, l, service->responseBuffer.space + service->responseOffset);
						service->responseOffset += l;
						infoGen.forceTruncation();
						}
						
					/***
					if ( (l = length = service->svc_resp_len) )
						length = MIN(end - (info + 5), l);
						
					if (!(info = INF_put_item(item, length, service->svc_resp_ptr, info, end))) 
						{
						THREAD_ENTER;
						return 0;
						}
						
					service->svc_resp_ptr += length;
					service->svc_resp_len -= length;
					
					if (length != l)
						*info++ = isc_info_truncated;
					***/
					break;

				case isc_info_svc_total_length:
					service_put(service, &item, 1);
					service_get(service, &item, 1, GET_BINARY, 0, &length);
					service_get(service, infoBuffer, 2, GET_BINARY, 0, &length);
					l = (USHORT) gds__vax_integer(infoBuffer, 2);
					service_get(service, infoBuffer, l, GET_BINARY, 0, &length);
					infoGen.put(item, length, infoBuffer);
					break;

				case isc_info_svc_line:
				case isc_info_svc_to_eof:
				case isc_info_svc_limbo_trans:
				case isc_info_svc_get_users:
					if (item == isc_info_svc_line)
						get_flags = GET_LINE;
					else if (item == isc_info_svc_to_eof)
						get_flags = GET_EOF;
					else
						get_flags = GET_BINARY;

					TempSpace tempSpace(sizeof(infoBuffer), infoBuffer);
					l = infoGen.maxRemaining();
					tempSpace.resize(l);
					service_get(service, tempSpace.space, l, get_flags, timeout, &length);
					infoGen.put(item, length, tempSpace.space);
					
					/* If the read timed out, return the data, if any, & a timeout
					item.  If the input buffer was not large enough
					to store a read to eof, return the data that was read along
					with an indication that more is available. */

					if (service->svc_flags & SVC_timeout)
						*info++ = isc_info_svc_timeout;
					else
						{
						if (!length && !(service->svc_flags & SVC_finished))
							*info++ = isc_info_data_not_ready;
						else
							if (item == isc_info_svc_to_eof && !(service->svc_flags & SVC_finished))
								*info++ = isc_info_truncated;
						}
						
					break;
					}

			if (service->svc_user_flag == SVC_user_none)
				break;
			}

		if (!infoPutError)
			infoGen.fini();

		THREAD_ENTER;

		// Clean up remaining paths probably left due error
		while (!databasePaths.isEmpty())
			{
			JString* databasePath = (JString*) databasePaths.pop();
			delete databasePath;
			}
		}
	catch (...)
		{
		// Clean up remaining paths probably left due exception
		while (!databasePaths.isEmpty())
			{
			JString* databasePath = (JString*) databasePaths.pop();
			delete databasePath;
			}
			
		throw;
		}

	
	//return tdbb->tdbb_status_vector[1];
	return 0;
}

void SVC_query(Service*		service,
			   USHORT		send_item_length,
			   const UCHAR*	send_items,
			   USHORT		recv_item_length,
			   const UCHAR*	recv_items,
			   USHORT		buffer_length,
			   UCHAR*		info)
{
/**************************************
 *
 *	S V C _ q u e r y
 *
 **************************************
 *
 * Functional description
 *	Provide information on service object.
 *
 **************************************/
	UCHAR	item;
	//char	buffer[256];
	TEXT	PathBuffer[MAXPATHLEN];
	UCHAR	infoBuffer[MAXPATHLEN];
	USHORT l, length, version, get_flags;

	THREAD_EXIT;

	/* Process the send portion of the query first. */

	USHORT timeout = 0;
	const UCHAR* items = send_items;
	const UCHAR* const end_items = items + send_item_length;
	
	while (items < end_items && *items != isc_info_end)
		{
		switch ((item = *items++))
			{
			case isc_info_end:
				break;

			default:
				if (items + 2 <= end_items)
					{
					l = (USHORT) gds__vax_integer(reinterpret_cast<const UCHAR*>(items), 2);
					items += 2;
					
					if (items + l <= end_items)
						switch (item) 
							{
							case isc_info_svc_line:
								service_put(service, items, l);
								break;
								
							case isc_info_svc_message:
								service_put(service, items - 3, l + 3);
								break;
								
							case isc_info_svc_timeout:
								timeout = (USHORT) gds__vax_integer(reinterpret_cast<const UCHAR*>(items), l);
								break;
								
							case isc_info_svc_version:
								version = (USHORT) gds__vax_integer(reinterpret_cast<const UCHAR*>(items), l);
								break;
							}
					items += l;
				}
				else
					items += 2;
				break;
			}
		}

	/* Process the receive portion of the query now. */

	//const UCHAR* const end = info + buffer_length;
	InfoGen infoGen(info, buffer_length);
	items = recv_items;
	const UCHAR* const end_items2 = items + recv_item_length;
	
	while (items < end_items2 && *items != isc_info_end)
		{
		switch ((item = *items++))
			{
			case isc_info_end:
				break;

#ifdef SERVER_SHUTDOWN
			case isc_info_svc_svr_db_info:
				{
				USHORT num_dbs = 0;
				int num_att = fb_shutdown_connections (fb_shutdown_count, 0);
				//length = INF_convert(num_att, buffer);
				//info = INF_put_item(item, length, buffer, info, end);
				infoGen.putInt(item, num_att);
				
				//length = INF_convert(num_dbs, buffer);
				//info = INF_put_item(item, length, buffer, info, end);
				infoGen.putInt(item, num_dbs);
				}
				break;

			case isc_info_svc_svr_online:
				//*info++ = item;
				
				if (service->svc_user_flag & SVC_user_dba) 
					{
					service->svc_do_shutdown = FALSE;
					WHY_set_shutdown(FALSE);
					//*info++ = 0;	/* Success */
					infoGen.putByte(item, 0); // Success
					}
				else
					//*info++ = 2;	/* No user authority */
					infoGen.putByte(item, 2); // No user authority
				break;

			case isc_info_svc_svr_offline:
				*info++ = item;
				
				if (service->svc_user_flag & SVC_user_dba) 
					{
					service->svc_do_shutdown = TRUE;
					WHY_set_shutdown(TRUE);
					*info++ = 0;	/* Success */
					}
				else
					*info++ = 2;	/* No user authority */
				break;
				
#endif /* SERVER_SHUTDOWN */

			/* The following 3 service commands (or items) stuff the response
			   buffer 'info' with values of environment variable INTERBASE,
			   INTERBASE_LOCK or INTERBASE_MSG. If the environment variable
			   is not set then default value is returned.
			*/
				
			case isc_info_svc_get_env:
			case isc_info_svc_get_env_lock:
			case isc_info_svc_get_env_msg:
				switch (item) 
					{
					case isc_info_svc_get_env:
						gds__prefix(PathBuffer, "");
						break;
					case isc_info_svc_get_env_lock:
						gds__prefix_lock(PathBuffer, "");
						break;
					case isc_info_svc_get_env_msg:
						gds__prefix_msg(PathBuffer, "");
					}

				/* Note: it is safe to use strlen to get a length of "buffer"
				   because gds_prefix[_lock|_msg] return a zero-terminated
				   string */
				
				infoGen.putString(item, PathBuffer);
					
				break;

#ifdef SUPERSERVER
			case isc_info_svc_dump_pool_info:
				{
				char fname[MAXPATHLEN];
				int length = isc_vax_integer((const char*) items, sizeof(USHORT));
				
				if (length >= sizeof(fname))
					length = sizeof(fname) - 1; // truncation
					
				items += sizeof(USHORT);
				strncpy(fname, (const char*) items, length);
				fname[length] = 0;
				//JRD_print_all_counters(fname);
				break;
				}
#endif
			/*
			case isc_info_svc_get_config:
				// TODO: iterate through all integer-based config values
				//		 and return them to the client
				break;

			case isc_info_svc_default_config:
				*info++ = item;
				if (service->svc_user_flag & SVC_user_dba) {
					THREAD_ENTER;
					// TODO: reset the config values to defaults
					THREAD_EXIT;
				}
				else
					need_admin_privs(&status, "isc_info_svc_default_config");
				break;

			case isc_info_svc_set_config:
				*info++ = item;
				if (service->svc_user_flag & SVC_user_dba) {
					THREAD_ENTER;
					// TODO: set the config values
					THREAD_EXIT;
				}
				else {
					need_admin_privs(&status, "isc_info_svc_set_config");
				}
				break;
			*/
			
			case isc_info_svc_version:
				/* The version of the service manager */
				infoGen.putInt(item, SERVICE_VERSION);
				break;

			case isc_info_svc_capabilities:
				/* bitmask defining any specific architectural differences */
				infoGen.putInt(item, SERVER_CAPABILITIES_FLAG);
				break;

			case isc_info_svc_server_version:
				{
				/* The version of the server engine */
				infoGen.putString(item, GDS_VERSION);
				break;
				}

			case isc_info_svc_implementation:
				/* The server implementation - e.g. Interbase/sun4 */
				infoGen.putInt(item, IMPLEMENTATION);
				break;


			case isc_info_svc_user_dbpath:
				/* The path to the user security database (security.fdb) */
				//SecurityDatabase::getPath(buffer);
				infoGen.putString(item, service->getSecurityDatabase());
				break;

			case isc_info_svc_response:
				{
				service_put(service, &item, 1);
				service_get(service, &item, 1, GET_BINARY, 0, &length);
				service_get(service, infoBuffer, 2, GET_BINARY, 0, &length);
				l = (USHORT) gds__vax_integer(infoBuffer, 2);
				service->responseBuffer.resize(l);
				service_get(service, service->responseBuffer.space, l, GET_BINARY, 0, &length);
				service->responseLength = length;
				//info = INF_put_item(item, length, info + 3, info, end);
				l = infoGen.maxRemaining();
				
				if (length < l)
					{
					infoGen.put(item, length, service->responseBuffer.space);
					service->responseOffset = length;
					}
				else
					{
					infoGen.put(item, l, service->responseBuffer.space);
					service->responseOffset = l;
					infoGen.forceTruncation();
					}
				
				break;
				}

			case isc_info_svc_response_more:
				length = service->responseLength - service->responseOffset;
				l = infoGen.maxRemaining();
				
				if (length < l)
					{
					infoGen.put(item, length, service->responseBuffer.space + service->responseOffset);
					service->responseOffset = 0;
					service->responseLength = 0;
					}
				else
					{
					infoGen.put(item, l, service->responseBuffer.space + service->responseOffset);
					service->responseOffset += l;
					infoGen.forceTruncation();
					}
					
				break;

			case isc_info_svc_total_length:
				service_put(service, &item, 1);
				service_get(service, &item, 1, GET_BINARY, 0, &length);
				service_get(service, infoBuffer, 2, GET_BINARY, 0, &length);
				l = (USHORT) gds__vax_integer(infoBuffer, 2);
				service_get(service, infoBuffer, l, GET_BINARY, 0, &length);
				infoGen.put(item, length, infoBuffer);
				break;

			case isc_info_svc_line:
			case isc_info_svc_to_eof:
				get_flags = (item == isc_info_svc_line) ? GET_LINE : GET_EOF;
				TempSpace tempSpace(sizeof(infoBuffer), infoBuffer);
				l = infoGen.maxRemaining();
				tempSpace.resize(l);
				service_get(service, tempSpace.space, l, get_flags, timeout, &length);
				infoGen.put(item, length, tempSpace.space);
				//service_get(service, info + 3, end - (info + 4), get_flags, timeout, &length);
				//info = INF_put_item(item, length, info + 3, info, end);

				/* If the read timed out, return the data, if any, & a timeout
				   item.  If the input buffer was not large enough
				   to store a read to eof, return the data that was read along
				   with an indication that more is available. */


				if (service->svc_flags & SVC_timeout)
					*info++ = isc_info_svc_timeout;
				else
					{
					if (!length && !(service->svc_flags & SVC_finished))
						*info++ = isc_info_data_not_ready;
					else if (item == isc_info_svc_to_eof && !(service->svc_flags & SVC_finished))
						*info++ = isc_info_truncated;
					}
				break;
			}
		}

	infoGen.fini();

	THREAD_ENTER;
}


void SVC_start(Service* service, USHORT spb_length, const UCHAR* spb)
{
/**************************************
 *
 *	S V C _ s t a r t
 *
 **************************************
 *
 * Functional description
 *      Start a Firebird service
 *
 **************************************/
	//thread_db *tdbb;
	USHORT opt_switch_len = 0;
	BOOLEAN flag_spb_options = FALSE;
#ifndef SUPERSERVER
	TEXT service_path[MAXPATHLEN];
#endif

	/* 	NOTE: The parameter RESERVED must not be used
	 *	for any purpose as there are networking issues
	 *	involved (as with any handle that goes over the
	 *	network).  This parameter will be implemented at
	 *	a later date.
	 */

	isc_resv_handle reserved = (isc_resv_handle) 0;	/* Reserved for future functionality */

	/* The name of the service is the first element of the buffer */
	
	const USHORT svc_id = *spb;
	const struct serv *serv;

	for (serv = (struct serv*)services; serv->serv_action; serv++)
		if (serv->serv_action == svc_id)
			break;

	if (!serv->serv_name)
		SVC_post(isc_service_att_err, isc_arg_gds, isc_service_not_supported, 0);

	/* currently we do not use "anonymous" service for any purposes but
	   isc_service_query() */
	   
	if (service->svc_user_flag == SVC_user_none)
		SVC_post(isc_bad_spb_form, 0);

	/***
	if (!thd_initialized) 
		{
		THD_MUTEX_INIT(thd_mutex);
		thd_initialized = TRUE;
		}
	***/
	
	//THD_MUTEX_LOCK(thd_mutex);
	Sync sync(&thd_mutex, "SVC_start");
	sync.lock(Exclusive);
		
	if (service->svc_flags & SVC_thd_running) 
		SVC_post(isc_svc_in_use, isc_arg_string, serv->serv_name, 0);

	/* Another service may have been started with this service block.  If so,
	 * we must reset the service flags.
	 */
		
	//if (service->svc_switches) 
	if (!service->svc_switches.IsEmpty() && !(service->svc_flags & SVC_cmd_line))
		service->svc_switches = "";

	if (!(service->svc_flags & SVC_detached))
		service->svc_flags = 0;
		
	service->svc_flags |= SVC_thd_running;
		
	sync.unlock();
	//tdbb = GET_THREAD_DATA;

	try 
		{
		/* Only need to add username and password information to those calls which need
		* to make a database connection
		*/
		
		if (*spb == isc_action_svc_backup ||
			*spb == isc_action_svc_restore ||
			*spb == isc_action_svc_repair ||
			*spb == isc_action_svc_add_user ||
			*spb == isc_action_svc_delete_user ||
			*spb == isc_action_svc_modify_user ||
			*spb == isc_action_svc_display_user ||
			*spb == isc_action_svc_db_stats ||
			*spb == isc_action_svc_properties)
			{
			/* the user issued a username when connecting to the service so
			* add the length of the username and switch to new_spb_length
			*/

			//if (*service->svc_username)
			if (!service->userName.IsEmpty())
				opt_switch_len += (USHORT) (service->userName.length() + 1 + sizeof(USERNAME_SWITCH));

			/* the user issued a password when connecting to the service so add
			 * the length of the password and switch to new_spb_length
			 */
			
			if (!service->encryptedPassword.IsEmpty())
				opt_switch_len += (USHORT) (service->encryptedPassword.length() + 1 + sizeof(PASSWORD_SWITCH));

			/* If svc_switches is not used -- call a command-line parsing utility */
			
			if (service->svc_switches.IsEmpty()) 
				service->svc_switches = conv_switches(spb_length, opt_switch_len, spb);
			else 
				{
				/* Command line options (isc_spb_options) is used.
				* Currently the only case in which it might happen is -- gbak utility
				* is called with a "-server" switch.
				*/
				flag_spb_options = TRUE;
				}

			/* add the username and password to the end of svc_switches if needed */

			JString	temp;
			
			if (!service->svc_switches.IsEmpty()) 
				{
				if (flag_spb_options)
					temp = service->svc_switches;
					//strcpy(tmp_ptr, service->svc_switches);

				if (!service->userName.IsEmpty()) 
					{
					if (!flag_spb_options)
						service->svc_switches.Format("%s %s %s",
								(const char*) service->svc_switches, USERNAME_SWITCH,
								(const char*) service->userName);
						/***
						sprintf(service->svc_switches, "%s %s %s",
								(const char*) service->svc_switches, USERNAME_SWITCH,
								(const char*) service->userName);
						***/
					else
						temp.Format("%s %s %s", (const char*) temp, USERNAME_SWITCH, (const char*) service->userName);
						/***
						sprintf(tmp_ptr, "%s %s %s", tmp_ptr, USERNAME_SWITCH,
								(const char*) service->userName);
						***/
					}

				if (!service->encryptedPassword.IsEmpty()) 
					{
					if (!flag_spb_options)
						service->svc_switches.Format("%s %s %s",
								(const char*) service->svc_switches, PASSWORD_SWITCH,
								(const char*) service->encryptedPassword);
						/***
						sprintf(service->svc_switches, "%s %s %s",
								service->svc_switches, PASSWORD_SWITCH,
								(const char*) service->encryptedPassword);
						***/
					else
						temp.Format("%s %s %s", (const char*) temp, PASSWORD_SWITCH,
								(const char*) service->encryptedPassword);
						/***
						sprintf(tmp_ptr, "%s %s %s", tmp_ptr, PASSWORD_SWITCH,
								(const char*) service->encryptedPassword);
						***/
					}

				if (flag_spb_options) 
					service->svc_switches = temp;
				}
			}
		else 
			{
			/* If svc_switches is not used -- call a command-line parsing utility */
			
			if (service->svc_switches.IsEmpty()) 
				service->svc_switches = conv_switches(spb_length, opt_switch_len, spb);
			else 
				fb_assert(service->svc_switches.IsEmpty()); // this makes no sense to me -- jas
			}
			
		/* All services except for get_ib_log require switches */
		
		if (service->svc_switches.IsEmpty() && *spb != isc_action_svc_get_ib_log)
			SVC_post(isc_bad_spb_form, 0);

#ifndef SUPERSERVER
		if (serv->serv_executable) 
			{
			gds__prefix(service_path, serv->serv_executable);
			service->svc_flags = SVC_forked;
			service_fork(service_path, service);
			}

#else

		/* Break up the command line into individual arguments. */
		
		if (service->svc_switches)
			{
			USHORT argc = 2;
			TEXT* p = service->setArgvStrings(service->svc_switches);
			
			while (*p)
				{
				if (*p == ' ') 
					{
					argc++;
					
					while (*p == ' ')
						p++;
					}
				else 
					{
					if (*p == SVC_TRMNTR) 
						{
						while (*p++ && *p != SVC_TRMNTR)
							;
							
						fb_assert(*p == SVC_TRMNTR);
						}
						
					p++;
					}
				}

			TEXT** arg = service->allocArgv(argc);
			*arg++ = (TEXT *)(serv->serv_thd);
			TEXT* q = p = service->svc_argv_strings;

			while (*q == ' ')
				q++;

			while (*q)
				 {
				*arg = p;
				while (*p = *q++) 
					{
					if (*p == ' ')
						break;

					if (*p == SVC_TRMNTR) 
						{
						*arg = ++p;	/* skip terminator */
						
						while (*p = *q++)
							/* If *q points to the last argument, then terminate the argument */
							if ((*q == 0 || *q == ' ') && *p == SVC_TRMNTR) 
								{
								*p = '\0';
								break;
								}
							else
								p++;
						}

					if (*p == '\\' && *q == ' ') 
						{
						*p = ' ';
						q++;
						}
						
					p++;
					}
					
				arg++;
				
				if (!*p)
					break;
					
				*p++ = '\0';
				while (*q == ' ')
					q++;
				}
				
			*arg = NULL;
			}

		/*
		 * the service block can be reused hence free a memory from the
		 * previous usage as well as init a status vector.
		 */

		memset((void *) service->svc_status, 0, ISC_STATUS_LENGTH * sizeof(ISC_STATUS));
		delete service->svc_stdout;
		service->svc_stdout = new UCHAR[SVC_STDOUT_BUFFER_SIZE + 1];
		
		if (serv->serv_thd)
			 {
			THREAD_EXIT;
			
			/* create an event for the service.  The event will be signaled once the
			* particular service has reached a point in which it can start to return
			* information to the client.  This will allow isc_service_start to
			* include in its status vector information about the service's ability to
			* start.  This is needed since THD_start_thread will almost always
			* succeed.
			*/

			service->svc_start_event->init(0, 0);
			SLONG count = service->svc_start_event->clear();		

			THD_start_thread(reinterpret_cast<FPTR_INT_VOID_PTR>(serv->serv_thd), 
							  service, THREAD_medium, 0,
							( void*) &service->svc_handle);

			/* check for the service being detached.  This will prevent the thread
			* from waiting infinitely if the client goes away
			*/
			
			while (!(service->svc_flags & SVC_detached)) 
				if (!service->svc_start_event->wait(count, 60 * 1000))
					break;

			service->svc_start_event->fini();
			
			THREAD_ENTER;
			}
		else
			SVC_post(isc_svcnotdef, isc_arg_string, serv->serv_name, 0);

#endif /* SUPERSERVER */

		}	// try
	catch (...) 
		{
		delete service;
			
		throw;
		}

	//return reinterpret_cast<void*>(reserved);
}


//int SVC_read_ib_log(Service* service)
THREAD_ENTRY_DECLARE SVC_read_ib_log(THREAD_ENTRY_PARAM arg)
{
/**************************************
 *
 *      S V C _ r e a d _ i b _ l o g
 *
 **************************************
 *
 * Functional description
 *   Service function which reads the Firebird
 *   log file into the service buffers.
 *
 **************************************/
 
	Service *service = (Service*) arg;
	IB_FILE *file;
	TEXT name[MAXPATHLEN], buffer[100];
	bool svc_started = false;
	
#ifdef SUPERSERVER
	ISC_STATUS *status = service->svc_status;
	*status++ = isc_arg_gds;
#endif

	gds__prefix(name, LOGFILE);
	
	if ((file = ib_fopen(name, "r")) != NULL) {
#ifdef SUPERSERVER
		*status++ = FB_SUCCESS;
		*status++ = isc_arg_end;
#endif
		SVC_STARTED(service);
		svc_started = true;
		
		while (!ib_feof(file) && !ib_ferror(file)) 
			{
			ib_fgets(buffer, sizeof(buffer), file);
#ifdef SUPERSERVER
			SVC_fprintf(service, "%s", buffer);
#else
			service_put(service, (UCHAR*) buffer, sizeof(buffer));
#endif
			}
		}

	if (!file || file && ib_ferror(file)) 
		{
#ifdef SUPERSERVER
		*status++ = isc_sys_request;
		
		if (!file) 
			{
			SVC_STATUS_ARG(status, isc_arg_string, (void*)"ib_fopen");
			}
		else 
			{
			SVC_STATUS_ARG(status, isc_arg_string, (void*)"ib_fgets");
			}
			
		*status++ = SYS_ARG;
		*status++ = errno;
		*status++ = isc_arg_end;
#endif
		if (!svc_started)
			{
			SVC_STARTED(service);
			}
		}

	if (file)
		ib_fclose(file);

	service->svc_handle = 0;
	
	if (service->svc_service->in_use != NULL)
		*(service->svc_service->in_use) = FALSE;

#ifdef SUPERSERVER
	SVC_finish(service, SVC_finished);
#else
	SVC_cleanup(service);
#endif

	return (FINI_OK);
}


static void get_options(const UCHAR*	spb,
						USHORT	spb_length,
						TEXT*	scratch,
						SPB*	options)
{
/**************************************
 *
 *	g e t _ o p t i o n s
 *
 **************************************
 *
 * Functional description
 *	Parse service parameter block picking up options and things.
 *
 **************************************/
	MOVE_CLEAR(options, (SLONG) sizeof(struct spb));
	const UCHAR* p = spb;
	const UCHAR* const end_spb = p + spb_length;

	if (p < end_spb && (*p != isc_spb_version1 && *p != isc_spb_version))
		SVC_post(isc_bad_spb_form, isc_arg_gds, isc_wrospbver, 0);

	while (p < end_spb)
		switch (*p++) {
		case isc_spb_version1:
			options->spb_version = isc_spb_version1;
			break;

		case isc_spb_version:
			options->spb_version = *p++;
			break;

		case isc_spb_sys_user_name:
			options->spb_sys_user_name = get_string_parameter(&p, &scratch);
			break;

		case isc_spb_user_name:
			options->spb_user_name = get_string_parameter(&p, &scratch);
			break;

		case isc_spb_password:
			options->spb_password = get_string_parameter(&p, &scratch);
			break;

		case isc_spb_password_enc:
			options->spb_password_enc = get_string_parameter(&p, &scratch);
			break;

		case isc_spb_command_line:
			options->spb_command_line = get_string_parameter(&p, &scratch);
			break;

		default:
			{
				const USHORT l = *p++;
				p += l;
			}
		}
}


static TEXT* get_string_parameter(const UCHAR** spb_ptr, TEXT** opt_ptr)
{
/**************************************
 *
 *	g e t _ s t r i n g _ p a r a m e t e r
 *
 **************************************
 *
 * Functional description
 *	Pick up a string valued parameter, copy it to a running temp,
 *	and return pointer to copied string.
 *
 **************************************/
	USHORT l;

	TEXT* opt = *opt_ptr;
	const UCHAR* spb = *spb_ptr;

	if ( (l = *spb++) )
		do {
			*opt++ = *spb++;
		} while (--l);

	*opt++ = 0;
	*spb_ptr = spb;
	spb = (UCHAR *) * opt_ptr;
	*opt_ptr = opt;

	return (TEXT *) spb;
}


#ifndef SUPERSERVER
static void io_error(const TEXT * string,
					 SLONG status,
					 const TEXT * filename, 
					 ISC_STATUS operation, 
					 BOOLEAN reenter_flag)
{
/**************************************
 *
 *	i o _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Report an I/O error.  If the reenter_flag
 *	is TRUE, re-enter the scheduler.
 *
 **************************************/

#ifdef MULTI_THREADxxx
	if (reenter_flag)
		THREAD_ENTER;
#endif

	SVC_post(isc_io_error, isc_arg_string, string, isc_arg_string, filename,
			 isc_arg_gds, operation, SYS_ERR, status, 0);
}
#endif

#ifdef WIN_NT
#ifndef SUPERSERVER
static void service_close(Service* service)
{
/**************************************
 *
 *	s e r v i c e _ c l o s e		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Shutdown the connection to a service.
 *	Simply close the input and output pipes.
 *
 **************************************/
 
	CloseHandle((HANDLE) service->svc_input);
	CloseHandle((HANDLE) service->svc_output);
	CloseHandle((HANDLE) service->svc_handle);
}


static void service_fork(TEXT * service_path, Service* service)
{
/**************************************
 *
 *	s e r v i c e _ f o r k		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Startup a service.
 *
 **************************************/
	//TEXT *argv_data, argv_data_buf[512], *p, *q, *arg;
	USHORT quote_flag, user_quote;
	HANDLE my_output, pipe_input, pipe_output, pipe_error;
	SECURITY_ATTRIBUTES attr;
	STARTUPINFO start_crud;
	PROCESS_INFORMATION pi;
	USHORT ret;
	BOOLEAN svc_flag;
	SLONG status;
	BOOLEAN windows_nt = ISC_is_WinNT();

	/* Only Create the pipes on Windows NT.  There is a bug on Windows
	   95 that prohibits these handles from being converted by the
	   child process
	*/
	
	HANDLE my_input = pipe_output = my_output = pipe_input = INVALID_HANDLE_VALUE;
	
	if (windows_nt) 
		{
		/* Set up input and output pipes and make them the ib_stdin, ib_stdout,
		   and ib_stderr of the forked process. */

		attr.nLength = sizeof(SECURITY_ATTRIBUTES);
		attr.bInheritHandle = TRUE;
		attr.lpSecurityDescriptor = NULL;

		if (!CreatePipe(&my_input, &pipe_output, &attr, 0) ||
			!CreatePipe(&pipe_input, &my_output, &attr, 0) ||
			!DuplicateHandle(GetCurrentProcess(), pipe_output,
							 GetCurrentProcess(), &pipe_error, 0, TRUE,
							 DUPLICATE_SAME_ACCESS)) 
			{
			status = GetLastError();
			CloseHandle(my_input);
			CloseHandle(pipe_output);
			CloseHandle(my_output);
			CloseHandle(pipe_input);
			SVC_post(isc_sys_request, isc_arg_string,
					 (my_output !=
					  INVALID_HANDLE_VALUE) ? "CreatePipe" :
					 "DuplicateHandle", SYS_ERR, status, 0);
			}
		}
	else 
		{
		/* Create a temporary file and get the OS handle to
		   the file created.  This handle will be used in subsequent
		   calls to the windows API functions for working with the files
		 */
		int tmp;
		char *fname;
		char tmpPath[MAXPATHLEN];

		GetTempPath(MAXPATHLEN, tmpPath);
		fname = _tempnam(tmpPath, "ibsvc");
		tmp =_open(fname, _O_RDWR | _O_CREAT | _O_TEMPORARY,_S_IREAD | _S_IWRITE);
		my_input = (HANDLE) _get_osfhandle(tmp);

		fname = _tempnam(tmpPath, "ibsvc");
		tmp =_open(fname, _O_RDWR | _O_CREAT | _O_TEMPORARY,_S_IREAD | _S_IWRITE);
		my_output = (HANDLE) _get_osfhandle(tmp);

		if (my_input == INVALID_HANDLE_VALUE ||
			my_output == INVALID_HANDLE_VALUE) 
			{
			CloseHandle(my_input);
			CloseHandle(my_output);
			SVC_post(isc_sys_request, isc_arg_string, "CreateFile", SYS_ERR,
					 errno, 0);
			}
		}

	/* Make sure we have buffers that are large enough to hold the number
	   and size of the command line arguments.  Add some extra space for
	   the pipe's file handles. */

	USHORT len = (USHORT) (strlen(service_path) + service->svc_switches.length() + 16);
	const TEXT *p;
	TEXT *q;
	
	for (p = service->svc_switches; *p;)
		if (*p++ == ' ')
			len += 2;
	
	TEXT argv_data_buf[512];
	TempSpace tempSpace(sizeof (argv_data_buf), argv_data_buf);
	tempSpace.resize(len);
	TEXT *argv_data = (TEXT*) tempSpace.space;
	
	/* Create a command line. */

	svc_flag = FALSE;

	for (q = argv_data, p = service_path; *q = *p++; q++)
		;

	p = service->svc_switches;
	
	if (*p)
		*q++ = ' ';

	while (*p == ' ')
		p++;
		
	user_quote = FALSE;
	
	while (*p) 
		{
		TEXT *arg = q;
		*q++ = '\"';
		quote_flag = FALSE;
		
		while (((*q = *p++) && *q != ' ') || user_quote) 
			{
			if (*q == '\\' && *p == ' ' && !user_quote) 
				{
				*q = ' ';
				p++;
				quote_flag = TRUE;
				}
				
			if (*q == '"') 
				{
				user_quote = !user_quote;
				q++;
				continue;
				}
				
			q++;
			}
			
		if (!quote_flag) 
			{
			*arg = ' ';
			
			if (!strncmp(arg, " -Service", q - arg)) 
				{
				if (windows_nt)
					sprintf(q, "_re %lu %x %x", pipe_input, pipe_output, pipe_error);
				else
					sprintf(q, "_re %u %u %u", my_output, my_input, my_output);
					
				q += strlen(q);
				*q = p[-1];
				svc_flag = TRUE;
				}
			}
		else 
			{
			q[1] = q[0];
			*q++ = '\"';
			}
			
		if (!*q)
			break;
			
		*q++ = ' ';
		
		while (*p == ' ')
			p++;
		}

	for (q = argv_data; *p; p++)
		if (*q == SVC_TRMNTR)
			*q = ' ';

	THREAD_EXIT;

	start_crud.cb = sizeof(STARTUPINFO);
	start_crud.lpReserved = NULL;
	start_crud.lpReserved2 = NULL;
	start_crud.cbReserved2 = 0;
	start_crud.lpDesktop = NULL;
	start_crud.lpTitle = NULL;
	start_crud.dwFlags = STARTF_USESHOWWINDOW;
	start_crud.wShowWindow = SW_HIDE;

	if (!(ret = CreateProcess(NULL,
							  argv_data,
							  NULL,
							  NULL,
							  TRUE,
							  NORMAL_PRIORITY_CLASS,
							  NULL,
							  NULL,
							  &start_crud, &pi))) status = GetLastError();

	if (windows_nt) 
		{
		CloseHandle(pipe_input);
		CloseHandle(pipe_output);
		CloseHandle(pipe_error);
		}
		
	THREAD_ENTER;

	if (!ret)
		SVC_post(isc_sys_request, isc_arg_string, "CreateProcess", SYS_ERR, status, 0);

	DuplicateHandle(GetCurrentProcess(), pi.hProcess,
					GetCurrentProcess(), (PHANDLE) & service->svc_handle,
					0, TRUE, DUPLICATE_SAME_ACCESS);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	service->svc_input = (void *) my_input;
	service->svc_output = (void *) my_output;
}


static void service_get(Service* service,
						UCHAR * buffer,
						USHORT length,
						USHORT flags,
						USHORT timeout,
						USHORT * return_length)
{
/**************************************
 *
 *      s e r v i c e _ g e t           ( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *      Get input from a service.  It is assumed
 *      that we have checked out of the scheduler.
 *
 **************************************/
	SHORT iter;
	SLONG n = 0L;
	UCHAR *buf = buffer;
	USHORT bytes_read;
	/* Kludge for PeekNamedPipe to work on Win NT 3.5 */
	UCHAR temp_buf[600];
	DWORD temp_len;
	BOOLEAN windows_nt = ISC_is_WinNT();

	*return_length = 0;
	service->svc_flags &= ~SVC_timeout;
	is_service_running(service);

	if (timeout) 
		{
		/* If a timeout period was given, check every .1 seconds to see if
		   input is available from the pipe.  When something shows up, read
		   what's available until all data has been read, or timeout occurs.
		   Otherwise, set the timeout flag and return.
		   Fall out of the loop if a BROKEN_PIPE error occurs.
		 */
		 
		iter = timeout * 10;
		
		while ((iter--) && ((buf - buffer) < length)) 
			{
			/* PeekNamedPipe sometimes return wrong &n, need to get real
			   length from &temp_len until it works */
			   
			if (windows_nt
				&& !PeekNamedPipe((HANDLE) service->svc_input, temp_buf, 600, &temp_len, (ULONG*) &n, NULL))
				{
				if (GetLastError() == ERROR_BROKEN_PIPE) 
					{
					service->svc_flags |= SVC_finished;
					break;
					}
					
				io_error("PeekNamedPipe", GetLastError(), "service pipe",
						 isc_io_read_err, TRUE);
				}
			else 
				{
				DWORD dwCurrentFilePosition;
				
				/* Set the file pointer to the beginning of the file if we are at the end of the
				   file. */
				   
				temp_len = GetFileSize(service->svc_input, NULL);
				dwCurrentFilePosition = SetFilePointer(service->svc_input, 0, NULL, FILE_CURRENT);

				if (temp_len && temp_len == dwCurrentFilePosition)
					SetFilePointer(service->svc_input, 0, NULL, FILE_BEGIN);
				}

			if (temp_len) 
				{
				/* If data is available, read as much as will fit in buffer */
				
				temp_len = MIN(temp_len, length - (buf - buffer));
				bytes_read = service_read(service, buf, (USHORT) temp_len, flags);
				buf += bytes_read;

				if (bytes_read < temp_len || service->svc_flags & SVC_finished) 
					{
					/* We stopped w/out reading full length, must have hit
					   a newline or special character. */
					break;
					}
				}
			else 
				{
				
				/* PeekNamedPipe() is not returning ERROR_BROKEN_PIPE in WIN95. So,
				   we are going to use WaitForSingleObject() to check if the process
				   on the other end of the pipe is still active. */
				   
				if (!windows_nt && WaitForSingleObject((HANDLE) service->svc_handle,
										1) != WAIT_TIMEOUT)
					{
					service->svc_flags |= SVC_finished;
					break;
					}

				/* No data, so wait a while */
				Sleep(100);
				}
			}
			
		/* If we timed out, set the appropriate flags */
		
		if (iter < 0 && !(service->svc_flags & SVC_finished))
			service->svc_flags |= SVC_timeout;
		}
	else 
		buf += service_read(service, buf, length, flags);

	*return_length = buf - buffer;
}


static void service_put(Service* service, const UCHAR* buffer, USHORT length)
{
/**************************************
 *
 *	s e r v i c e _ p u t		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Send output to a service.  It is assumed
 *	that we have checked out of the scheduler.
 *
 **************************************/
	SLONG n;

	is_service_running(service);

	while (length) 
		{
		if (!WriteFile((HANDLE) service->svc_output, buffer, (SLONG) length, (ULONG*) &n, NULL))
			io_error("WriteFile", GetLastError(), "service pipe", isc_io_write_err, TRUE);

		length -= (USHORT) n;
		buffer += n;
		}

	if (!FlushFileBuffers((HANDLE) service->svc_output))
		io_error("FlushFileBuffers", GetLastError(), "service pipe", isc_io_write_err, TRUE);
}


static USHORT service_read(Service* service, UCHAR * buffer, USHORT length, USHORT flags)
{
/**************************************
 *
 *      s e r v i c e _ r e a d         ( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *      Read data from the named pipe.
 *      Returns true if there is more data to be
 *      read, false if we found the newline or
 *      special character.
 *
 **************************************/
	SLONG len, n = 0;
	UCHAR *buf = buffer;

	while (length) {
		n = 0;
		len = (flags & GET_BINARY) ? length : 1;
		if (ReadFile((HANDLE) service->svc_input, buf, len, (ULONG*) &n, NULL) ||
			GetLastError() == ERROR_BROKEN_PIPE)
		{
			if (!n) {
				service->svc_flags |= SVC_finished;
				break;
			}
			length -= (USHORT) n;
			buf += n;
			if (((flags & GET_LINE) && buf[-1] == '\n') ||
				(!(flags & GET_BINARY) && buf[-1] == '\001'))
				break;
		}
		else
			io_error("ReadFile", GetLastError(), "service pipe",
					 isc_io_read_err, TRUE);
	}

	if ((flags & GET_LINE) && buf[-1] == '\n')
		buf[-1] = ' ';

	return buf - buffer;
}
#endif /* ifndef SUPERSERVER */
#endif /* WIN_NT */


#ifdef SUPERSERVER
static USHORT service_add_one(USHORT i)
{
/**************************************
 *
 *	s e r v i c e _ a d d _ o n e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	return ((i % SVC_STDOUT_BUFFER_SIZE) + 1);
}


static USHORT service_empty(Service* service)
{
/**************************************
 *
 *	s e r v i c e _ e m p t y
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	if (service_add_one(service->svc_stdout_tail) == service->svc_stdout_head)
		return 1;

	return 0;
}


static USHORT service_full(Service* service)
{
/**************************************
 *
 *	s e r v i c e _ f u l l
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	if (service_add_one(service_add_one(service->svc_stdout_tail)) ==
		service->svc_stdout_head) return (1);
	else
		return (0);
}


static UCHAR service_dequeue_byte(Service* service)
{
/**************************************
 *
 *	s e r v i c e _ d e q u e u e _ b y t e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	UCHAR ch;

	ch = service->svc_stdout[service->svc_stdout_head];
	service->svc_stdout_head = service_add_one(service->svc_stdout_head);

	return (ch);
}


static void service_enqueue_byte(UCHAR ch, Service* service)
{
/**************************************
 *
 *	s e r v i c e _ e n q u e u e _ b y t e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
 
	/* Wait for space in buffer while service is not detached. */
	
	while (service_full(service) && !(service->svc_flags & SVC_detached))
		THREAD_SLEEP(1);

	/* Ensure that service is not detached. */
	
	if (!(service->svc_flags & SVC_detached)) 
		{
		service->svc_stdout_tail = service_add_one(service->svc_stdout_tail);
		service->svc_stdout[service->svc_stdout_tail] = ch;
		}
}


static void service_fork(void (*service_executable) (), Service* service)
{
/**************************************
 *
 *	s e r v i c e _ f o r k
 *
 **************************************
 *
 * Functional description
 *	Startup a service.
 *
 **************************************/
	
	// First, count the arguments

	USHORT argc = 2;
	TEXT *p = service->setArgvStrings(service->svc_switches);
	
	while (*p)
		if (*p++ == ' ')
			{
			argc++;
			while (*p == ' ')
				++p;
			}

	TEXT **arg = service->allocArgv(argc);
	*arg++ = (TEXT *)(service_executable);
	
	/* Break up the command line into individual arguments. */

	TEXT *q = p = service->svc_argv_strings;

	while (*q == ' ')
		q++;
		
	while (*q) 
		{
		*arg++ = p;
		
		while ((*p = *q++) && *p != ' ') 
			{
			if (*p == '\\' && *q == ' ') 
				{
				*p = ' ';
				q++;
				}
			p++;
			}
			
		if (!*p)
			break;
			
		*p++ = 0;
		
		while (*q == ' ')
			q++;
		}
		
	*arg = NULL;

	service->svc_stdout = new UCHAR[SVC_STDOUT_BUFFER_SIZE + 1];

	THREAD_EXIT;
	THD_start_thread(reinterpret_cast<FPTR_INT_VOID_PTR>(service_executable),
						service, 0, 0, (void*)&service->svc_handle);
	THREAD_ENTER;
}


static void service_get(Service*		service,
						UCHAR*	buffer,
						USHORT	length,
						USHORT	flags,
						USHORT	timeout,
						USHORT*	return_length)
{
/**************************************
 *
 *	s e r v i c e _ g e t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	int ch = 'Z';
	time_t elapsed_time;

#ifdef HAVE_GETTIMEOFDAY
	struct timeval start_time, end_time;
	GETTIMEOFDAY(&start_time);
#else
	time_t start_time, end_time;
	time(&start_time);
#endif

	*return_length = 0;
	service->svc_flags &= ~SVC_timeout;

	while (length) {
		if (service_empty(service))
			THREAD_SLEEP(1);
#ifdef HAVE_GETTIMEOFDAY
		GETTIMEOFDAY(&end_time);
		elapsed_time = end_time.tv_sec - start_time.tv_sec;
#else
		time(&end_time);
		elapsed_time = end_time - start_time;
#endif
		if ((timeout) && (elapsed_time >= timeout)) {
			service->svc_flags &= SVC_timeout;
			return;
		}

		while (!service_empty(service) && length > 0) {
			ch = service_dequeue_byte(service);
			length--;

			/* If returning a line of information, replace all new line
			 * characters with a space.  This will ensure that the output is
			 * consistent when returning a line or to eof
			 */
			if ((flags & GET_LINE) && (ch == '\n')) {
				buffer[(*return_length)++] = ' ';
				return;
			}
			else
				buffer[(*return_length)++] = ch;
		}

		if (service_empty(service) && (service->svc_flags & SVC_finished))
			return;
	}
}


static void service_put(Service* service, const UCHAR* buffer, USHORT length)
{
/**************************************
 *
 *	s e r v i c e _ p u t
 *
 **************************************
 *
 * Functional description
 *	Send output to a service.  It is assumed
 *	that we have checked out of the scheduler.
 *
 **************************************/

/* Nothing */
}
#endif /* SUPERSERVER */


#if !defined(WIN_NT) && !defined(SUPERSERVER)
static void service_close(Service* service)
{
/**************************************
 *
 *	s e r v i c e _ c l o s e		( G E N E R I C )
 *
 **************************************
 *
 * Functional description
 *	Shutdown the connection to a service.
 *	Simply close the input and output pipes.
 *
 **************************************/

	ib_fclose((IB_FILE *) service->svc_input);
	ib_fclose((IB_FILE *) service->svc_output);
}


static void service_fork(TEXT * service_path, Service* service)
{
/**************************************
 *
 *	s e r v i c e _ f o r k		( G E N E R I C )
 *
 **************************************
 *
 * Functional description
 *	Startup a service.
 *
 **************************************/
	int pair1[2], pair2[2], pid;
	struct stat stat_buf;
	TEXT **argv, *argv_buf[20], **arg, *argv_data, argv_data_buf[512], *p, *q;
	USHORT argc, len;

	if (pipe(pair1) < 0 || pipe(pair2) < 0)
		io_error("pipe", errno, "", isc_io_create_err, FALSE);

	/* Probe service executable to see it if plausibly exists. */

	if (statistics(service_path, &stat_buf) == -1)
		io_error("stat", errno, service_path, isc_io_access_err, FALSE);

	/* Make sure we have buffers that are large enough to hold the number
	   and size of the command line arguments. */

	for (argc = 2, p = service->svc_switches; *p;)
		{
		if (*p == ' ')
			{
			argc++;
			while (*p == ' ')
				p++;
			}
		else
			{
			if (*p == SVC_TRMNTR)
				{
				while (*p++ && *p != SVC_TRMNTR)
					;
				fb_assert (*p == SVC_TRMNTR);
				}
			p++;
			}
		}

	if (argc > FB_NELEM(argv_buf))
		argv = (TEXT **) gds__alloc((SLONG) (argc * sizeof(TEXT *)));
	else
		argv = argv_buf;
		
	/* FREE: at procedure return */
	
	if (!argv)					/* NOMEM: */
		SVC_post(isc_virmemexh, 0);
		
	service->svc_argc = argc;

	len = strlen(service->svc_switches) + 1;
	
	if (len > sizeof(argv_data_buf))
		argv_data = (TEXT *) gds__alloc((SLONG) len);
	else
		argv_data = argv_data_buf;
		
	/* FREE: at procedure return */
	
	if (!argv_data) 
		{			/* NOMEM: */
		if (argv != argv_buf)
			gds__free(argv);
		SVC_post(isc_virmemexh, 0);
		}

	/* Break up the command line into individual arguments. */

	arg = argv;
	*arg++ = service_path;

	p = argv_data;
	q = service->svc_switches;

	while (*q == ' ')
		q++;
		
	while (*q)
		{
		*arg = p;
		while (*p = *q++)
			{
			if (*p == ' ') 
				break;

			if (*p == SVC_TRMNTR)
				{
				*arg = ++p;	/* skip terminator */
				while (*p = *q++)
					/* If *q points to the last argument, then terminate the argument */
					if ((*q == 0 || *q == ' ') && *p == SVC_TRMNTR)
						{
						*p = '\0';
						break;
						}
					else
						p++;
				}

			if (*p == '\\' && *q == ' ')
				{
				*p = ' ';
				q++;
				}
				
			p++;
			}
			
		arg++;
		
		if (!*p)
			break;
			
		*p++ = '\0';
		
		while (*q == ' ')
			q++;
		}
		
	*arg = NULL;

	/* At last we can fork the sub-process.  If the fork succeeds, repeat
	   it so that we don't have defunct processes hanging around. */

	THREAD_EXIT;

	switch (pid = vfork()) 
		{
		case -1:
			THREAD_ENTER;
			if (argv != argv_buf)
				gds__free(argv);
			if (argv_data != argv_data_buf)
				gds__free(argv_data);
			SVC_post(isc_sys_request, isc_arg_string, "vfork", SYS_ERR, errno, 0);
			break;

		case 0:
			if (vfork() != 0)
				_exit(FINI_OK);

			close(pair1[0]);
			close(pair2[1]);
			
			if (pair2[0] != 0) 
				{
				close(0);
				dup(pair2[0]);
				close(pair2[0]);
				}
				
			if (pair1[1] != 1) 
				{
				close(1);
				dup(pair1[1]);
				close(pair1[1]);
				}
			close(2);
			dup(1);
			
#ifdef DEV_BUILD
			{
				char buf[2 * MAXPATHLEN];
				char **s = argv;

				strcpy (buf, "service_fork:");
				while (*s != (char *)0)
				{
					strcat (buf, " ");
					strcat (buf, *s);
					s++;
				}
				gds__log(buf);
			}
#endif
			execvp(argv[0], argv);
			_exit(FINI_ERROR);
		}

	close(pair1[1]);
	close(pair2[0]);

	waitpid(pid, NULL, 0);

	THREAD_ENTER;

	if (argv != argv_buf)
		gds__free(argv);
		
	if (argv_data != argv_data_buf)
		gds__free(argv_data);

	if (!(service->svc_input = (void *) ib_fdopen(pair1[0], "r")) ||
		!(service->svc_output = (void *) ib_fdopen(pair2[1], "w")))
		io_error("ib_fdopen", errno, "service path", isc_io_access_err,
				 FALSE);
}


static void service_get(Service* service,
						SCHAR * buffer,
						USHORT length,
						USHORT flags, USHORT timeout, USHORT * return_length)
{
/**************************************
 *
 *	s e r v i c e _ g e t		( G E N E R I C )
 *
 **************************************
 *
 * Functional description
 *	Get input from a service.  It is assumed
 *	that we have checked out of the scheduler.
 *
 **************************************/
	struct itimerval sv_timr;
	struct sigaction sv_hndlr;
	int c;
	//USHORT timed_out;
	SCHAR *buf;
	SSHORT iter = 0;
	int errno_save;

	is_service_running(service);

	errno = 0;
	service->svc_flags &= ~SVC_timeout;
	buf = buffer;

	if (timeout) {
		ISC_set_timer((SLONG) (timeout * 100000), timeout_handler, service,
					  (SLONG*)&sv_timr, (void**)&sv_hndlr);
		iter = timeout * 10;
	}

	while (!timeout || iter) {
		if ((c = ib_getc((IB_FILE *) service->svc_input)) != EOF) {
			*buf++ = c;
			if (!(--length))
				break;
			if (((flags & GET_LINE) && c == '\n') ||
				(!(flags & GET_BINARY) && c == '\001'))
				break;
		}
		else if (!errno) {
			service->svc_flags |= SVC_finished;
			break;
		}
		else if (SYSCALL_INTERRUPTED(errno)) {
			if (timeout)
				--iter;
		}
		else {
			errno_save = errno;
			if (timeout)
				ISC_reset_timer(timeout_handler, service, (SLONG*)&sv_timr,
								(void**)&sv_hndlr);
			io_error("ib_getc", errno_save, "service pipe", isc_io_read_err,
					 TRUE);
		}
	}

	if (timeout) {
		ISC_reset_timer(timeout_handler, service, (SLONG*)&sv_timr, (void**)&sv_hndlr);
		if (!iter)
			service->svc_flags |= SVC_timeout;
	}

	*return_length = buf - buffer;
}


static void service_put(Service* service, const SCHAR* buffer, USHORT length)
{
/**************************************
 *
 *	s e r v i c e _ p u t		( G E N E R I C )
 *
 **************************************
 *
 * Functional description
 *	Send output to a service.  It is assumed
 *	that we have checked out of the scheduler.
 *
 **************************************/

	is_service_running(service);

	while (length--) 
		{
		if (ib_putc(*buffer, (IB_FILE *) service->svc_output) != EOF)
			buffer++;
		else if (SYSCALL_INTERRUPTED(errno)) 
			{
			ib_rewind((IB_FILE *) service->svc_output);
			length++;
			}
		else
			io_error("ib_putc", errno, "service pipe", isc_io_write_err, TRUE);
		}

	if (ib_fflush((IB_FILE *) service->svc_output) == EOF)
		io_error("ib_fflush", errno, "service pipe", isc_io_write_err, TRUE);
}


static void timeout_handler(void *service)
{
/**************************************
 *
 *	t i m e o u t _ h a n d l e r
 *
 **************************************
 *
 * Functional description
 *	Called when no input has been received
 *	through a pipe for a specificed period
 *	of time.
 *
 **************************************/
}
#endif // !defined(WIN_NT) && !defined(SUPERSERVER)


void SVC_cleanup(Service* service)
{
/**************************************
 *
 *	S V C _ c l e a n u p
 *
 **************************************
 *
 * Functional description
 *	Cleanup memory used by service.
 *
 **************************************/

#ifndef SUPERSERVER
	/* If we forked an executable, close down it's pipes */

	if (service->svc_flags & SVC_forked)
		service_close(service);
#else
	/***
	if (service->svc_stdout) 
		{
		gds__free((SLONG *) service->svc_stdout);
		service->svc_stdout = NULL;
		}
		
	if (service->svc_argv) 
		{
		gds__free((SLONG *) service->svc_argv);
		service->svc_argv = NULL;
		}
	***/
#endif

	delete service;
}


void SVC_finish(Service* service, USHORT flag)
{
/**************************************
 *
 *	S V C _ f i n i s h
 *
 **************************************
 *
 * Functional description
 *	o Set the flag (either SVC_finished for the main service thread or
 *	  SVC_detached for the client thread).
 *	o If both main thread and client thread are completed that is main
 *	  thread is finished and client is detached then free memory
 *	  used by service.
 *
 **************************************/

	Sync sync(&svc_mutex, "SVC_finish");
	sync.lock(Exclusive);
	
	if (service && ((flag == SVC_finished) || (flag == SVC_detached))) 
		{
		service->svc_flags |= flag;
		
		if ((service->svc_flags & SVC_finished) &&
			(service->svc_flags & SVC_detached)) SVC_cleanup(service);
		else 
			if (service->svc_flags & SVC_finished) 
				{
				service->svc_flags &= ~SVC_thd_running;
				service->svc_handle = 0;
				}
		}

}


static JString conv_switches(USHORT spb_length,
						  USHORT opt_switch_len,
						  const UCHAR* spb)
{
/**************************************
 *
 *	c o n v _ s w i t c h e s
 *
 **************************************
 *
 * Functional description
 *	Convert spb flags to utility switches.
 *
 **************************************/
	USHORT total;
	const UCHAR* p = spb;
	JString text;

	if (*p < isc_action_min || *p > isc_action_max)
		return "";					/* error action not defined */

	/* Calculate the total length */
	
	if ((total = process_switches(spb_length, p, NULL)) == 0)
		return "";

	TEXT *switches = text.getBuffer(total + opt_switch_len + sizeof(SERVICE_THD_PARAM) + 1);
	
	/***
	*switches = TEXT *) gds__alloc(total + opt_switch_len + sizeof(SERVICE_THD_PARAM) + 1);
	// NOMEM: return error 
	
	if (!*switches)
		SVC_post(isc_virmemexh, 0);
	***/
	
	strcpy(switches, SERVICE_THD_PARAM);
	strcat(switches, " ");
	process_switches(spb_length, p, switches + strlen(SERVICE_THD_PARAM) + 1);
	text.releaseBuffer();
	
	return text;
}


static const TEXT* find_switch(int in_spb_sw, IN_SW_TAB table)
{
/**************************************
 *
 *      f i n d _ s w i t c h
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	IN_SW_TAB in_sw_tab;

	for (in_sw_tab = table; in_sw_tab->in_sw_name; in_sw_tab++) {
		if (in_spb_sw == in_sw_tab->in_spb_sw)
			return in_sw_tab->in_sw_name;
	}

	return NULL;
}


static USHORT process_switches(USHORT spb_length, const UCHAR* spb, TEXT* switches)
{
/**************************************
 *
 *      p r o c e s s _ s w i t c h e s
 *
 **************************************
 *
 *Functional description
 *   Loop through the appropriate switch table
 *   looking for the text for the given command switch.
 *
 *   Calling this function with switches = NULL returns
 *   the number of bytes to allocate for the switches and
 *   parameters.
 *
 **************************************/
	USHORT len, total;
	BOOLEAN found = FALSE;

	if (spb_length == 0)
		return 0;

	const UCHAR* p = spb;
	len = spb_length;
	TEXT* sw = switches;

	// CVC: Isn't ISC_USHORT an external name?
	
	const ISC_USHORT svc_action = *p;

	if (len > 1) 
		{
		++p;
		--len;
		}

	total = 0;					/* total length of the command line */

	while (len > 0) 
		{
		switch (svc_action) 
			{
			case isc_action_svc_delete_user:
			case isc_action_svc_display_user:
				if (!found) 
					{
					if (spb != p) 
						{
						--p;
						++len;
						}
						
					fb_assert(spb == p);
					
					if (!get_action_svc_parameter(&p, gsec_action_in_sw_table, &sw, &total, &len))
						return 0;
						
					found = TRUE;
					/* in case of "display all users" the spb buffer contains
					nothing but isc_action_svc_display_user */
					if (len == 0)
						break;
					}

				switch (*p) 
					{
					case isc_spb_sec_username:
						++p;
						--len;
						get_action_svc_string(&p, &sw, &total, &len);
						break;

					default:
						return 0;
						break;
					}
				break;

			case isc_action_svc_add_user:
			case isc_action_svc_modify_user:
				if (!found) 
					{
					if (spb != p) 
						{
						--p;
						++len;
						}
						
					fb_assert(spb == p);
					
					if (!get_action_svc_parameter(&p, gsec_action_in_sw_table,
												&sw, &total, &len))
						return 0;
						
					found = TRUE;
					
					if (*p != isc_spb_sec_username) 
						/* unexpected service parameter block: expected %d, encountered %d */
						SVC_post(isc_unexp_spb_form, isc_arg_string,SPB_SEC_USERNAME, 0);
					}

				switch (*p) 
					{
					case isc_spb_sec_userid:
					case isc_spb_sec_groupid:
						if (!get_action_svc_parameter(&p, gsec_in_sw_table,
													&sw, &total, &len))
							return 0;
							
						get_action_svc_data(&p, &sw, &total, &len);
						break;

					case isc_spb_sec_username:
						++p;
						--len;
						get_action_svc_string(&p, &sw, &total, &len);
						break;
						
					case isc_spb_sql_role_name:
					case isc_spb_sec_password:
					case isc_spb_sec_groupname:
					case isc_spb_sec_firstname:
					case isc_spb_sec_middlename:
					case isc_spb_sec_lastname:
						if (!get_action_svc_parameter(&p, gsec_in_sw_table,
													&sw, &total, &len))
							return 0;
						get_action_svc_string(&p, &sw, &total, &len);
						break;

					default:
						return 0;
						break;
					}
				break;

			case isc_action_svc_db_stats:
				switch (*p) 
					{
					case isc_spb_dbname:
						++p;
						--len;
						get_action_svc_string(&p, &sw, &total, &len);
						break;

					case isc_spb_options:
						++p;
						--len;
						if (!get_action_svc_bitmask(&p, dba_in_sw_table,
													&sw, &total, &len))
							return 0;
						break;

					case isc_spb_command_line: 
						++p;
						--len;
						get_action_svc_string(&p, &sw, &total, &len);
						break;

					default:
						return 0;
						break;
					}
				break;

			case isc_action_svc_backup:
			case isc_action_svc_restore:
				switch (*p) 
					{
					case isc_spb_bkp_file:
					case isc_spb_dbname:
					case isc_spb_sql_role_name:
						++p;
						--len;
						get_action_svc_string(&p, &sw, &total, &len);
						break;
						
					case isc_spb_options:
						++p;
						--len;
						if (!get_action_svc_bitmask(&p, burp_in_sw_table,
													&sw, &total, &len))
							return 0;
						break;
						
					case isc_spb_bkp_length:
					case isc_spb_res_length:
						++p;
						--len;
						get_action_svc_data(&p, &sw, &total, &len);
						break;
						
					case isc_spb_bkp_factor:
					case isc_spb_res_buffers:
					case isc_spb_res_page_size:
						if (!get_action_svc_parameter(&p, burp_in_sw_table,
													&sw, &total, &len))
							return 0;
						get_action_svc_data(&p, &sw, &total, &len);
						break;
						
					case isc_spb_res_access_mode:
						++p;
						--len;
						if (!get_action_svc_parameter(&p, burp_in_sw_table,
													&sw, &total, &len))
							return 0;
						break;
						
					case isc_spb_verbose:
						if (!get_action_svc_parameter(&p, burp_in_sw_table,
													&sw, &total, &len))
							return 0;
						break;
						
					default:
						return 0;
					}
				break;

			case isc_action_svc_repair:
			case isc_action_svc_properties:
				switch (*p) 
					{
					case isc_spb_dbname:
						++p;
						--len;
						get_action_svc_string(&p, &sw, &total, &len);
						break;
						
					case isc_spb_options:
						++p;
						--len;
						if (!get_action_svc_bitmask(&p, alice_in_sw_table,
													&sw, &total, &len))
							return 0;
						break;
						
					case isc_spb_prp_page_buffers:
					case isc_spb_prp_sweep_interval:
					case isc_spb_prp_shutdown_db:
					case isc_spb_prp_deny_new_attachments:
					case isc_spb_prp_deny_new_transactions:
					case isc_spb_prp_set_sql_dialect:
					case isc_spb_rpr_commit_trans:
					case isc_spb_rpr_rollback_trans:
					case isc_spb_rpr_recover_two_phase:
						if (!get_action_svc_parameter(&p, alice_in_sw_table,
													&sw, &total, &len))
							return 0;
						get_action_svc_data(&p, &sw, &total, &len);
						break;
						
					case isc_spb_prp_write_mode:
					case isc_spb_prp_access_mode:
					case isc_spb_prp_reserve_space:
						++p;
						--len;
						if (!get_action_svc_parameter(&p, alice_in_sw_table,
													&sw, &total, &len))
							return 0;
						break;
						
					default:
						return 0;

					}
				break;
				
			default:
				return 0;
			}
		}

	if (sw && *(sw - 1) == ' ')
		*--sw = '\0';

	return total;
}


static BOOLEAN get_action_svc_bitmask( const UCHAR** spb,
									  const IN_SW_TAB table,
									  TEXT** cmd,
									  USHORT* total, USHORT* len)
{
/**************************************
 *
 *	g e t _ a c t i o n _ s v c _ b i t m a s k
 *
 **************************************
 *
 * Functional description
 *	Get bitmask from within spb buffer,
 *	find corresponding switches within specified table,
 *	add them to the command line,
 *	adjust pointers.
 *
 **************************************/
 
	const TEXT* s_ptr;
	const ISC_ULONG opt = gds__vax_integer(*spb, sizeof(ISC_ULONG));
	ISC_ULONG mask = 1;
	
	for (int count = (sizeof(ISC_ULONG) * 8) - 1; count; --count) 
		{
		if (opt & mask) 
			{
			if (!(s_ptr = find_switch((opt & mask), table)))
				return FALSE;
				
			if (*cmd) 
				{
				sprintf(*cmd, "-%s ", s_ptr);
				*cmd += 1 + strlen(s_ptr) + 1;
				}
				
			*total += (USHORT) (1 + strlen(s_ptr) + 1);
			}
			
		mask = mask << 1;
		}

	*spb += sizeof(ISC_ULONG);
	*len -= sizeof(ISC_ULONG);
	
	return TRUE;
}


static void get_action_svc_string(const UCHAR** spb,
								  TEXT** cmd, USHORT* total, USHORT* len)
{
/**************************************
 *
 *      g e t _ a c t i o n _ s v c _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *      Get string from within spb buffer,
 *      add it to the command line, adjust pointers.
 *
 *      All string parameters are delimited by SVC_TRMNTR.  This
 *      is done to ensure that paths with spaces are handled correctly
 *      when creating the argc / argv paramters for the service.
 *
 **************************************/
	const ISC_USHORT l = gds__vax_integer(reinterpret_cast<const UCHAR*>(*spb),
						 sizeof(ISC_USHORT));

/* Do not go beyond the bounds of the spb buffer */
	if (l > *len)
		SVC_post(isc_bad_spb_form, 0);

	*spb += sizeof(ISC_USHORT);
	if (*cmd)
	{
		**cmd = SVC_TRMNTR;
		*cmd += 1;
		MOVE_FASTER(*spb, *cmd, l);
		*cmd += l;
		**cmd = SVC_TRMNTR;
		*cmd += 1;
		**cmd = ' ';
		*cmd += 1;
	}
	*spb += l;
	*total += l + 1 + 2;		/* Two SVC_TRMNTR for strings */
	*len -= sizeof(ISC_USHORT) + l;
}


static void get_action_svc_data(const UCHAR** spb, TEXT** cmd, USHORT* total, USHORT* len)
{
/**************************************
 *
 *      g e t _ a c t i o n _ s v c _ d a t a
 *
 **************************************
 *
 * Functional description
 *      Get data from within spb buffer,
 *      add it to the command line, adjust pointers.
 *
 **************************************/
	TEXT buf[64];

	const ISC_ULONG ll = gds__vax_integer(*spb, sizeof(ISC_ULONG));
	sprintf(buf, "%u ", ll);
	
	if (*cmd) 
		{
		sprintf(*cmd, "%lu ", ll);
		*cmd += strlen(buf);
		}
		
	*spb += sizeof(ISC_ULONG);
	*total += (USHORT) (strlen(buf) + 1);
	*len -= sizeof(ISC_ULONG);
}


static BOOLEAN get_action_svc_parameter(const UCHAR** spb,
										IN_SW_TAB table,
										TEXT** cmd,
										USHORT* total, USHORT* len)
{
/**************************************
 *
 *      g e t _ a c t i o n _ s v c _ p a r a m e t e r
 *
 **************************************
 *
 * Functional description
 *      Get parameter from within spb buffer,
 *      find corresponding switch within specified table,
 *	add it to the command line,
 *	adjust pointers.
 *
 **************************************/
	const TEXT* s_ptr;

	if (!(s_ptr = find_switch(**spb, table)))
		return FALSE;

	if (*cmd) 
		{
		sprintf(*cmd, "-%s ", s_ptr);
		*cmd += 1 + strlen(s_ptr) + 1;
		}

	*spb += 1;
	*total += (USHORT) (1 + strlen(s_ptr) + 1);
	*len -= 1;
	
	return TRUE;
}

#ifdef DEBUG
/* The following two functions are temporary stubs and will be
 * removed as the services API takes shape.  They are used to
 * test that the paths for starting services and parsing command-lines
 * are followed correctly.
 */
int test_thread(Service* service)
{
	gds__log("Starting service");
	return FINI_OK;
}

void test_cmd(USHORT spb_length, SCHAR * spb, TEXT ** switches)
{
	gds__log("test_cmd called");
}
#endif

