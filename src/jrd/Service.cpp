// Service.cpp: implementation of the Service class.
//
//////////////////////////////////////////////////////////////////////

#include "firebird.h"
#include "common.h"
#include "Service.h"
#include "../common/classes/alloc.h"
#include "jrd_pwd.h"
#include "enc_proto.h"
#include "ConfObject.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Service::Service(ConfObject *config)
{
	configuration = config;
	svc_input = NULL;
	svc_output = NULL;
	svc_stdout = NULL;
	svc_argv = NULL;
	svc_service = NULL;
	svc_argv_strings = NULL;
	
	svc_stdout_head = 0;
	svc_stdout_tail = 0;
	svc_argc = 0;
	responseOffset = 0;
	responseLength = 0;
	svc_flags = 0;
	svc_user_flag = 0;
	svc_spb_version = 0;
	svc_do_shutdown = 0;
}

Service::~Service()
{
	delete [] svc_stdout;
	delete [] svc_argv;
	delete [] svc_argv_strings;
}

void Service::setEncryptedPassword(const char* password)
{
	encryptedPassword = password;
}

void Service::setPassword(const char * password)
{
	TEXT encrypted[128];
	ENC_crypt(encrypted, sizeof(encrypted), password, PASSWORD_SALT);
	encryptedPassword = encrypted;
}

const char* Service::getSecurityDatabase(void)
{
	return configuration->getValue("SecurityDatabase", "security.fdb");
}

TEXT** Service::allocArgv(int count)
{
	delete svc_argv;
	svc_argc = count;
	svc_argv = new TEXT*[count + 1];
	
	return svc_argv;
}

TEXT* Service::setArgvStrings(const TEXT* string)
{
	delete [] svc_argv_strings;
	svc_argv_strings = new char [strlen(string) + 1];
	strcpy (svc_argv_strings, string);
	
	return svc_argv_strings;
}

void Service::svc_started(void)
{
}
