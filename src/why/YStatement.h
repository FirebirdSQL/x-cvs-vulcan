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
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

// YStatement.h: interface for the YStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_YSTATEMENT_H__B160A6BC_70D4_453C_9DE4_7AB579B0323B__INCLUDED_)
#define AFX_YSTATEMENT_H__B160A6BC_70D4_453C_9DE4_7AB579B0323B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Subsystem.h"
#include "YSQLDA.h"

class SubsysHandle;
class StatusVector;

class YStatement  
{
public:
	YStatement(SubsysHandle *subsys, DsqlHandle orgHandle);
	virtual ~YStatement();

	SubsysHandle	*subsystem;
	DsqlHandle		handle;
	DsqlHandle		*userPtr;
	YStatement		*next;
	YStatement		*prior;
	YSQLDA			outYSqlda;
	ISC_STATUS releaseStatement(StatusVector& statusVector, int options, bool force);
};

#endif // !defined(AFX_YSTATEMENT_H__B160A6BC_70D4_453C_9DE4_7AB579B0323B__INCLUDED_)
