/* $Id$ */
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

// YSQLDA.h: interface for the YSQLDA class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_YSQLDA_H__C6F6748F_A746_42EA_AB7C_B1508FB9B1D5__INCLUDED_)
#define AFX_YSQLDA_H__C6F6748F_A746_42EA_AB7C_B1508FB9B1D5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//struct dsc;

static const int LOCAL_SQLVARS = 24;

struct YSQLVAR
{
	short			sqltype;			/* datatype of field */
	short			sqlscale;			/* scale factor */
	short			sqlsubtype;			/* datatype subtype - BLOBs & Text types only */
	short			sqllen;				/* length of data area */
};

class YSQLDA  
{
public:
	YSQLDA(int userDialect, XSQLDA *sqlda);
	void allocBuffer();
	virtual ~YSQLDA();

	XSQLDA		*sqlda;
	int			userDialect;
	int			bufferLength;
	int			blrLength;
	int			msgLength;
	int			dscLength;
	int			allocLength;
	int			numberVariables;
	UCHAR		*buffer;
	UCHAR		localBuffer [1024];
	UCHAR		*msg;
	UCHAR		localMsg [1024];
	dsc			*descriptors;
	dsc			localDescriptors[64];
	YSQLVAR		localSqlvars[LOCAL_SQLVARS];
	YSQLVAR		*sqlvars;
	
	void populateSqlda(const UCHAR *info, int length);
	int getNumber(const UCHAR* ptr, int length);
	void genBlr();
	void allocateBuffer(int length);
	void allocateMessage(void);
	void allocateDescriptors(void);
	void copyToMessage(void);
	void copyFromMessage(void);
	int getDtype(int sqlType);
	int align(int dtype, int offset);
	YSQLDA(void);
	void setSqlda(int userDialect, XSQLDA* sqlda);
	bool validateBlr(void);
	void init(void);
};

#endif // !defined(AFX_YSQLDA_H__C6F6748F_A746_42EA_AB7C_B1508FB9B1D5__INCLUDED_)
