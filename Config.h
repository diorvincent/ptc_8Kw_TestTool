#if !defined(AFX_CONFIG_H__19B12A1B_CDC7_496F_B848_33458C613571__INCLUDED_)
#define AFX_CONFIG_H__19B12A1B_CDC7_496F_B848_33458C613571__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Config.h : header file
//
#include "CXPButton.h"
/////////////////////////////////////////////////////////////////////////////
// CConfig dialog

#define SOFTWARE_VERSION				4
#define POWER_CALIBRATION			2
#define CALI_POWRER_WAIT_TIME0	10000
#define CALI_POWRER_WAIT_TIME1	40000
#define CALI_POWRER_WAIT_TIME		60000


class CConfig : public CDialog
{
// Construction
public:
	CConfig(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfig)
	enum { IDD = IDD_CONFIG_DIALOG };
	CComboBox	m_check_iac;
	CComboBox	m_data_iac;
	CComboBox	m_baud_iac;
	CComboBox	m_port_iac;
	CComboBox	m_check_mod;
	CComboBox	m_stop_mod;
	CComboBox	m_data_mod;
	CComboBox	m_baud_mod;
	CComboBox	m_port_mod;
	CComboBox	m_check_tem;
	CComboBox	m_stop_tem;
	CComboBox	m_data_tem;
	CComboBox	m_baud_tem;
	CComboBox	m_port_tem;
	CComboBox	m_check_pre;
	CComboBox	m_stop_pre;
	CComboBox	m_data_pre;
	CComboBox	m_port_pre;
	CComboBox	m_baud_pre;
	CComboBox m_LINDevNum;
	CComboBox m_LINChannel;
	CComboBox m_software_v1;
	CComboBox m_software_v2;
	CComboBox m_software_v3;
	CComboBox m_software_v4;
	CComboBox m_BUSADAPTER;
	CCXPButton	m_button2;
	CCXPButton	m_button1;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfig)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void  InitSoftwareCtrl();
	
	// Generated message map functions
	//{{AFX_MSG(CConfig)
	virtual BOOL OnInitDialog();
	afx_msg void OnSave();
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	
public:
	BOOL m_bportPreOpened; 
	void  ReadWriteSoftwareVer(bool bWrite);


	afx_msg void OnBnClickedCancel();
	afx_msg void OnCbnSelchangeCbBusadapter();
	CCXPButton m_savesetting;
	CCXPButton m_return;
	CButton m_canfd;
	CButton m_brs;
	afx_msg void OnBnClickedCkCanfd();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIG_H__19B12A1B_CDC7_496F_B848_33458C613571__INCLUDED_)
