// Attachment.cpp: implementation of the Attachment class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include "IscDbc.h"
#include "Attachment.h"
#include "SQLError.h"
#include "Parameters.h"
#include "IscConnection.h"
#include "PBGen.h"

static char databaseInfoItems [] = { 
	isc_info_db_SQL_dialect,
	isc_info_version, 
	isc_info_end 
	};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Attachment::Attachment()
{
	useCount = 1;
	databaseHandle = 0;
}

Attachment::~Attachment()
{
	ISC_STATUS statusVector [20];

	if (databaseHandle)
		isc_detach_database (statusVector, &databaseHandle);
}

void Attachment::openDatabase(const char *dbName, Properties *properties)
{
	databaseName = dbName;
	PBGen dpb(isc_dpb_version1);

	const char *user = properties->findValue ("user", NULL);

	if (user)
		dpb.putParameter(isc_dpb_user_name, user);

	const char *password = properties->findValue ("password", NULL);

	if (password)
		dpb.putParameter(isc_dpb_password, password);

	const char *role = properties->findValue ("role", NULL);

	if (role)
		dpb.putParameter(isc_dpb_sql_role_name, role);
		
	ISC_STATUS statusVector [20];

	if (isc_attach_database (statusVector, strlen (dbName), (char*) dbName, &databaseHandle, 
							 dpb.getLength(), (char*) dpb.buffer))
		{
		JString text = IscConnection::getIscStatusText (statusVector);
		throw SQLEXCEPTION (statusVector [1], text);
		}

	char result [100];
	dialect = 0;

	if (!isc_database_info (statusVector, &databaseHandle, sizeof (databaseInfoItems), databaseInfoItems, sizeof (result), result))
		{
		for (char *p = result; p < result + sizeof (result) && *p != isc_info_end;)
			{
			char item = *p++;
			int length = isc_vax_integer (p, 2);
			p += 2;
			switch (item)
				{
				case isc_info_db_SQL_dialect:
					dialect = isc_vax_integer (p, length);
					break;

				case isc_info_version:
					databaseVersion = JString (p + 2, p [1]);
					break;
				}
			p += length;
			}
		}

	switch (dialect)
		{
		case 0:
			quotedIdentifiers = false;
			break;

		case SQL_DIALECT_V5:
		case SQL_DIALECT_V6:
		default:
			quotedIdentifiers = true;
		}
}

void Attachment::addRef()
{
	++useCount;
}

int Attachment::release()
{
	if (--useCount == 0)
		{
		delete this;
		return 0;
		}

	return useCount;
}
