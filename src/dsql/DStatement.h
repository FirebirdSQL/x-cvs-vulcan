// DStatement.h: interface for the DStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DSTATEMENT_H__B1B6D6F4_E1AF_4D55_B877_F5CD061E7F71__INCLUDED_)
#define AFX_DSTATEMENT_H__B1B6D6F4_E1AF_4D55_B877_F5CD061E7F71__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class Attachment;
class CStatement;
class Cursor;
class Transaction;
class Request;
class Procedure;
class dsql_lls;
class dsql_nod;
class dsql_sym;
class dsql_blb;
class dsql_str;
class dsql_msg;
class dsql_rel;
//class dsql_prc;
class dsql_ctx;
class par;
class opn;
class DsqlMemoryPool;
class tsql;
struct tdbb;

struct dsc;

enum req_flags_vals {
	REQ_cursor_open			= 1,
	REQ_save_metadata		= 2,
	REQ_prepared			= 4,
	REQ_embedded_sql_cursor	= 8,
	REQ_procedure			= 16,
	REQ_trigger				= 32,
	REQ_orphan				= 64,
	REQ_enforce_scope		= 128,
	REQ_no_batch			= 256,
	REQ_backwards			= 512,
	REQ_blr_version4		= 1024,
	REQ_blr_version5		= 2048,
	REQ_block				= 4096
};


class DStatement  
{
public:
	void notYetImplemented();
	void instantiateRequest(tdbb* tdsql);
	bool getIndices (int *explain_length_ptr, const UCHAR **explain_ptr, int *plan_length_ptr, UCHAR **plan_ptr);
	bool getRsbItem (int *explain_length_ptr, const UCHAR **explain_ptr, int* plan_length_ptr, UCHAR** plan_ptr, USHORT* parent_join_count, USHORT* level_ptr);
	UCHAR* getVariableInfo (dsql_msg *message, const UCHAR *items, const UCHAR *end_describe, UCHAR *info, const UCHAR* end, int first_index);
	int getPlanInfo (tdbb *threadData, int bufferLength, UCHAR **bufferPtr);
	int getRequestInfo (tdbb *threadData, int bufferLength, UCHAR *buffer);
	UCHAR* put_item (UCHAR item, int length, const UCHAR *string, UCHAR *ptr, const UCHAR *end);
	int convert (int number, UCHAR* buffer);
	ISC_STATUS returnSuccess(ISC_STATUS *statusVector);
	ISC_STATUS getSqlInfo (ISC_STATUS *statusVector, int itemsLength, const UCHAR *items, int bufferLength, UCHAR *buffer);
	void reset();
	ISC_STATUS prepare (ISC_STATUS *statusVector, Transaction *transaction, int sqlLength, const TEXT *sql, int dialect, int itemLength, const UCHAR *items, int bufferLength, UCHAR *buffer);
	DStatement(Attachment *attachment);
	virtual ~DStatement();

	Attachment	*attachment;
	CStatement	*statement;
	Transaction	*transaction;
	DStatement	*parent;
	DStatement	*sibling;
	DStatement	*offspring;
	Cursor		*cursor;
	USHORT		req_flags;			//!< generic flag
	UCHAR		*sendMessage;
	UCHAR		*receiveMessage;
	int			requestInstantiation;
	bool		singletonFetched;


	ISC_STATUS execute(ISC_STATUS* statusVector, Transaction** transactionHandle, int inBlrLength, const UCHAR* inBlr, int inMsgType, int inMsgLnegth, const UCHAR* inMsg, int outBlrLength, const UCHAR* outBlr, int outMsgType, int outMsgLength, UCHAR* outMsg);
	ISC_STATUS executeRequest(ISC_STATUS *statusVector, Transaction** transactionHandle, int blrLength, const UCHAR* inBlr, int inMsgLength, const UCHAR* inMsg, int outBlrLength, const UCHAR* outBlr, int outMsgLength, UCHAR* outMsg, bool singleton);
	void copyData(dsql_msg* source, UCHAR *msgBuffer, int blrLength, const UCHAR* blr, int msgLength, const UCHAR *inMsg, UCHAR* outMsg);
	ISC_STATUS fetch(ISC_STATUS* statusVector, int blrLength, const UCHAR* blr, int msgType, int msgLength, UCHAR* msg);
	ISC_STATUS freeStatement(ISC_STATUS* statusVector, int options);
	ISC_STATUS setCursor(ISC_STATUS* statusVector, const TEXT* cursorName, int cursorType);
	void closeCursor(void);
	void closeStatement(void);
	ISC_STATUS executeImmediate(ISC_STATUS* statusVector, Transaction** transactionHandle, 
								int sqlLength, const char* sql, int dialect, 
								int inBlrLength, const UCHAR* inBlr, 
								int inMsgType, int inMsgLength, const UCHAR* inMsg, 
								int outBlrLength, const UCHAR* outBlr, 
								int outMsgType, int outMsgLength, UCHAR* outMsg);
	ISC_STATUS executeDDL(ISC_STATUS* statusVector);
	void clearCursor(void);
	void addChild(DStatement* child);
	void removeChild(DStatement* child);
};

#endif // !defined(AFX_DSTATEMENT_H__B1B6D6F4_E1AF_4D55_B877_F5CD061E7F71__INCLUDED_)
