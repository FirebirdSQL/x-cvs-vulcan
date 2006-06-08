// EngineInfo.cpp: implementation of the EngineInfo class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <string.h>
#include "ibase.h"
#include "fbdev.h"
#include "EngineInfo.h"
#include "DatabaseManager.h"
#include "Database.h"
#include "Attachment.h"
#include "EngineInfoGen.h"
#include "license.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

EngineInfoRequest::EngineInfoRequest()
{
	init();
}

void EngineInfoRequest::init()
{
	databases = false;
	databasePaths = false;
	databaseAttachments = false;
	engineVersion = false;
}

bool EngineInfoRequest::databaseList()
{
	return (databasePaths || databaseAttachments);
}

EngineInfo::EngineInfo(DatabaseManager* dbManager)
{
	databaseManager = dbManager;
}

ISC_STATUS EngineInfo::getInfo(ISC_STATUS* userStatus, int itemsLength, const UCHAR *items, 
								int bufferLength, UCHAR *buffer)
{
	EngineInfoRequest engineInfoRequest;

	UCHAR requestItem;
	const UCHAR* requestItems = items;
	const UCHAR* const endRequestItems = requestItems + itemsLength;
	
	while (requestItems < endRequestItems && *requestItems != isc_info_end)
		switch ((requestItem = *requestItems++))
		{
			case isc_info_engine_req_database_paths:
				engineInfoRequest.databasePaths = true;
				break;

			case isc_info_engine_req_num_attachments:
				engineInfoRequest.databaseAttachments = true;
				break;

			case isc_info_engine_req_num_databases:
				engineInfoRequest.databases = true;
				break;

			case isc_info_engine_req_engine_version:
				engineInfoRequest.engineVersion = true;
				break;

			default:
				break;
		}

	EngineInfoGen infoGen(buffer, bufferLength);
	infoGen.putItem(isc_info_engine_begin);

	bool dbList = engineInfoRequest.databaseList();
	if (dbList)
	{
		infoGen.putItem(isc_info_engine_databases);
		infoGen.putListBegin();
	}
		
	int databaseCount = 0;
	for (Database* dbb = databaseManager->databases; dbb; dbb = dbb->dbb_next) 
	{
		if (!(dbb->dbb_flags & (DBB_bugcheck | DBB_not_in_use | DBB_security_db)) &&
			!(dbb->dbb_ast_flags & DBB_shutdown
			&& dbb->dbb_ast_flags & DBB_shutdown_locks)) 
		{
			databaseCount++;

			if (dbList && engineInfoRequest.databasePaths)
				infoGen.putItemValueString(isc_info_engine_db_path, dbb->dbb_filename);

			if (dbList && engineInfoRequest.databaseAttachments)
			{
				int attachmentCount = 0;

				for (const Attachment* attach = dbb->dbb_attachments; attach; attach = attach->att_next)
					attachmentCount++;

				infoGen.putItemValueInt(isc_info_engine_db_attachments, attachmentCount);
			}
		}
	}		

	if (dbList)
		infoGen.putListEnd();

	if (engineInfoRequest.databases)
	{
		infoGen.putItem(isc_info_engine_num_databases);
		infoGen.putInt(isc_info_engine_value, databaseCount);
	}

	if (engineInfoRequest.engineVersion)
	{
		infoGen.putItem(isc_info_engine_engine_version);
		infoGen.putString(isc_info_engine_value, GDS_VERSION);
	}

	infoGen.putItem(isc_info_engine_end);
	infoGen.putItem(isc_info_end);

	return FB_SUCCESS;
}
