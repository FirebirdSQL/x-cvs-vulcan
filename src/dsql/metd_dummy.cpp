#if defined(UNIX) && defined(SUPERSERVER)
#include <setjmp.h>
#endif

#include "fbdev.h"
#include <string.h>
#include "../dsql/dsql.h"
#include "../dsql/node.h"
#include "../dsql/sym.h"
#include "../jrd/gds.h"
#include "../jrd/align.h"
#include "../jrd/intl.h"
//#include "../jrd/thd.h"
#include "../dsql/alld_proto.h"
#include "../dsql/ddl_proto.h"
#include "../dsql/metd_proto.h"
#include "../dsql/hsh_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/errd_proto.h"
#include "../jrd/gds_proto.h"
//#include "../jrd/sch_proto.h"
//#include "../jrd/thd_proto.h"
#include "../jrd/constants.h"

//#include "../jrd/cmp_proto.h"
//#include "../jrd/exe_proto.h"

class str;
class dsql_intlsym;


static void notYetImplemented ()
	{
	}

void METD_drop_function(dsql_req*, const dsql_str*)
	{
	notYetImplemented();
	}

void METD_drop_procedure(dsql_req*, const dsql_str*)
	{
	notYetImplemented();
	}


void METD_drop_relation(dsql_req*, const dsql_str*)
	{
	notYetImplemented();
	}



dsql_intlsym*  METD_get_charset(dsql_req*, USHORT, const char* name /* UTF-8 */)
	{
	notYetImplemented();
	return NULL;
	}


USHORT   METD_get_charset_bpc (struct dsql_req *, SSHORT)
	{
	notYetImplemented();
	return 0;
	}


dsql_intlsym*  METD_get_collation(dsql_req*, const dsql_str*)
	{
	notYetImplemented();
	return NULL;
	}


void METD_get_col_default(dsql_req*, const char*, const char*, bool*, TEXT*, USHORT)
	{
	notYetImplemented();
	}

dsql_str*      METD_get_default_charset(dsql_req*)
	{
	notYetImplemented();
	return NULL;
	}

USHORT METD_get_domain(dsql_req*, class dsql_fld*, const char* name /* UTF-8 */)
	{
	notYetImplemented();
	return 0;
	}

void METD_get_domain_default(dsql_req*, const TEXT*, bool*, TEXT*, USHORT)
	{
	notYetImplemented();
	}

/***
dsql_udf*      METD_get_function(dsql_req*, const dsql_str*)
	{
	notYetImplemented();
	return NULL;
	}
***/

DSQL_NOD METD_get_primary_key(dsql_req*, const dsql_str*)
	{
	notYetImplemented();
	return NULL;
	}


Procedure *METD_get_procedure(dsql_req*, const dsql_str*)
	{
	notYetImplemented();
	return NULL;
	}


dsql_rel* METD_get_relation(dsql_req*, const dsql_str*)
	{
	notYetImplemented();
	return NULL;
	}


dsql_str*      METD_get_trigger_relation(dsql_req*, const dsql_str*, USHORT*)
	{
	notYetImplemented();
	return NULL;
	}


USHORT   METD_get_type(dsql_req*, const dsql_str*, char*, SSHORT*)
	{
	notYetImplemented();
	return 0;
	}


dsql_rel* METD_get_view_relation(dsql_req*   request,
								const char* view_name         /* UTF-8 */,
								const char* relation_or_alias /* UTF-8 */,
								USHORT      level)
	{
	notYetImplemented();
	return NULL;
	}


