// ACPS.h : main header file for the ACPS application
//

#if !defined(AFX_ACPS_H__EF861405_27E9_4FDC_9B04_568D74B375BE__INCLUDED_)
#define AFX_ACPS_H__EF861405_27E9_4FDC_9B04_568D74B375BE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CACPSApp:
// See ACPS.cpp for the implementation of this class
//

class CACPSApp : public CWinApp
{
public:
	CACPSApp();
	int m_gSubSysNumber = 0;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CACPSApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CACPSApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACPS_H__EF861405_27E9_4FDC_9B04_568D74B375BE__INCLUDED_)
