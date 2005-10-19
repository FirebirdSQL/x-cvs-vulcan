// MsgMgr.h : main header file for the MsgMgr application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols


// CMsgMgrApp:
// See MsgMgr.cpp for the implementation of this class
//

class CMsgMgrApp : public CWinApp
{
public:
	CMsgMgrApp();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	virtual void OnFileOpen(void);
};

extern CMsgMgrApp theApp;