// Config.cpp : implementation file
//

#include "stdafx.h"
#include "ACPS.h"
#include "Config.h"
#include "ACPSDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CANFD gCanFD;
extern int gDevHandle[2];
extern LINConfig linCfg[LINNUM];
extern Config config[COMNUM];
extern HIGHVOLPOWER_ADDR HVP_ADDR;
extern CString g_software[SOFTWARE_VERSION];//软件版本号

/////////////////////////////////////////////////////////////////////////////
// CConfig dialog
#pragma warning(disable: 4996)

CConfig::CConfig(CWnd* pParent /*=NULL*/)
	: CDialog(CConfig::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfig)
	//}}AFX_DATA_INIT
}

void CConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfig)
	DDX_Control(pDX, IDC_COMBO20, m_check_iac);
	DDX_Control(pDX, IDC_COMBO18, m_data_iac);
	DDX_Control(pDX, IDC_COMBO17, m_baud_iac);
	DDX_Control(pDX, IDC_COMBO16, m_port_iac);
	DDX_Control(pDX, IDC_COMBO10, m_check_tem);
	DDX_Control(pDX, IDC_COMBO9, m_stop_tem);
	DDX_Control(pDX, IDC_COMBO8, m_data_tem);
	DDX_Control(pDX, IDC_COMBO7, m_baud_tem);
	DDX_Control(pDX, IDC_COMBO6, m_port_tem);
	DDX_Control(pDX, IDC_COMBO5, m_check_pre);
	DDX_Control(pDX, IDC_COMBO4, m_stop_pre);
	DDX_Control(pDX, IDC_COMBO3, m_data_pre);
	DDX_Control(pDX, IDC_COMBO2, m_baud_pre);
	DDX_Control(pDX, IDC_COMBO1, m_port_pre);
	DDX_Control(pDX, IDC_COMBOLINDEVNUM, m_LINDevNum);
	DDX_Control(pDX, IDC_COMBOLINCHANNEL, m_LINChannel);
	DDX_Control(pDX, IDC_CB_SOFTWARE_V1, m_software_v1);
	DDX_Control(pDX, IDC_CB_SOFTWARE_V2, m_software_v2);
	DDX_Control(pDX, IDC_CB_SOFTWARE_V3, m_software_v3);
	DDX_Control(pDX, IDC_CB_SOFTWARE_V4, m_software_v4);
	DDX_Control(pDX, IDC_CB_BUSADAPTER, m_BUSADAPTER);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_BUTTON1, m_savesetting);
	DDX_Control(pDX, IDCANCEL, m_return);
	DDX_Control(pDX, IDC_CK_CANFD, m_canfd);
	DDX_Control(pDX, IDC_CK_BRS, m_brs);
}


BEGIN_MESSAGE_MAP(CConfig, CDialog)
	//{{AFX_MSG_MAP(CConfig)
	ON_BN_CLICKED(IDC_BUTTON1, OnSave)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDCANCEL, &CConfig::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_CB_BUSADAPTER, &CConfig::OnCbnSelchangeCbBusadapter)
	ON_BN_CLICKED(IDC_CK_CANFD, &CConfig::OnBnClickedCkCanfd)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfig message handlers

BOOL CConfig::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here
	UpdateData(true);

	for(int i=0;i<100;i++)
	{
		CString s;
		s.Format("COM%d",i+1);
		m_port_pre.InsertString(i,s);
		m_port_tem.InsertString(i,s);
	}
	m_port_pre.SetCurSel(config[0].PORT-1);
	m_port_tem.SetCurSel(config[1].PORT-1);
	
	for(int i=0;i<18;i++)
	{
		CString s;s.Format("%d",CommBaud[i]);
		m_baud_pre.InsertString(i,s);
		m_baud_tem.InsertString(i,s);
	}
	m_baud_pre.SetCurSel(config[0].BUND);
	m_baud_tem.SetCurSel(config[1].BUND);
	
	for(int i=0;i<4;i++)
	{
		CString s;s.Format("%d",i+5);
		m_data_pre.InsertString(i,s);
		m_data_tem.InsertString(i,s);
	}
	m_data_pre.SetCurSel(config[0].DATA);
	m_data_tem.SetCurSel(config[1].DATA);
	
	for(int i=0;i<3;i++)
	{
		CString s;s.Format("%.1f",i*0.5+1);
		m_stop_pre.InsertString(i,s);
		m_stop_tem.InsertString(i,s);
	}
	m_stop_pre.SetCurSel(config[0].STOP);
	m_stop_tem.SetCurSel(config[1].STOP);
	
	for(int i=0;i<5;i++)
	{
		m_check_pre.InsertString(i,CommCheck[i]);
		m_check_tem.InsertString(i,CommCheck[i]);
	}
	
// 	Standrad Extended
	m_port_iac.InsertString(0, "Standrad");
	m_port_iac.InsertString(1, "Extended");
	m_baud_iac.InsertString(0, "Data");
	m_baud_iac.InsertString(1, "Romote");
	m_data_iac.InsertString(0, "0");
	m_data_iac.InsertString(1, "1");
	m_check_iac.InsertString(0, "USB_CAN");
	m_check_iac.InsertString(1, "USB_CAN2");

	for (int i = 0; i < 5; i++)
	{
		m_port_iac.SetCurSel(config[3].PORT);
		m_baud_iac.SetCurSel(config[3].BUND);
		m_data_iac.SetCurSel(config[3].DATA);
		m_check_iac.SetCurSel(config[3].CHECK);
	}
	m_check_pre.SetCurSel(config[0].CHECK);
	m_check_tem.SetCurSel(config[1].CHECK);

	m_canfd.SetCheck(gCanFD.CANFD);
	m_brs.SetCheck(gCanFD.BRS);

	if (m_canfd.GetCheck())
		m_brs.EnableWindow(TRUE);
	else
		m_brs.EnableWindow(FALSE);

//LIN device
	TCHAR trans[2];
	memset(trans, 0, sizeof(trans));

	CString strDevNum, strLINChannel;
	for (int i = 0; i < 2; i++)
	{
		_itoa(gDevHandle[i], trans, 10);
		m_LINDevNum.InsertString(i, (LPCTSTR)trans);

		strLINChannel.Format("%d", i+1);
		m_LINChannel.InsertString(i, strLINChannel);
	}
	m_LINDevNum.SetCurSel(linCfg[0].DEVNUM);
	m_LINChannel.SetCurSel(linCfg[0].LINCHANNEL);

//software version
	InitSoftwareCtrl(); //初始化软件版本控件
	ReadWriteSoftwareVer(false);//读取软件版本

//BUS type
	m_BUSADAPTER.SetCurSel(config[4].PORT);

	UpdateData(false);

	if (config[4].PORT == 0) {
		m_port_iac.EnableWindow(true);
		m_baud_iac.EnableWindow(true);
		m_data_iac.EnableWindow(true);
		m_check_iac.EnableWindow(true);
		m_canfd.EnableWindow(true);

		m_LINDevNum.EnableWindow(false);
		m_LINChannel.EnableWindow(false);
	}
	else
	{
		m_port_iac.EnableWindow(false);
		m_baud_iac.EnableWindow(false);
		m_data_iac.EnableWindow(false);
		m_check_iac.EnableWindow(false);
		m_canfd.EnableWindow(false);

		m_LINDevNum.EnableWindow(true);
		m_LINChannel.EnableWindow(true);
	}
	return TRUE;
}

void CConfig::OnSave() 
{
	// TODO: Add your control notification handler code here
	CString sFile;
	TCHAR ch[100];
	memset(ch,0,sizeof(ch));
	
	GetModuleFileName(NULL,sFile.GetBuffer(MAX_PATH),MAX_PATH);
	sFile.ReleaseBuffer(MAX_PATH);
	sFile = sFile.Left(sFile.ReverseFind('\\') + 1);
	sFile += _T("config.ini"); 
	
	CFileFind finder;
	
	if (!finder.FindFile(sFile))
	{
		AfxMessageBox("当前目录没有发现config.ini配置文件");
	}

	UpdateData(TRUE);
	
	CString portStr,IpStr,bundStr, dataStr, stopStr, checkStr, canFD, BRS;
	
	//PRE
	portStr.Format(_T("%d"),m_port_pre.GetCurSel()+1);
	dataStr.Format(_T("%d"),m_data_pre.GetCurSel());
	stopStr.Format(_T("%d"),m_stop_pre.GetCurSel());
	checkStr.Format(_T("%d"),m_check_pre.GetCurSel());
	bundStr.Format(_T("%d"),m_baud_pre.GetCurSel());

	::WritePrivateProfileString("PRE","PORT", portStr, sFile);
	::WritePrivateProfileString("PRE","BUND", bundStr, sFile);
	::WritePrivateProfileString("PRE","DATA", dataStr, sFile);
	::WritePrivateProfileString("PRE","STOP", stopStr, sFile);
	::WritePrivateProfileString("PRE","CHECK", checkStr, sFile);
	
	config[0].PORT=m_port_pre.GetCurSel()+1;
	config[0].BUND=m_baud_pre.GetCurSel();
	config[0].DATA=m_data_pre.GetCurSel();
	config[0].STOP=m_stop_pre.GetCurSel();
	config[0].CHECK=m_check_pre.GetCurSel();
	
	//TEM
	portStr.Format(_T("%d"),m_port_tem.GetCurSel()+1);
	dataStr.Format(_T("%d"),m_data_tem.GetCurSel());
	stopStr.Format(_T("%d"),m_stop_tem.GetCurSel());
	checkStr.Format(_T("%d"),m_check_tem.GetCurSel());
	bundStr.Format(_T("%d"),m_baud_tem.GetCurSel());
	
	::WritePrivateProfileString("TEM","PORT", portStr, sFile);
	::WritePrivateProfileString("TEM","BUND", bundStr, sFile);
	::WritePrivateProfileString("TEM","DATA", dataStr, sFile);
	::WritePrivateProfileString("TEM","STOP", stopStr, sFile);
	::WritePrivateProfileString("TEM","CHECK", checkStr, sFile);
		
	config[1].PORT=m_port_tem.GetCurSel()+1;
	config[1].BUND=m_baud_tem.GetCurSel();
	config[1].DATA=m_data_tem.GetCurSel();
	config[1].STOP=m_stop_tem.GetCurSel();
	config[1].CHECK=m_check_tem.GetCurSel();

	
	//IAC
	portStr.Format(_T("%d"),m_port_iac.GetCurSel());
	dataStr.Format(_T("%d"),m_data_iac.GetCurSel());
	bundStr.Format(_T("%d"),m_baud_iac.GetCurSel());
	checkStr.Format(_T("%d"), m_check_iac.GetCurSel());
	canFD.Format(_T("%d"), m_canfd.GetCheck());
	BRS.Format(_T("%d"), m_brs.GetCheck());

	::WritePrivateProfileString("CAN","FRAME", portStr, sFile);
	::WritePrivateProfileString("CAN","TYPE", bundStr, sFile);
	::WritePrivateProfileString("CAN","CANDEX", dataStr, sFile);
	::WritePrivateProfileString("CAN","DEVTYPE", checkStr, sFile);
	::WritePrivateProfileString("CAN", "CANFD", canFD, sFile);
	::WritePrivateProfileString("CAN", "BRS", BRS, sFile);

	config[3].PORT=m_port_iac.GetCurSel(); //FEAME
	config[3].BUND=m_baud_iac.GetCurSel(); //TYPE
	config[3].DATA=m_data_iac.GetCurSel(); //CANDEX
	config[3].CHECK=m_check_iac.GetCurSel(); //DEVTYPE
	gCanFD.CANFD = m_canfd.GetCheck();	//CANFD
	gCanFD.BRS = m_brs.GetCheck();			//BRS

	//20230915 for LIN 
	CString strLinDevNum, strLinChannel;
	strLinDevNum.Format(_T("%d"),m_LINDevNum.GetCurSel());
	strLinChannel.Format(_T("%d"), m_LINChannel.GetCurSel());

	::WritePrivateProfileString("LIN", "DEVNUM", strLinDevNum, sFile);
	::WritePrivateProfileString("LIN", "CHANNEL", strLinChannel, sFile);

	linCfg[0].DEVNUM = m_LINDevNum.GetCurSel(); //Device Id
	linCfg[0].LINCHANNEL = m_LINChannel.GetCurSel(); //Channel

	//总线类型
	CString strBusType;
	strBusType.Format(_T("%d"), m_BUSADAPTER.GetCurSel());
	::WritePrivateProfileString("BUS", "TYPE", strBusType, sFile);
	config[4].PORT = m_BUSADAPTER.GetCurSel();

	ReadWriteSoftwareVer(true);

	AfxMessageBox("配置文件保存成功");
	UpdateData(FALSE);
}

void CConfig::OnBnClickedCancel()
{
	CDialog::OnCancel();
}

//初始化界面软件版本控件
void  CConfig::InitSoftwareCtrl()
{
	unsigned char cV;
	CString strSoftVer;
	for (int x = 0; x < 24; x++)
	{
		cV = x + 'A';
		strSoftVer.Format("%c", cV);
		m_software_v1.InsertString(x, strSoftVer);
	}
	for (int x = 0; x < 256; x++)
	{
		cV = x;
		strSoftVer.Format("%X", cV);
		m_software_v2.InsertString(x, strSoftVer);
	}
	for (int x = 0; x < 24; x++)
	{
		cV = x + 'A';
		strSoftVer.Format("%c", cV);
		m_software_v3.InsertString(x, strSoftVer);
	}
	for (int x = 0; x < 256; x++)
	{
		cV = x;
		strSoftVer.Format("%X", cV);
		m_software_v4.InsertString(x, strSoftVer);
	}

}

void  CConfig::ReadWriteSoftwareVer(bool bWrite)
{
	//读取软件版本配置
	CString sFile;
	TCHAR ch[10];
	memset(ch, 0, sizeof(ch));

	GetModuleFileName(NULL, sFile.GetBuffer(MAX_PATH), MAX_PATH);
	sFile.ReleaseBuffer(MAX_PATH);
	sFile = sFile.Left(sFile.ReverseFind('\\') + 1);
	sFile += _T("config.ini");

	CFileFind finder;
	CString cfgname, tmpname;
	int nVer = 0;

	if (bWrite)
	{
		for (int i = 0; i < SOFTWARE_VERSION; i++)
		{
			tmpname.Format("SOFTWARE_V%d", i + 1);

			if (i == 0) {
				nVer = m_software_v1.GetCurSel();
			}
			else if (i == 1)
			{
				nVer = m_software_v2.GetCurSel();
			}
			else if (i == 2)
			{
				nVer = m_software_v3.GetCurSel();
			}
			else if (i == 3)
			{
				nVer = m_software_v4.GetCurSel();
			}

			cfgname.Format("%d", nVer);
			::WritePrivateProfileString("SOFTWARE_VERSION", tmpname, cfgname, sFile);
			g_software[i] = cfgname;
		}
	}
	else
	{
		CString temp, strtemp;
		temp = "SOFTWARE_VERSION";
		for (int i = 0; i < SOFTWARE_VERSION; i++)
		{
			tmpname.Format("SOFTWARE_V%d", i + 1);
			GetPrivateProfileString(_T(temp), tmpname, _T("0"), ch, 100, sFile);
			strtemp = ch;
			g_software[i] = strtemp;
		}
	}

	nVer = atoi(g_software[0]);
	m_software_v1.SetCurSel(nVer);

	nVer = atoi(g_software[1]);
	m_software_v2.SetCurSel(nVer);

	nVer = atoi(g_software[2]);
	m_software_v3.SetCurSel(nVer);

	nVer = atoi(g_software[3]);
	m_software_v4.SetCurSel(nVer);
}

void CConfig::OnCbnSelchangeCbBusadapter()
{
	if (m_BUSADAPTER.GetCurSel() == 0)
	{
		m_port_iac.EnableWindow(true);
		m_baud_iac.EnableWindow(true);
		m_data_iac.EnableWindow(true);
		m_check_iac.EnableWindow(true);
		m_canfd.EnableWindow(true);

		m_LINDevNum.EnableWindow(false);
		m_LINChannel.EnableWindow(false);
	}
	else
	{
		m_port_iac.EnableWindow(false);
		m_baud_iac.EnableWindow(false);
		m_data_iac.EnableWindow(false);
		m_check_iac.EnableWindow(false);
		m_canfd.EnableWindow(false);

		m_LINDevNum.EnableWindow(true);
		m_LINChannel.EnableWindow(true);
	}
}


void CConfig::OnBnClickedCkCanfd()
{
	if (m_canfd.GetCheck())
	{
		m_brs.EnableWindow(TRUE);
	}
	else
	{
		m_brs.EnableWindow(FALSE);
	}
}
