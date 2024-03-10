// fluids.h : main header file for the fluids application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CfluidsApp:
// See fluids.cpp for the implementation of this class
//

class CfluidsApp : public CWinApp
{
public:
	CfluidsApp();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CfluidsApp theApp;