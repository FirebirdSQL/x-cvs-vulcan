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

// YSQLDA.cpp: implementation of the YSQLDA class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include "fbdev.h"
#include "common.h"
//#include "fb_types.h"
#include "../dsql/sqlda.h"
#include "inf.h"
#include "align.h"
#include "blr.h"
#include "YSQLDA.h"
#include "GenericMove.h"

static const UCHAR blrTypes [] = 
	{
	0,					// dtype_unknown
	blr_text,
	blr_text,			// blr_cstring,
	blr_varying,
	0,
	0,
	0,
	0, //blr_byte,
	blr_short,
	blr_long,
	blr_quad,
	blr_float,
	blr_double,
	blr_d_float,
	blr_sql_date,
	blr_sql_time,
	blr_timestamp,
	blr_quad,
	blr_quad,
	blr_int64
	};

inline void stuff (UCHAR*& p, int byte)
	{
	*p++ = (UCHAR)byte;
	}
	
inline void stuffWord (UCHAR*& p, int word)
	{
	stuff (p, word);
	stuff (p, word >> 8);	
	}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

YSQLDA::YSQLDA(void)
{
	userDialect = -1;
	sqlda = NULL;
	init();
}

YSQLDA::YSQLDA(int dialect, XSQLDA *userSqlda)
{
	userDialect = dialect;
	sqlda = userSqlda;
	init();
}


void YSQLDA::init(void)
{
	buffer = NULL;
	descriptors = NULL;
	msg = NULL;
	blrLength = 0;
	msgLength = 0;
	allocLength = 0;
	dscLength = 0;
	sqlvars = NULL;
}

YSQLDA::~YSQLDA()
{
	if (buffer && buffer != localBuffer)
		delete [] buffer;
	
	if (msg && msg != localMsg)
		delete [] msg;
	
	if (sqlvars && sqlvars != localSqlvars)
		delete [] sqlvars;
		
	if (descriptors && descriptors != localDescriptors)
		delete [] descriptors;
}

void YSQLDA::allocBuffer()
{
	int dialect = userDialect / 10;

	if (dialect == 0)
		dialect = userDialect;

	int numberVariables = 0;

	if (sqlda)
		numberVariables = (dialect >= DIALECT_xsqlda) ? 
							sqlda->sqln : ((SQLDA*) sqlda)->sqln;

	int length = 32 + numberVariables * 172;
	allocateBuffer (length);
}

void YSQLDA::allocateBuffer(int length)
{
	if (buffer)
		{
		if (bufferLength >= length)
			return;
		if (buffer != localBuffer)
			delete [] buffer;
		}

	bufferLength = length;
	
	if (bufferLength <= sizeof (localBuffer))
		buffer = localBuffer;
	else
		buffer = new UCHAR [bufferLength];
}


void YSQLDA::allocateMessage(void)
{
	if (msg)
		{
		if (msgLength <= allocLength)
			return;
		if (msg != localMsg)
			delete [] msg;
		}
	
	if (msgLength <= sizeof (localMsg))
		{
		msg = localMsg;
		allocLength = sizeof (localMsg);
		return;
		}
	
	msg = new UCHAR [msgLength];
	allocLength = msgLength;
}

void YSQLDA::allocateDescriptors(void)
{
	if (dscLength <= sizeof (localDescriptors) / sizeof(dsc))
		{
		descriptors = localDescriptors;
		return;
		}
	
	if (descriptors && descriptors != localDescriptors)
		delete [] descriptors;
	
	descriptors = new dsc [dscLength];
}

void YSQLDA::populateSqlda(const UCHAR *info, int infoLength)
{
	if (!sqlda)
		return;

	const UCHAR *p = info;
	const UCHAR *end = p + infoLength;
	int length;
	int numberVariables = 0;
	int sequence = -1;
	XSQLVAR *variable = 0;
	 
	// First byte is either isc_info_sql_select or isc_info_sql_bind
	
	if (*p == isc_info_sql_select)
		++p;
	else if (*p == isc_info_sql_bind)
		++p;
	
	for (; p < end; p += length)
		{
		UCHAR item = *p++;
	
		if (item == isc_info_sql_describe_end)
			{
			length = 0;
			continue;
			}
		else if (item == isc_info_end)
			break;
		else if (item == isc_info_truncated)
			break;
	
		length = *p++;
		length += 256 * *p++;

		switch (item)
			{
			case isc_info_sql_describe_vars:
				numberVariables = getNumber (p, length);
				sqlda->sqld = (short) numberVariables;
				break;
				
			case isc_info_sql_sqlda_seq:
				sequence = getNumber (p, length);
				if (sequence <= sqlda->sqln)
					variable = sqlda->sqlvar + sequence - 1;
				else 
					variable = NULL;
				break;
				
			case isc_info_sql_type:
				if (variable) 
					variable->sqltype = (short) getNumber (p, length);
				break;
				
			case isc_info_sql_sub_type:
				if (variable) 
					variable->sqlsubtype = (short) getNumber (p, length);
				break;
				
			case isc_info_sql_scale:
				if (variable) 
					variable->sqlscale = (short) getNumber (p, length);
				break;
				
			case isc_info_sql_length:
				if (variable)
					variable->sqllen = (short) getNumber (p, length);
				/***
				if (variable->sqltype == SQL_VARYING)
					variable->sqllen -= 2;
				***/
				break;
				
			case isc_info_sql_field:
				if (variable)
					{
					memcpy (variable->sqlname, p, length);
					variable->sqlname_length = (short) length;
					variable->sqlname [length] = 0;
					}
				break;
	
			case isc_info_sql_relation:
				if (variable)
					{
					memcpy (variable->relname, p, length);
					variable->relname_length = (short) length;
					variable->relname [length] = 0;
					}
				break;
	
			case isc_info_sql_owner:
				if (variable)
					{
					memcpy (variable->ownname, p, length);
					variable->ownname_length = (short) length;
					variable->ownname [length] = 0;
					}
				break;
	
			case isc_info_sql_alias:
				if (variable)
					{
					memcpy (variable->aliasname, p, length);
					variable->aliasname_length = (short) length;
					variable->aliasname [length] = 0;
					}
				break;
	
			default:
				printf ("not implemented\n");
				
			}
		}
}

int YSQLDA::getNumber(const UCHAR* ptr, int length)
{
	int number = 0;
	
	for (int n = 0; n < length; ++n)
		number |= *ptr++ << (n * 8);
	
	return number;	
}

void YSQLDA::genBlr()
{
	if (!sqlda)
		return;
	
	numberVariables = sqlda->sqld;
	int parameters = sqlda->sqld * 2;
	XSQLVAR *variable;
	XSQLVAR *variableEnd = sqlda->sqlvar + sqlda->sqld;
	blrLength = 8;
	sqlvars = (sqlda->sqld <= LOCAL_SQLVARS) ? localSqlvars : new YSQLVAR [sqlda->sqld];
	YSQLVAR *sqlvar = sqlvars;
	msgLength = 0;	
	
	for (variable = sqlda->sqlvar; variable < variableEnd; ++variable, ++sqlvar)
		{
		sqlvar->sqltype = variable->sqltype;
		sqlvar->sqlscale = variable->sqlscale;
		sqlvar->sqlsubtype = variable->sqlsubtype;
		sqlvar->sqllen = variable->sqllen;
		int dtype = variable->sqltype & ~1;
		blrLength += 2;		// for null indicator
		switch (dtype)
			{
			case SQL_VARYING:
			case SQL_TEXT:
				blrLength += 3;
				break;
			
			case SQL_SHORT:
			case SQL_LONG:
			case SQL_INT64:
			case SQL_QUAD:
			case SQL_BLOB:
			case SQL_ARRAY:
				blrLength += 2;
				break;
			
			default:
				++blrLength;
			}
		}
	
	allocateBuffer (blrLength);	
	UCHAR *p = buffer;
	stuff (p, blr_version5);
	stuff (p, blr_begin);
	stuff (p, blr_message);
	stuff (p, 0);
	stuffWord (p, parameters);
	if (sqlda->sqld * 2 > dscLength)
	{
		dscLength = sqlda->sqld * 2;
		allocateDescriptors();
	}
	memset (descriptors, 0, sizeof (dsc) * sqlda->sqld * 2);
	dsc *desc = descriptors;
	
	for (variable = sqlda->sqlvar; variable < variableEnd; ++variable, ++desc)
		{
		int sqltype = variable->sqltype & ~1;
		int sqlLength = variable->sqllen;
		int dtype = getDtype (sqltype);
		stuff (p, blrTypes [dtype]);
		int dataLength = sqlLength;
		
		switch (sqltype)
			{
			case SQL_VARYING:
				stuffWord (p, sqlLength);
				dataLength += 2;
				break;
				
			case SQL_TEXT:
				stuffWord (p, sqlLength);
				break;

			case SQL_SHORT:
			case SQL_LONG:
			case SQL_INT64:
			case SQL_QUAD:
				stuff (p, variable->sqlscale);
				break;

			case SQL_BLOB:
			case SQL_ARRAY:
				stuff (p, 0);
				break;
		
			case SQL_DOUBLE:
			case SQL_FLOAT:
			case SQL_D_FLOAT:
			case SQL_TYPE_DATE:
			case SQL_TYPE_TIME:
			case SQL_TIMESTAMP:
				break;
	
			default:
				++blrLength;
			}
		
		msgLength = align (dtype, msgLength);
		desc->dsc_dtype = dtype;
		desc->dsc_length = dataLength;
		desc->dsc_address = (UCHAR*) msgLength;
		msgLength += dataLength;
		
		msgLength = align (dtype_short, msgLength);
		stuff (p, blr_short);				// null indicator
		stuff (p, 0);
		++desc;						// with no scale factor
		desc->dsc_dtype = dtype_short;
		desc->dsc_length = 2;
		desc->dsc_address = (UCHAR*) msgLength;
		msgLength += sizeof (short);
		}

	stuff (p, blr_end);
	stuff (p, blr_eoc);
	blrLength = p - buffer;
}

void YSQLDA::copyToMessage(void)
{
	if (!sqlda)
		return;

	if (!msg)
		allocateMessage();
		
	//isc_print_blr ((const char*) buffer, NULL, NULL, NULL);
	dsc *desc = descriptors;
	
	for (int n = 0; n < sqlda->sqld; ++n, desc += 2)
		{
		XSQLVAR *variable = sqlda->sqlvar + n;
		int sqlLength = variable->sqllen;
		int sqlType = variable->sqltype;
		int dtype = getDtype (variable->sqltype & ~1);
		UCHAR *from = (UCHAR*) variable->sqldata;
		dsc toDesc = *desc;
		toDesc.dsc_address = msg + (long) toDesc.dsc_address;
		UCHAR *to = toDesc.dsc_address;
				
		switch (dtype)
			{
			case dtype_varying:
				((vary*) to)->vary_length = ((vary*) from)->vary_length;
				if (!*variable->sqlind) /* this seems to be the check that FB15 uses for NULL */
										/* NULL data. (see null_ind in utld.cpp, near line 628 */
					memcpy (((vary*) to)->vary_string, ((vary*)from)->vary_string, ((vary*)from)->vary_length);
				break;
			
			default:
				memcpy (to, from, sqlLength);
			}
		
		const UCHAR *ptr = msg + (long) desc [1].dsc_address;
		*(short*) ptr = (sqlType & 1) ? *variable->sqlind : 0;
		}
}

void YSQLDA::copyFromMessage(void)
{
	if (!sqlda)
		return;
		
	dsc *desc = descriptors;
	
	for (int n = 0; n < sqlda->sqld; ++n, desc += 2)
		{
		XSQLVAR *variable = sqlda->sqlvar + n;
		int sqlLength = variable->sqllen;
		int sqlType = variable->sqltype;			// may include null flag!
		UCHAR *to = (UCHAR*) variable->sqldata;
		int dtype = getDtype (sqlType);
		dsc fromDesc = *desc;
		fromDesc.dsc_address = msg + (long) fromDesc.dsc_address;
		UCHAR *from = fromDesc.dsc_address;
		
		switch (dtype)
			{
			case dtype_varying:
				((vary*) to)->vary_length = ((vary*) from)->vary_length;
				memcpy (((vary*) to)->vary_string, ((vary*)from)->vary_string, ((vary*)from)->vary_length);
				//offset += 2;
				break;
			
			default:
				memcpy (to, from, sqlLength);
			}
		
		if (sqlType & 1)
			*variable->sqlind = *(short*) (msg + (long) desc [1].dsc_address);
		}
}

int YSQLDA::getDtype(int sqlType)
{
	switch (sqlType & ~1)
		{
		case SQL_VARYING:
			return dtype_varying;
			
		case SQL_TEXT:
			return dtype_text;

		case SQL_SHORT:
			return dtype_short;

		case SQL_LONG:
			return dtype_long;

		case SQL_INT64:
			return dtype_int64;

		case SQL_QUAD:
			return dtype_quad;

		case SQL_BLOB:
		case SQL_ARRAY:
			return dtype_quad;
	
		case SQL_DOUBLE:
			return dtype_double;

		case SQL_FLOAT:
			return dtype_real;

		case SQL_D_FLOAT:
			return dtype_d_float;

		case SQL_TYPE_DATE:
			return dtype_sql_date;

		case SQL_TYPE_TIME:
			return dtype_sql_time;

		case SQL_TIMESTAMP:
			return dtype_timestamp;
		}
			
	return 0;
}

int YSQLDA::align(int dtype, int offset)
{
	int alignment = type_alignments[dtype];
	
	if (alignment)
		return FB_ALIGN (offset, alignment);
	
	return offset;
}

void YSQLDA::setSqlda(int dialect, XSQLDA* userSqlda)
{
	userDialect = dialect;
	sqlda = userSqlda;
	
	if (!blrLength || !validateBlr())
		genBlr();
		
	allocateMessage();
}

bool YSQLDA::validateBlr(void)
{
	if (numberVariables != sqlda->sqld)
		return false;

	YSQLVAR *sqlvar = sqlvars;
	
	for (XSQLVAR *variable = sqlda->sqlvar, *end = variable + sqlda->sqld; variable < end; ++variable, ++sqlvar)
		if (sqlvar->sqltype != variable->sqltype ||
			 sqlvar->sqlscale != variable->sqlscale ||
			 sqlvar->sqlsubtype != variable->sqlsubtype ||
			 sqlvar->sqllen != variable->sqllen)
			return false;
	
	return true;
}
