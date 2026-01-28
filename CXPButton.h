#if !defined(AFX_CXPBUTTON_H__E740E8B8_B118_44A6_8C43_43C71117A842__INCLUDED_)
#define AFX_CXPBUTTON_H__E740E8B8_B118_44A6_8C43_43C71117A842__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CXPButton.h : header file
//
#include "CXPButton.h"
/////////////////////////////////////////////////////////////////////////////
// CCXPButton window

class CCXPButton : public CButton
{
	// Construction
public:
	CCXPButton();
protected:
	//按钮的外边框
	CPen m_BoundryPen;
	
	//鼠标指针置于按钮之上时按钮的内边框
	CPen m_InsideBoundryPenLeft;
	CPen m_InsideBoundryPenRight;
	CPen m_InsideBoundryPenTop;
	CPen m_InsideBoundryPenBottom;
	
	//按钮获得焦点时按钮的内边框
	CPen m_InsideBoundryPenLeftSel;
	CPen m_InsideBoundryPenRightSel;
	CPen m_InsideBoundryPenTopSel;
	CPen m_InsideBoundryPenBottomSel;
	
	//按钮的底色，包括有效和无效两种状态
	CBrush m_FillActive;
	CBrush m_FillInactive;
	
	//按钮的状态
	BOOL m_bOver;	//鼠标位于按钮之上时该值为true，反之为flase
	BOOL m_bTracking;	//在鼠标按下没有释放时该值为true
	BOOL m_bSelected;	//按钮被按下是该值为true
	BOOL m_bFocus;	//按钮为当前焦点所在时该值为true
	
	// Operations// Attributes
public:
	
	// Operations
public:
	
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCXPButton)
protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL
	
	// Implementation
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void DoGradientFill(CDC *pDC, CRect* rect);
	virtual void DrawInsideBorder(CDC *pDC, CRect* rect);
	virtual ~CCXPButton();
	
	// Generated message map functions
protected:
	//{{AFX_MSG(CCXPButton)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnMouseHover(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CXPBUTTON_H__E740E8B8_B118_44A6_8C43_43C71117A842__INCLUDED_)
