// ActionStatement.h: interface for the ActionStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONSTATEMENT_H__1451126A_B98B_11D4_98FB_0000C01D2301__INCLUDED_)
#define AFX_ACTIONSTATEMENT_H__1451126A_B98B_11D4_98FB_0000C01D2301__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Action.h"

enum StatementType {
	GenHtml,
	Disconnect,
	Commit,
	Rollback,
	Select,
	Update,
	};

class ActionConnect;
class PreparedStatement;

class ActionStatement  : public Action  
{
public:
	virtual void close();
	void execUpdate();
	void execSelect();
	void genHtml (Stat *stat);
	virtual Action* eval(Stat *stat);
	virtual Action* compile (Context *context);
	ActionStatement(StatementType actionType);
	virtual ~ActionStatement();

	StatementType		type;
	ActionConnect		*connection;
	ActionStatement		*nextStatement;
	PreparedStatement	*statement;
	const char			*sql;
	bool				recompile;
};

#endif // !defined(AFX_ACTIONSTATEMENT_H__1451126A_B98B_11D4_98FB_0000C01D2301__INCLUDED_)
