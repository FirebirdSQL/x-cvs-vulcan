/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */

#include "firebird.h"
#include "common.h"
#include "all.h"
#include "CharSetManager.h"
#include "CharSet.h"
#include "Sync.h"
#include "tdbb.h"
#include "Database.h"
#include "Attachment.h"
#include "../intl/charsets.h"
#include "met_proto.h"
#include "CsConvertArray.h"
#include "Collation.h"
#include "Connect.h"
#include "PStatement.h"
#include "RSet.h"
#include "intl_classes.h"
#include "intl.h"

CharSetManager::CharSetManager(Database *dbb)
{
	database = dbb;
	charSets = NULL;
	collations = NULL;
}

CharSetManager::~CharSetManager(void)
{
	for (Collation *collation; collation = collations;)
		{
		collations = collation->next;
		delete collation;
		}
		
	for (CharSetContainer *charSet; charSet = charSets;)
		{
		charSets = charSet->next;
		delete charSet;
		}
}

CharSetContainer* CharSetManager::findCharset(tdbb* tdbb, int ttype)
{
	int charSetId = TTYPE_TO_CHARSET(ttype);

	if (charSetId == CS_dynamic)
		charSetId = tdbb->tdbb_attachment->att_charset;

	CharSetContainer *container;
	
	if (charSetId < charSetVector.size() && (container = charSetVector[charSetId]))
		return container;
		
	Sync sync (&syncObject, "CharSetManager::findCharset");
	sync.lock (Shared);
	
	if (charSetId < charSetVector.size() && (container = charSetVector[charSetId]))
		return container;
		
	sync.unlock();
	
	return loadCharset (tdbb, charSetId);
}

CharSetContainer* CharSetManager::findCharset(tdbb* tdbb, const char* name)
{
	Sync sync (&syncObject, "CharSetManager::findCharset");
	sync.lock (Shared);
	
	for (CharSetContainer * container = charSets; container; container = container->next)
		if (container->isNamed (name))
			return container;
	
	sync.unlock();
	int id = MET_resolve_charset_and_collation (tdbb, name, NULL);
	
	if (id < 0)
		return NULL;

	int charSetId = TTYPE_TO_CHARSET(id);
	
	return loadCharset (tdbb, charSetId);
}

CharSetContainer* CharSetManager::loadCharset(tdbb* tdbb, int charSetId)
{
	CharSetContainer* container = new CharSetContainer (tdbb, database->dbb_permanent, charSetId);
	
	if (container->getCharSet() == NULL)
		{
		delete container;
		return NULL;
		}
	
	Sync sync (&syncObject, "CharSetManager::loadCharset");
	sync.lock (Exclusive);
	container->next = charSets;
	charSets = container;
	
	if (charSetId >= charSetVector.size())
		charSetVector.resize(charSetId + 10);
	
	charSetVector[charSetId] = container;
	
	return container;
}

CharSetContainer* CharSetManager::findCollation(tdbb* tdbb, const char* name)
{
	Sync sync (&syncObject, "CharSetManager::findCollation");
	sync.lock (Shared);
	Collation *collation;
	
	for (collation = collations; collation; collation = collation->next)
		if (collation->name == name)
			return collation;
			
	sync.unlock();
	/***
	int id = MET_resolve_charset_and_collation (tdbb, NULL, name);
	if (id < 0)
		return NULL;
	***/
	
	Connect connection = tdbb->tdbb_attachment->getUserConnection(tdbb->tdbb_database->dbb_sys_trans);
	PStatement statement = connection->prepareStatement (
		"select rdb$character_set_id, rdb$collation_id "
		"  from rdb$collations " 
		"  where rdb$collation_name=?");
	statement->setString(1,name);
	RSet resultSet = statement->executeQuery();
	int characterSetId;
	int collationId;
	
	if (resultSet->next())
		{
		characterSetId = resultSet->getInt(1);
		collationId = resultSet->getInt(2);
		}
	else
		return NULL;

	CharSetContainer *charSet = findCharset(tdbb, characterSetId);//loadCharset (tdbb, id);
	
	if (!charSet)
		return NULL;
	
	TextType textType = charSet->lookupCollation(tdbb, INTL_CS_COLL_TO_TTYPE(characterSetId, collationId));
	texttype *type = textType.getStruct();
	
	if (!type)
		return NULL;
	
	sync.lock (Exclusive);
	collation = new Collation(name, characterSetId, collationId, type);
	collation->next = collations;
	collations = collation;
	
	return collation;	
}
