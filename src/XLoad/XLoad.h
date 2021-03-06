/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by [Initial Developer's Name]
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c)  2004 James A. Starkey
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): James A. Starkey
 */

#ifndef _XLoad_H_
#define _XLoad_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "JString.h"

class Connection;
CLASS(Element);
CLASS(InputStream);

class XLoad
{
public:
	XLoad(Connection *connect);
	virtual ~XLoad(void);
	void loadFile(const char* filename);
	void load(InputStream* stream);
	
	Connection	*connection;
	Element		*tree;
	Element		*metaData;
	Element		*data;
	void loadMetaData(void);
	void loadData(void);
	void createTable(Element* element);
};

#endif
