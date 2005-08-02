// Service.cpp: implementation of the Service class.
//
//////////////////////////////////////////////////////////////////////

#include "firebird.h"
#include "common.h"
#include "Service.h"
//#include "all.h"
#include "../common/classes/alloc.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Service::Service()
{
	svc_input = NULL;
	svc_output = NULL;
	svc_stdout = NULL;
	svc_argv = NULL;
	svc_service = NULL;
	svc_resp_buf = NULL;
	svc_resp_ptr = NULL;
	svc_switches = NULL;
	
	svc_stdout_head = 0;
	svc_stdout_tail = 0;
	svc_argc = 0;
	svc_resp_buf_len = 0;
	svc_resp_len = 0;
	svc_flags = 0;
	svc_user_flag = 0;
	svc_spb_version = 0;
	svc_do_shutdown = 0;
}

Service::~Service()
{

}
