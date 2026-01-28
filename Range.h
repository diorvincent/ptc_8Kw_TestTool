#if !defined(AFX_RANGE_H__F3E2F9AF_9D4D_4500_859D_F0DD63BFD1B1__INCLUDED_)
#define AFX_RANGE_H__F3E2F9AF_9D4D_4500_859D_F0DD63BFD1B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Range.h : header file
//
#include "CXPButton.h"
#include "afxcmn.h"
#include "afxwin.h"
#include "NumberEdit.h"
/////////////////////////////////////////////////////////////////////////////
// CRange dialog

class CRange : public CDialog
{
// Construction
public:
	CRange(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRange)
	enum { IDD = IDD_RANGE_DIALOG };
	CCXPButton	m_cancel;
	CCXPButton	m_save;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRange)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRange)
	afx_msg void OnSave();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_cfglist;
	int m_Row;//获得选中的行  
	int m_Col;//获得选中列
	afx_msg void OnNMDblclkListcfg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnKillfocusCfgedit();
	//CNumberEdit m_numedit;
	CEdit m_numedit;
	int cfgindex;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RANGE_H__F3E2F9AF_9D4D_4500_859D_F0DD63BFD1B1__INCLUDED_)
