// SymbolManager.h: interface for the SymbolManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYMBOLMANAGER_H__E0895BC1_FD2A_11D4_990D_0000C01D2301__INCLUDED_)
#define AFX_SYMBOLMANAGER_H__E0895BC1_FD2A_11D4_990D_0000C01D2301__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SyncObject.h"

#define SYMBOL_HASH_SIZE		503

struct Symbol {
	Symbol		*collision;
	char		symbol [1];
	};

struct SymbolSection
	{
	SymbolSection	*next;
	char			space [5000];
	};

class SymbolManager  
{
public:
	const char* getString (const char *string);
	const char* findString (const char *string);
	//const char* getSymbol (const WCString *string);
	bool isSymbol (const char *string);
	const char* getSymbol (const char *string);
	const char* findSymbol (const char *string);
	SymbolManager();
	virtual ~SymbolManager();

	SymbolSection	*sections;
	Symbol			*hashTable [SYMBOL_HASH_SIZE];
	char			*next;
	SyncObject		syncObject;
};

#endif // !defined(AFX_SYMBOLMANAGER_H__E0895BC1_FD2A_11D4_990D_0000C01D2301__INCLUDED_)
