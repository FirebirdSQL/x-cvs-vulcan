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
 
// CStatement.h: interface for the CStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATEMENT_H__026FC194_5AED_4A07_8657_F71B69C70C91__INCLUDED_)
#define AFX_CSTATEMENT_H__026FC194_5AED_4A07_8657_F71B69C70C91__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RefObject.h"
#include "JString.h"
#include "BlrGen.h"
#include "dialect.h"
#include "Stack.h"

enum REQ_TYPE
{
	REQ_SELECT, REQ_SELECT_UPD, REQ_INSERT, REQ_DELETE, REQ_UPDATE,
	REQ_UPDATE_CURSOR, REQ_DELETE_CURSOR,
	REQ_COMMIT, REQ_ROLLBACK, REQ_DDL, REQ_EMBED_SELECT,
	REQ_START_TRANS, REQ_GET_SEGMENT, REQ_PUT_SEGMENT, REQ_EXEC_PROCEDURE,
	REQ_COMMIT_RETAIN, REQ_SET_GENERATOR, REQ_SAVEPOINT, 
	REQ_EXEC_BLOCK, REQ_SELECT_BLOCK
};

enum trigger_type {
	PRE_STORE_TRIGGER = 1,
	POST_STORE_TRIGGER = 2,
	PRE_MODIFY_TRIGGER = 3,
	POST_MODIFY_TRIGGER = 4,
	PRE_ERASE_TRIGGER = 5,
	POST_ERASE_TRIGGER = 6
};


class Database;
class Attachment;
class Relation;
class Request;
class Transaction;
class BlrGen;
class DsqlMemoryPool;
class dsql_msg;
class dsql_rel;
//class dsql_prc;
class dsql_str;
class dsql_blb;
//class dsql_lls;
class dsql_nod;
//class dsql_udf;
class dsql_fld;
class dsql_intlsym;
class par;
class Relation;
class Request;
class Procedure;
class CharSetContainer;
class Function;

struct thread_db;

//#define FRBRD	void

class CStatement : public RefObject  
{
public:
	void genMatchingKey (const dsql_nod *for_columns, const dsql_nod *prim_columns);
	void genFiringCondition (dsql_nod *prim_columns);
	static void notYetImplemented();
	dsql_nod* getPrimaryKey (const TEXT *relationName);
	dsql_str* getTriggerRelation (const TEXT *triggerName, USHORT *triggerType);
	bool getColumnDefault (const TEXT *relationName, const TEXT *columnName, TEXT *buffer, int bufferLength);
	bool getDomain (const TEXT *domainName, dsql_fld *field);
	void genUnnamedTriggerBeginning (bool onUpdateTrigger, const TEXT *primaryRelationName, dsql_nod *primaryColumns, const TEXT *foreignRelationName, dsql_nod *foreignColumns);
	bool getDomainDefault (const TEXT *domainName, TEXT *buffer, int bufferLength);
	void blrEnd();
	void blrBegin (UCHAR verb);
	dsql_str* getDefaultCharset();
	int getCharsetBytesPerCharacter (int charsetId);
	int getType (const TEXT *typeName, const TEXT *fieldName, short *value);
	void dropFunction (const TEXT *functionName);
	void dropProcedure (const TEXT *procedureName);
	void dropRelation (const TEXT *relationName);
	Function* findFunction (const TEXT *functionName);
	dsql_rel* findViewRelation (const TEXT *viewName, const TEXT *relationName, int level);
	CharSetContainer* findCollation (const char *name);
	CharSetContainer* findCharset (const TEXT *name);
	Procedure* findProcedure (const TEXT *procedureName);
	dsql_rel* findRelation (const char *relationName);
	void prepare (thread_db* threadDdata, int sqlLength, const TEXT *sql, int userDialect);
	CStatement(Attachment *attachment);
	void convertDType (dsql_fld *field, int blrType);
	JString stripString (const TEXT *string);
	virtual ~CStatement();

	inline void appendUCHAR (UCHAR value)
		{ blrGen->appendUCHAR (value); }
	inline void appendUCHARs (UCHAR value, int count)
		{ blrGen->appendUCHARs (value, count); }
	inline void appendShort (int value)
		{ blrGen->appendShort (value); }
	inline void appendInt (int value)
		{ blrGen->appendInt (value); }
	inline void appendNumber (int value)
		{ blrGen->appendNumber (value); }
	inline void appendNumber (UCHAR verb, int value)
		{ blrGen->appendNumber (verb, value); }
	inline void appendDynString (UCHAR verb, const TEXT *string, int length)
		{ blrGen->appendDynString (verb, length, string); }
	inline void appendDynString (UCHAR verb, const TEXT *string)
		{ blrGen->appendDynString (verb, string); }
	inline void appendBlrString (const TEXT *string, int length)
		{ blrGen->appendBlrString (length, string); }
	inline void appendBlrString (const TEXT *string)
		{ blrGen->appendBlrString (string); }

	Database	*database;
	Attachment	*attachment;
	Transaction	*transaction;		// for ddl prepare only!!!
	CStatement	*parent;
	CStatement	*sibling;
	CStatement	*offspring;
	JString		sql;
	JString		cursorName;			// as in "current of <cursorName>
	BlrGen		*blrGen;
	thread_db	*threadData; 
	DsqlMemoryPool	*pool;

	Request		*request;

	bool		oldParameterOrdering;
	
	REQ_TYPE	req_type;
	dsql_msg	*sendMessage;
	dsql_msg	*receiveMessage;
	dsql_rel	*req_relation;
	int			dialect;
	//int			dbSqlDialect;
	int			charset;
	int			contextNumber;
	int			scopeLevel;
	int			loopLevel;
	int			flags;
	int			dbb_flags;
	bool		inOuterJoin;
	dsql_str	*aliasRelationPrefix;
	Stack		context;
	Stack		unionContext;
	Stack		cursors;
	Stack		labels;
	Procedure	*procedure;
	dsql_nod	*ddlNode;
	dsql_nod	*execBlockNode;
	dsql_nod	*syntaxTree;
	dsql_blb	*blob;
	int			req_in_where_clause;	//!< processing "where clause"
	int			req_in_group_by_clause;	//!< processing "group by clause"
	int			req_in_having_clause;	//!< processing "having clause"
	int			req_in_order_by_clause;	//!< processing "order by clause"
	int			req_in_outer_join;		//!< processing inside outer-join part
	int			req_in_select_list;		//!< now processing "select list"
	int			req_error_handlers;		//!< count of active error handlers
	int			req_client_dialect;		//!< dialect passed into the API call
	int			cursorNumber;
	par			*req_eof;				//!< End of file parameter
	par			*req_dbkey;				//!< Database key for current of
	par			*recordVersion;		//!< Record Version for current of
	par			*parentRecordVersion;//!< parent record version
	par			*parentDbkey;		//!< Parent database key for current of
	
	par* makeParameter(dsql_msg* message, bool sqldaFlag, bool nullFlag, int sqldaIndex);
	const UCHAR* getBlr(void);
	int getBlrLength(void);
	static void staticInitialization(void);
	int getInstantiation(void);
	void releaseInstantiation(int instantiation);
	void deleteSyntaxTree(void);
	dsql_rel* getRelation(const TEXT* relationName);
	void getRequestInfo(thread_db* threadData, int instantiation, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer);
	int findColumn(const char *columnName);
};

#endif // !defined(AFX_CSTATEMENT_H__026FC194_5AED_4A07_8657_F71B69C70C91__INCLUDED_)
