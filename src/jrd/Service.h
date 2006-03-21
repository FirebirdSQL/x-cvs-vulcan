// Service.h: interface for the Service class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVICE_H__64F0DF89_DA26_48DD_951A_5BCDAF8CCDA4__INCLUDED_)
#define AFX_SERVICE_H__64F0DF89_DA26_48DD_951A_5BCDAF8CCDA4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../jrd/jrd_blks.h"
#include "../include/fb_blk.h"
#include "AsyncEvent.h"
#include "JString.h"
#include "TempSpace.h"
#include "thd.h"
//#include "jrd_pwd.h"

/***
#ifndef MAX_PASSWORD_ENC_LENGTH
#define MAX_PASSWORD_ENC_LENGTH 12
#endif
***/

/* Bitmask values for the svc_flags variable */

#define SVC_eof			 1
#define SVC_timeout		 2
#define SVC_forked		 4
#define SVC_detached	 8
#define SVC_finished	16
#define SVC_thd_running	32
#define SVC_evnt_fired	64
#define SVC_cmd_line	128

class Service;
CLASS(ConfObject);

//typedef int (*pfn_svc_main) (Service*);
typedef int (*pfn_svc_output)(Service*, const UCHAR*);

struct serv
{
	USHORT				serv_action;
	const TEXT*			serv_name;
	const TEXT*			serv_std_switches;
	const TEXT*			serv_executable;
	//pfn_svc_main		serv_thd;
	ThreadEntryPoint*	serv_thd;
	BOOLEAN*			in_use;
};

typedef serv *SERV;

class Service //: public pool_alloc<type_svc>
{
public:
	Service(ConfObject *configuration);
	virtual		~Service();
	ISC_STATUS		svc_status[ISC_STATUS_LENGTH];
	SLONG			svc_handle;			/* "handle" of process/thread running service */
	void*			svc_input;			/* input to service */
	void*			svc_output;			/* output from service */
	ULONG			svc_stdout_head;
	ULONG			svc_stdout_tail;
	UCHAR*			svc_stdout;
	ULONG			svc_argc;
	TEXT**			svc_argv;
	TEXT*			svc_argv_strings;
	AsyncEvent		svc_start_event[1];	/* fired once service has started successfully */
	const serv*		svc_service;
	//UCHAR*		svc_resp_buf;
	//USHORT		svc_resp_buf_len;
	TempSpace		responseBuffer;
	USHORT			responseOffset;
	USHORT			responseLength;
	USHORT			svc_flags;
	USHORT			svc_user_flag;
	USHORT			svc_spb_version;
	BOOLEAN			svc_do_shutdown;
	//TEXT			svc_enc_password[MAX_PASSWORD_ENC_LENGTH];
	TEXT			svc_reserved[1];
	JString			svc_switches;
	JString			encryptedPassword;
	JString			userName;
	ConfObject		*configuration;
	
	void setEncryptedPassword(const char* password);
	void setPassword(const char * password);
	const char* getSecurityDatabase(void);
	TEXT** allocArgv(int count);
	TEXT* setArgvStrings(const TEXT* string);
	void svc_started(void);
};


#endif // !defined(AFX_SERVICE_H__64F0DF89_DA26_48DD_951A_5BCDAF8CCDA4__INCLUDED_)
