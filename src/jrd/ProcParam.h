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
 *  The Original Code was created by Ann W. Harrison for IBPhoenix.
 *
 *  Copyright (c) 2004 Ann W. Harrison
 *  All Rights Reserved.
 */
#ifndef _ProcParam_H_
#define _ProcParam_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Field.h"
#include "../jrd/dsc_proto.h"

class Procedure;
class Jstring;
class dsql_fld;


class ProcParam : public Field
{
public:
	ProcParam(int id);
	ProcParam(dsql_fld *field);

	virtual ~ProcParam();

	inline ProcParam *findNext()
		{return paramNext;};
	inline void setNext (ProcParam *param)
		{ paramNext = param; };

	inline int findParamFlags ()
		{ return paramFlags; };
	inline void setParamFlag (int flag)
		{ paramFlags |= flag; };
	inline void clearParamFlag (int flag)
		{ paramFlags &= ~flag; };
	inline void clearParamFlags ()
		{ paramFlags = 0; };

	inline void setProcedure (Procedure *procedure)
		{ paramProcedure = procedure; };
	inline bool isNamed (const TEXT *name)
		{ return (!(fld_name.IsEmpty()) && fld_name == name); }
	inline void setId (int id)
		{ paramId = id; };
	inline int findId ()
		{ return paramId; };

	inline DSC findDescriptor ()
		{ return paramDescriptor; };
	void setDescriptor (DSC descriptor);

	friend class	Procedure;
	DSC				paramDescriptor;

protected:
	ProcParam*	paramNext;				//!< Next parameter of type
	Procedure*	paramProcedure;			//!< Parent procedure
	int			paramId;
	int			paramFlags;

// values used in paramflags
#if !defined(MVS)
static const int
	paramComputed	= 1,
	paramDrop		= 2,
	paramDbKey		= 4,
	paramNational	= 8, //!< field uses NATIONAL character set
	paramNullable	= 16;
#endif
public:
	dsql_fld* getDsqlField(void);
};

#endif

