// OcsArchive.cpp: implementation of the COcsArchive class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Storage.h"
#include "OcsArchive.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

COcsArchive::COcsArchive(CArchive& ar)
{
	env = OCS_get_env();
	env->get = ocsArchiveGet;
	env->put = ocsArchivePut;
	env->error = ocsArchiveError;
	env->alloc = ocsAlloc;
	env->free = ocsFree;
	archive = &ar;
}

COcsArchive::~COcsArchive()
{
	(env->delete_fn)(env);
}


int COcsArchive::ocsArchiveGet (void* read_handle, char *data, int length, struct ocs_env*)
{
/**************************************
 *
 *		o c s A r c h i v e G e t
 *
 **************************************
 *
 * Functional description
 *		OCS function for peaceful cooperation with MS archive
 *		mechanism.
 *
 **************************************/
CArchive *ar = (CArchive*) read_handle;
int n = ar->Read (data, length);

return n;
}

int COcsArchive::ocsArchivePut (void* write_handle, char *data, int length, struct ocs_env*)
{
/**************************************
 *
 *		o c s A r c h i v e P u t
 *
 **************************************
 *
 * Functional description
 *		OCS function for peaceful cooperation with MS archive
 *		mechanism.
 *
 **************************************/
CArchive *ar = (CArchive*) write_handle;
ar->Write (data, length);

return length;
}

void* COcsArchive::ocsAlloc (int size, struct ocs_env *env)
{
/**************************************
 *
 *		o c s A l l o c
 *
 **************************************
 *
 * Functional description
 *		Allocate a block of memory for OCS.
 *
 **************************************/

return malloc (size);
}

void COcsArchive::ocsFree (void *block, struct ocs_env *env)
{
/**************************************
 *
 *		o c s F r e e
 *
 **************************************
 *
 * Functional description
 *		deallocate a block of memory for OCS.
 *
 **************************************/

free (block);
}


COcsArchive::operator OCS_ENV()
{
	return env;
}

void COcsArchive::ocsArchiveError(int number, char * text, struct ocs_env * env)
{
/**************************************
 *
 *		o c s A r c h i v e E r r o r
 *
 **************************************
 *
 * Functional description
 *		Report an error.
 *
 **************************************/

AfxMessageBox (text);
}
