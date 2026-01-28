// ACPSDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ACPS.h"
#include "ACPSDlg.h"
#include "Config.h"
#include "range.h"
#include "serial.h"
#include "excel.h"
#include "math.h"
#include "comdef.h"
#include "comutil.h"
#include "CSpreadSheet.h"
#include "CSnapWnd.h"
#include "afx.h"
#include "afxwin.h"
#include "afxmt.h"
#include "methord.h"
#include "stdio.h"

#include "usb_device.h"
#include "usb2lin_ex.h"

#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <thread>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSerial myserial_pre, myserial_tem;
std::mutex g_mtx;
HANDLE g_MsgCycleSignaled; 
HANDLE g_DiagRespSignaled;

bool g_bScanGunOpen;
CString g_Serialnumber, g_Serialnumber2;
//用例单功能计时器
long g_starttm, g_endtm;
long g_difftm;

float g_power_cali1; //功率校准1
float g_power_cali2; //功率校准2
//输出功率
float g_fPower1, g_fPower2;
bool g_NotRecvAppMsg_BeforeTestFlowStart;

_Application app;
Workbooks books; //工作薄集合
_Workbook book;
Worksheets sheets; //工作表集合
_Worksheet sheet;  //工作表
Range erange;	   //Excel中针对单元格的操作都应先获取其对应的Range对象
Font ft;
Range cols;
LPDISPATCH lpDisp = NULL;

//诊断响应ID
DWORD g_DiagReq;
DWORD g_DiagResp;
bool g_3EReq;

//配置结构体
CANFD	gCanFD;
LINConfig linCfg[LINNUM];
Config config[COMNUM];
CfgSet cfg[MAX_SIZE];
PcbaStru pcba;
HIGHVOLPOWER_ADDR HVP_ADDR;
int g_cfgindex;//用户选中的配置序号

CString g_boxcfg[MAX_SIZE];//
CWinThread *pWinThread;	//主测试流程线程
CWinThread* pSendMsgThread; //高分辨率时钟(功率请求报文发送)
CSerial *portPre; // RS485
CSerial *portTem; // 低压电源设备

int StopFlag = 0;
int TimerFlag = 0;

//CAN message ID
//request
u32 ECURequst;
//resposne
u32 ECUFeedback1;
u32 ECUFeedback2;

//LIN device handle
int gDevHandle[2];
int gLINChannelIndex;
int gLINMasterIndex;
int gLINSlaveIndex;
int gDevIndex;
u32 gSlavePID;		

int g_nCancel_PW1;
UINT gHighVolTimes;
CWinThread* CAN_Handle;    //CANN报文接收线程
CWinThread* LIN_Handle;    //LIN报文接收线程
bool m_bSuspendThread;
bool gLINThreadBegin = false;  //线程退出/执行控制
bool m_ExistPreThread;//系统测试主流程退出标志
HANDLE gEventLin;

CString g_software[SOFTWARE_VERSION];//软件版本号
CString g_projectinfo[SOFTWARE_VERSION];//设置的项目信息
u8 g_resp_prjinfo1[SOFTWARE_VERSION]; //读取的项目信息
u8 g_resp_prjinfo2[SOFTWARE_VERSION]; //读取的项目信息2
u8 g_resp_software[SOFTWARE_VERSION]; //读取的软件版本号
u8 g_resp_software2[SOFTWARE_VERSION]; //读取的软件版本号2
u8 g_resp_power_cali1[POWER_CALIBRATION]; //功率校准响应1
u8 g_resp_power_cali2[POWER_CALIBRATION]; //功率校准响应2
u8 g_resp_seed1[SOFTWARE_VERSION]; //安全访问种子1
u8 g_resp_seed2[SOFTWARE_VERSION]; //安全访问种子2
u8 g_resp_key1[SOFTWARE_VERSION]; //安全访问密钥1
u8 g_resp_key2[SOFTWARE_VERSION]; //安全访问密钥2

//平均功率
int g_kPowerRunTime;
float gfPower;
std::vector<float> g_powerList;

//CAN message object
PECU_HVH_Frame1 ECU_HVH_Frm1;
PHVH_ECU_Frame1 HVH_ECU_Frm1, HVH_ECU_Frm21;
PHVH_ECU_Frame2 HVH_ECU_Frm2, HVH_ECU_Frm22;

//LIN message object
PLIN_MSG_RECV LINMSG_RECV;

HWND g_hWnd; //主窗口句柄
HWND gLst3Wnd;// LIST3窗口句柄
CString gSnapWndPath; //报文窗口截图保存路径
int gSelectedBUSAdapter; //已选择的总线适配器类型  0: CAN; 1:LIN


class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum
	{
		IDD = IDD_ABOUTBOX
	};
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange *pDX); // DDX/DDV support
													 //}}AFX_VIRTUAL

	// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CACPSDlg::CACPSDlg(CWnd *pParent /*=NULL*/)
	: CDialog(CACPSDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CACPSDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
#ifdef NDEBUG
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
#endif
}

void CACPSDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CACPSDlg)
	DDX_Control(pDX, IDCANCEL, m_exit);
	DDX_Control(pDX, IDC_LIST1, m_list1);
	DDX_Control(pDX, IDC_LIST5, m_listexcel);
	DDX_Control(pDX, IDC_OFFMSG, m_buttonmsg);
	DDX_Control(pDX, IDC_STATIC_TITLE, m_static1);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_OVEREDIT, m_overedit);
	DDX_Control(pDX, IDC_LOGO, m_ManHuiLogo);
	DDX_Control(pDX, IDC_LIST3, m_msglst);
	DDX_Control(pDX, IDC_BTNSTART, m_StartTestTH);
	DDX_Control(pDX, IDC_BTNSTOP, m_PauseTestTH);
	DDX_Control(pDX, IDC_COMMSETTING, m_CommSetting);
	DDX_Control(pDX, IDC_REPORT_EXPORT, m_ReportExport);
	DDX_Control(pDX, IDC_TEST_PARAMS, m_ParamsSetting);
	DDX_Control(pDX, IDC_OVEREDIT2, m_overedit2);
	DDX_Control(pDX, IDC_STATICPCBA, m_TestStatus);
}

BEGIN_MESSAGE_MAP(CACPSDlg, CDialog)
	//{{AFX_MSG_MAP(CACPSDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDCANCEL, OnACPSCancel)
	ON_WM_DEVICECHANGE()
	//}}AFX_MSG_MAP
	
ON_BN_CLICKED(IDC_BTNSTART, &CACPSDlg::OnBnClickedBtnstart)
ON_WM_TIMER()
ON_BN_CLICKED(IDC_OFFMSG, &CACPSDlg::OnBnClickedOffmsg)
ON_BN_CLICKED(IDC_BTNSTOP, &CACPSDlg::OnBnClickedBtnstop)
ON_CBN_DBLCLK(IDC_COMBOCFG, &CACPSDlg::OnCbnDblclkCombocfg)
ON_CBN_SELCHANGE(IDC_COMBOCFG, &CACPSDlg::OnCbnSelchangeCombocfg)
ON_CBN_KILLFOCUS(IDC_COMBOCFG, &CACPSDlg::OnCbnKillfocusCombocfg)
ON_WM_CTLCOLOR()
ON_BN_CLICKED(IDC_BOXCFG, &CACPSDlg::OnBnClickedBoxcfg)
ON_CBN_SELCHANGE(IDC_COMBOLINMODE, &CACPSDlg::OnSelchangeCombolinmode)
ON_CBN_KILLFOCUS(IDC_COMBOLINMODE, &CACPSDlg::OnKillfocusCombolinmode)
ON_CBN_DROPDOWN(IDC_COMBOLINMODE, &CACPSDlg::OnDropdownCombolinmode)

ON_MESSAGE(WM_UPDATEDATA, OnUpdateData)
ON_MESSAGE(WM_TEST_ABNORMAL, OnAbnormalTest)

ON_BN_CLICKED(IDC_RADIO_POWER, &CACPSDlg::OnBnClickedRadioPower)
ON_BN_CLICKED(IDC_RADIO_GEAR, &CACPSDlg::OnBnClickedRadioGear)
ON_BN_CLICKED(IDC_RADIO_TEMP, &CACPSDlg::OnBnClickedRadioTemp)
ON_EN_CHANGE(IDC_EDIT_MODE, &CACPSDlg::OnEnChangeEditMode)
ON_EN_SETFOCUS(IDC_EDIT_MODE, &CACPSDlg::OnEnSetfocusEditMode)
ON_EN_CHANGE(IDC_EDIT_WAITTIME, &CACPSDlg::OnEnChangeEditWaittime)
ON_EN_SETFOCUS(IDC_EDIT_WAITTIME, &CACPSDlg::OnEnSetfocusEditWaittime)
ON_BN_CLICKED(IDC_COMMSETTING, &CACPSDlg::OnBnClickedCommsetting)
ON_BN_CLICKED(IDC_REPORT_EXPORT, &CACPSDlg::OnBnClickedReportExport)
ON_BN_CLICKED(IDC_TEST_PARAMS, &CACPSDlg::OnBnClickedTestParams)

ON_WM_CLOSE()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CACPSDlg message handlers
BOOL IsDirExist(LPCTSTR szDir)
{
	HANDLE hFile = ::CreateFile(szDir, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return FALSE;
	}
	::CloseHandle(hFile);
	return TRUE;
}

BOOL CreateDir(LPCTSTR szDir)
{
	return CreateDirectory(szDir, NULL);
}
/// <summary>
/// 创建报文控件CList截图保存路径
/// </summary>
/// <returns></returns>
BOOL CreateSnapWndPath(CString* SnapWndPath)
{
	BOOL bRight = FALSE;
	char* strTemp;
	strTemp = new char[MAX_PATH];
	//char strTemp[MAX_PATH];
	//memset(strTemp, 0, sizeof(MAX_PATH));
	GetCurrentDirectory(MAX_PATH, strTemp);

	*SnapWndPath = strTemp;
	*SnapWndPath = *SnapWndPath + "\\SnapWnd\\"; 

	if (!IsDirExist(*SnapWndPath))
		bRight = CreateDir(*SnapWndPath);
	else
		bRight = TRUE;

	delete[] strTemp;
	strTemp = NULL;
	return bRight;
}

//初始化和复位部分全局变量
void CACPSDlg::_InitVARs()
{
	m_can_message_lst.clear();

	m_bportPreOpened = FALSE;
	m_bportTemOpened = FALSE;

	m_bDevLost1 = false;
	m_bDevLost2 = false;
	m_bLowPowerLost = false;

	g_3EReq = false;
	g_bScanGunOpen = false;
	m_bStartCollectPower = false;
	g_NotRecvAppMsg_BeforeTestFlowStart = false;

	g_kPowerRunTime = 0;
	gfPower = 0.0f;
	g_powerList.clear();
	m_bEEPWrote = false;
	m_bReadProjectInfo1 = false;
	m_bReadProjectInfo2 = false;
	m_bSoftwareVerVerified = false;
	m_bSoftwareVerVerified2 = false;

	m_ulTicktCount1 = 0;
	m_ulTicktCount2 = 0;
	m_bLostPtc1 = false;
	m_bLostPtc2 = false;

	g_mCANDevOK = 0; //CAN device connected
	StopFlag = 0;
	TimerFlag = 0;
	gLINChannelIndex = 0;
	//LIN device handle
	gDevHandle[0] =0;
	gDevHandle[1] = 1;
	gLINSlaveIndex = 1;
	gDevIndex = 0; //for lin1

	//CAN
	//request
	g_DiagResp = 0x7E8;
	g_DiagReq = 0x7E0;
	ECURequst = 0x18EF4421;
	//resposne
	ECUFeedback1 = 0x18FFA044;
	ECUFeedback2 = 0x18FFA144;

	//LIN
	gSlavePID = 0x26;

	g_nCancel_PW1 = 0;
	gHighVolTimes = 0;
	m_bSuspendThread = false;
	m_ExistPreThread = false;
	m_bTested_Msg_Cycle = false;
	m_bErrMsg = false;

	//测试报告结果复位
	//记录测试结果时，未测试用例默认通过测试
	pcba.serise_num1 = "";
	pcba.serise_num2 = "";
	pcba.eep_mark_byte1 = false;	//标志位
	pcba.eep_mark_byte2 = false;
	pcba.lowpwr_curr1 = false;			//低压电流
	pcba.lowpwr_curr2 = false;
	pcba.cycle_time1 = false;			//报文周期
	pcba.cycle_time2 = false;
	pcba.project_info1 = false;			//项目信息
	pcba.project_info2 = false;
	pcba.software_ver1 = false;		//软件版本
	pcba.software_ver2 = false;
	pcba.ov1 = false;						//低压过压
	pcba.ov2 = false;
	pcba.uv1 = false;						//低压欠压
	pcba.uv2 = false;
	pcba.igbt_temp1 = false;			//igbt温度传感器
	pcba.igbt_temp2 = false;
	pcba.ptc_temp1 = false;				//ptc传感器
	pcba.ptc_temp2 = false;
	pcba.ptc_env_temp1 = false;			//pcb温度传感器
	pcba.ptc_env_temp2 = false;
	pcba.PowerMode_power = 0.0f;	  	//4Kw功率值	ptc0
	pcba.PowerError_1 = 0.0f;		  		//4Kw功率误差	ptc0
	pcba.PowerMode_power_1 = 0.0f;	//4Kw功率值	ptc1
	pcba.PowerError_2 = 0.0f;				//4Kw功率误差	ptc1
	pcba.PowerMode_power_2 = 0.0f;	// 8Kw功率值	ptc0
	pcba.PowerError_3 = 0.0f;				//8Kw功率误差	ptc0
	pcba.PowerMode_power_3 = 0.0f;	//8Kw功率值	ptc1
	pcba.PowerError_4 = 0.0f;				//8Kw功率误差	ptc1
	pcba.highvol_overvol1 = false;
	pcba.highvol_overvol2 = false;		//高压过压
	pcba.highvol_undervol1 = false;		//高压欠压
	pcba.highvol_undervol2 = false;
	pcba.fCalied_Power1 = 0.0f;		//ptc0 校准后功率
	pcba.fCalied_Power2 = 0.0f;		//ptc1 校准后功率
	pcba.fCalied_50Slope1 = 0.0f;		//ptc0 50%占空比斜率
	pcba.fCalied_50Slope2 = 0.0f;		//ptc1 50%占空比斜率
	pcba.fCalied_100Slope1 = 0.0f;		//ptc0 100%占空比斜率
	pcba.fCalied_100Slope2 = 0.0f;		//ptc1 100%占空比斜率


	//总线信号值初值
	HVH_ECU_Frm1->HVH_Status = 0;
	HVH_ECU_Frm1->HVH_HVIL_Status = 0;
	HVH_ECU_Frm1->HVH_ActiveDis_Status = 0;
	HVH_ECU_Frm1->HVH_Coolant_In_Temp = 0;
	HVH_ECU_Frm1->HVH_Coolant_Out_Temp = 0;
	HVH_ECU_Frm1->HVH_HV_Current = 0;
	HVH_ECU_Frm1->HVH_HV_Voltage = 0;
	HVH_ECU_Frm1->HVH_ActualPower = 0;

	HVH_ECU_Frm21->HVH_Status = 0;
	HVH_ECU_Frm21->HVH_HVIL_Status = 0;
	HVH_ECU_Frm21->HVH_ActiveDis_Status = 0;
	HVH_ECU_Frm21->HVH_Coolant_In_Temp = 0;
	HVH_ECU_Frm21->HVH_Coolant_Out_Temp = 0;
	HVH_ECU_Frm21->HVH_HV_Current = 0;
	HVH_ECU_Frm21->HVH_HV_Voltage = 0;
	HVH_ECU_Frm21->HVH_ActualPower = 0;

	HVH_ECU_Frm2->HVH_ProtocolError = 0;
	HVH_ECU_Frm2->HVH_ComponentProtection = 0;
	HVH_ECU_Frm2->HVH_WarnHVOutOfRng = 0;
	HVH_ECU_Frm2->HVH_WarnULoOutOfRang = 0;
	HVH_ECU_Frm2->HVH_WarnOverheat = 0;
	HVH_ECU_Frm2->HVH_FailHotspot = 0;
	HVH_ECU_Frm2->HVH_FailTempSensor = 0;
	HVH_ECU_Frm2->HVH_FailDriver = 0;
	HVH_ECU_Frm2->HVH_FailHighCurrent = 0;
	HVH_ECU_Frm2->HVH_FailMemory_InterCom = 0;
	HVH_ECU_Frm2->HVH_FailDCDCConverter = 0;
	
	HVH_ECU_Frm22->HVH_ProtocolError = 0;
	HVH_ECU_Frm22->HVH_ComponentProtection = 0;
	HVH_ECU_Frm22->HVH_WarnHVOutOfRng = 0;
	HVH_ECU_Frm22->HVH_WarnULoOutOfRang = 0;
	HVH_ECU_Frm22->HVH_WarnOverheat = 0;
	HVH_ECU_Frm22->HVH_FailHotspot = 0;
	HVH_ECU_Frm22->HVH_FailTempSensor = 0;
	HVH_ECU_Frm22->HVH_FailDriver = 0;
	HVH_ECU_Frm22->HVH_FailHighCurrent = 0;
	HVH_ECU_Frm22->HVH_FailMemory_InterCom = 0;
	HVH_ECU_Frm22->HVH_FailDCDCConverter = 0;

}

void CACPSDlg::ReadConfig()
{
	///读取配置文件
	CString sFile;

	TCHAR ch[100];
	memset(ch, 0, sizeof(ch));

	GetModuleFileName(NULL, sFile.GetBuffer(MAX_PATH), MAX_PATH);
	sFile.ReleaseBuffer(MAX_PATH);
	sFile = sFile.Left(sFile.ReverseFind('\\') + 1);
	sFile += _T("config.ini");

	CFileFind finder;

	if (!finder.FindFile(sFile))
	{
		//AfxMessageBox("当前目录没有发现config.ini配置文件，确认生成默认配置?");

		CString portStr, IpStr, bundStr, dataStr, stopStr, checkStr, canFD, BRS;

		//PRE
		portStr.Format(_T("%d"), 1);  //串口
		bundStr.Format(_T("%d"), 11); //波特率
		dataStr.Format(_T("%d"), 3);  //数据位
		stopStr.Format(_T("%d"), 0);  //停止位
		checkStr.Format(_T("%d"), 0); //校验位
		canFD.Format(_T("%d"), 0); //CANFD
		BRS.Format(_T("%d"), 0); //BRS

		::WritePrivateProfileString("PRE", "PORT", portStr, sFile);
		::WritePrivateProfileString("PRE", "BUND", bundStr, sFile);
		::WritePrivateProfileString("PRE", "DATA", dataStr, sFile);
		::WritePrivateProfileString("PRE", "STOP", stopStr, sFile);
		::WritePrivateProfileString("PRE", "CHECK", checkStr, sFile);

		config[0].PORT = 1;
		config[0].BUND = 11;
		config[0].DATA = 3;
		config[0].STOP = 0;
		config[0].CHECK = 0;

		//TEM

		::WritePrivateProfileString("TEM", "PORT", portStr, sFile);
		::WritePrivateProfileString("TEM", "BUND", bundStr, sFile);
		::WritePrivateProfileString("TEM", "DATA", dataStr, sFile);
		::WritePrivateProfileString("TEM", "STOP", stopStr, sFile);
		::WritePrivateProfileString("TEM", "CHECK", checkStr, sFile);

		config[1].PORT = 1;
		config[1].BUND = 11;
		config[1].DATA = 3;
		config[1].STOP = 0;
		config[1].CHECK = 0;

		//MOD

		::WritePrivateProfileString("MOD", "PORT", portStr, sFile);
		::WritePrivateProfileString("MOD", "BUND", bundStr, sFile);
		::WritePrivateProfileString("MOD", "DATA", dataStr, sFile);
		::WritePrivateProfileString("MOD", "STOP", stopStr, sFile);
		::WritePrivateProfileString("MOD", "CHECK", checkStr, sFile);

		config[2].PORT = 1;
		config[2].BUND = 11;
		config[2].DATA = 3;
		config[2].STOP = 0;
		config[2].CHECK = 0;

		//CAN

		::WritePrivateProfileString("CAN", "FRAME", portStr, sFile);
		::WritePrivateProfileString("CAN", "TYPE", "0", sFile);
		::WritePrivateProfileString("CAN", "CANDEX", "0", sFile);
		::WritePrivateProfileString("CAN", "DEVTYPE", "0", sFile);
		::WritePrivateProfileString("CAN", "CANFD", canFD, sFile);
		::WritePrivateProfileString("CAN", "BRS", BRS, sFile);

		config[3].PORT = 0;
		config[3].BUND = 0;
		config[3].DATA = 0;
		config[3].CHECK = 0;
		gCanFD.CANFD = 0;
		gCanFD.BRS = 0;

		//LIN
		::WritePrivateProfileString("LIN", "DEVNUM", "0", sFile);
		::WritePrivateProfileString("LIN", "CHANNEL", "0", sFile);

		linCfg[0].DEVNUM = 0; //Device Id
		linCfg[0].LINCHANNEL = 0; //Channel

		//Software version
		::WritePrivateProfileString("SOFTWARE_VERSION", "SOFTWARE_V1", "3", sFile);
		::WritePrivateProfileString("SOFTWARE_VERSION", "SOFTWARE_V2", "0", sFile);
		::WritePrivateProfileString("SOFTWARE_VERSION", "SOFTWARE_V3", "15", sFile);
		::WritePrivateProfileString("SOFTWARE_VERSION", "SOFTWARE_V4", "2", sFile);
		g_software[0] = "44";
		g_software[1] = "0";
		g_software[2] = "50";
		g_software[3] = "22";

		//BUS type
		::WritePrivateProfileString("BUS", "TYPE", "0", sFile);
		config[4].PORT = 0;
	}
	else
	{
		CString temp, strtemp;
		temp = "PRE";
		GetPrivateProfileString(_T(temp), _T("PORT"), _T("1"), ch, 100, sFile);
		strtemp = ch;
		config[0].PORT = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("BUND"), _T("4800"), ch, 100, sFile);
		strtemp = ch;
		config[0].BUND = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("DATA"), _T("8"), ch, 100, sFile);
		strtemp = ch;
		config[0].DATA = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("STOP"), _T("1"), ch, 100, sFile);
		strtemp = ch;
		config[0].STOP = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("CHECK"), _T("0"), ch, 100, sFile);
		strtemp = ch;
		config[0].CHECK = atoi(strtemp.GetBuffer(strtemp.GetLength()));
		/////////////////////////
		temp = "TEM";

		GetPrivateProfileString(_T(temp), _T("PORT"), _T("1"), ch, 100, sFile);
		strtemp = ch;
		config[1].PORT = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("BUND"), _T("4800"), ch, 100, sFile);
		strtemp = ch;
		config[1].BUND = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("DATA"), _T("8"), ch, 100, sFile);
		strtemp = ch;
		config[1].DATA = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("STOP"), _T("1"), ch, 100, sFile);
		strtemp = ch;
		config[1].STOP = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("CHECK"), _T("0"), ch, 100, sFile);
		strtemp = ch;
		config[1].CHECK = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		/////////////////////////
		temp = "MOD";

		GetPrivateProfileString(_T(temp), _T("PORT"), _T("1"), ch, 100, sFile);
		strtemp = ch;
		config[2].PORT = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("BUND"), _T("4800"), ch, 100, sFile);
		strtemp = ch;
		config[2].BUND = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("DATA"), _T("8"), ch, 100, sFile);
		strtemp = ch;
		config[2].DATA = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("STOP"), _T("1"), ch, 100, sFile);
		strtemp = ch;
		config[2].STOP = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("CHECK"), _T("0"), ch, 100, sFile);
		strtemp = ch;
		config[2].CHECK = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		/////////////////////////
		temp = "CAN";
		GetPrivateProfileString(_T(temp), _T("FRAME"), _T("0"), ch, 100, sFile);
		strtemp = ch;
		config[3].PORT = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("TYPE"), _T("0"), ch, 100, sFile);
		strtemp = ch;
		config[3].BUND = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("CANDEX"), _T("0"), ch, 100, sFile);
		strtemp = ch;
		config[3].DATA = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("DEVTYPE"), _T("0"), ch, 100, sFile);
		strtemp = ch;
		config[3].CHECK = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("CANFD"), _T("0"), ch, 100, sFile);
		strtemp = ch;
		gCanFD.CANFD = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("BRS"), _T("0"), ch, 100, sFile);
		strtemp = ch;
		gCanFD.BRS = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		/////////////////////////
		temp = "LIN";
		GetPrivateProfileString(_T(temp), _T("DEVNUM"), _T("0"), ch, 10, sFile);
		strtemp = ch;
		linCfg[0].DEVNUM = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("CHANNEL"), _T("0"), ch, 10, sFile);
		strtemp = ch;
		linCfg[0].LINCHANNEL = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		gDevIndex = linCfg[0].DEVNUM;
		gLINMasterIndex = linCfg[0].LINCHANNEL;


		////////////////////////
		temp = "SOFTWARE_VERSION";
		GetPrivateProfileString(_T(temp), _T("SOFTWARE_V1"), _T("3"), ch, 10, sFile);
		g_software[0] = ch;
		GetPrivateProfileString(_T(temp), _T("SOFTWARE_V2"), _T("0"), ch, 10, sFile);
		g_software[1] = ch;
		GetPrivateProfileString(_T(temp), _T("SOFTWARE_V3"), _T("15"), ch, 10, sFile);
		g_software[2] = ch;
		GetPrivateProfileString(_T(temp), _T("SOFTWARE_V4"), _T("2"), ch, 10, sFile);
		g_software[3] = ch;

		/////////////////////////
		temp = "BUS";
		GetPrivateProfileString(_T(temp), _T("TYPE"), _T("0"), ch, 10, sFile);
		strtemp = ch;
		config[4].PORT = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		strtemp.ReleaseBuffer();
	}
}

void CACPSDlg::ReadRange(int cfgnum)
{
	///读取量程配置文件
	CString sFile;

	TCHAR ch[100];
	memset(ch, 0, sizeof(ch));

	GetModuleFileName(NULL, sFile.GetBuffer(MAX_PATH), MAX_PATH);
	sFile.ReleaseBuffer(MAX_PATH);
	sFile = sFile.Left(sFile.ReverseFind('\\') + 1);

	CString filename;
	filename.Format("range%d.ini", cfgnum);
	filename.Replace(" ", "");
	sFile += filename;

	CFileFind finder;

	if (!finder.FindFile(sFile))
	{
		CString ov1, ov2, uv1, uv2;
		CString highvol_over_vol, highvol_over_restore_vol, highvol_under_vol, highvol_under_restore_vol; //高压过压/欠压故障，高压过压/欠压故障恢复电压
		CString ptc_over_temp, ptc_restore_temp, igbt_over_temp, igbt_restore_temp;
		CString ptc_max_power, waitting_time, ptc_current;
		CString power_error1, ptc_max_power_5Kw, power_error2, waitting_time_5Kw;
		CString powerCali_SlopeBase1, powerCali_SlopeBase2, powerCali_SlopeError1, powerCali_SlopeError2;
		CString project_info, lowPower_vol, lowPower_curr, msg_lost_monitor_cycle;
		CString per50_wait_time, per100_wait_time;

		ov1.Format("%f", 32.5);	
		ov2.Format("%f", 32.0); 
		uv1.Format("%f", 17.5); 
		uv2.Format("%f", 18.0); 

		highvol_over_vol.Format("%f", 760.0);
		highvol_over_restore_vol.Format("%f", 730.0);
		highvol_under_vol.Format("%f", 400.0);
		highvol_under_restore_vol.Format("%f", 380.0);

		ptc_over_temp.Format("%f", 95.0);
		ptc_restore_temp.Format("%f", 75.0);
		igbt_over_temp.Format("%f", 80.0);
		igbt_restore_temp.Format("%f", 75.0);

		ptc_max_power.Format("%f", 4.5f);
		waitting_time.Format("%d", 30);
		ptc_current.Format("%f", 12.0f);

		power_error1.Format("%f", 0.03f);
		ptc_max_power_5Kw.Format("%f", 5.0f);
		power_error2.Format("%f", 0.03f);
		waitting_time_5Kw.Format("%d", 65);

		powerCali_SlopeBase1.Format("%1.15f", 0.434371490196078f);
		powerCali_SlopeBase2.Format("%1.16f", 0.0172416838709677f);
		powerCali_SlopeError1.Format("%1.15f", 0.108001708984375f);
		powerCali_SlopeError2.Format("%1.15f", 0.003997802734375f);

		project_info.Format("%s", "0000");
		lowPower_vol.Format("%f", 24.0f);	
		lowPower_curr.Format("%f", 1.2f);

		per50_wait_time.Format("%7d", 45);
		per100_wait_time.Format("%7d", 70);

		msg_lost_monitor_cycle.Format("%d", 3);

		::WritePrivateProfileString("RANGE", "OVONE", ov1, sFile);
		::WritePrivateProfileString("RANGE", "OVTWO", ov2, sFile);
		::WritePrivateProfileString("RANGE", "UVONE", uv1, sFile);
		::WritePrivateProfileString("RANGE", "UVTWO", uv2, sFile);

		::WritePrivateProfileString("RANGE", "HI_OVER_VOL", highvol_over_vol, sFile);
		::WritePrivateProfileString("RANGE", "HI_OVER_RESTORE_VOL", highvol_over_restore_vol, sFile);
		::WritePrivateProfileString("RANGE", "HI_UNDER_VOL", highvol_under_vol, sFile);
		::WritePrivateProfileString("RANGE", "HI_UNDER_RESTORE_VOL", highvol_under_restore_vol, sFile);

		::WritePrivateProfileString("RANGE", "PTC_OVER_TEMP", ptc_over_temp, sFile);
		::WritePrivateProfileString("RANGE", "PTC_RESTORE_TEMP", ptc_restore_temp, sFile);
		::WritePrivateProfileString("RANGE", "IGBT_OVER_TEMP", igbt_over_temp, sFile);
		::WritePrivateProfileString("RANGE", "IGBT_RESTORE_TEMP", igbt_restore_temp, sFile);

		::WritePrivateProfileString("RANGE", "MAX_PTC_POWER", ptc_max_power, sFile);
		::WritePrivateProfileString("RANGE", "MAX_PTC_POWER_ERROR", power_error1, sFile);
		::WritePrivateProfileString("RANGE", "WAITTING_TIME", waitting_time, sFile);

		::WritePrivateProfileString("RANGE", "MAX_PTC_POWER_5Kw", ptc_max_power_5Kw, sFile);
		::WritePrivateProfileString("RANGE", "MAX_PTC_POWER_5Kw_ERROR", power_error2, sFile);
		::WritePrivateProfileString("RANGE", "WAITTING_TIME_5Kw", waitting_time_5Kw, sFile);
		::WritePrivateProfileString("RANGE", "PTC_CURRENT", ptc_current, sFile);

		//功率校准斜率基准与误差
		::WritePrivateProfileString("RANGE", "SLOPE_BASE1", powerCali_SlopeBase1, sFile);
		::WritePrivateProfileString("RANGE", "SLOPE_BASE2", powerCali_SlopeBase2, sFile);
		::WritePrivateProfileString("RANGE", "SLOPE_ERROR1", powerCali_SlopeError1, sFile);
		::WritePrivateProfileString("RANGE", "SLOPE_ERROR2", powerCali_SlopeError2, sFile);

		::WritePrivateProfileString("RANGE", "PER50_WAIT_TIME", per50_wait_time, sFile);
		::WritePrivateProfileString("RANGE", "PER100_WAIT_TIME", per100_wait_time, sFile);

		//project_info
		::WritePrivateProfileString("RANGE", "PROJECT_INFO", project_info, sFile);

		//low power voltage & current
		::WritePrivateProfileString("RANGE", "LOWPRE", lowPower_vol, sFile);
		::WritePrivateProfileString("RANGE", "LOWCUR", lowPower_curr, sFile);

		////报文丢失监控周期
		::WritePrivateProfileString("RANGE", "MSG_LOST_MONITOR_CYCLE", msg_lost_monitor_cycle, sFile);

		cfg[cfgnum].ov1 = 32.5f;
		cfg[cfgnum].ov2 = 32.0f;	
		cfg[cfgnum].uv1 = 17.5f;
		cfg[cfgnum].uv2 = 18.0f;

		cfg[cfgnum].highvol_over_vol = 750.0f;
		cfg[cfgnum].highvol_over_restore_vol = 730.0f;
		cfg[cfgnum].highvol_under_vol = 380.0f;
		cfg[cfgnum].highvol_under_restore_vol = 400.0f;

		cfg[cfgnum].ptc_overtemp_low = 95.0f;
		cfg[cfgnum].ptc_overtemp_high = 75.0f;
		cfg[cfgnum].igbt_overtemp_low = 80.0f;
		cfg[cfgnum].igbt_overtemp_high = 75.0f;

		cfg[cfgnum].max_ptc_power = 2.0f;
		cfg[cfgnum].max_ptc_power_error = 0.03f;
		cfg[cfgnum].waitting_time = 45;
		cfg[cfgnum].run_current = 25.0f;

		cfg[cfgnum].max_ptc_power_5Kw = 5.0f;
		cfg[cfgnum].max_ptc_power_5Kw_error = 0.03f;
		cfg[cfgnum].waitting_time_5Kw = 65;
		cfg[cfgnum].run_current = 25.0f;

		cfg[cfgnum].slope_base1 = 0.434371490196078f;
		cfg[cfgnum].slope_base2 = 0.0172416838709677f;
		cfg[cfgnum].slope_error1 = 0.108001708984375f;
		cfg[cfgnum].slope_error2 = 0.003997802734375f;

		cfg[cfgnum].per50_wait_time = 45;
		cfg[cfgnum].per100_wait_time = 70;

		cfg[cfgnum].project_info = "0000";
		cfg[cfgnum].lowpre = 24.0f;
		cfg[cfgnum].lowcur = 1.2f;

		cfg[cfgnum].msg_lost_monitor_cycle = 3;
	}
	else
	{
		CString temp, strtemp;
		temp = "RANGE";

		cfg[cfgnum].lowpre = 24.0f;
		cfg[cfgnum].lowcur = 1.2f;

		GetPrivateProfileString(_T(temp), _T("OVONE"), _T("32.5"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].ov1 = atof(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("OVTWO"), _T("32.0"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].ov2 = atof(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("UVONE"), _T("17.5"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].uv1 = atof(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("UVTWO"), _T("18.0"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].uv2 = atof(strtemp.GetBuffer(strtemp.GetLength()));
	

		//高压过压/欠压
		GetPrivateProfileString(_T(temp), _T("HI_OVER_VOL"), _T("760.0"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].highvol_over_vol = atof(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("HI_OVER_RESTORE_VOL"), _T("730.0"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].highvol_over_restore_vol = atof(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("HI_UNDER_VOL"), _T("380.0"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].highvol_under_vol = atof(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("HI_UNDER_RESTORE_VOL"), _T("400.0"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].highvol_under_restore_vol = atof(strtemp.GetBuffer(strtemp.GetLength()));

		//PTC, IGBT过温
		GetPrivateProfileString(_T(temp), _T("PTC_OVER_TEMP"), _T("95"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].ptc_overtemp_low = atof(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("PTC_RESTORE_TEMP"), _T("75"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].ptc_overtemp_high = atof(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("IGBT_OVER_TEMP"), _T("80"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].igbt_overtemp_low = atof(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("IGBT_RESTORE_TEMP"), _T("75"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].igbt_overtemp_high = atof(strtemp.GetBuffer(strtemp.GetLength()));

		//PTC最大功率
		GetPrivateProfileString(_T(temp), _T("MAX_PTC_POWER"), _T("4.5"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].max_ptc_power = atof(strtemp.GetBuffer(strtemp.GetLength()));
		//PTC最大功率误差
		GetPrivateProfileString(_T(temp), _T("MAX_PTC_POWER_ERROR"), _T("0.1"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].max_ptc_power_error = atof(strtemp.GetBuffer(strtemp.GetLength()));
		//功率测试等待时间
		GetPrivateProfileString(_T(temp), _T("WAITTING_TIME"), _T("30"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].waitting_time = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		//PTC最大功率6Kw
		GetPrivateProfileString(_T(temp), _T("MAX_PTC_POWER_5Kw"), _T("2.0"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].max_ptc_power_5Kw = atof(strtemp.GetBuffer(strtemp.GetLength()));
		//PTC最大功率误差6Kw
		GetPrivateProfileString(_T(temp), _T("MAX_PTC_POWER_5Kw_ERROR"), _T("0.1"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].max_ptc_power_5Kw_error = atof(strtemp.GetBuffer(strtemp.GetLength()));
		//功率测试等待时间6Kw
		GetPrivateProfileString(_T(temp), _T("WAITTING_TIME_5Kw"), _T("30"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].waitting_time_5Kw = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		//测试电流
		GetPrivateProfileString(_T(temp), _T("PTC_CURRENT"), _T("12.0"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].run_current = atof(strtemp.GetBuffer(strtemp.GetLength()));

		//功率校准斜率基准与误差
		//基准1
		GetPrivateProfileString(_T(temp), _T("SLOPE_BASE1"), _T("0.434371490196078"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].slope_base1 = atof(strtemp.GetBuffer(strtemp.GetLength()));
		//基准2
		GetPrivateProfileString(_T(temp), _T("SLOPE_BASE2"), _T("0.0172416838709677"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].slope_base2 = atof(strtemp.GetBuffer(strtemp.GetLength()));
		//误差1
		GetPrivateProfileString(_T(temp), _T("SLOPE_ERROR1"), _T("0.108001708984375"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].slope_error1 = atof(strtemp.GetBuffer(strtemp.GetLength()));
		//误差2
		GetPrivateProfileString(_T(temp), _T("SLOPE_ERROR2"), _T("0.003997802734375"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].slope_error2 = atof(strtemp.GetBuffer(strtemp.GetLength()));

		//占空比请求时间
		GetPrivateProfileString(_T(temp), _T("PER50_WAIT_TIME"), _T("45"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].per50_wait_time = atoi(strtemp.GetBuffer(strtemp.GetLength()));
		GetPrivateProfileString(_T(temp), _T("PER100_WAIT_TIME"), _T("70"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].per100_wait_time = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		//项目信息
		GetPrivateProfileString(_T(temp), _T("PROJECT_INFO"), _T("0000"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].project_info = strtemp.GetBuffer(strtemp.GetLength());

		//低压电源(电压/电流)
		GetPrivateProfileString(_T(temp), _T("LOWPRE"), _T("24.0"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].lowpre = atof(strtemp.GetBuffer(strtemp.GetLength()));

		GetPrivateProfileString(_T(temp), _T("LOWCUR"), _T("1.2"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].lowcur = atof(strtemp.GetBuffer(strtemp.GetLength()));

		//报文丢失监控周期
		GetPrivateProfileString(_T(temp), _T("MSG_LOST_MONITOR_CYCLE"), _T("3"), ch, 100, sFile);
		strtemp = ch;
		cfg[cfgnum].msg_lost_monitor_cycle = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		strtemp.ReleaseBuffer();
	}
}

void CACPSDlg::ReadBoxCfg()
{
	//读取模块配置文件
	CString sFile;

	TCHAR ch[100];
	memset(ch, 0, sizeof(ch));

	GetModuleFileName(NULL, sFile.GetBuffer(MAX_PATH), MAX_PATH);
	sFile.ReleaseBuffer(MAX_PATH);
	sFile = sFile.Left(sFile.ReverseFind('\\') + 1);
	sFile += _T("boxcfg.ini");

	CFileFind finder;
	CString cfgname, tmpname;

	if (!finder.FindFile(sFile))
	{
		::WritePrivateProfileString("BOXCFG", "BOXINDEX", "1", sFile);
		for (int i = 0; i < MAX_SIZE; i++) //默认3个传感器
		{
			cfgname.Format("pcba%d", i + 1);
			tmpname.Format("BOXINDEX%d", i + 1);
			::WritePrivateProfileString("BOXCFG", tmpname, cfgname, sFile);
			g_boxcfg[i] = cfgname;
		}
	}
	else
	{
		CString temp, strtemp;
		temp = "BOXCFG";
		GetPrivateProfileString(_T(temp), _T("BOXINDEX"), _T("0"), ch, 100, sFile);
		strtemp = ch;
		g_cfgindex = atoi(strtemp.GetBuffer(strtemp.GetLength())) - 1;

		for (int i = 0; i < MAX_SIZE; i++)
		{
			tmpname.Format("BOXINDEX%d", i + 1);
			GetPrivateProfileString(_T(temp), tmpname, _T("0"), ch, 100, sFile);
			strtemp = ch;
			g_boxcfg[i] = strtemp;
		}
	}

	/*
		if (!finder.FindFile(sFile))
	{
		::WritePrivateProfileString("BOXCFG", "BOXINDEX", "0", sFile);

		for (int i = 0; i < MAX_SIZE; i++) //默认3个传感器
		{
			cfgname.Format("pcba%d", i + 1);
			tmpname.Format("BOXINDEX%d", i + 1);
			::WritePrivateProfileString("BOXCFG", tmpname, cfgname, sFile);
			g_boxcfg[i] = cfgname;
		}		
	}
	else
	{
		CString temp, strtemp;
		temp = "BOXCFG";
		GetPrivateProfileString(_T(temp), _T("BOXINDEX"), _T("0"), ch, 100, sFile);
		strtemp = ch;
		g_cfgindex = atoi(strtemp.GetBuffer(strtemp.GetLength()));

		for (int i = 0; i < MAX_SIZE; i++)
		{
			tmpname.Format("BOXINDEX%d", i + 1);
			GetPrivateProfileString(_T(temp), tmpname, _T("0"), ch, 100, sFile);
			strtemp = ch;
			g_boxcfg[i] = strtemp;
		}
	}
	
	*/

}

BOOL CACPSDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	//CreateSnapWndPath(&gSnapWndPath);

	CFont font0;
	font0.CreateFont(25, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Verdana"));
	m_msglst.SetFont(&font0, true);
	UpdateData(false);
	GetDlgItem(IDC_STATICMSG)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_LIST3)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_LIST3)->EnableWindow(TRUE);
	GetDlgItem(IDC_LIST1)->EnableWindow(TRUE);
	GetDlgItem(IDC_LIST1)->ShowWindow(SW_SHOW);

	GetDlgItem(IDC_STATICLISTINFO)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_OVEREDIT)->SetFocus();

#ifdef _CALI_POWERSLOPE
	m_static1.SetWindowTextA("8Kw PTC控制板检测");
#else
	m_static1.SetWindowTextA("8Kw PTC控制板检测\r\n(无功率校准)");
#endif


	//实例化中介类
	m_Mediator = new ConcreteHardwareCom();

	//让低压电源类 认识 中介类
	m_lowPwr = new LowVolPowerCom(m_Mediator);
	//让高压电源类 认识 中介类
	m_highPwr = new HighVolPowerCom(m_Mediator);
	//实例化总线对象
	m_exec = new Executer();

	//报文周期测试event句柄
	g_MsgCycleSignaled = CreateEvent(NULL, TRUE, FALSE, NULL);

	//诊断响应event句柄
	//g_DiagRespSignaled = CreateEvent(NULL, TRUE, FALSE, NULL);

	//for CAN	
	ECU_HVH_Frm1 = new ECU_HVH_Frame1();
	HVH_ECU_Frm1 = new HVH_ECU_Frame1();
	HVH_ECU_Frm2 = new HVH_ECU_Frame2();
	HVH_ECU_Frm21 = new HVH_ECU_Frame1();
	HVH_ECU_Frm22 = new HVH_ECU_Frame2();
	//for LIN
	LINMSG_RECV = new LIN_MSG_RECV();


#ifdef PCAN_RECV_TH
	m_eventHandleToReadCAN = CreateEvent(NULL, FALSE, FALSE, NULL); // Read event
	m_eventHandleToStopWaitingForObject = CreateEvent(NULL, TRUE, FALSE, NULL); // Stop thread that is waiting for CAN Read event
#endif

	g_hWnd = m_hWnd;
	_InitVARs();
	//报文控件窗口句柄
	gLst3Wnd = GetDlgItem(IDC_LIST3)->m_hWnd;

	GetDlgItem(IDC_BTNSTOP)->EnableWindow(FALSE);
	// Add "About..." menu item to system menu.
	int i;
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu *pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);	 // Set big icon
	SetIcon(m_hIcon, FALSE); // Set small icon

	// TODO: Add extra initialization here
	CRect rect;
	GetClientRect(&rect);     //取客户区大小    
	old.x = rect.right - rect.left;  //Point old,用于记录窗口原始坐标
	old.y = rect.bottom - rect.top;

	//设置Static字体
	//m_Font.CreatePointFont(150, "Arial", NULL);
	m_Font.CreateFont(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
					  ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					  DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("宋体"));
	m_static1.SetFont(&m_Font, true);
	UpdateData(false);
	ReadBoxCfg();//读取下拉框名称 
	ReadConfig(); //读取通信配置参数

	//打开扫描枪
	//Switch_ScannerGun_Port(true);
	//m_lowPwr->Open_ScannerGun(true);

	gSelectedBUSAdapter = config[4].PORT;
	if (gSelectedBUSAdapter == 0)		//CAN
	{
#ifdef PCAN_RECV_TH
		m_Bus = new PCAN_Bus();
#else
		m_Bus = new CAN_Bus();
#endif
	}
	else		//LIN
	{
		m_pLinBus = new LIN_Bus();
	}
	
	for (i = 0; i < MAX_SIZE;i++)
	{
		ReadRange(i);  //读取参数配置
	}

	//构建测试项目
	BuildTestItems();

	m_brushEdit = CreateSolidBrush(RGB(0, 0, 0));
	m_brushList = CreateSolidBrush(RGB(0, 0, 0));
	//SetTimer(1, 1000, NULL);//设置焦点永远在编辑框
	//SetTimer(2, 500, NULL);//每500ms发送

	return FALSE; // return TRUE  unless you set the focus to a control
}

//创建测试项目
void CACPSDlg::BuildTestItems()
{
	LONG lStyle;
	lStyle = GetWindowLong(m_listexcel.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT; //设置style
	SetWindowLong(m_listexcel.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_listexcel.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;//选中某行使整行高亮（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_GRIDLINES;//网格线（只适用与report风格的listctrl）
	m_listexcel.SetExtendedStyle(dwStyle); //设置扩展风格

	CFont font;
	font.CreateFont(26, 0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("宋体"));
	m_listexcel.SetFont(&font, true);

	m_listexcel.InsertColumn(0, "项目", LVCFMT_LEFT, 60); //插入列
	m_listexcel.InsertColumn(1, "  低压电流  ", LVCFMT_CENTER, 360);
	m_listexcel.InsertColumn(2, "     诊断标志位写入    ", LVCFMT_CENTER, 1500);
	m_listexcel.InsertColumn(3, "  CAN/LIN通讯  ", LVCFMT_CENTER, 800);//插入列


	CString str;
	str = "设置";
	int row = m_listexcel.InsertItem(1, str); //插入行
	str = "实测A";
	row = m_listexcel.InsertItem(2, str); //插入行
	str = "实测B";
	row = m_listexcel.InsertItem(3, str); //插入行

	for (int i = 0; i < 4; i++)
	{
		str = "项目";
		row = m_listexcel.InsertItem(4 + i * 4, str); //插入行
		str = "设置";
		row = m_listexcel.InsertItem(5 + i * 4, str); //插入行
		str = "实测A";
		row = m_listexcel.InsertItem(6 + i * 4, str); //插入行
		str = "实测B";
		row = m_listexcel.InsertItem(7 + i * 4, str); //插入行
	}

	m_listexcel.SetItemText(3, 1, "  检测项目信息  ");
	m_listexcel.SetItemText(3, 2, "  检测软件版本 ");
	m_listexcel.SetItemText(3, 3, "  低压电压诊断 ");

	m_listexcel.SetItemText(7, 1, "  检测IGBT传感器电路  ");
	m_listexcel.SetItemText(7, 2, "  检测PTC传感器电路	");
	m_listexcel.SetItemText(7, 3, "  检测PCB温度传感器电路	");

	m_listexcel.SetItemText(11, 1, "  校准模式	");
	m_listexcel.SetItemText(11, 2, "  功率控制(4Kw) ");
	m_listexcel.SetItemText(11, 3, "  功率控制(8Kw) ");

	m_listexcel.SetItemText(15, 1, " 高压电压诊断 ");
	m_listexcel.SetItemText(15, 2, "        高压电压、电流和功率(CAN)        ");
	m_listexcel.SetItemText(15, 3, "        高压电压、电流和功率(8710)        ");

	setcfg_tolist(0);

	CFont* pfont1 = m_listexcel.GetFont();
	pfont1->GetLogFont(&logfont);
	logfont.lfHeight = (long)(logfont.lfHeight / 1.2f); //这里可以修改字体的高比例
	logfont.lfWidth = (long)(logfont.lfWidth / 1.2f); //这里可以修改字体的宽比例
	static CFont font1;
	font1.CreateFontIndirect(&logfont);
	m_listexcel.SetFont(&font1);
	font1.Detach();

	AutoAdjustColumnWidth(&m_listexcel);
}

unsigned char GetPID(unsigned char PID)
{
	union LIN_PID Pid_data;
	Pid_data.PID = PID;
	Pid_data.bit.bit7 = ~(Pid_data.bit.bit1 ^ Pid_data.bit.bit3 ^ Pid_data.bit.bit4 ^ Pid_data.bit.bit5);
	Pid_data.bit.bit6 = (Pid_data.bit.bit0 ^ Pid_data.bit.bit1 ^ Pid_data.bit.bit2 ^ Pid_data.bit.bit4);
	return Pid_data.PID;
}

#ifdef _P_CAN_

void CACPSDlg::readMessages()
{
	TPCANTPStatus sts;
	TPCANTPMsg msg;
	TPCANTPTimestamp ts;
	VCI_CAN_OBJ pCANObj[10];
	CString strRecvMsg;
	DWORD ID;
	// We read at least one time the queue looking for messages.
	// If a message is found, we look again trying to find more.
	// If the queue is empty or an error occurr, we get out from
	// the dowhile statement.
	do
	{
		// Reads and process a single ISO-TP message
		sts = m_Bus->Receive(pCANObj, 0, &strRecvMsg, &ID);
		if (sts == PCANTP_ERROR_OK)
		{
			//received message analyz...


		}
	} while (m_Bus->isConnected() && sts == PCANTP_ERROR_OK);
}

//CAN报文接收线程(P-CAN)
UINT CanReadThreadForEvent(void* param)
{
	HANDLE hEvents[2];
	CACPSDlg* dlg = (CACPSDlg*)AfxGetApp()->GetMainWnd();
	hEvents[0] = dlg->m_eventHandleToReadCAN;
	hEvents[1] = dlg->m_eventHandleToStopWaitingForObject;
	TPCANTPStatus sts = PCANTP_ERROR_OK;

	// Sets the handle of the Receive-Event.
	sts = dlg->m_Bus->SetValue(dlg->m_Bus->m_CanParams.PCANParam.m_pctpHandle, &dlg->m_eventHandleToReadCAN);

	DWORD dwWaitResult = -1;
	// Call Thread member function
	do {
		// Waits for Receive-Event
		dwWaitResult = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
		// Event object was signaled
		if (dwWaitResult == WAIT_OBJECT_0)
		{
			// Process Receive-Event
			// in order to interact with the UI (calling the 
			// function readMessages)
			dlg->readMessages();
			ResetEvent(hEvents[0]);
		}
	} while ((dlg->m_stopReadThread == FALSE) && (dwWaitResult != WAIT_OBJECT_0 + 1));

	return IDOK;
}
#endif


//CAN报文接收线程(创新CAN适配器)
UINT CANReceiveThread(void* param)
{
	CACPSDlg* dlg = (CACPSDlg*)AfxGetApp()->GetMainWnd();
	int k = 0;
	CString valuestr;
	VCI_CAN_OBJ pCanObj[200];
	unsigned char pBuff[8];

	int NumValue=0;
	CString str1;
	int num = 0;
	CString str;
	int Len = 0;
	int x = 0, y = 0; 
	DWORD ID;
	CString strID, strRevMsg0, strRevMsg;

	//auto start = std::chrono::high_resolution_clock::now();
	while (1)
	{
		if (dlg->g_mCANDevOK)
		{
			//CAN
#ifdef _COMMAND_PATTERN
			ReceiveCmd recv(dlg->m_Bus);
			Command* p_receive = &recv;
			//发送指令
			dlg->m_exec->SetCmd(p_receive);
			NumValue = dlg->m_exec->Receive(pCanObj, dlg->m_nCanIndex, &strRevMsg0, &ID);
#else
			if (dlg->m_Bus == NULL)
				break;

			memset(pCanObj, 0, sizeof(pCanObj));
			NumValue = dlg->m_Bus->Receive(pCanObj, 0, &strRevMsg0, &ID);
#endif

			if (NumValue == 0)	//PTC离线监控
				dlg->m_ulTicktCount1++;
			else
				dlg->m_ulTicktCount1 = 0;

			//接收信息列表显示
			for (num = 0; num < NumValue; num++)
			{
				strRevMsg = "";
				if (!g_NotRecvAppMsg_BeforeTestFlowStart)
				{
					memcpy(pBuff, pCanObj[num].Data, sizeof(pBuff));
					if (pCanObj[num].ID == ECUFeedback1)
					{
						HVH_ECU_Frm1->HVH_Status = pBuff[0] & 0x07;
						HVH_ECU_Frm1->HVH_HVIL_Status = (pBuff[0] >> 3) & 0x03;
						HVH_ECU_Frm1->HVH_ActiveDis_Status = (pBuff[0] >> 5) & 0x03;
						HVH_ECU_Frm1->HVH_Coolant_In_Temp = pBuff[1] - 50;
						HVH_ECU_Frm1->HVH_Coolant_Out_Temp = pBuff[2] - 50;
						HVH_ECU_Frm1->HVH_HV_Current = pBuff[3] * 0.25;
						HVH_ECU_Frm1->HVH_HV_Voltage = (pBuff[4] | (pBuff[5] & 0x03) << 8);
						HVH_ECU_Frm1->HVH_ActualPower = (pBuff[6] | pBuff[7] << 8);

						x = num;
						for (int j = 0; j < pCanObj[x].DataLen; j++) {
							str.Format("0x%02X ", pCanObj[x].Data[j]);
							strRevMsg += str;
						}
						strID.Format("%X::", pCanObj[num].ID);
						strRevMsg = strID + strRevMsg;

						if (WaitForSingleObject(g_MsgCycleSignaled, 10) == WAIT_OBJECT_0)
						{
							dlg->m_recv_msglst1.emplace_back();
							//auto now = std::chrono::high_resolution_clock::now();
							//auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
							//if (duration >= 10)		//每100毫秒执行一次
							//{
							//	start = now;
							//	dlg->m_recv_msglst1.emplace_back(duration);
							//}
						}
					}
					else if (pCanObj[num].ID == ECUFeedback2)
					{
						HVH_ECU_Frm2->HVH_ProtocolError = pBuff[0] & 0x01;
						HVH_ECU_Frm2->HVH_ComponentProtection = (pBuff[0] >> 1) & 0x03;
						HVH_ECU_Frm2->HVH_WarnHVOutOfRng = (pBuff[0] >> 3) & 0x03;
						HVH_ECU_Frm2->HVH_WarnULoOutOfRang = (pBuff[0] >> 5) & 0x03;
						HVH_ECU_Frm2->HVH_WarnOverheat = (pBuff[1] >> 6) & 0x03;
						HVH_ECU_Frm2->HVH_FailHotspot = pBuff[2] & 0x03;
						HVH_ECU_Frm2->HVH_FailTempSensor = (pBuff[2] >> 6) & 0x03;
						HVH_ECU_Frm2->HVH_FailDriver = pBuff[3] & 0x03;
						HVH_ECU_Frm2->HVH_FailHighCurrent = (pBuff[3] >> 2) & 0x03;
						HVH_ECU_Frm2->HVH_FailMemory_InterCom = (pBuff[3] >> 4) & 0x03;
						HVH_ECU_Frm2->HVH_FailDCDCConverter = (pBuff[3] >> 6) & 0x03;

						y = num;
						for (int k = 0; k < pCanObj[y].DataLen; k++) {
							str1.Format("0x%02X ", pCanObj[y].Data[k]);
							strRevMsg += str1;
						}
						strID.Format("%X::", pCanObj[num].ID);
						strRevMsg = strID + strRevMsg;

						if (WaitForSingleObject(g_MsgCycleSignaled, 10) == WAIT_OBJECT_0)
						{
							dlg->m_recv_msglst2.emplace_back();
							//auto now = std::chrono::high_resolution_clock::now();
							//auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
							//if (duration >= 10)		//每100毫秒执行一次
							//{
							//	start = now;
							//	dlg->m_recv_msglst2.emplace_back(duration);
							//}
						}
					}
				}
				if (pCanObj[num].ID == g_DiagResp)
				{
					dlg->UDS_Respose(pCanObj[num].Data, 0);
				}

				if (strRevMsg != "")
				{
					dlg->DispalyCurrentMsg(strRevMsg);
					strRevMsg == "";
				}
			}

			Sleep(10);

			if (StopFlag == 1)
			{
#ifdef _COMMAND_PATTERN
				delete p_receive;
				p_receive = NULL;
#endif
				return 0;
			}
		}
	}
	return 1;
}

//LIN报文接收线程
UINT LINReceiveThread(void* param)
{
	CACPSDlg* dlg = (CACPSDlg*)AfxGetApp()->GetMainWnd();
	
	int NumValue;
	int x = 0; // record received message pos in receive data array
	int ret = -1;

	CString strID, strRevMsg0, str, strRevMsg;
	LIN_EX_MSG LINOutMsg[10];	
	unsigned char pid;

	while (1)
	{		
		if (dlg->g_LINDevOK)
		{	
			//std::lock_guard<std::mutex> lock(g_mtx);
			//LIN
#ifdef _COMMAND_PATTERN
			ReceiveCmd recv2(dlg->m_Bus);
			Command* p_receive2 = &recv2;
			//发送指令		
			dlg->m_exec->SetCmd(p_receive2);
				
			memset(LINOutMsg, 0, 10);
			pid = GetPID(gSlavePID);
			NumValue = dlg->m_exec->Receive(pid, LINOutMsg);
#else
			memset(LINOutMsg, 0, 10);
			pid = GetPID(gSlavePID);
			NumValue = dlg->m_Bus->Receive(pid, LINOutMsg);
#endif
			for (int i = 0; i < NumValue; i++)
			{
				if (pid == LINOutMsg[i].PID)
				{			
					LINMSG_RECV->byte0 = LINOutMsg[i].Data[0];
					LINMSG_RECV->byte1 = LINOutMsg[i].Data[1];
					LINMSG_RECV->byte2 = LINOutMsg[i].Data[2];
					LINMSG_RECV->byte3 = LINOutMsg[i].Data[3];
					LINMSG_RECV->byte4 = LINOutMsg[i].Data[4];
					LINMSG_RECV->byte5 = LINOutMsg[i].Data[5];
					LINMSG_RECV->byte6 = LINOutMsg[i].Data[6];
					LINMSG_RECV->byte7 = LINOutMsg[i].Data[7];

					x = i;

					for (int j = 0; j < 8/*LINOutMsg[x].DataLen*/; j++) {
						str.Format("0x%02X ", LINOutMsg[x].Data[j]);
						strRevMsg += str;
					}
					strRevMsg = " 26:: " + strRevMsg;
				}
					
				if (strRevMsg != "")
				{				
					dlg->DispalyCurrentMsg(strRevMsg);
					strRevMsg = _T("");
				}
			}


			Sleep(100);
			if (StopFlag == 1)
			{
#ifdef _COMMAND_PATTERN
				delete p_receive2;
				p_receive2 = NULL;
#endif
				return 0;
			}
		}
	}
	return 1;
}

string wchar_tToString(wchar_t* pWideChar)
{
	if (NULL == pWideChar)
		return NULL;
	char* pAnsi = NULL;
	int needBytes = WideCharToMultiByte(CP_ACP, 0, pWideChar, -1, NULL, 0, NULL, NULL);
	if (needBytes > 0)
	{
		pAnsi = new char[needBytes + 1];
		ZeroMemory(pAnsi, needBytes + 1);
		WideCharToMultiByte(CP_ACP, 0, pWideChar, -1, pAnsi, needBytes, NULL, NULL);
	}
	string strValue(pAnsi);
	delete[]pAnsi;

	return strValue;
}

// 用例单功能花费时间统计
// bStart:开始计时
// lExpenditureTime：
// nIntervalFrm:输出时间统计刻度	0:秒	1:毫秒
void  CACPSDlg::Cal_UnitTest_Time(bool bStart, long* lExpenditureTime, int nIntervalFrm)
{
	if (bStart)
	{
		g_starttm = ::GetTickCount();
	}
	else {
		CString valuestr;
		g_endtm = ::GetTickCount();
		valuestr.Format("%dms", g_endtm - g_starttm);
		if(nIntervalFrm == 0)
			g_difftm = atoi(valuestr) / 1000; //秒
		else
			g_difftm = atoi(valuestr); //毫秒

		*lExpenditureTime = g_difftm;
	}
}

//未输入产品序列号
LRESULT CACPSDlg::OnAbnormalTest(WPARAM wParam, LPARAM lParam)
{
	bool nAbnormalTest = (bool)wParam;
	if (nAbnormalTest)
	{
		//停止线程
		StopThreads();
	}
	return S_OK;
}

LRESULT CACPSDlg::OnUpdateData(WPARAM wParam, LPARAM lParam)
{
	int nCount = 0;
	CString strmsg;
	
	wchar_t* pBuf = (wchar_t*)wParam;
	wchar_t* pTime = (wchar_t*)lParam;

	string msg_t = wchar_tToString(pBuf);
	CString strRecvMsg = msg_t.c_str();

	string time_t = wchar_tToString(pTime);
	CString strTime = time_t.c_str();
	CString strOutStr = strTime + _T(" ") + strRecvMsg;

	//output_DEBUG_info(pBuf);
	m_msglst.AddString(strOutStr);

	nCount = m_msglst.GetCount();
	m_msglst.SetCurSel(nCount - 1);

	//写报文日志
	m_can_message_lst.emplace_back(strOutStr);
	//CACPSDlg::_Log(strOutStr);
	//strOutStr = "";

	if (nCount > 100)
		m_msglst.ResetContent();

	UpdateData(false);
	::SysFreeString(pBuf);
	pBuf = NULL;
	::SysFreeString(pTime);
	pTime = NULL;

	return S_OK;
}
//显示发送与接收的报文数据
void CACPSDlg::DispalyCurrentMsg(CString strCurrMsg, bool bRecv, int nChannel)
{
	CString strMsg=_T("");
	if (bRecv)
	{
		if (nChannel == 1)
			strMsg = "CHN1 Rx:" + strCurrMsg;
		else if (nChannel == 2)
			strMsg = "CHN2 Rx:" + strCurrMsg;
	}
	else
	{
		if (nChannel == 1)
			strMsg = "CHN1 Tx:" + strCurrMsg;
		else if (nChannel == 2)
			strMsg = "CHN2 Tx:" + strCurrMsg;
	}

	USES_CONVERSION;
	wchar_t* pWchar = strMsg.AllocSysString();

	CString timestr;
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	// 使用 GetLocalTime(&st); 获取本地时间
	int hour = lt.wHour;
	int minute = lt.wMinute;
	int second = lt.wSecond;
	int milliseconds = lt.wMilliseconds; // 获取毫秒部分
	timestr.Format("%02d:%02d:%02d:%02d", hour, minute, second, milliseconds);
	wchar_t* pTime = timestr.AllocSysString();

	if (!::PostMessage(g_hWnd, WM_UPDATEDATA, (WPARAM)pWchar, (LPARAM)pTime))
	{
		::SysFreeString(pWchar); pWchar = NULL;
		::SysFreeString(pTime);	pTime = NULL;
	}
}

void CACPSDlg::output_debug_info(const char* pStr)
{
	CString strInfo;
	strInfo.Format(pStr);
	OutputDebugString(strInfo);
}

void CACPSDlg::setcfg_tolist(int cfgnum)
{
	CString valuestr, strOK;
	strOK.Format("OK");

	//valuestr.Format("≥%7.2f~≤%7.2f", cfg[cfgnum].ov1, cfg[cfgnum].ov2);
	//valuestr.Format("≥%7.2f~≤%7.2f mA", 200.0f, 230.0f);
	//valuestr.Replace(" ", "");
	valuestr.Format("≤%7.2f mA", 200.0f);
	m_listexcel.SetItemText(0, 1, valuestr);

	//valuestr.Format("≤%7.2f~≥%7.2f", cfg[cfgnum].uv1, cfg[cfgnum].uv2);
	//valuestr.Replace(" ", "");
	m_listexcel.SetItemText(0, 2, strOK);

	//valuestr.Format(">%7.2f~≤%7.2f", cfg[cfgnum].highvol_over_vol, cfg[cfgnum].highvol_over_restore_vol);
	//valuestr.Replace(" ", "");
	m_listexcel.SetItemText(0, 3, strOK);
	
	//valuestr.Format("<%7.2f~≥%7.2f", cfg[cfgnum].highvol_under_vol, cfg[cfgnum].highvol_under_restore_vol);
	//valuestr.Replace(" ", "");
	//valuestr.Format("%x:%x:%x:%x", cfg[0].project_info[0], cfg[0].project_info[1], cfg[0].project_info[2], cfg[0].project_info[3]);
	//valuestr.Replace(" ", "");
	m_listexcel.SetItemText(4, 1, strOK);
	//
	//valuestr.Format("%x:%x:%x:%x", g_software[0], g_software[1], g_software[2], g_software[3]);
	//valuestr.Replace(" ", "");
	m_listexcel.SetItemText(4, 2, strOK);
	m_listexcel.SetItemText(4, 3, strOK);

	//valuestr.Format("≥%7.2f~≤%7.2f", cfg[cfgnum].ptc_overtemp_low, cfg[cfgnum].ptc_overtemp_high);
	//valuestr.Replace(" ", "");
	m_listexcel.SetItemText(8, 1, strOK);
	//valuestr.Format("≥%7.2f~≤%7.2f", cfg[cfgnum].igbt_overtemp_low, cfg[cfgnum].igbt_overtemp_high);
	//valuestr.Replace(" ", "");
	m_listexcel.SetItemText(8, 2, strOK);
	m_listexcel.SetItemText(8, 3, strOK);

	m_listexcel.SetItemText(12, 1, strOK);

	valuestr.Format("%7.2f", cfg[cfgnum].max_ptc_power);
	m_listexcel.SetItemText(12, 2, valuestr);

	valuestr.Format("%7.2f", cfg[cfgnum].max_ptc_power_5Kw);
	m_listexcel.SetItemText(12, 3, valuestr);

	m_listexcel.SetItemText(16, 1, strOK);

}

void CACPSDlg::AutoAdjustColumnWidth(CListCtrl *pListCtrl)
{
	pListCtrl->SetRedraw(FALSE);
	CHeaderCtrl *pHeader = pListCtrl->GetHeaderCtrl();
	int nColumnCount = pHeader->GetItemCount();
	for (int i = 0; i < nColumnCount; i++)
	{
		pListCtrl->SetColumnWidth(i, LVSCW_AUTOSIZE);
		int nColumnWidth = pListCtrl->GetColumnWidth(i);
		pListCtrl->SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
		int nHeaderWidth = pListCtrl->GetColumnWidth(i);
		pListCtrl->SetColumnWidth(i, max(nColumnWidth, nHeaderWidth) + 5);
	}
	pListCtrl->SetRedraw(TRUE);
}

//监测通信丢失
bool CACPSDlg::Monitoring_ComLost()
{
	if (m_ulTicktCount1 > cfg[0].msg_lost_monitor_cycle * 1000 * 10)	//1000:报文周期		10:接收报文线程周期
	{
		m_bLostPtc1 = true;
			
		KillTimer(2);
		KillTimer(3);
		KillTimer(4);
	}
	else if (m_ulTicktCount1 == 0)
	{
		m_bLostPtc1 = false;
	}

	return true;
}

void CACPSDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CString valuestr;
	float volvalue=0.0f, curvalue=0.0f, fPower = 0.0f;
	float fLowVol = 0.0f;
	switch (nIDEvent)
	{
	case 1:
		Monitoring_ComLost();
		break;
	case 2:
		Cycyle_0x3E();	
		break;
	case 3:
		SendCANMsg(ECU_HVH_Frm1->HVH_MaxPower, ECU_HVH_Frm1->HVH_TargetCoolantTemp, ECU_HVH_Frm1->HVH_HeaterEnable, ECU_HVH_Frm1->HVH_ActiveDischarge);		
		break;	
	case 4:
		//高压报文值	can1
		volvalue = HVH_ECU_Frm1->HVH_HV_Voltage;
		curvalue = HVH_ECU_Frm1->HVH_HV_Current;
		fPower = HVH_ECU_Frm1->HVH_ActualPower;	
		valuestr.Format("%3.2f V / %3.2f A / %3.2f W", volvalue, curvalue, fPower);
		m_listexcel.SetItemText(17, 2, valuestr);
		break;
	case 5:
		m_Mediator->lowPower->Get_Dev_Vol(&fLowVol);
		if (fLowVol <= 0.0f)
			m_bLowPowerLost = true;
		else if(fLowVol >= cfg[0].uv1)
			m_bLowPowerLost = false;
		break;
	}

	CDialog::OnTimer(nIDEvent);
}

//高精度发送报文
UINT PowerReqThread(LPVOID param)
{
	CACPSDlg* pDlg = (CACPSDlg*)param;
	int nWaitTime = cfg[0].waitting_time + cfg[0].waitting_time_5Kw + 3;

	//高精度时钟发送功率请求报文
	pDlg->timerFunction(true, nWaitTime);

	return S_OK;
}

//高精度时钟函数任务
float CACPSDlg::timerFunction(bool bStartTimer, int nExpireTime)
{
	bool bSwitchECU = false; //2个PTC交替发送功率请求报文
	int nReadTimes = 0;
	float volvalue = 0.0f, curvalue = 0.0f, power = 0.0f, fRealPower = 0.0f;

	auto start = std::chrono::high_resolution_clock::now();
	auto start2 = std::chrono::high_resolution_clock::now();
	while (bStartTimer)
	{
		auto now = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

		auto now2 = std::chrono::high_resolution_clock::now();
		auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(now2 - start2).count();

		if (duration >= 50/*25*/)		//每50毫秒执行一次
		{
			start = now;

			// 处理定时任务
			//if (!bSwitchECU)
			{
				SendCANMsg(ECU_HVH_Frm1->HVH_MaxPower, ECU_HVH_Frm1->HVH_TargetCoolantTemp, ECU_HVH_Frm1->HVH_HeaterEnable, ECU_HVH_Frm1->HVH_ActiveDischarge);
				//bSwitchECU = true;
			}
		/*	else
			{
				SendCANMsg(ECU_HVH_Frm1->HVH_MaxPower, ECU_HVH_Frm1->HVH_TargetCoolantTemp, ECU_HVH_Frm1->HVH_HeaterEnable, ECU_HVH_Frm1->HVH_ActiveDischarge, 2);
				bSwitchECU = false;
			}			*/
		}

		//超时，结束报文发送
		if (duration2 >= 1000 * nExpireTime) //设定时间nExpireTime 秒
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 减少CPU占用，但仍保持响应性
	}

	return power;
}

//保持诊断会话状态
void CACPSDlg::Cycyle_0x3E()
{
	int i;
	int datanum = 8;
	int newflag = 1;
	int nCount = 0;
	int nWaitTime = 0;
	byte pData[8];
	CString strOutStr;
	CString strMsg, strMT;

	m_nSendFrameType = config[3].PORT;
	m_nSendFrameFormat = config[3].BUND;
	m_nCanIndex = config[3].DATA;
	m_nDevType = config[3].CHECK;

	//int IDnum = 0x7E0;
	//sendbuf->ExternFlag = 0;
	//sendbuf->DataLen = datanum;
	//sendbuf->RemoteFlag = m_nSendFrameFormat;

	memset(pData, 0, sizeof(pData));
	pData[0] = 0x02;
	pData[1] = 0x3E;
	if (!g_3EReq)
	{
		pData[2] = 0x00;
		g_3EReq = true;
	}
	else
	{
		pData[2] = 0x80;
	}
	pData[3] = 0x00;
	pData[4] = 0x00;
	pData[5] = 0x00;
	pData[6] = 0x00;
	pData[7] = 0x00;

	/*
	if (m_nSendFrameFormat == 1)//if remote frame, data area is invalid
	{
		for (i = 0; i < datanum; i++)
		{
			pData[i] = 0;
			strMT.Format("0x%02X ", pData[i]);
			strMsg += strMT;
		}
	}
	else
	{
		for (i = 0; i < datanum; i++)
		{
			sendbuf->Data[i] = pData[i];
			strMT.Format("0x%02X ", pData[i]);
			strMsg += strMT;
		}
	}

	sendbuf->ID = IDnum;
	sendbuf->SendType = 1;
	sendbuf->TimeFlag = 0;
	sendbuf->TimeStamp = 0;

	//CHN1
	flag = VCI_Transmit(m_DevType, 0, 0, sendbuf, 1);//CAN message send on can-0
	if (flag < 1)
	{
		if (flag == -1)
			MessageBox("failed- device not open can-0\n");
		return;
	}
	strOutStr = "0x7E0::" + strMsg;
	DispalyCurrentMsg(strOutStr, false, 1);

	//CHN2
	flag = VCI_Transmit(m_DevType, 0, 1, sendbuf, 1);//CAN message send on can-1
	if (flag < 1)
	{
		if (flag == -1)
			MessageBox("failed- device not open can-1\n");
		return;
	}
	DispalyCurrentMsg(strOutStr, false, 2);
	*/


	CString strmsg;
	SendCmd send(m_Bus);
	Command* p_send = &send;
	//发送指令
	m_exec->SetCmd(p_send);
	//ptc0
	m_Bus->m_CanParams.m_SendFrameType = 0;
	m_exec->Send(g_DiagReq, 0, pData, &strmsg);
	DispalyCurrentMsg(strmsg, false);
	////ptc1
	//m_exec->Send(g_DiagReq, 1, pData, &strmsg);
	//DispalyCurrentMsg(strmsg, false);

	m_Bus->m_CanParams.m_SendFrameType = 1;
}

//写入标志位
bool CACPSDlg::WriteMarkByte(int nWaitTimes)
{
	int NumValue = 0;
	int nWaitTime = 0;
	bool bEEPWrote = false;
	CString strmsg;
	DWORD ID = g_DiagReq;
	byte pMarkByte[8] = {0x03, 0x2E, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 };

	SendCmd send(m_Bus);
	Command* p_send = &send;
	//发送指令
	m_exec->SetCmd(p_send);
	//ptc0
	m_Bus->m_CanParams.m_SendFrameType = 0;
	for (int k = 0; k < 3; k++)
	{
		m_exec->Send(ID, 0, 0, pMarkByte, &strmsg);
		Sleep(10);
	}
	
	DispalyCurrentMsg(strmsg, false);
	////ptc1
	//m_exec->Send(ID, 1, pMarkByte, &strmsg);
	//DispalyCurrentMsg(strmsg, false);
	m_Bus->m_CanParams.m_SendFrameType = 1;

	while (nWaitTime < nWaitTimes)
	{
		if (m_bEEPWrote)
		{
			bEEPWrote = true;
			break;
		}

		Sleep(10);
		nWaitTime++;
	}
	return bEEPWrote;
}

bool CACPSDlg::SendMsgOnCAN(DWORD ID, PHVH_ECU_Frame1 pData)
{
	int NumValue = 0;
	int nWaitTime = 0;
	CString strmsg;
	unsigned char Data[8];
	memset(Data, 0, sizeof(Data));

	SendCmd send(m_Bus);
	Command* p_send = &send;
	//发送指令
	m_exec->SetCmd(p_send);

	//Data[0] = pData->Set_Pwm_Dutycycle_FAN1;
	//Data[1] = pData->Set_Pwm_Dutycycle_FAN2;
	//Data[2] = pData->Set_Pwm_Dutycycle_FAN3;
	//Data[3] = pData->Set_Pwm_Dutycycle_Resvd;
	//Data[4] = pData->Set_Pwm_Dutycycle_DSO;
	//Data[5] = pData->Set_Pwm_Dutycycle_DSOResvd;
	//Data[6] = pData->Set_Pwm_Dutycycle_PUMP;
	//Data[7] = ((pData->Set_Pwm_Freq << 4) | (pData->Set_FAN_Type<<2) | (pData->Set_PUMP_Power<<1) | pData->Set_FAN_Power);
	
	m_exec->Send(ID, 0, Data, &strmsg);
	DispalyCurrentMsg(strmsg, false);

	//m_exec->Send(ID, 1, Data, &strmsg);
	//DispalyCurrentMsg(strmsg, false);
	return true;
}

//重启动(低压重新上电)
void CACPSDlg::ResetLowPower()
{
	//重启
	m_list1.AddString("重启(5s)");
	//低压电源Close
	m_lowPwr->Set_Dev_Vol_Curr(0.0f, 0.0f);
	Sleep(5000);
	//使能低压电源额定低压
	m_lowPwr->Set_Dev_Vol_Curr(cfg[0].lowpre, cfg[0].lowcur);
}

CString GetModuleFilePath() {
	TCHAR szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH);
	CString csPath(szPath);
	int nEnd = csPath.ReverseFind('\\');
	csPath = csPath.Left(nEnd);

	return csPath;
}

void CACPSDlg::_Log(CString strInfo)
{
	CString filepath;
	CString strLogPath;

	CString strTimestr;
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	int year = lt.wYear;
	int month = lt.wMonth;
	int day = lt.wDay;
	int hour = lt.wHour;
	int minute = lt.wMinute;
	int second = lt.wSecond;

	strLogPath = GetModuleFilePath() + _T("\\log");

	if (!IsDirExist(strLogPath))
		CreateDir(strLogPath);

	//strTimestr.Format("%s_%02d_%02d_%02d", g_Serialnumber, hour, minute, second);
	strTimestr.Format("%s", g_Serialnumber);
	filepath.Format("%s\\%s.log", strLogPath, strTimestr);

	CStdioFile file;
	if (file.Open(filepath, CFile::modeWrite | CFile::modeCreate | CFile::modeNoTruncate))
	{
		for (auto tr : m_can_message_lst)
		{
			CString strMsg;
			va_list args;
			va_start(args, tr);
			strMsg.FormatV(tr, args);
			va_end(args);

			CString strFullLog = strMsg + _T("\r\n");
			file.WriteString(strFullLog);
		}
		m_can_message_lst.clear();

		//file.SeekToEnd();
		//// 格式化日志内容
		//CString strMsg;
		//va_list args;		
		//va_start(args, strInfo);
		//strMsg.FormatV(strInfo, args);
		//va_end(args);
		//CString strFullLog = strMsg + _T("\r\n");
		//file.WriteString(strFullLog);
		file.Close();
	}
}

void CACPSDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

void CACPSDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
		CDialog::UpdateWindow(); //UpdateWindow一下
	}
}

HCURSOR CACPSDlg::OnQueryDragIcon()
{
	return (HCURSOR)m_hIcon;
}

void Delay(DWORD dwMilliseconds) //延时函数，防止UI假死
{
	DWORD dwStart = GetTickCount();
	MSG msg;
	while (GetTickCount() - dwStart < dwMilliseconds)
	{
		if (PeekMessage(&msg, (HWND)NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_CLOSE)
				return; // 用户点击了关闭，没有必要在继续等待，程序应该退出
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Sleep(1); //关键，否则CPU占用很大
		}
	}
}

void ZeroObject(unsigned char *buffer, int size)
{
	//int i;
	//for (i = 0; i < size; i++)
	//	buffer[i] = 0;
	memset(buffer, 0, size);
	return;
}

void CACPSDlg::SendMsg(byte power, byte gear, byte temp, byte ac_setptcsts, bool bPowerTest)
{
	if (g_mCANDevOK == 1)
	{
		int i = 0;
		SendCANMsg(ECU_HVH_Frm1->HVH_MaxPower, ECU_HVH_Frm1->HVH_TargetCoolantTemp, ECU_HVH_Frm1->HVH_HeaterEnable, ECU_HVH_Frm1->HVH_ActiveDischarge);
	}
	else
	{
		int i = 0;
		BYTE buf[5] = { 0, 0, 0, 0, 0 };
		buf[0] = ac_setptcsts;
		buf[2] = temp;
		buf[3] = gear;
		buf[4] = power;
		if (!bPowerTest)
		{
			while (i++ < 3)
			{
				Send_Master_Data(buf);
			}
		}
		else
			Send_Master_Data(buf);
	}
}

//产品序列号有效性
bool CACPSDlg::Check_SerialNumber()
{
	bool bRight = false;
	CString strSerialNumber, strSerialNumber2;
	//判断有没有扫描序列号
	m_overedit.GetWindowText(strSerialNumber);
	m_overedit2.GetWindowText(strSerialNumber2);
	if (strSerialNumber.Trim() == "" || strSerialNumber2.Trim() == "")
	{
		AfxMessageBox("*************************\t\n请扫描序列号后再启动测试\t\n*************************");
		::PostMessage(g_hWnd, WM_TEST_ABNORMAL, (WPARAM)true, 0);
		return bRight;
	}
	else
	{
		OffRedLed(portPre);
		OffGreenLed(portPre);
		OffYellowLed(portPre);

		g_Serialnumber = strSerialNumber;
		g_Serialnumber2 = strSerialNumber2;
		GetDlgItem(IDC_STATICSTATE)->SetWindowText("测试中");
		GetDlgItem(IDC_STATICPCBA)->SetWindowText("正常");
		bRight = true;
	}

	return bRight;
}

//闭合继电器J3(接通低压回路)
bool CACPSDlg::PowerJ3Relay(bool bOn)
{
	bool bRight = false;

	if (bOn)	// 闭合J3
	{
		//OnHighPowerOut(m_highPwr);
		////点亮黄色灯，指示测试中
		//if (OnYellowLed(portPre) == false)
		//{
		//	GetDlgItem(IDC_STATICSTATE)->SetWindowText("测试中");
		//	GetDlgItem(IDC_STATICPCBA)->SetWindowText("正常");
		//	m_list1.AddString("黄灯已点亮,测试中....");
		//	bRight = true;
		//}
		//else
		//{
		//	WriteErrMsg("黄灯未点亮,流程退出", this);
		//	WriteTestReport();
		//	::PostMessage(g_hWnd, WM_TEST_ABNORMAL, (WPARAM)true, 0);
		//	return bRight;
		//}
	}
	else//断开
	{
		//OffHighPowerOut(m_highPwr);
	}
	return bRight;
}

//闭合/断开继电器J4(接通/断开高压回路)
bool CACPSDlg::PowerJ4Relay(bool bOn)
{
	bool bRight = false;
	if (bOn)
	{
		//闭合 J4继电器，允许高压供电
		//bRight = !OnHighPowerIn(m_highPwr);//J4
		//上高压
		bRight = !m_Mediator->Set_Dev_Vol_Curr(m_highPwr, 50.0f, cfg[0].run_current);
		SetTimer(4, 1000, NULL);
	}
	else
	{
		//断开J4继电器
		//bRight = !OffHighPowerIn(m_highPwr);//J4
		KillTimer(4);
	}

	return bRight;
}

//低压电源控制
bool CACPSDlg::LowPower_CTRL(LowPowerCtrl lowPwrCtrl, float fVol, float fCurr)
{
	bool bRight = false;
	CString valuestr;

	if (lowPwrCtrl == POWER_ON)		//使能IT6322远程控制
	{
		m_Mediator->lowPower->EnableRemoteCtrl(true);

		//使能低压电源额定低压
		//if (!m_Mediator->Set_Dev_Vol_Curr(m_lowPwr, cfg[0].lowpre, cfg[0].lowcur))
		//{
		//	WriteErrMsg("设置低压电源额定低压失败", this);
		//}
		//else
		m_Mediator->Set_Dev_Vol_Curr(m_lowPwr, cfg[0].lowpre, cfg[0].lowcur);
		{
			bRight = true;
			valuestr.Format("%2.1fV", 24.0f);
			m_list1.AddString("设置低压电源额定低压" + valuestr + " 成功");
		}
		
		//使能通道1输出
		m_Mediator->lowPower->SelectChannelOutput(1, true);
		//使能通道2输出
		//m_Mediator->lowPower->SelectChannelOutput(2, true);
	}
	else if (lowPwrCtrl == POWER_OFF)
	{
		//使能低压电源额定低压
		//if (!m_Mediator->Set_Dev_Vol_Curr(m_lowPwr, 0.0f, 0.0f))
		//{
		//	WriteErrMsg("设置低压电源额定低压失败", this);
		//}
		//else
		m_Mediator->Set_Dev_Vol_Curr(m_lowPwr, 0.0f, 0.0f);
		{
			bRight = true;
			valuestr.Format("%2.1fV", 0.0f);
			m_list1.AddString("设置低压电源额定低压" + valuestr + " 成功");
		}
		if (m_Mediator != NULL)
		{
			//使能通道1输出
			m_Mediator->lowPower->SelectChannelOutput(1, false);
			//使能通道2输出
			//m_Mediator->lowPower->SelectChannelOutput(2, false);

			m_Mediator->lowPower->EnableRemoteCtrl(false);
		}
	}
	else if (lowPwrCtrl == POWER_SET)
	{
		//使能低压电源额定低压
		//if (!m_Mediator->Set_Dev_Vol_Curr(m_lowPwr, fVol, fCurr))
		//{
		//	WriteErrMsg("设置低压电源额定低压失败", this);
		//}
		//else
		m_Mediator->Set_Dev_Vol_Curr(m_lowPwr, fVol, fCurr);
		{
			bRight = true;
			valuestr.Format("%2.1fV, %2.1fA", fVol, fCurr);
			m_list1.AddString("设置低压电源额定低压" + valuestr + " 成功");
		}
	}
	return bRight;
}

//序列号判断
bool CACPSDlg::CheckSeriseNum(CACPSDlg* pDlg)
{
	CString strSerialNumber, strSerialNumber2;
	pDlg->m_overedit.GetWindowText(strSerialNumber);
	//pDlg->m_overedit2.GetWindowText(strSerialNumber2);
	if (strSerialNumber.Trim() == "" /*|| strSerialNumber2.Trim() == ""*/)
	{
		AfxMessageBox("*************************\t\n请扫描序列号后再启动测试\t\n*************************");
		::PostMessage(g_hWnd, WM_TEST_ABNORMAL, (WPARAM)true, 0);
		return false;
	}
	else
	{
		//pDlg->OffRedLed(portPre);
		//pDlg->OffGreenLed(portPre);
		//pDlg->OffYellowLed(portPre);
		pDlg->KillTimer(1);

		g_Serialnumber = strSerialNumber;
		g_Serialnumber2 = strSerialNumber2;
		pcba.serise_num1 = strSerialNumber;
		pcba.serise_num2 = strSerialNumber2;
		pDlg->GetDlgItem(IDC_STATICSTATE)->SetWindowText("测试中");
		pDlg->GetDlgItem(IDC_STATICPCBA)->SetWindowText("正常");
	}
	return true;
}

bool CACPSDlg::WriteMarkByte(byte* pData, int nWaitTimes)
{
	int nWaitTime = 0;
	bool bRight = false;
	DWORD nID = g_DiagReq;
	CString strmsg;
	SendCmd send(m_Bus);
	Command* p_send = &send;
	//发送指令
	m_exec->SetCmd(p_send);

	m_Bus->m_CanParams.m_SendFrameType = 0;
	//ptc0
	m_exec->Send(nID, 0, pData, &strmsg);
	DispalyCurrentMsg(strmsg, false);
	m_Bus->m_CanParams.m_SendFrameType = 1;

	while (nWaitTime < nWaitTimes)
	{
		if ((m_bEEPWrote || m_bSoftwareVerVerified))
		{
			bRight = true;
			break;
		}

		if (m_bClearDTC1)
		{
			bRight = true;
			break;
		}

		Sleep(10);
		nWaitTime++;
	}
	return bRight;
}

//点亮黄灯
bool CACPSDlg::YellowLed_On(CACPSDlg* pDlg)
{
	bool bRight = false;
	// 闭合J3
	pDlg->OnHighPowerOut(portPre);
	//点亮黄色灯，指示测试中
	if (pDlg->OnYellowLed(portPre) == false)
	{
		pDlg->GetDlgItem(IDC_STATICSTATE)->SetWindowText("测试中");
		pDlg->GetDlgItem(IDC_STATICPCBA)->SetWindowText("正常");
		pDlg->m_list1.AddString("黄灯已点亮,测试中....");
		bRight = true;
	}
	else
	{
		pDlg->m_bErrMsg |= pDlg->WriteErrMsg("黄灯未点亮,流程退出", pDlg);
		bRight = false;
	}
	return bRight;
}

//清除故障码
bool CACPSDlg::ClearDTC(CACPSDlg* pDlg)
{
	bool bRight = false;
	byte MarkByte[8] = { 0x04, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00 };

	pDlg->m_list1.AddString("清除故障码(<10s)");
	pDlg->m_list1.SetCurSel(pDlg->m_list1.GetCount() - 1);
	if (!pDlg->WriteMarkByte(MarkByte, 1000))
	{
		pDlg->m_list1.AddString("清除故障码失敗！");
		pDlg->GetDlgItem(IDC_BTNSTOP)->EnableWindow(TRUE);
	}
	else
	{
		//复位"清除故障码"状态
		pDlg->m_bClearDTC1 = false;
		pDlg->m_bClearDTC2 = false;
		pDlg->m_list1.AddString("清除故障码成功！");
		bRight = true;
	}
	return bRight;
}

//禁止APP报文
int CACPSDlg::UDS_DisableAppMsg()
{
	int nRight = 3;
	u8 reqBytes[8]{ 0x03,0x28,0x01,0x01,0x00,0x00,0x00,0x00 };

	ResetUDSRespMark();
	m_UDS1REQ2801 = true;
	if (!CaliPowerReq(reqBytes, 0, 5))
		return nRight = 1;
	else
		nRight = 0;

	//m_UDS2REQ2801 = true;
	//if (!CaliPowerReq(reqBytes, 1, 5))
	//	return nRight = 2;
	//else
	//	nRight = 0;

	return nRight;
}

//使能APP报文
int CACPSDlg::UDS_EnableAppMsg()
{
	int nRight = 3;
	u8 reqBytes[8] = { 0x03,0x28,0x00,0x01,0x00,0x00,0x00,0x00 };

	ResetUDSRespMark();

	m_UDS1REQ2800 = true;
	if (!CaliPowerReq(reqBytes, 0, 5))
		return nRight = 1;
	else
		nRight = 0;

	//m_UDS2REQ2800 = true;
	//if (!CaliPowerReq(reqBytes, 1, 5))
	//	return nRight = 2;
	//else
	//	nRight = 0;
	return nRight;
}

//UDS 0x10 03 request
int CACPSDlg::UDS_0x1003Request()
{
	int nRight = 3;
	u8 reqBytes[8] = { 0x02, 0x10, 0x03, 0x00, 0x00 , 0x00 , 0x00 ,0x00 };
	ResetUDSRespMark();

	//entry extend session of UDS
	m_UDS1REQ1003 = true;
	if (!CaliPowerReq(reqBytes, 0, 15))
		return nRight = 1;
	else
		nRight = 0;

	//m_UDS2REQ1003 = true;
	//if (!CaliPowerReq(reqBytes, 1, 5))
	//	return nRight = 2;
	//else
	//	nRight = 0;

	return nRight;
}

//UDS 0x27 01 request seed
int CACPSDlg::UDS_0x2701Request()
{
	int nRight = 3;
	u8 reqBytes[8] = { 0x02, 0x27, 0x01, 0x00,0x00, 0x00, 0x00, 0x00 };
	ResetUDSRespMark();

	//request seed
	m_UDS1REQ2701 = true;
	if (!CaliPowerReq(reqBytes, 0, 15))
		return nRight = 1;
	else
		nRight = 0;

	//m_UDS2REQ2701 = true;
	//if (!CaliPowerReq(reqBytes, 1, 5))
	//	return nRight = 2;
	//else
	//	nRight = 0;
	return nRight;
}

//UDS 0x27 02 send key
int CACPSDlg::UDS_0x2702Request()
{
	int nRight = 3;

	//send key to ptc1
	memset(g_resp_key1, 0, sizeof(g_resp_key1));
	methord::UDS_Seed2Key(g_resp_seed1, g_resp_key1);
	u8 reqBytes1[8] = { 0x06, 0x27, 0x02, g_resp_key1[0], g_resp_key1[1], g_resp_key1[2], g_resp_key1[3], 0x00 };

	ResetUDSRespMark();
	m_UDS_KEYSEND1 = true;
	if (!CaliPowerReq(reqBytes1, 0, 15))
		return nRight = 1;
	else
		nRight = 0;

	////send key to ptc2
	//memset(g_resp_key2, 0, sizeof(g_resp_key2));
	//methord::UDS_Seed2Key(g_resp_seed2, g_resp_key2);
	//u8 reqBytes2[8] = { 0x06, 0x27, 0x02, g_resp_key2[0], g_resp_key2[1],  g_resp_key2[2], g_resp_key2[3], 0x00 };

	//ResetUDSRespMark();
	//m_UDS_KEYSEND2 = true;
	//if (!CaliPowerReq(reqBytes2, 1, 5))
	//	return nRight = 2;
	//else
	//	nRight = 0;

	return nRight;
}

//UDS访问0x31服务前流程
int CACPSDlg::UDS_RequestBefore0x31Svr()
{
	int nRight = 3;

	//enter extend session
	nRight = UDS_0x1003Request();
	if (nRight != 0)
		return nRight;

#ifdef _SVR_0x28
	//disable app message
	UDS_DisableAppMsg();
#endif
	//request seed
	nRight = UDS_0x2701Request();
	if (nRight != 0)
		return nRight;

	//send key
	nRight = UDS_0x2702Request();
	if (nRight != 0)
		return nRight;

	return nRight;
}

//功率校准请求
//pData: 请求报文指针
//nPTCIndex:CAN通道(被测试PTC编号 0:第一个PTC； 1:第二个PTC; 2:给2个PTC同时发送请求)
//nWaitTimes：等待响应时间
bool CACPSDlg::CaliPowerReq(byte* pData, int nPTCIndex, int nWaitTimes)
{
	/*
	int i;
	int flag;
	int datanum = 8;
	int IDnum = 0x7E0;
	int newflag = 1;
	int nCount = 0;
	bool bEEPWrote = false;

	m_nSendFrameType = config[3].PORT;
	m_nSendFrameFormat = config[3].BUND;
	m_nCanIndex = config[3].DATA;
	m_nDevType = config[3].CHECK;
	VCI_CAN_OBJ sendbuf[1];
	CString strMsg, strMT;
	CString strOutStr;
	int nWaitTime = 0;

	sendbuf->ExternFlag = 0;
	sendbuf->DataLen = datanum;
	sendbuf->RemoteFlag = m_nSendFrameFormat;

	if (m_nSendFrameFormat == 1)//if remote frame, data area is invalid
	{
		for (i = 0; i < datanum; i++)
		{
			pData[i] = 0;
			strMT.Format("0x%02X ", pData[i]);
			strMsg += strMT;
		}
	}
	else
	{
		for (i = 0; i < datanum; i++)
		{
			sendbuf->Data[i] = pData[i];
			strMT.Format("0x%02X ", pData[i]);
			strMsg += strMT;
		}
	}

	sendbuf->ID = IDnum;
	sendbuf->SendType = 1;
	sendbuf->TimeFlag = 0;
	sendbuf->TimeStamp = 0;

	if ((m_nCanIndex == 1) && (m_DevType != VCI_USBCAN2))
	{
		MessageBox("the device only support CAN index 0 on VCI_USBCAN2");
		m_nCanIndex = 0;
	}
	//调用动态链接库发送函数
	if (nPTCIndex == 0)		//ptc1
	{
		flag = VCI_Transmit(m_DevType, 0, 0, sendbuf, 1);//CAN message send on can-0
		if (flag < 1)
		{
			if (flag == -1)
				MessageBox("failed- device not open can-0\n");
			return bEEPWrote;
		}
		strOutStr = "0x7E0::" + strMsg;
		DispalyCurrentMsg(strOutStr, false, 1);
	}
	else if (nPTCIndex == 1)		//ptc2
	{
		flag = VCI_Transmit(m_DevType, 0, 1, sendbuf, 1);//CAN message send on can-1
		if (flag < 1)
		{
			if (flag == -1)
				MessageBox("failed- device not open can-1\n");
			return bEEPWrote;
		}
		strOutStr = "0x7E0::" + strMsg;
		DispalyCurrentMsg(strOutStr, false, 2);
	}
	else if (nPTCIndex == 2)		//ptc1 && ptc2
	{
		//CHN1
		flag = VCI_Transmit(m_DevType, 0, 0, sendbuf, 1);//CAN message send on can-0
		if (flag < 1)
		{
			if (flag == -1)
				MessageBox("failed- device not open can-0\n");
			return bEEPWrote;
		}
		strOutStr = "0x7E0::" + strMsg;
		DispalyCurrentMsg(strOutStr, false, 1);

		//CHN2
		flag = VCI_Transmit(m_DevType, 0, 1, sendbuf, 1);//CAN message send on can-1
		if (flag < 1)
		{
			if (flag == -1)
				MessageBox("failed- device not open can-1\n");
			return bEEPWrote;
		}
		strOutStr = "0x7E0::" + strMsg;
		DispalyCurrentMsg(strOutStr, false, 2);
	}
	*/

	bool bRight = false;
	int nWaitTime = 0;
	DWORD nID = g_DiagReq;
	CString strmsg;
	SendCmd send(m_Bus);
	Command* p_send = &send;
	//发送指令
	m_exec->SetCmd(p_send);

	m_Bus->m_CanParams.m_SendFrameType = 0;//diagnostic send standard can frame
	if (nPTCIndex == 0) {
		//ptc0
		m_exec->Send(nID, 0, 0, pData, &strmsg);
		DispalyCurrentMsg(strmsg, false);
	}
	else if (nPTCIndex == 1) {
		//ptc1
		m_exec->Send(nID, 1, 0, pData, &strmsg);
		DispalyCurrentMsg(strmsg, false);
	}
	else if (nPTCIndex == 2)
	{
		//ptc0
		m_exec->Send(nID, 0, 0, pData, &strmsg);
		DispalyCurrentMsg(strmsg, false);
		//ptc1
		m_exec->Send(nID, 1, 0, pData, &strmsg);
		DispalyCurrentMsg(strmsg, false);
	}
	m_Bus->m_CanParams.m_SendFrameType = 1;//restore frame type to extend after dianostic request sent

	//
	//if (WaitForSingleObject(g_DiagRespSignaled, 500) == WAIT_OBJECT_0)
	//{
	//	ResetEvent(g_DiagRespSignaled);
	//	bRight = true;
	//}

	// old ways not fixed each boolean variabels
		
		
	while (nWaitTime < nWaitTimes)
	{
		if (m_UDS1RESP1003)	//0x10 03 response on ptc1
		{
			m_UDS1REQ1003 = false;
			m_UDS1RESP1003 = false;
			bRight = true;
			break;
		}
		else if (m_UDS2RESP1003)	//0x10 03 response on ptc2
		{
			m_UDS2REQ1003 = false;
			m_UDS2RESP1003 = false;
			bRight = true;
			break;
		}
		else if (m_UDS1RESP2801)	//0x28 01 01 response on ptc1
		{
			m_UDS1REQ2801 = false;
			m_UDS1RESP2801 = false;
			bRight = true;
			break;
		}
		else if (m_UDS2RESP2801)	//0x28 01 01 response on ptc2
		{
			m_UDS2REQ2801 = false;
			m_UDS2RESP2801 = false;
			bRight = true;
			break;
		}
		else if (m_UDS1RESP2800)	//0x28 00 01 response on ptc1
		{
			m_UDS1REQ2800 = false;
			m_UDS1RESP2800 = false;
			bRight = true;
			break;
		}
		else if (m_UDS2RESP2800)	//0x28 00 01 response on ptc2
		{
			m_UDS2REQ2800 = false;
			m_UDS2RESP2800 = false;
			bRight = true;
			break;
		}
		else if (m_UDS1RESP2701)	//0x27 02 response on ptc1
		{
			m_UDS1REQ2701 = false;
			m_UDS1RESP2701 = false;
			bRight = true;
			break;
		}
		else if (m_UDS2RESP2701)	//0x27 02 response on ptc2
		{
			m_UDS2REQ2701 = false;
			m_UDS2RESP2701 = false;
			bRight = true;
			break;
		}
		else if ((m_UDS_KEYRESP1))		//key response on ptc1
		{
			m_UDS_KEYSEND1 = false;
			m_UDS_KEYRESP1 = false;
			bRight = true;
			break;
		}
		else if (m_UDS_KEYRESP2)		//key response on ptc2
		{
			m_UDS_KEYSEND2 = false;
			m_UDS_KEYRESP2 = false;
			bRight = true;
			break;
		}
		else if (m_UDSRESP1 )
		{
			m_UDSREQ1 = false;
			m_UDSRESP1 = false;
			bRight = true;
			break;
		}
		else if (m_UDSRESP2)
		{
			m_UDSREQ2 = false;
			m_UDSRESP2 = false;
			bRight = true;
			break;
		}
		else if (m_bEndCaliPowerOK1 )			//&& m_bEndCaliPowerOK2))
		{
			m_bStartCaliPowerOK1 = false;
			m_bEndCaliPowerOK1 = false;

			//m_bStartCaliPowerOK2 = false;	
			//m_bEndCaliPowerOK2 = false;
			bRight = true;
			break;
		}
		else if (m_bEndCaliPower100vOK1)		// && m_bEndCaliPower100vOK2))
		{
			m_bStartCaliPower100vOK1 = false;
			m_bEndCaliPower100vOK1 = false;

			//m_bStartCaliPower100vOK2 = false;
			//m_bEndCaliPower100vOK2 = false;
			bRight = true;
			break;
		}
		else if (m_bEndCaliPower400vOK1)		//&& m_bEndCaliPower400vOK2))
		{
			m_bStartCaliPower400vOK1 = false;
			m_bEndCaliPower400vOK1 = false;

			//m_bStartCaliPower400vOK2 = false;
			//m_bEndCaliPower400vOK2 = false;
			bRight = true;
			break;
		}
		else if (m_bEndKuOK1)
		{
			m_bStartKuOK1 = false;
			m_bEndKuOK1 = false;
			bRight = true;
			break;
		}
		else if (m_bEndKuOK2)
		{
			m_bStartKuOK2 = false;
			m_bEndKuOK2 = false;
			bRight = true;
			break;
		}
		else if (m_bEnd50KuOK1)		// && m_bEnd50KuOK2)
		{
			m_bStart50KuOK1 = false;
			m_bEnd50KuOK1 = false;

			//m_bStart50KuOK2 = false;
			//m_bEnd50KuOK2 = false;
			bRight = true;
			break;
		}
		else if (m_bEndCaliPower11OK1)		//&& m_bEndCaliPower11OK2)
		{
			m_bStartCaliPower11OK1 = false;
			m_bEndCaliPower11OK1 = false;

			//m_bStartCaliPower11OK2 = false;
			//m_bEndCaliPower11OK2 = false;
			bRight = true;
			break;
		}
		else if (m_bEnd100KuOK1)
		{
			m_bStart100KuOK1 = false;
			m_bEnd100KuOK1 = false;
			bRight = true;
			break;
		}
		else if (m_bEnd100KuOK2)
		{
			m_bStart100KuOK2 = false;
			m_bEnd100KuOK2 = false;
			bRight = true;
			break;
		}
		else if (m_bEndGetADCValue1 )		//&& m_bEndGetADCValue2)
		{
			m_bStartGetADCValue1 = false;
			m_bEndGetADCValue1 = false;

			//m_bStartGetADCValue2 = false;
			//m_bEndGetADCValue2 = false;
			bRight = true;
			break;
		}
		else if (m_bEndGetADCValue3)
		{
			m_bStartGetADCValue3 = false;

			m_bEndGetADCValue3 = false;
			bRight = true;
			break;
		}
		else if (m_bEndGetADCValue4)
		{
			m_bStartGetADCValue4 = false;
			m_bEndGetADCValue4 = false;
			bRight = true;
			break;
		}
		else if (m_bEndReadCaliedVolOK1)	// && m_bEndReadCaliedVolOK2)
		{
			m_bReadCaliedVolOK1 = false;
			m_bEndReadCaliedVolOK1 = false;

			//m_bReadCaliedVolOK2 = false;
			//m_bEndReadCaliedVolOK2 = false;
			bRight = true;
			break;
		}
		else if (m_bEndReadCaliedCurrOK1 )	//&& m_bEndReadCaliedCurrOK2)
		{
			m_bReadCaliedCurrOK1 = false;
			m_bEndReadCaliedCurrOK1 = false;

			//m_bReadCaliedCurrOK2 = false;
			//m_bEndReadCaliedCurrOK2 = false;
			bRight = true;
			break;
		}
		else if (m_bEndCaliPowerExist1 )		//&& m_bEndCaliPowerExist2)
		{
			m_bStartCaliPowerExist1 = false;
			m_bEndCaliPowerExist1 = false;

			//m_bStartCaliPowerExist2 = false;
			//m_bEndCaliPowerExist2 = false;
			bRight = true;
			break;
		}

		Sleep(10);
		nWaitTime++;
	}
	
	return bRight;
}

//清空标志位
void CACPSDlg::ResetUDSRespMark()
{
	//for step 6
	m_bSplit_ReqADCCounterFail = true;

	//request mark variabes reset
	m_UDSREQ1 = false;	 //UDS诊断请求1
	m_UDSREQ2 = false;  //UDS诊断请求2
	m_UDS1REQ1003 = false;
	m_UDS2REQ1003 = false; // 0x10 03请求
	m_UDS1REQ2701 = false;
	m_UDS2REQ2701 = false; // 0x27 01请求

	m_UDS1REQ2801 = false;
	m_UDS2REQ2801 = false;
	m_UDS1REQ2800 = false;
	m_UDS2REQ2800 = false;

	m_UDS1RESP2801 = false;
	m_UDS2RESP2801 = false;
	m_UDS1RESP2800 = false;
	m_UDS2RESP2800 = false;

	m_UDS_KEYSEND1 = false;
	m_UDS_KEYSEND2 = false; //UDS诊断安全访问请求

	m_bStartCaliPowerOK1 = false;
	m_bStartCaliPowerOK2 = false; //功率校准开始	//step1
	m_bStartCaliPower100vOK1 = false;
	m_bStartCaliPower100vOK2 = false; //100v功率校准开始		//step2
	m_bStartCaliPower400vOK1 = false;
	m_bStartCaliPower400vOK2 = false; //400v功率校准开始		//step3
	m_bStartKuOK1 = false;
	m_bStartKuOK2 = false; //斜率计算开始		//step4
	m_bEndGetADCValue1 = false;
	m_bStart50KuOK1 = false;
	m_bStart50KuOK2 = false; //占空比为50%开始		//step5
	m_bStartGetADCValue1 = false;
	m_bStartGetADCValue2 = false; //检测高压电源电流值开始	//step6
	m_bStart100KuOK1 = false;
	m_bStart100KuOK2 = false; //占空比为100%开始		//step7
	m_bStartGetADCValue3 = false;
	m_bStartGetADCValue4 = false; //检测高压电源电流值开始	//step8
	m_bStartCaliPower11OK1 = false;
	m_bStartCaliPower11OK2 = false; //确认复位成功后，通过31服务请求进入校正确认开始	//step11
	m_bReadCaliedVolOK1 = false;
	m_bReadCaliedVolOK2 = false; //通过31服务读取对应校正后的电压值	//step14
	m_bReadCaliedCurrOK1 = false;
	m_bReadCaliedCurrOK2 = false;// 通过31服务读取校正后的电流值  //step15
	m_bStartCaliPowerExist1 = false;
	m_bStartCaliPowerExist2 = false; //功率校准退出开始	//step17

	//response mark variabes reset
	m_UDS1RESP1003 = false;
	m_UDS2RESP1003 = false;
	m_UDS1RESP2701 = false;
	m_UDS2RESP2701 = false;
	m_UDS_KEYRESP1 = false;
	m_UDS_KEYRESP2 = false;
	//m_UDS_KEYRESP1_2 = false;
	//m_UDS_KEYRESP2_2 = false;

	m_UDSRESP1 = false;
	m_UDSRESP2 = false;
	m_bEndCaliPowerOK1 = false;
	m_bEndCaliPowerOK2 = false;
	m_bEndCaliPower100vOK1 = false;
	m_bEndCaliPower100vOK2 = false;
	m_bEndCaliPower400vOK1 = false;
	m_bEndCaliPower400vOK2 = false;
	m_bEndKuOK1 = false;
	m_bEndKuOK2 = false;
	m_bEnd50KuOK1 = false;
	m_bEnd50KuOK2 = false;
	m_bStartGetADCValue1 = false;
	m_bStartGetADCValue2 = false;
	m_bEnd100KuOK1 = false;
	m_bEnd100KuOK2 = false;
	m_bEndGetADCValue3 = false;
	m_bEndGetADCValue4 = false;

	m_bEndReadCaliedVolOK1 = false;
	m_bEndReadCaliedVolOK2 = false;
	m_bEndReadCaliedCurrOK1 = false;
	m_bEndReadCaliedCurrOK2 = false;

	m_bEndCaliPowerExist1 = false;
	m_bEndCaliPowerExist2 = false;

	m_bEndCaliPower11OK1 = false;
}

//普通报文发送打开/关闭
void CACPSDlg::SendAppMsg_OnTimer(int nStep)
{
	int nPower = 2;
	byte limit = 0;
	if (nStep == 0)
	{
		SetTimer(1, 1000, NULL);
		Sleep(cfg[0].msg_lost_monitor_cycle * 1000);
	}
	else if (nStep == 1)
	{
		limit = 0x1F * nPower;
		ECU_HVH_Frm1->HVH_MaxPower = limit;
		ECU_HVH_Frm1->HVH_TargetCoolantTemp = 0x50;
		ECU_HVH_Frm1->HVH_HeaterEnable = 0x01;
		ECU_HVH_Frm1->HVH_ActiveDischarge = 0x00;
		SetTimer(3, 100, NULL);
	}
	else if (nStep == 3)
	{
		SetTimer(5, 1000, NULL);
		Sleep(2000);
	}
	else
	{
		KillTimer(3);
	}
}

//执行测试用例
UINT PreThread(LPVOID param)
{
	CACPSDlg* pDlg = (CACPSDlg*)param;
	pDlg->GetDlgItem(IDC_BTNSTART)->EnableWindow(FALSE);
	bool bStepTestResult = false;

	//判断有没有扫描序列号
	bStepTestResult = pDlg->CheckSeriseNum(pDlg);
	if (!bStepTestResult)
	{
		StopFlag = 1;
		pDlg->_Log();
		return 1;
	}

	//闭合J3继电器
	pDlg->PowerJ3Relay();

	//将低压电压调到预设值(24v/12v, 1.2A)
	pDlg->LowPower_CTRL(POWER_ON, cfg[0].lowpre, cfg[0].lowcur);

	//诊断标志位写入
	bStepTestResult &= pDlg->Invoke_WriteMarkByte();
	if (!bStepTestResult)
	{
		if (pDlg->m_bLostPtc1 || pDlg->m_bDevLost1 ||
			pDlg->m_bDevLost2 || pDlg->m_bLowPowerLost)
			return 1;
	}
	pDlg->m_bErrMsg &= bStepTestResult;

	//重启低压电源
	pDlg->ResetLowPower();

	//低压电流
	bStepTestResult &= pDlg->lowPower_CurrentTest();
	if (!bStepTestResult)
	{
		if (pDlg->m_bLostPtc1 || pDlg->m_bDevLost1 ||
			pDlg->m_bDevLost2 || pDlg->m_bLowPowerLost)
			return 1;
	}
	pDlg->m_bErrMsg &= bStepTestResult;

	//CAN/LIN通信(报文周期检查)
	bStepTestResult &= pDlg->Check_MsgCycle();
	if (!bStepTestResult)
	{
		if (pDlg->m_bLostPtc1 || pDlg->m_bDevLost1 ||
			pDlg->m_bDevLost2 || pDlg->m_bLowPowerLost)
			return 1;
	}
	pDlg->m_bErrMsg &= bStepTestResult;

	//打开通信( OnTimer报文发送)
	pDlg->SendAppMsg_OnTimer(1);
	//低压电源监控(低压掉电)
	pDlg->SendAppMsg_OnTimer(3);
	//打开通信监控(ptc通信丢失)
	pDlg->SendAppMsg_OnTimer(0);

	//项目信息
	bStepTestResult &= pDlg->Check_ProjectInfo();
	if (!bStepTestResult)
	{
		if (pDlg->m_bLostPtc1 || pDlg->m_bDevLost1 ||
			pDlg->m_bDevLost2 || pDlg->m_bLowPowerLost)
			return 1;
	}
	pDlg->m_bErrMsg &= bStepTestResult;

	//软件版本
	bStepTestResult &=pDlg->Check_SoftwareVerion();
	if (!bStepTestResult)
	{
		if (pDlg->m_bLostPtc1 || pDlg->m_bDevLost1 ||
			pDlg->m_bDevLost2 || pDlg->m_bLowPowerLost)
			return 1;
	}
	pDlg->m_bErrMsg &= bStepTestResult;

	//低压电压诊断
	bStepTestResult &= pDlg->Check_LowPowerCircuit();
	if (!bStepTestResult)
	{
		if (pDlg->m_bLostPtc1 || pDlg->m_bDevLost1 ||
			pDlg->m_bDevLost2 || pDlg->m_bLowPowerLost)
			return 1;
	}
	pDlg->m_bErrMsg &= bStepTestResult;

	//检测IGBT传感器电路
	bStepTestResult &= pDlg->Check_IGBTCircuit();
	if (!bStepTestResult)
	{
		if (pDlg->m_bLostPtc1 || pDlg->m_bDevLost1 ||
			pDlg->m_bDevLost2 || pDlg->m_bLowPowerLost)
			return 1;
	}
	pDlg->m_bErrMsg &= bStepTestResult;

	//检测PTC传感器电路
	bStepTestResult &= pDlg->Check_PTCCircuit();
	if (!bStepTestResult)
	{
		if (pDlg->m_bLostPtc1 || pDlg->m_bDevLost1 ||
			pDlg->m_bDevLost2 || pDlg->m_bLowPowerLost)
			return 1;
	}
	pDlg->m_bErrMsg &= bStepTestResult;

	////检测PCB温度传感器电路   --- 不测试
	////pDlg->m_bErrMsg &= pDlg->Check_PCBTempCircuit();

	//闭合J4继电器
	pDlg->PowerJ4Relay();

#ifdef _CALI_POWERSLOPE
	//关闭通信监控(ptc断路)
	pDlg->SendAppMsg_OnTimer(2);

	//功率校准
	bStepTestResult &= pDlg->ptc_Power_Calibration(pDlg);
	if (!bStepTestResult)
	{
		if (pDlg->m_bLostPtc1 || pDlg->m_bDevLost1 ||
			pDlg->m_bDevLost2 || pDlg->m_bLowPowerLost)
			return 1;
	}
	pDlg->m_bErrMsg &= bStepTestResult;
#endif

	//打开通信监控(ptc通信丢失监控和周期发送报文)
	pDlg->SendAppMsg_OnTimer(0);
	pDlg->SendAppMsg_OnTimer(1);
	//高压电压诊断
	pDlg->m_bErrMsg &= pDlg->HighVol_Diagnostic();
	pDlg->SendAppMsg_OnTimer(4);//关闭OnTimer报文发送

	//功率控制
	pSendMsgThread->ResumeThread();
	bStepTestResult &= pDlg->Run_Selected_PowerMode(pDlg);
	if (!bStepTestResult)
	{
		if (pDlg->m_bLostPtc1 || pDlg->m_bDevLost1 ||
			pDlg->m_bDevLost2 || pDlg->m_bLowPowerLost)
			return 1;
	}
	pDlg->m_bErrMsg &= bStepTestResult;
	pSendMsgThread->SuspendThread();

	//运行结束
	pDlg->TestEndProcess();

	return 0;
}

//运行结束
void CACPSDlg::TestEndProcess()
{
	//写总线报文
	_Log();

	//关闭电源
	ResetPower();
	m_overedit.SetFocus();

	//保存测试报告
	WriteTestReport();

	m_PauseTestTH.EnableWindow(FALSE);
	m_StartTestTH.EnableWindow(TRUE);
}

void Char2Byte(char szCh, u8* pCh)
{
	u8 VerIndex = 0;

	if (szCh >= 'A' && szCh <= 'F')
		VerIndex = static_cast<u8>(szCh - 'A');
	else if (szCh >= 'a' && szCh <= 'f')
		VerIndex = szCh - ('a' - 'A');
	else
		VerIndex = szCh;

	*pCh = VerIndex;
}

//项目信息验证
bool CACPSDlg::VerifyProjectInfo(int nPTC)
{
	bool bResult = false;
	int nVerIndex = 0;
	CString strProjectInfo, strProjectInfo1, strProjectInfo2;
	bool bProjectInfoOK1 = false, bProjectInfoOK2 = false;
	u8 cPrj1, cPrj2, cPrj3, cPrj4;

	//ver1
	Char2Byte(cfg[0].project_info[0], &cPrj1);

	//ver2
	Char2Byte(cfg[0].project_info[1], &cPrj2);

	//ver3
	Char2Byte(cfg[0].project_info[2], &cPrj3);

	//ver4
	Char2Byte(cfg[0].project_info[3], &cPrj4);


	strProjectInfo.Format("项目信息 -> %X:%X:%X:%X", cPrj1, cPrj2, cPrj3, cPrj4);
	m_list1.AddString(strProjectInfo);

	
	strProjectInfo1.Format("读取项目信息1 -> %X:%X:%X:%X", g_resp_prjinfo1[0], g_resp_prjinfo1[1], g_resp_prjinfo1[2], g_resp_prjinfo1[3]);
	m_list1.AddString(strProjectInfo1);

	if (g_resp_prjinfo1[0] == cPrj1 && g_resp_prjinfo1[1] == cPrj2 &&
		g_resp_prjinfo1[2] == cPrj3 && g_resp_prjinfo1[3] == cPrj4)
	{
		bProjectInfoOK1 = true;
		return bProjectInfoOK1;
	}
	else
		m_list1.AddString("ptc1 读取项目信息和设置的项目信息不一致");

	return bResult;
}

//软件版本号验证
bool CACPSDlg::VerifySoftwareVer(int nPTC)
{
	bool bResult = false;
	int nVerIndex = 0;
	u8 cVer1, cVer2, cVer3, cVer4;

	//ver1
	nVerIndex = atoi(g_software[0]);
	cVer1 = nVerIndex + 'A';

	//ver2
	nVerIndex = atoi(g_software[1]);
	cVer2 = nVerIndex /*+ '0'*/;

	//ver3
	nVerIndex = atoi(g_software[2]);
	cVer3 = nVerIndex + 'A';

	//ver4
	nVerIndex = atoi(g_software[3]);
	cVer4 = nVerIndex /*+ '0'*/;

	CString strSoftwareVer, strSoftwareVer1, strSoftwareVer2;
	bool bSoftwareVerOK1 = false, bSoftwareVerOK2 = false;

	strSoftwareVer.Format("选择软件版本 -> %X:%X:%X:%X", cVer1, cVer2, cVer3, cVer4);
	m_list1.AddString(strSoftwareVer);

	if (nPTC == 0) //PTC1
	{
		strSoftwareVer1.Format("读取软件版本1 -> %x:%x:%x:%x", g_resp_software[0], g_resp_software[1], g_resp_software[2], g_resp_software[3]);
		m_list1.AddString(strSoftwareVer1);

		if (g_resp_software[0] == cVer1 && g_resp_software[1] == cVer2 &&
			g_resp_software[2] == cVer3 && g_resp_software[3] == cVer4)
		{
			bSoftwareVerOK1 = true;
			return bSoftwareVerOK1;
		}
		else
			m_list1.AddString("ptc1 读取软件版本和选择的版本不一致");
	}

	return bResult;
}

//低压电流(检测低压电路情况)
bool CACPSDlg::lowPower_CurrentTest()
{
	bool bRight = false;
	CString strOutstr, valuestr;
	float fCurr_1 = 0.0f, fCurr=0.0f;
	float fCurr_2 = 0.0f;

	Sleep(2000);
	m_Mediator->lowPower->Get_Dev_Curr(&fCurr_1, &fCurr_2);
	if(fCurr_1 < 1)
		fCurr = fCurr_1 * 1000.0f;
	
	if (m_bLostPtc1 || m_bDevLost1 || 
		m_bDevLost2 || m_bLowPowerLost)
	{
		strOutstr.Format("%s", "NOK");
		m_listexcel.SetItemText(1, 1, strOutstr);
		if (m_bLostPtc1)
		{
			WriteErrMsg("chn1 通信丢失", this, 1, 1);
		}
		else if (m_bDevLost1)
		{
			WriteErrMsg("chn1 低压通信Rs232断开", this, 1, 1);
		}
		else if (m_bDevLost2)
		{
			WriteErrMsg("chn1 高压通信Rs485断开", this, 1, 1);
		}
		else if (m_bLowPowerLost)
		{
			WriteErrMsg("chn1 低压掉电", this, 1, 1);
		}
		WriteTestReport();
		_Log();
		StopFlag = 1;
		KillTimer(3);
		pcba.lowpwr_curr1 = false;
		bRight = false;
	}
	else
	{
		if (fCurr < 200.0f && fCurr > 0.0f)
		//if(abs(fCurr - 200.0f) > 30.0f)
		{
			valuestr.Format("chn1 低压电流::%2.2fmA", fCurr);
			m_list1.AddString(valuestr);

			strOutstr.Format("chn1 低压电流正常");
			m_list1.AddString(strOutstr);
			m_listexcel.SetItemText(1, 1, "OK");
			pcba.lowpwr_curr1 = true;
			bRight = true;
		}
		else
		{
			valuestr.Format("chn1 低压电流::%2.2fmA", fCurr);
			m_list1.AddString(valuestr);

			m_listexcel.SetItemText(1, 1, "NOK");
			WriteErrMsg("chn1 低压电流异常", this, 1, 1);
			pcba.lowpwr_curr1 = false;
		}
	}

	return bRight;
}

//调用写标志位
bool CACPSDlg::Invoke_WriteMarkByte()
{
	bool bRight = false;
	CString strOutstr;
	m_list1.AddString("写标志位(10s)");
	m_list1.SetCurSel(m_list1.GetCount() - 1);

	if (m_bLostPtc1 || m_bDevLost1 ||
		m_bDevLost2 || m_bLowPowerLost)
	{
		strOutstr.Format("%s", "NOK");
		m_listexcel.SetItemText(1, 2, strOutstr);

		if (m_bLostPtc1)
		{
			WriteErrMsg("chn1 通信丢失", this, 1, 2);
		}
		else if (m_bDevLost1)
		{
			WriteErrMsg("chn1 低压通信Rs232断开", this, 1, 2);
		}
		else if (m_bDevLost2)
		{
			WriteErrMsg("chn1 高压通信Rs485断开", this, 1, 2);
		}
		else if (m_bLowPowerLost)
		{
			WriteErrMsg("chn1 低压掉电", this, 1, 2);
		}

		pcba.eep_mark_byte1 = false;
		bRight = false;
		WriteTestReport();
		StopFlag = 1;
		_Log();
	}
	else
	{
		if (!WriteMarkByte(1000))
		{
			if (m_bEEPWrote == false) /*&& m_bEEPWrote2 == false)*/
			{
				m_list1.AddString("写标志位失败,测试终止");
				GetDlgItem(IDC_BTNSTOP)->EnableWindow(TRUE);

				WriteTestReport();
				LowPower_CTRL(POWER_OFF);
				m_Mediator->Set_Dev_Vol_Curr(m_highPwr, 0.0f, 0.0f);
				_Log();
				StopFlag = 1;

				pcba.eep_mark_byte1 = false;
				pcba.eep_mark_byte2 = false;
				return bRight;
			}
		}
		else
		{
			if (m_bEEPWrote)
			{
				strOutstr.Format("chn1 写标志位成功");
				m_list1.AddString(strOutstr);
				m_listexcel.SetItemText(1, 2, "OK");
				pcba.eep_mark_byte1 = true;
				bRight = true;
			}
			else
			{
				m_listexcel.SetItemText(1, 2, "NOK");
				WriteErrMsg("chn1 写标志位失败", this, 1, 2);

				WriteTestReport();
				LowPower_CTRL(POWER_OFF);
				OffHighPowerOut(portPre);
				pcba.eep_mark_byte1 = false;
				pcba.eep_mark_byte2 = true;
			}
		}
	}

	return bRight;
}

//诊断响应处理
bool CACPSDlg::UDS_Respose(byte* pData, int nCanIdx)
{
	int j = 0, k = 0;
	CString strMT = "", strMsg = "", strOutStr = "", strSoftwareVer = "";
	CString str1, str2, strOpt;

	//CAN1
	if (nCanIdx == 0)
	{
		if (pData[1] == 0x7F)  //NRC
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
		}

		//mark byte received(0x03 0x6E 0x00 0x01)
		if (pData[1] == 0x6E && pData[2] == 0x00 && pData[3] == 0x01)
		{
			m_bEEPWrote = true;
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);	
		}
		
		//project info 0xF170
		if (pData[2] == 0xF1 && pData[3] == 0x70)
		{
			int j = 0;
			memset(g_resp_prjinfo1, 0, sizeof(g_resp_prjinfo1));
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				if (k > 3 && k <= 7)
				{
					g_resp_prjinfo1[j++] = pData[k];
				}
				strMsg += strMT;
			}
			//比较软件版本号(接收到的版本号 == 选择的版本号 ?)
			if (VerifyProjectInfo(0))
				m_bReadProjectInfo1 = true;

			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
		}

		//software version 0xF189
		if (pData[2] == 0xF1 && pData[3] == 0x89)
		{
			int j = 0;
			memset(g_resp_software, 0, sizeof(g_resp_software));
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				if (k > 3 && k <= 7)
				{
					g_resp_software[j++] = pData[k];
				}

				strMsg += strMT;
			}
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);

			//比较软件版本号(接收到的版本号 == 选择的版本号 ?)
			if (VerifySoftwareVer(0))
				m_bSoftwareVerVerified = true;
		}

		//0x06 50 03 00 32 02 BC FF
		if (pData[1] == 0x50 && pData[2] == 0x03 && m_UDS1REQ1003)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}	
			m_UDS1RESP1003 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);			
			
		}

		//02 68 01 FF FF FF FF FF
		if (pData[1] == 0x68 && pData[2] == 0x01&& m_UDS1REQ2801)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			m_UDS1RESP2801 = true;
		}

		//06 50 02 00 32 02 BC FF
		if (pData[1] == 0x50 && pData[2] == 0x02&& m_UDSREQ1)
		{
			m_UDSRESP1 = true;
		}

		//06 67 01 E3 8A 1E E5 FF
		if (pData[1] == 0x67 && pData[2] == 0x01 && m_UDS1REQ2701)
		{
			memset(g_resp_seed1, 0, sizeof(g_resp_seed1));
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;

				if (k > 2 && k < 7)
					g_resp_seed1[j++] = pData[k];
			}
			m_UDS1RESP2701 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);		
			
		}

		//02 67 02 FF FF FF FF FF
		if (pData[1] == 0x67 && pData[2] == 0x02 && m_UDS_KEYSEND1)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			m_UDS_KEYRESP1 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);		
			
		}

		//02 68 00 FF FF FF FF FF
		if (pData[1] == 0x68 && pData[2] == 0x00&& m_UDS1REQ2800)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			m_UDS1RESP2800 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);				
		}

		//02 51 01 FF FF FF FF FF
		if (pData[1] == 0x51 && pData[2] == 0x01 && m_bStartCaliPower11OK1)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			m_bEndCaliPower11OK1 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);						
		}

		//1、功率校准开始
		if (pData[1] == 0x71 && pData[2] == 0x01
			&& pData[3] == 0x01 && pData[4] == 0x00
			&& m_bStartCaliPowerOK1)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			m_bEndCaliPowerOK1 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			
		}

		//2、	上位机设定电压为300V，稳定后通过31服务读取对应高压电压ADC采集counter值（0 - 4095）
		if (pData[1] == 0x71 && pData[2] == 0x01 &&
			pData[3] == 0x01 && pData[4] == 0x01
			&&m_bStartCaliPower100vOK1)
		{
			int j = 0;
			memset(g_resp_power_cali1, 0, sizeof(g_resp_power_cali1));
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				if (k > 4 && k < 7)
				{
					g_resp_power_cali1[j++] = pData[k];
				}

				strMsg += strMT;
			}
			m_bEndCaliPower100vOK1 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);	
		}

		//3、	上位机设定电压为600V，稳定后通过31服务读取对应高压电压ADC采集counter值（0 - 4095）
		if (pData[1] == 0x71 && pData[2] == 0x01 &&
			pData[3] == 0x01 && pData[4] == 0x01
			&&m_bStartCaliPower400vOK1)
		{
			int j = 0;
			memset(g_resp_power_cali1, 0, sizeof(g_resp_power_cali1));
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				if (k > 4 && k < 7)
				{
					g_resp_power_cali1[j++] = pData[k];
				}
				strMsg += strMT;
			}
			m_bEndCaliPower400vOK1 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			
		}

		//4、	上位机利用两次的电压设定值和读取到的counter值计算出斜率ku，通过31服务发送给下位机存储到EEPROM；下位机拿到数据存储成功后才可返回正响应
		if (pData[1] == 0x71 && pData[2] == 0x01 &&
			pData[3] == 0x01 && pData[4] == 0x02
			&&m_bStartKuOK1)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			m_bEndKuOK1 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			
		}

		//5、	上位机通过31服务设定占空比为50%
		if (pData[1] == 0x71 && pData[2] == 0x01 &&
			pData[3] == 0x01 && pData[4] == 0x03
			&&m_bStart50KuOK1)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			m_bEnd50KuOK1 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			
		}

		//6、	上位机得到设置占空比请求的正响应，延时等待稳定后，检测高压电源电流值
		if (pData[1] == 0x71 && pData[2] == 0x01 &&
			pData[3] == 0x01 && pData[4] == 0x04
		    &&m_bStartGetADCValue1)
		{
			memset(g_resp_power_cali1, 0, sizeof(g_resp_power_cali1));
			for (int k = 0; k < 8; k++)
			{
				if (k > 4 && k < 7)
				{
					g_resp_power_cali1[j++] = pData[k];

					strMT.Format("0x%02X ", pData[k]);
					strMsg += strMT;
				}
			}
			m_bEndGetADCValue1 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			
		}

		//7、	上位机通过31服务设定占空比为100%
		if (pData[1] == 0x71 && pData[2] == 0x01 &&
			pData[3] == 0x01 && pData[4] == 0x03
			&&m_bStart100KuOK1)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			m_bEnd100KuOK1 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			
		}

		//8、确认复位成功后，通过31服务请求进入校正确认
		if (pData[1] == 0x71 && pData[2] == 0x01 &&
			pData[3] == 0x01 && pData[4] == 0x04
			&& m_bStartGetADCValue3)
		{
			memset(g_resp_power_cali1, 0, sizeof(g_resp_power_cali1));
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				if (k > 4 && k < 7)
				{
					g_resp_power_cali1[j++] = pData[k];
				}
				strMsg += strMT;

			}
			m_bEndGetADCValue3 = true;
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
		}

		//9、上位机利用两次检测到的高压电源电流值和读取到的counter值计算出斜率ki，通过31服务发送给下位机存储到EEPROM
		if (pData[1] == 0x71 && pData[2] == 0x01 &&
			pData[3] == 0x01 && pData[4] == 0x05
			&& m_bStart100KuOK1)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			m_bEnd100KuOK1 = true;
		}

		//11、确认复位成功后，通过31服务请求进入校正确认
		if (pData[1] == 0x71 && pData[2] == 0x01
			&& pData[3] == 0x01 && pData[4] == 0x00
			&& m_bStartCaliPower11OK1)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			m_bEndCaliPower11OK1 = true;
		}

		//13、上位机通过31服务设定占空比为90%
		if (pData[1] == 0x71 && pData[2] == 0x01 &&
			pData[3] == 0x01 && pData[4] == 0x03
			&& m_bStartGetADCValue1)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			m_bEndGetADCValue1 = true;
		}

		//14、上位机得到设置占空比请求的正响应，延时等地稳定后，通过31服务读取对应高压电压值，此时读取到的值为校正后的电压值
		if (pData[1] == 0x71 && pData[2] == 0x01
			&& pData[3] == 0x01 && pData[4] == 0x06
			&& m_bReadCaliedVolOK1)
		{
			memset(g_resp_power_cali1, 0, sizeof(g_resp_power_cali1));
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
				if (k > 4 && k < 7)
				{
					g_resp_power_cali1[j++] = pData[k];
				}
			}
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			m_bEndReadCaliedVolOK1 = true;
		}

		//15、 上位机通过31服务读取对应高压电流值，此时读取到的值为校正后的电流值
		if (pData[1] == 0x71 && pData[2] == 0x01
			&& pData[3] == 0x01 && pData[4] == 0x07
			&& m_bReadCaliedCurrOK1)
		{
			memset(g_resp_power_cali1, 0, sizeof(g_resp_power_cali1));
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
				if (k > 4 && k < 7)
				{
					g_resp_power_cali1[j++] = pData[k];
				}
			}
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			m_bEndReadCaliedCurrOK1 = true;
		}

		//17、退出功率校准模式
		if (pData[1] == 0x71 && pData[2] == 0x01 &&
			pData[3] == 0x01 && pData[4] == 0x08 &&m_bStartCaliPowerExist1)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			m_bEndCaliPowerExist1 = true;
		}

		//故障码清除
		if (pData[0] == 0x01 && pData[1] == 0x54)
		{
			for (int k = 0; k < 8; k++)
			{
				strMT.Format("0x%02X ", pData[k]);
				strMsg += strMT;
			}
			strOutStr = "0x7E8::" + strMsg;
			DispalyCurrentMsg(strOutStr, true, 1);
			m_bClearDTC1 = true;
		}

		//SetEvent(g_DiagRespSignaled);
	}

	return true;
}

//界面提示信息(功率校准)
void CACPSDlg::ShowStepMessage(const char* pMsg, u16 ADCCounter, double resVal, u8* pKu)
{
	CString strRespValue;
	if (ADCCounter != 0 && resVal == 0.0f)
		strRespValue.Format("%s:: %d", pMsg, ADCCounter);
	else if (ADCCounter == 0 && resVal != 0.0f)
		strRespValue.Format("%s:: %3.3f", pMsg, resVal);
	else if (pKu[0] != 0x00 || pKu[1] != 0x00)
		strRespValue.Format("%s:: pKu[0]->0x%X, pKu[1]->0x%X", pMsg, pKu[0], pKu[1]);
	else
		strRespValue.Format("%s", pMsg);

	m_list1.AddString(strRespValue);
	m_list1.SetCurSel(m_list1.GetCount() - 1);
}

//等待功率计到达目标电压
bool CACPSDlg::Waitfor_8710ArrivedDstVol(float fDstVol, int nWaitTime, float* fVol1, float* fVol2, bool bSend0x3E)
{
	bool bRight = false;
	int nTimes = 0;
	CString strStepMsg;
	float fCurr1 = 0.0f, fCurr2 = 0.0f;
	byte pKu[2];
	memset(pKu, 0, sizeof(pKu));

	if(bSend0x3E)
		SetTimer(2, 2000, NULL);

	while (true)
	{
		m_highPwr->Get_8710Dev_Vol_Curr(fVol1, &fCurr1);
		strStepMsg.Format("检测高压电源电压值ptc1_1->%3.2f 电流值1->%3.2f", *fVol1, fCurr1);
		ShowStepMessage(strStepMsg, 0, 0, pKu);

		//m_highPwr->Get_8710Dev_Vol_Curr(fVol1, &fCurr1, 2);		
		//strStepMsg.Format("检测高压电源电压值ptc2_2->%3.2f 电流值2->%3.2f", *fVol2, fCurr2);
		//ShowStepMessage(strStepMsg, 0, 0, pKu);

		if (*fVol1 >= fDstVol /*&& *fVol2 >= fDstVol*/)
		{
			bRight = true;
			Sleep(3000);
			break;
		}
		else if (*fVol1 < fDstVol /*&& *fVol2 == 0.0f*/)
		{
			bRight = true;
			break;
		}
		else if (*fVol1 == 0.0f /*&& *fVol2 == 0.0f*/)
		{
			bRight = true;
			break;
		}

		if (nTimes > nWaitTime)
		{
			break;
		}

		nTimes += 1000;
		Sleep(1000);
	}
	if (bSend0x3E)
	{
		KillTimer(2);
		Sleep(20);
	}

	return bRight;
}

//等待功率计到达目标电流
bool CACPSDlg::Waitfor_8710ArrivedDstCurr(float fDstCurr, int nWaitTime, float* fCurr, int nCHN, bool bSend0x3E)
{
	int nTimes = 0;
	CString strStepMsg;
	float fVol = 0.0f;
	float fTotalCaptureCurr = 0.0f;
	byte pKu[2];
	memset(pKu, 0, sizeof(pKu));

	if (bSend0x3E)
		SetTimer(2, 2000, NULL);

	while (true)
	{
		m_highPwr->Get_8710Dev_Vol_Curr(&fVol, fCurr /*,nCHN*/);
		strStepMsg.Format("通道::%d 检测高压电源电流值:: %3.2f 电流值1->%3.2f", nCHN, fVol, *fCurr);
		ShowStepMessage(strStepMsg, 0, 0, pKu);

		m_DutyCycle_Current.emplace_back(*fCurr);

		if (*fCurr >= fDstCurr)
		{
			if(m_DutyCycle_Current.size()> 10)
				m_DutyCycle_Current.erase(m_DutyCycle_Current.begin(), m_DutyCycle_Current.begin() + 10); //剔除刚开始不稳定电流值
			for (auto it = m_DutyCycle_Current.begin(); it != m_DutyCycle_Current.end(); ++it)
			{
				fTotalCaptureCurr += *it;
			}

			*fCurr = fTotalCaptureCurr / m_DutyCycle_Current.size();
			m_DutyCycle_Current.clear();

			break;
		}
		else if (*fCurr == 0.0f)
		{
			break;
		}

		if (nTimes > nWaitTime)
		{
			break;
		}

		nTimes += 1000;
		Sleep(1000);
	}
	if (bSend0x3E)
	{
		KillTimer(2);
		Sleep(20);
	}

	return true;
}

//校准期间监控设备异常
bool CACPSDlg::Moniter_DevLostInCali(int* nPTC)
{
	bool bRight = true;
	if (m_bLostPtc1)
	{
		WriteErrMsg("chn1 通信丢失", this, 13, 1);
		bRight = false;
	}
	else if (m_bDevLost1)
	{
		WriteErrMsg("chn1 低压通信Rs232断开", this, 13, 1);
		bRight = false;
	}
	else if (m_bDevLost2)
	{
		WriteErrMsg("chn1 高压通信Rs485断开", this, 13, 1);
		bRight = false;
	}
	else if (m_bLowPowerLost)
	{
		WriteErrMsg("chn1 低压掉电", this, 13, 1);
		bRight = false;
	}

	if (!bRight)
	{
		*nPTC = 1;
		StopFlag = 1;
		AdjustHighPowerVol(25.0f, 5.0f);
	}
	else
	{
		bRight = true;
		*nPTC = 0;
	}

	return bRight;
}

//功率斜率计算
bool CACPSDlg::Cali_PowerSlope(int* nPTC)
{
	CString strStepMsg;
	CString strOutStr, strMsg;
	bool bRight = false, bStep1 = false, bStep2 = false;
	bool bSlope50CheckOK1 = false, bSlope50CheckOK2 = false;
	bool bSlope100CheckOK1 = false, bSlope100CheckOK2 = false;
	bool bGetRightResp = false;
	int nRaw = 0, nWaitOnQueryCurr = 0;
	float fVoltage = 0.0f, fCurrent = 25.0f, fGetVol = 0.0f, fGetCurr1 = 0.0f, fGetCurr2 = 0.0f, fGetCurr2_1 = 0.0f, fGetCurr2_2 = 0.0f;
	float f300Vol1 = 0.0f, f300Curr1 = 0.0f, f600Vol1 = 0.0f, f600Curr1 = 0.0f;
	float f300Vol2 = 0.0f, f300Curr2 = 0.0f, f600Vol2 = 0.0f, f600Curr2 = 0.0f;
	float volvalue = 0.0f, curvalue = 0.0f;
	float _power_cali_vol1 = 0.0f, _power_cali_vol2 = 0.0f, _power_cali_curr1 = 0.0f, _power_cali_curr2 = 0.0f;
	float fVolError1 = 0.0f, fVolError2 = 0.0f, fCurrError1 = 0.0f, fCurrError2 = 0.0f, fPowerError1 = 0.0f, fPowerError2 = 0.0f;
	double fKu = 0.0f, fKi = 0.0f, fKu16_4 = 0.0f, fKi16_4 = 0.0f, fKu16_5 = 0.5f, fKu16_7 = 1.0f, fKu16_8 = 0.9f;
	double dError50_1, dError100_1; //, dError50_2, dError100_2;
	float fDstCurr1 = 5.0f, fDstCurr2 = 5.0f;
	FixedPoint fp;
	u8 Ku8 = 0;
	u16 ADCCounter1_1 = 0, ADCCounter1_2 = 0, ADCCounter2_1 = 0, ADCCounter2_2 = 0;
	u16 ADCCounter3_1 = 0, ADCCounter3_2 = 0, ADCCounter4_1, ADCCounter4_2 = 0;
	u16 u1 = 300, u2 = 600, u3 = 20;
	u8 U1[2], U2[2], pKu[2], pKi[2];
	u8 MarkByte[8], MarkByte2[8];

	memset(MarkByte, 0, sizeof(MarkByte));
	memset(MarkByte2, 0, sizeof(MarkByte2));
	memset(U1, 0, sizeof(U1));
	memset(U2, 0, sizeof(U2));
	memset(pKu, 0, sizeof(pKu));
	memset(pKi, 0, sizeof(pKi));

	//高压电源设置成30v
	fVoltage = 30.0f;
	fCurrent = 5.0f;
	ShowStepMessage("高压电源电压设置为：vol->30v, curr->5A", 0, 0, pKu);
	m_Mediator->Set_Dev_Vol_Curr(m_highPwr, fVoltage, fCurrent);
	//Sleep(1000);

	//UDS服务0x10 03, 0x27 01, 0x27 02
	int nSecurityAccess = UDS_RequestBefore0x31Svr();
	if (nSecurityAccess == 3)
	{
		ShowStepMessage("UDS安全访问未通过，功率校准流程退出", 0, 0, pKu);
		return bRight;
	}
	else if (nSecurityAccess == 1)
	{
		ShowStepMessage("PTC A UDS安全访问未通过", 0, 0, pKu);
		return bRight;
	}
	/*else if (nSecurityAccess == 2)
	{
		ShowStepMessage("PTC B UDS安全访问未通过", 0, 0, pKu);
		return bRight;
	}*/

	//req:: 04 31 01 01 00 FF FF FF
	//resp::04 71 01 01 00 FF FF FF
	MarkByte[0] = 0x04;
	MarkByte[1] = 0x31;
	MarkByte[2] = 0x01;
	MarkByte[3] = 0x01;
	MarkByte[4] = 0x00;
	MarkByte[5] = 0xFF;
	MarkByte[6] = 0xFF;
	MarkByte[7] = 0xFF;

	ResetUDSRespMark();
	m_bStartCaliPowerOK1 = true;
	if (CaliPowerReq(MarkByte, 0, 1000))	//1
	{
#ifdef _per30DutyCycle
		_per10DutyCycleReq(0.3f);
#endif

		fVoltage = u1;
		fCurrent = 25.0f;
		ShowStepMessage("高压电源电压设置为：vol->300v, curr->25A", 0, 0, pKu);
		m_Mediator->Set_Dev_Vol_Curr(m_highPwr, fVoltage, fCurrent);
		Sleep(2000);

		if (!Moniter_DevLostInCali(nPTC))
			return bRight;

		//req::04 31 01 01 01 FF FF FF
		//resp::06 71 01 01 01 XX XX FF
		MarkByte[0] = 0x04;
		MarkByte[1] = 0x31;
		MarkByte[2] = 0x01;
		MarkByte[3] = 0x01;
		MarkByte[4] = 0x01;
		MarkByte[5] = 0xFF;
		MarkByte[6] = 0xFF;
		MarkByte[7] = 0xFF;
		ResetUDSRespMark();
		m_bStartCaliPower100vOK1 = true;
		if (CaliPowerReq(MarkByte, 0, 1000))	//2
		{
			SetTimer(2, 2000, NULL);
			Sleep(CALI_POWRER_WAIT_TIME0);
			KillTimer(2);
			Sleep(20);

			if (!Moniter_DevLostInCali(nPTC))
				return bRight;

			m_highPwr->Get_8710Dev_Vol_Curr(&f300Vol1, &f300Curr1);

			//ptc1
			ADCCounter1_1 = g_resp_power_cali1[0] << 8 | g_resp_power_cali1[1];
			ShowStepMessage("ADC counter1_1", ADCCounter1_1, 0, pKu);

			fVoltage = u2;
			fCurrent = 25.0f;
			ShowStepMessage("高压电源电压设置为：vol->600v, curr->25A", 0, 0, pKu);
			m_Mediator->Set_Dev_Vol_Curr(m_highPwr, fVoltage, fCurrent);

			SetTimer(2, 1000, NULL);
			Sleep(CALI_POWRER_WAIT_TIME1);
			KillTimer(2);
			Sleep(20);
			//Waitfor_8710ArrivedDstVol(u2, CALI_POWRER_WAIT_TIME1, &f600Vol1, &f600Vol2, &fDstCurr1, &fDstCurr2);
			if (!Moniter_DevLostInCali(nPTC))
				return bRight;

			MarkByte[0] = 0x04;
			MarkByte[1] = 0x31;
			MarkByte[2] = 0x01;
			MarkByte[3] = 0x01;
			MarkByte[4] = 0x01;
			MarkByte[5] = 0xFF;
			MarkByte[6] = 0xFF;
			MarkByte[7] = 0xFF;

			ResetUDSRespMark();
			m_bStartCaliPower400vOK1 = true;
			if (CaliPowerReq(MarkByte, 0, 1000))  //3
			{
				SetTimer(2, 2000, NULL);
				Sleep(CALI_POWRER_WAIT_TIME0);
				KillTimer(2);
				Sleep(20);
				
				if (!Moniter_DevLostInCali(nPTC))
					return bRight;

				m_highPwr->Get_8710Dev_Vol_Curr(&f600Vol1, &f600Curr1);

				//以下处理 ptc1
				ADCCounter2_1 = g_resp_power_cali1[0] << 8 | g_resp_power_cali1[1];
				ShowStepMessage("ADC counter2_1", ADCCounter2_1, 0, pKu);

				//斜率ku=（U2-U1）/（C2-C1）
				fKu16_4 = ((double)(f600Vol1 - f300Vol1) / (double)(ADCCounter2_1 - ADCCounter1_1));
				ShowStepMessage("斜率Ku1", 0, fKu16_4, pKu);
				pcba.fCalied_50Slope1 = (float)fKu16_4;

				//斜率结果判断1
				dError50_1 = abs(fKu16_4 - cfg[0].slope_base1);
				if (dError50_1 < cfg[0].slope_error1)
					bSlope50CheckOK1 = true;
				else
					bSlope50CheckOK1 = false;

				//4.斜率转换为定点数	
				nRaw = fp.makeRaw(fKu16_4, 16);
				methord::u16toPu8(nRaw, pKu);
				ShowStepMessage("斜率请求Ku1", 0, 0.0f, pKu);

				/*************************************************debug log for record variabel value***********************************************************************/
#ifdef _DEBUG			
				memset(U1, 0, sizeof(U1));
				strOutStr.Format("f300Vol1::%3.2f", f300Vol1);
				ShowStepMessage(strOutStr, 0, 0.0f, U1);

				strOutStr.Format("f600Vol1::%3.2f", f600Vol1);
				ShowStepMessage(strOutStr, 0, 0.0f, U1);

				strMsg.Format("ADCCounter1_1:%d", ADCCounter1_1);
				ShowStepMessage(strMsg, 0, 0.0f, U1);

				strMsg.Format("ADCCounter2_1:%d", ADCCounter2_1);
				ShowStepMessage(strMsg, 0, 0.0f, U1);
#endif
				/*************************************************debug log for record variabel value***********************************************************************/

				MarkByte[0] = 0x06;
				MarkByte[1] = 0x31;
				MarkByte[2] = 0x01;
				MarkByte[3] = 0x01;
				MarkByte[4] = 0x02;
				MarkByte[5] = pKu[0];
				MarkByte[6] = pKu[1];
				MarkByte[7] = 0xFF;

				ResetUDSRespMark();
				m_bStartKuOK1 = true;
				m_bStartKuOK2 = true;
				bStep1 = CaliPowerReq(MarkByte, 0, 1000);	//4
				if(bStep1)
				{
					bStep1 = false;
					bStep2 = false;
					//5、	通过31服务设定占空比为50%
					nRaw = fp.makeRaw(fKu16_5, 15);
					memset(pKu, 0, sizeof(pKu));
					methord::u16toPu8(nRaw, pKu);
					ShowStepMessage("占空比为50%斜率请求Ku1", 0, 0.0f, pKu);

					MarkByte[0] = 0x06;
					MarkByte[1] = 0x31;
					MarkByte[2] = 0x01;
					MarkByte[3] = 0x01;
					MarkByte[4] = 0x03;
					MarkByte[5] = pKu[0];
					MarkByte[6] = pKu[1];
					MarkByte[7] = 0xFF;

					ResetUDSRespMark();
					m_bStart50KuOK1 = true;
					if (CaliPowerReq(MarkByte, 0, 1000))	//5
					{
						//6、	上位机得到设置占空比请求的正响应，延时等待稳定后，检测高压电源电流值
						//保持诊断会话在线
						SetTimer(2, 2000, NULL);
						Sleep(cfg[0].per50_wait_time * 1000);
						KillTimer(2);
						Sleep(20);
						
						if (!Moniter_DevLostInCali(nPTC))
							return bRight;

						m_highPwr->Get_8710Dev_Vol_Curr(&fGetVol, &fGetCurr1);
						strStepMsg.Format("检测高压电源电压值ptc1_1->%3.2f 电流值1->%3.2f", fGetVol, fGetCurr1);
						ShowStepMessage(strStepMsg, 0, 0, pKu);

						//nWaitOnQueryCurr = cfg[0].per50_wait_time * 1000;
						//Waitfor_8710ArrivedDstCurr(2.0f, nWaitOnQueryCurr, &fDstCurr1, 1, true);

						//req:: 04 31 01 01 00 FF FF FF
						//resp::04 71 01 01 00 FF FF FF
						MarkByte[0] = 0x04;
						MarkByte[1] = 0x31;
						MarkByte[2] = 0x01;
						MarkByte[3] = 0x01;
						MarkByte[4] = 0x04;
						MarkByte[5] = 0xFF;
						MarkByte[6] = 0xFF;
						MarkByte[7] = 0xFF;

						ResetUDSRespMark();
						m_bStartGetADCValue1 = true;
						m_bStartGetADCValue2 = true;
						bGetRightResp = CaliPowerReq(MarkByte, 0, 1000);
						if (bGetRightResp)	//6
						{
							ADCCounter3_1 = g_resp_power_cali1[0] << 8 | g_resp_power_cali1[1];
							ShowStepMessage("ADC counter3_1", ADCCounter3_1, 0, pKu);

							//6、	上位机通过31服务设定占空比为100%
							nRaw = fp.makeRaw(fKu16_7, 15);
							memset(pKu, 0, sizeof(pKu));
							methord::u16toPu8(nRaw, pKu);
							ShowStepMessage("占空比为100%斜率请求Ku1", 0, 0.0f, pKu);

							MarkByte[0] = 0x06;
							MarkByte[1] = 0x31;
							MarkByte[2] = 0x01;
							MarkByte[3] = 0x01;
							MarkByte[4] = 0x03;
							MarkByte[5] = pKu[0];
							MarkByte[6] = pKu[1];
							MarkByte[7] = 0xFF;

							ResetUDSRespMark();
							m_bStart100KuOK1 = true;
							m_bStart100KuOK2 = true;
							if (CaliPowerReq(MarkByte, 0, 1000))	//7
							{
								//保持诊断会话在线
								SetTimer(2, 2000, NULL);
								Sleep(cfg[0].per100_wait_time * 1000);
								KillTimer(2);
								Sleep(20);

								if (!Moniter_DevLostInCali(nPTC))
									return bRight;

								m_highPwr->Get_8710Dev_Vol_Curr(&fGetVol, &fGetCurr2);
								strStepMsg.Format("检测高压电源电压值ptc2_1->%3.2f 电流值2->%3.2f", fGetVol, fGetCurr2);
								ShowStepMessage(strStepMsg, 0, 0, pKu);

								/*nWaitOnQueryCurr = cfg[0].per100_wait_time * 1000;
								Waitfor_8710ArrivedDstCurr(5.0f, nWaitOnQueryCurr, &fDstCurr2, 2, true);*/

								MarkByte[0] = 0x04;
								MarkByte[1] = 0x31;
								MarkByte[2] = 0x01;
								MarkByte[3] = 0x01;
								MarkByte[4] = 0x04;
								MarkByte[5] = 0xFF;
								MarkByte[6] = 0xFF;
								MarkByte[7] = 0xFF;

								ResetUDSRespMark();
								m_bEnd100KuOK1 = false;
								m_bEnd100KuOK2 = false;
								m_bStartGetADCValue3 = true;
								m_bStartGetADCValue4 = true;

								if (CaliPowerReq(MarkByte, 0, 1000))
								{
									bStep1 = false;
									bStep2 = false;
									//ptc1
									ADCCounter4_1 = g_resp_power_cali1[0] << 8 | g_resp_power_cali1[1];
									ShowStepMessage("ADC counter4_1", ADCCounter4_1, 0, pKu);

									//斜率ku=（I2-I1）/（C2-C1）
									fKi16_4 = (double)(fGetCurr2 - fGetCurr1) / (double)(ADCCounter4_1 - ADCCounter3_1);
									ShowStepMessage("斜率Ku3", 0, fKi16_4, pKu);
									pcba.fCalied_100Slope1 = (float)fKi16_4;

									//斜率结果判断3
									dError100_1 = abs(fKi16_4 - cfg[0].slope_base2);
									if (dError100_1 < cfg[0].slope_error2)
										bSlope100CheckOK1 = true;
									else
										bSlope100CheckOK1 = false;

									/*************************************************debug log for record variabel value***********************************************************************/
#ifdef _DEBUG
									memset(U2, 0, sizeof(U2));
									strOutStr.Format("%s::%3.3f", "fGetCurr1", fGetCurr1);
									ShowStepMessage(strOutStr, 0, 0.0f, U2);

									strOutStr.Format("%s::%3.3f", "fGetCurr2", fGetCurr2);
									ShowStepMessage(strOutStr, 0, 0.0f, U2);

									strMsg.Format(" ADCCounter4_1:%d", ADCCounter4_1);
									ShowStepMessage(strMsg, 0, 0.0f, U2);

									strMsg.Format(" ADCCounter3_1:%d", ADCCounter3_1);
									ShowStepMessage(strMsg, 0, 0.0f, U2);
#endif

									/*************************************************debug log for record variabel value***********************************************************************/

									////4.斜率转换为定点数
									nRaw = fp.makeRaw(fKi16_4, 16);
									memset(pKi, 0, sizeof(pKi));
									methord::u16toPu8(nRaw, pKi);
									ShowStepMessage("斜率请求Ku3", 0, 0.0f, pKi);

									MarkByte[0] = 0x06;
									MarkByte[1] = 0x31;
									MarkByte[2] = 0x01;
									MarkByte[3] = 0x01;
									MarkByte[4] = 0x05;
									MarkByte[5] = pKi[0];
									MarkByte[6] = pKi[1];
									MarkByte[7] = 0xFF;				

									ResetUDSRespMark();
									m_bStart100KuOK1 = true;
									if (CaliPowerReq(MarkByte, 0, 1000))				//9
									{
										memset(pKu, 0, sizeof(pKu));
										ShowStepMessage("重新启动...", 0, 0, pKu);
										//重新启动
										Sleep(1000);

										MarkByte[0] = 0x02;
										MarkByte[1] = 0x11;
										MarkByte[2] = 0x01;
										MarkByte[3] = 0x00;
										MarkByte[4] = 0x00;
										MarkByte[5] = 0x00;
										MarkByte[6] = 0x00;
										MarkByte[7] = 0x00;

										ResetUDSRespMark();
										m_bStartCaliPower11OK1 = true;
										m_bStartCaliPower11OK2 = true;
										if (!CaliPowerReq(MarkByte, 0, 1000))		//10
										{
											*nPTC = 3;
											AdjustHighPowerVol(25.0f, 5.0f);
											ShowStepMessage("重新启动失败", 0, 0, pKu);
											return bRight;
										}
										Sleep(2000);

										//重启后，需要再次安全访问，UDS服务0x10 03, 0x27 01, 0x27 02
										ResetUDSRespMark();
										int nSecurityAccess = UDS_RequestBefore0x31Svr();
										if (nSecurityAccess == 3)
										{
											*nPTC = 1;
											AdjustHighPowerVol(25.0f, 5.0f);
											ShowStepMessage("UDS安全访问未通过，功率校准流程退出", 0, 0, pKu);
											return bRight;
										}
										else if (nSecurityAccess == 1)
										{
											*nPTC = 1;
											AdjustHighPowerVol(25.0f, 5.0f);
											ShowStepMessage("PTC A UDS安全访问未通过", 0, 0, pKu);
											return bRight;
										}
									/*	else if (nSecurityAccess == 2)
										{
											*nPTC = 2;
											AdjustHighPowerVol(25.0f, 5.0f);
											ShowStepMessage("PTC B UDS安全访问未通过", 0, 0, pKu);
											return bRight;
										}*/

										MarkByte[0] = 0x04;
										MarkByte[1] = 0x31;
										MarkByte[2] = 0x01;
										MarkByte[3] = 0x01;
										MarkByte[4] = 0x00;
										MarkByte[5] = 0xFF;
										MarkByte[6] = 0xFF;
										MarkByte[7] = 0xFF;

										ResetUDSRespMark();
										m_bStartCaliPower11OK1 = true;
										m_bStartCaliPower11OK2 = true;
										if (CaliPowerReq(MarkByte, 0, 1000))	//11
										{
											//12、设置高压电压为额定电压+20V；
											fVoltage = u2 + u3;
											fCurrent = 25.0f;
											ShowStepMessage("设置高压电源电压->620v, 电流->25A", 0, 0, pKu);
											m_Mediator->Set_Dev_Vol_Curr(m_highPwr, fVoltage, fCurrent);
											Sleep(2000);
											if (!Moniter_DevLostInCali(nPTC))
												return bRight;

											//13、上位机通过31服务设定占空比为90%
											nRaw = fp.makeRaw(fKu16_8, 15);
											memset(pKu, 0, sizeof(pKu));
											methord::u16toPu8(nRaw, pKu);
											ShowStepMessage("占空比为90%,斜率请求Ku5", 0, 0.0f, pKu);

											MarkByte[0] = 0x06;
											MarkByte[1] = 0x31;
											MarkByte[2] = 0x01;
											MarkByte[3] = 0x01;
											MarkByte[4] = 0x03;
											MarkByte[5] = pKu[0];
											MarkByte[6] = pKu[1];
											MarkByte[7] = 0xFF;

											ResetUDSRespMark();
											m_bStartGetADCValue1 = true;
											m_bStartGetADCValue2 = true;
											if (CaliPowerReq(MarkByte, 0, 1000))	//13
											{
												//保持诊断会话在线
												SetTimer(2, 2000, NULL);
												Sleep(CALI_POWRER_WAIT_TIME);
												KillTimer(2);
												Sleep(20);

												if (!Moniter_DevLostInCali(nPTC))
													return bRight;
												//Waitfor_8710ArrivedDstVol(fVoltage, CALI_POWRER_WAIT_TIME, &f600Vol1, &f600Vol2, &fDstCurr1, &fDstCurr2);

												//14、上位机得到设置占空比请求的正响应，延时等地稳定后，通过31服务读取对应高压电压值，此时读取到的值为校正后的电压值
												MarkByte[0] = 0x04;
												MarkByte[1] = 0x31;
												MarkByte[2] = 0x01;
												MarkByte[3] = 0x01;
												MarkByte[4] = 0x06;
												MarkByte[5] = 0xFF;
												MarkByte[6] = 0xFF;
												MarkByte[7] = 0xFF;

												ResetUDSRespMark();
												m_bReadCaliedVolOK1 = true;
												m_bReadCaliedVolOK2 = true;
												if (CaliPowerReq(MarkByte, 0, 1000))		//14
												{
													ADCCounter1_1 = g_resp_power_cali1[0] << 8 | g_resp_power_cali1[1];
													//定点数转换为浮点数
													_power_cali_vol1 = fp.toFloat(ADCCounter1_1, 6);
													memset(pKu, 0, sizeof(pKu));
													ShowStepMessage("ptc1校正后的电压值", 0, _power_cali_vol1, pKu);

													//ADCCounter1_2 = g_resp_power_cali2[0] << 8 | g_resp_power_cali2[1];
													////定点数转换为浮点数
													//_power_cali_vol2 = fp.toFloat(ADCCounter1_2, 6);
													//memset(pKu, 0, sizeof(pKu));
													//ShowStepMessage("ptc2校正后的电压值", 0, _power_cali_vol2, pKu);
												}
												else
												{
													*nPTC = 1;
													AdjustHighPowerVol(25.0f, 5.0f);
													memset(pKu, 0, sizeof(pKu));
													ShowStepMessage("读取高压电压值失败", 0, 0, pKu);
													return bRight;
												}

												//15、 上位机通过31服务读取对应高压电流值，此时读取到的值为校正后的电流值
												MarkByte[0] = 0x04;
												MarkByte[1] = 0x31;
												MarkByte[2] = 0x01;
												MarkByte[3] = 0x01;
												MarkByte[4] = 0x07;
												MarkByte[5] = 0xFF;
												MarkByte[6] = 0xFF;
												MarkByte[7] = 0xFF;

												ResetUDSRespMark();
												m_bReadCaliedCurrOK1 = true;
												m_bReadCaliedCurrOK2 = true;
												if (CaliPowerReq(MarkByte, 0, 1000))
												{
													ADCCounter1_1 = g_resp_power_cali1[0] << 8 | g_resp_power_cali1[1];
													//定点数转换为浮点数
													_power_cali_curr1 = fp.toFloat(ADCCounter1_1, 9);
													ShowStepMessage("ptc1校正后的电流值", 0, _power_cali_curr1, pKu);

													//ADCCounter1_2 = g_resp_power_cali2[0] << 8 | g_resp_power_cali2[1];
													////定点数转换为浮点数
													//_power_cali_curr2 = fp.toFloat(ADCCounter1_2, 9);
													//ShowStepMessage("ptc2校正后的电流值", 0, _power_cali_curr2, pKu);
												}
												else
												{
													AdjustHighPowerVol(25.0f, 5.0f);
													memset(pKu, 0, sizeof(pKu));
													ShowStepMessage("读取高压电流值失败", 0, 0, pKu);
													*nPTC = 3;
													return bRight;
												}

												g_power_cali1 = _power_cali_vol1 * _power_cali_curr1;
												//g_power_cali2 = _power_cali_vol2 * _power_cali_curr2;
												pcba.fCalied_Power1 = g_power_cali1;
												//pcba.fCalied_Power2 = g_power_cali2;

#ifdef _DEBUG
												strOutStr.Format("ptc1校正后的电压值::%3.2f", _power_cali_vol1);
												DispalyCurrentMsg(strOutStr, false, 1);
												strOutStr.Format("ptc1校正后的电流值::%3.2f", _power_cali_curr1);
												DispalyCurrentMsg(strOutStr, false, 1);

												ShowStepMessage("ptc1校正后的电压值", 0, _power_cali_vol1, pKu);
												ShowStepMessage("ptc1校正后的电流值", 0, _power_cali_curr1, pKu);

#endif
												ShowStepMessage("ptc1校正后的功率值", 0, g_power_cali1, pKu);

												if (!Moniter_DevLostInCali(nPTC))
													return bRight;

												m_highPwr->Get_8710Dev_Vol_Curr(&volvalue, &curvalue);
												g_fPower1 = volvalue * curvalue;
												fVolError1 = abs(volvalue - _power_cali_vol1);
												fCurrError1 = abs(curvalue - _power_cali_curr1);
												fPowerError1 = abs(g_power_cali1 - g_fPower1);

												//m_highPwr->Get_8710Dev_Vol_Curr(&volvalue, &curvalue, 2);
												//g_fPower2 = volvalue * curvalue;
												//fVolError2 = abs(volvalue - _power_cali_vol2);
												//fCurrError2 = abs(curvalue - _power_cali_curr1);
												//fPowerError2 = abs(g_power_cali2 - g_fPower2);

												pcba.fCalied_PowerErr1 = fPowerError1;
												//pcba.fCalied_PowerErr2 = fPowerError2;

#ifdef _per30DutyCycle
												//通过31服务设定占空比为30%
												_per10DutyCycleReq(0.3f);
#endif
												int nPTCTestResult1 = 0, nPTCTestResult2 = 0;
												CString strInfo;

												//斜率结果判断3
												if (bSlope50CheckOK1 && bSlope100CheckOK1)
												{
													if (fVolError1 < 10.0f && fCurrError1 < 0.5f && fPowerError1 < g_fPower1 * 0.1f)
													{
														strInfo.Format("电压误差：%2.2f, 电流误差:%2.2f, 功率误差:%2.2f", fVolError1, fCurrError1, fPowerError1);
														m_list1.AddString(strInfo);
														m_list1.AddString("PTC A功率校准成功");

														m_list1.SetCurSel(m_list1.GetCount() - 1);
														m_listexcel.SetItemText(13, 1, "OK");
														bRight = true;
														nPTCTestResult1 = 0;
													}
													else
													{
														strInfo.Format("电压误差：%2.2f, 电流误差:%2.2f, 功率误差:%2.2f", fVolError1, fCurrError1, fPowerError1);
														m_list1.AddString(strInfo);
														m_listexcel.SetItemText(13, 1, "NOK");
														WriteErrMsg("PTC A功率校准失败", this, 13, 1);
														bRight = false;
														nPTCTestResult1 = 1;
														AdjustHighPowerVol(25.0f, 5.0f);
													}
												}
												else
												{
													if (!bSlope50CheckOK1)
													{
														strInfo.Format("百分之50占空比斜率误差1：%1.15f", dError50_1);
														m_list1.AddString(strInfo);
													}
													if (!bSlope100CheckOK1)
													{
														strInfo.Format("百分之100占空比斜率误差1：%1.15f", dError100_1);
														m_list1.AddString(strInfo);
													}

													m_listexcel.SetItemText(13, 1, "NOK");
													WriteErrMsg("PTC A功率校准失败", this, 13, 1);

													bRight = false;
													nPTCTestResult1 = 1;
													AdjustHighPowerVol(25.0f, 5.0f);
												}

												////斜率结果判断4
												//if (bSlope50CheckOK2 && bSlope100CheckOK2)
												//{
												//	if (fVolError2 < 10.0f && fCurrError2 < 0.5f && fPowerError2 < g_fPower1 * 0.1f)
												//	{
												//		strInfo.Format("电压误差：%2.2f, 电流误差:%2.2f, 功率误差:%2.2f", fVolError2, fCurrError2, fPowerError2);
												//		m_list1.AddString(strInfo);

												//		m_list1.AddString("PTC B功率校准成功");
												//		m_list1.SetCurSel(m_list1.GetCount() - 1);
												//		m_listexcel.SetItemText(14, 1, "OK");
												//		bRight &= true;
												//		nPTCTestResult2 = 0;
												//	}
												//	else
												//	{
												//		strInfo.Format("电压误差：%2.2f, 电流误差:%2.2f, 功率误差:%2.2f", fVolError2, fCurrError2, fPowerError2);
												//		m_list1.AddString(strInfo);
												//		m_listexcel.SetItemText(14, 1, "NOK");
												//		WriteErrMsg("PTC B功率校准失败", this, 14, 1);
												//		bRight = false;
												//		nPTCTestResult2 = 1;
												//		AdjustHighPowerVol(25.0f, 5.0f);
												//	}
												//}
												//else
												//{
												//	if (!bSlope50CheckOK2)
												//	{
												//		strInfo.Format("百分之50占空比斜率误差2：%1.15f", dError50_2);
												//		m_list1.AddString(strInfo);
												//	}
												//	if (!bSlope100CheckOK2)
												//	{
												//		strInfo.Format("百分之100占空比斜率误差2：%1.15f", dError100_2);
												//		m_list1.AddString(strInfo);
												//	}

												//	m_listexcel.SetItemText(14, 1, "NOK");
												//	WriteErrMsg("PTC B功率校准失败", this, 14, 1);
												//	bRight = false;
												//	nPTCTestResult2 = 1;
												//	AdjustHighPowerVol(25.0f, 5.0f);
												//}


												//测试结果标志
												if (nPTCTestResult1 == 1 /*&& nPTCTestResult2 == 1*/)
												{
													bRight = FALSE;
													*nPTC = 1;//3;
												}
												else
												{
													if (nPTCTestResult1 == 0 /*&& nPTCTestResult2 == 0*/)
													{
														bRight = TRUE;
														*nPTC = 0;
													}
													if (nPTCTestResult1 == 1)
													{
														bRight = FALSE;
														*nPTC = 1;
													}
													//else if (nPTCTestResult2 == 1)
													//{
													//	bRight = FALSE;
													//	*nPTC = 2;
													//}

												}

												//17、 判断结束后，上位机通过 31服务请求退出校准模式
												//04 31 01 01 08 FF FF FF←04 71 01 01 08 FF FF FFK
												MarkByte[0] = 0x04;
												MarkByte[1] = 0x31;
												MarkByte[2] = 0x01;
												MarkByte[3] = 0x01;
												MarkByte[4] = 0x08;
												MarkByte[5] = 0xFF;
												MarkByte[6] = 0xFF;
												MarkByte[7] = 0xFF;

												m_bStartCaliPowerExist1 = true;
												m_bStartCaliPowerExist2 = true;
												if (CaliPowerReq(MarkByte, 0, 1000))
												{
													bRight &= true;
												}
#ifdef _SVR_0x28
												//Enabel app message
												UDS_EnableAppMsg();
#endif
											}
											else
											{
												*nPTC = 1;
												AdjustHighPowerVol(25.0f, 5.0f);
											}
										}
										else
										{
											*nPTC = 1;
											AdjustHighPowerVol(25.0f, 5.0f);
										}
									}
									else
									{
										*nPTC = 1;
										AdjustHighPowerVol(25.0f, 5.0f);
									}
								}
								else
								{
									*nPTC = 1;
									AdjustHighPowerVol(25.0f, 5.0f);
								}
							}
							else
							{
								*nPTC = 1;
								AdjustHighPowerVol(25.0f, 5.0f);
							}
						}
						else
						{
							*nPTC = 1;
							AdjustHighPowerVol(25.0f, 5.0f);
						}
					}
					else
					{
						*nPTC = 1;
						AdjustHighPowerVol(25.0f, 5.0f);
					}
				}
				else
				{
					*nPTC = 1;
					AdjustHighPowerVol(25.0f, 5.0f);
				}
			}
			else
			{
				*nPTC = 1;
				AdjustHighPowerVol(25.0f, 5.0f);
			}
		}
		else
		{
			*nPTC = 1;
			AdjustHighPowerVol(25.0f, 5.0f);
		}
	}


	return bRight;
}

//功率校准
bool CACPSDlg::ptc_Power_Calibration(CACPSDlg* pDlg)
{
	bool bRight = false;
	int nPTC = 3;
	g_NotRecvAppMsg_BeforeTestFlowStart = true;		//校准期间关闭app报文接收

	pDlg->m_bErrMsg = pDlg->Cali_PowerSlope(&nPTC);

	if (nPTC == 0 && pDlg->m_bErrMsg!=false)
	{
		pDlg->m_list1.AddString("功率校准成功");
		pDlg->m_list1.SetCurSel(pDlg->m_list1.GetCount() - 1);
		
		bRight = true;
	}
	else
	{
		if (nPTC != 0)
		{
#ifdef _SVR_0x28
			pDlg->UDS_0x1003Request();
			//Enabel app message
			pDlg->UDS_EnableAppMsg();
#endif
			pDlg->m_listexcel.SetItemText(13, 1, "NOK");
			pDlg->m_list1.AddString("PTC A功率校准失败");
			pDlg->m_list1.SetCurSel(pDlg->m_list1.GetCount() - 1);
			g_NotRecvAppMsg_BeforeTestFlowStart = false;

			pDlg->WriteTestReport();
			pDlg->OffHighPowerOut(portPre);
			pDlg->LowPower_CTRL(POWER_OFF);
			pDlg->_Log();

			StopFlag = 1;//停止接收报文
			KillTimer(1);
			KillTimer(2);
			KillTimer(3);
			KillTimer(4);
			return bRight;
		}
		//else if (nPTC == 2)
		//{
		//	pDlg->UDS_0x1003Request();
		//	//Enabel app message
		//	pDlg->UDS_EnableAppMsg();
		//	pDlg->m_listexcel.SetItemText(14, 1, "NOK");
		//	pDlg->m_list1.AddString("PTC B功率校准失败");
		//	pDlg->m_list1.SetCurSel(pDlg->m_list1.GetCount() - 1);
		//	g_NotRecvAppMsg_BeforeTestFlowStart = false;
		//	pDlg->WriteTestReport();
		//	pDlg->OffHighPowerOut(portPre);
		//	pDlg->LowPower_CTRL(POWER_OFF);
		//	return bRight;
		//}
		//else if (nPTC == 3)
		//{
		//	pDlg->m_listexcel.SetItemText(13, 1, "NOK");
		//	pDlg->m_listexcel.SetItemText(14, 1, "NOK");
		//	pDlg->m_list1.AddString("功率校准失败");
		//	pDlg->m_list1.SetCurSel(pDlg->m_list1.GetCount() - 1);
		//	g_NotRecvAppMsg_BeforeTestFlowStart = false;
		//	pDlg->WriteTestReport();
		//	pDlg->OffHighPowerOut(portPre);
		//	pDlg->LowPower_CTRL(POWER_OFF);
		//	//::PostMessage(g_hWnd, WM_TEST_ABNORMAL, (WPARAM)true, 0);
		//	return bRight;
		//}
	}

	g_NotRecvAppMsg_BeforeTestFlowStart = false;
	return bRight;
}

bool CACPSDlg::_per10DutyCycleReq(float fDutyCycle)
{
	byte pKu[2];
	byte MarkByte[8];
	FixedPoint fp;
	bool bResponsed = false;

	//安全访问
	UDS_RequestBefore0x31Svr();

	//下电
	float fVoltage = 30.0f;
	float fCurrent = 5.0f;
	ShowStepMessage("设置高压电源电压->25v, 电流->5A", 0, 0, pKu);
	m_Mediator->Set_Dev_Vol_Curr(m_highPwr, fVoltage, fCurrent);
	Sleep(2000);
	//_
	int nRaw = fp.makeRaw(fDutyCycle, 15);
	memset(pKu, 0, sizeof(pKu));
	methord::u16toPu8(nRaw, pKu);
	ShowStepMessage("占空比为30%斜率请求Ku1", 0, 0.0f, pKu);

	MarkByte[0] = 0x06;
	MarkByte[1] = 0x31;
	MarkByte[2] = 0x01;
	MarkByte[3] = 0x01;
	MarkByte[4] = 0x03;
	MarkByte[5] = pKu[0];
	MarkByte[6] = pKu[1];
	MarkByte[7] = 0xFF;

	m_bStart50KuOK1 = true;
	m_bStart50KuOK2 = true;
	bResponsed = CaliPowerReq(MarkByte, 0, 1000);

	SetTimer(2, 2000, NULL);
	Sleep(CALI_POWRER_WAIT_TIME0);
	KillTimer(2);
	Sleep(20);

#ifdef _SVR_0x28
	////使能app报文
	UDS_EnableAppMsg();
#endif

	return bResponsed;
}

//高压/低压下电
bool CACPSDlg::LowHigh_PowerDown()
{
	bool bRight = false;
	byte pKu[2] = { 0x00, 0x00 };
	float fVol1 = 0.0f, fVol2 = 0.0f;
	
	if (m_Mediator->highPower != NULL) {

		ShowStepMessage("高压电源电压设置为：vol->0v, curr->0A", 0, 0, pKu);
		m_Mediator->Set_Dev_Vol_Curr(m_highPwr, 0.0f, 0.0f);
		//Sleep(2000);
		//周期读取直至符合，最大延迟60S，否则直接关闭高压电源
		bRight = Waitfor_8710ArrivedDstVol(30.0f, CALI_POWRER_WAIT_TIME0 / 2, &fVol1, &fVol2);

		if (!bRight)
		{
			//30%占空比
			bRight &= _per10DutyCycleReq(20.0f);
			if (!bRight)
				ShowStepMessage("发送30%占空比驱动高压电源下电失败，请手动下电", 0, 0, pKu);
		}
		m_Mediator->highPower->SetDWWState(false);
	}

	if (m_Mediator->lowPower != NULL)
	{
		//低压下电 //can
		ShowStepMessage("低压电源电压设置为：vol->0v, curr->0A", 0, 0, pKu);
		m_Mediator->Set_Dev_Vol_Curr(m_lowPwr, 0.0f, 0.0f);
		//关闭IT6322远程控制
		m_Mediator->lowPower->EnableRemoteCtrl(false);
	}

	return bRight;
}

void CACPSDlg::AdjustHighPowerVol(float fVol, float fCurr)
{
	CString strInfo;
	byte pKu[2];
	memset(pKu, 0, sizeof(pKu));
	strInfo.Format("%s %2.2f %2.2f", "高压电源电压设置为：vol->", fVol, fCurr);
	ShowStepMessage(strInfo, 0, 0, pKu);
	m_Mediator->Set_Dev_Vol_Curr(m_highPwr, fVol, fCurr);
	Sleep(1000);
}

//报文周期检查
bool CACPSDlg::Check_MsgCycle()
{
	bool bRight = false;
	long lExpTime = 0;
	int nWaitTime = 6000;
	int nWaitT = 0;
	int nExceptMsgCycle = 1000;
	int nMsgCycle1_err = 0, nMsgCycle2_err = 0;
	int nMsgCycle21_err = 0, nMsgCycle22_err = 0;
	CString strOutstr;

	if (m_bLostPtc1 || m_bDevLost1 || 
		m_bDevLost2 || m_bLowPowerLost)
	{
		strOutstr.Format("%s", "NOK");
		m_listexcel.SetItemText(1, 3, strOutstr);
		WriteErrMsg("chn1 通信丢失", this, 1, 3);

		if (m_bLostPtc1)
		{
			WriteErrMsg("chn1 通信丢失", this, 1, 3);
		}
		else if (m_bDevLost1)
		{
			WriteErrMsg("chn1 低压通信Rs232断开", this, 1, 3);
		}
		else if (m_bDevLost2)
		{
			WriteErrMsg("chn1 高压通信Rs485断开", this, 1, 3);
		}
		else if (m_bLowPowerLost)
		{
			WriteErrMsg("chn1 低压掉电", this, 1, 3);
		}

		WriteTestReport();
		StopFlag = 1;
		_Log();
		pcba.cycle_time1 = false;
		bRight = false;
	}
	else
	{
		m_recv_msglst1.clear();
		m_recv_msglst2.clear();

		SetEvent(g_MsgCycleSignaled);
		Sleep(nWaitTime);
		ResetEvent(g_MsgCycleSignaled);

		nWaitT = nWaitTime / nExceptMsgCycle;

		//ptc1 message cycle time caculate
		{
			//size_t nCycleTime = 0;
			//size_t nCycleTolTime = 0;
			//for (size_t i = 1; i < m_recv_msglst1.size(); i++)//丢弃第一个数据(非报文正常周期数)
			//{
			//	nCycleTolTime += (size_t)m_recv_msglst1[i];
			//}
			//nCycleTime = nCycleTolTime / (m_recv_msglst1.size() - 1);
			//size_t nCycleTime2 = 0;
			//size_t nCycleTolTime2 = 0;
			//for (size_t j = 1; j < m_recv_msglst2.size(); j++)//丢弃第一个数据(非报文正常周期数)
			//{
			//	nCycleTolTime2 += (size_t)m_recv_msglst2[j];
			//}
			//nCycleTime2 = nCycleTolTime2 / (m_recv_msglst2.size() - 1);
			//if (abs((int)nCycleTime - 100) < 3 && abs((int)nCycleTime2 - 100) < 3)
			//{
			//}
			
			nMsgCycle1_err = abs((int)m_recv_msglst1.size() - nWaitT);
			nMsgCycle2_err = abs((int)m_recv_msglst2.size() - nWaitT);
			if (abs(nMsgCycle1_err) <= 3 && abs(nMsgCycle2_err) <= 3)
			{
				bRight = true;
				pcba.cycle_time1 = true;
				strOutstr.Format("chn1 总线报文周期正常");
				m_list1.AddString(strOutstr);
				m_listexcel.SetItemText(1, 3, "OK");
			}
			else
			{
				pcba.cycle_time1 = false;
				if (abs(nMsgCycle1_err) > 3)
				{
					strOutstr.Format("chn1 总线报文HVH_ECU_Frame1周期异常::%d", nMsgCycle1_err );
					m_list1.AddString(strOutstr);
				}

				if (abs(nMsgCycle2_err) > 3)
				{
					strOutstr.Format("chn1 总线报文HVH_ECU_Frame2周期异常::%d", nMsgCycle2_err );
					m_list1.AddString(strOutstr);
				}
				m_listexcel.SetItemText(1, 3, "NOK");
				WriteErrMsg("chn1 总线报文周期异常", this, 1, 3);
			}
		}
	}

	return bRight;
}

//项目信息检测
bool CACPSDlg::Check_ProjectInfo()
{
	bool bRight = false;
	int NumValue = 0;
	int nWaitTime = 0;
	CString strmsg, strOutstr;
	DWORD ID = g_DiagReq;

	byte MarkByte[8] = { 0x03, 0x22, 0xF1, 0x70, 0x00, 0x00, 0x00, 0x00 };

	m_list1.AddString("读取项目信息(3s)");
	m_list1.SetCurSel(m_list1.GetCount() - 1);

	SendCmd send(m_Bus);
	Command* p_send = &send;
	m_exec->SetCmd(p_send);

	m_Bus->m_CanParams.m_SendFrameType = 0;

	for (int k = 0; k < 3; k++)
	{
		m_exec->Send(ID, 0, 0, MarkByte, &strmsg);
		Sleep(10);
	}

	DispalyCurrentMsg(strmsg, false);
	m_Bus->m_CanParams.m_SendFrameType = 1;

	while (nWaitTime < 1000)
	{
		if (m_bReadProjectInfo1)
		{
			bRight = true;
			break;
		}

		Sleep(10);
		nWaitTime++;
	}

	if (m_bLostPtc1 || m_bDevLost1 ||
		m_bDevLost2 || m_bLowPowerLost)
	{
		strOutstr.Format("%s", "NOK");
		m_listexcel.SetItemText(5, 1, strOutstr);

		if (m_bLostPtc1)
		{
			WriteErrMsg("chn1 通信丢失", this, 5, 1);
		}
		else if (m_bDevLost1)
		{
			WriteErrMsg("chn1 低压通信Rs232断开", this, 5, 1);
		}
		else if (m_bDevLost2)
		{
			WriteErrMsg("chn1 高压通信Rs485断开", this, 5, 1);
		}
		else if (m_bLowPowerLost)
		{
			WriteErrMsg("chn1 低压掉电", this, 5, 1);
		}

		StopFlag = 1;
		KillTimer(3);
		_Log();
		WriteTestReport();
		pcba.project_info1 = false;
		bRight = false;
	}
	else
	{
		if (m_bReadProjectInfo1)
		{
			strOutstr.Format("ch1 项目信息一致");
			m_list1.AddString(strOutstr);
			m_listexcel.SetItemText(5, 1, "OK");
			bRight = true;
			pcba.project_info1 = true;
		}
		else
		{
			m_listexcel.SetItemText(5, 1, "NOK");
			WriteErrMsg("chn1 项目信息不一致", this, 5, 1);
			bRight = false;
			pcba.project_info1 = false;
		}
	}

	if (!bRight)
	{
		StopFlag = 1;
		KillTimer(3);
		_Log();
		WriteTestReport();
		LowPower_CTRL(POWER_OFF);
		OffHighPowerOut(portPre);
	}

	return bRight;
}

//软件版本验证
bool CACPSDlg::Check_SoftwareVerion()
{
	bool bRight = false;
	int NumValue = 0;
	int nWaitTime = 0;
	CString strmsg, strOutstr;
	DWORD ID = g_DiagReq;
	
	byte MarkByte[8] = {0x03, 0x22, 0xF1, 0x89, 0x00, 0x00, 0x00, 0x00 };
	m_list1.AddString("验证软件版本号(1s)");
	m_list1.SetCurSel(m_list1.GetCount() - 1);

	SendCmd send(m_Bus);
	Command* p_send = &send;
	m_exec->SetCmd(p_send);

	m_Bus->m_CanParams.m_SendFrameType = 0;

	for (int k = 0; k < 3; k++)
	{
		m_exec->Send(ID, 0, 0, MarkByte, &strmsg);
		Sleep(10);
	}

	DispalyCurrentMsg(strmsg, false);

	m_Bus->m_CanParams.m_SendFrameType = 1;
	while (nWaitTime < 500)
	{
		if (m_bSoftwareVerVerified )
		{
			break;
		}

		Sleep(10);
		nWaitTime++;
	}

	//ptc0
	if (m_bLostPtc1 ||m_bDevLost1 ||
		m_bDevLost2 || m_bLowPowerLost)
	{
		strOutstr.Format("%s", "NOK");
		m_listexcel.SetItemText(5, 2, strOutstr);
		
		if (m_bLostPtc1)
		{
			WriteErrMsg("chn1 通信丢失", this, 5, 2);
		}
		else if (m_bDevLost1)
		{
			WriteErrMsg("chn1 低压通信Rs232断开", this, 5, 2);
		}
		else if (m_bDevLost2)
		{
			WriteErrMsg("chn1 高压通信Rs485断开", this, 5, 2);
		}
		else if (m_bLowPowerLost)
		{
			WriteErrMsg("chn1 低压掉电", this, 5, 2);
		}

		pcba.software_ver1 = false;
		bRight = false;
	}
	else
	{
		if (m_bSoftwareVerVerified)
		{
			strOutstr.Format("ch1 软件版本号一致");
			m_list1.AddString(strOutstr);
			m_listexcel.SetItemText(5, 2, "OK");
			bRight = true;
			pcba.software_ver1 = true;
		}
		else
		{
			m_listexcel.SetItemText(5, 2, "NOK");
			WriteErrMsg("chn1 软件版本号不一致", this, 5, 2);
			bRight = false;
			pcba.software_ver1 = false;
		}
	}
	
	if (!bRight)
	{
		StopFlag = 1;
		KillTimer(3);
		_Log();

		WriteTestReport();
		LowPower_CTRL(POWER_OFF);
		//OffHighPowerOut(portPre);
	}

	return bRight;
}

//检测低压电压诊断电路
bool CACPSDlg::Check_LowPowerCircuit()
{
	bool bRight = false;
	CString strOutstr, valuestr;

	/*----------------------------------过压故障------------------------------------------*/

	//设置低压电源电压33.0v, 电流1.5A(过压)
	LowPower_CTRL(POWER_SET, cfg[0].ov1, cfg[0].lowcur);
	//保持1000ms
	Sleep(6000);

	//ptc 0
	if (m_bLostPtc1 || m_bDevLost1 || 
		m_bDevLost2 || m_bLowPowerLost)
	{
		strOutstr.Format("%s", "NOK");
		m_listexcel.SetItemText(5, 3, strOutstr);

		if (m_bLostPtc1)
		{
			WriteErrMsg("chn1 通信丢失", this, 5, 3);
		}
		else if (m_bDevLost1)
		{
			WriteErrMsg("chn1 低压通信Rs232断开", this, 5, 3);
		}
		else if (m_bDevLost2)
		{
			WriteErrMsg("chn1 高压通信Rs485断开", this, 5, 3);
		}
		else if (m_bLowPowerLost)
		{
			WriteErrMsg("chn1 低压掉电", this, 5, 3);
		}

		StopFlag = 1;
		KillTimer(3);
		_Log();

		pcba.ov1 = false;
		bRight = false;
		WriteTestReport();

		//将低压电压调到预设值(24v/12v, 1.2A)
		LowPower_CTRL(POWER_SET, cfg[0].lowpre, cfg[0].lowcur);
		return bRight;
	}
	else
	{
		if (HVH_ECU_Frm2->HVH_WarnULoOutOfRang == 0x02)	//Over voltage error
		{
			strOutstr.Format("ch1 低压过压故障记录正常");
			m_list1.AddString(strOutstr);
			m_list1.SetCurSel(m_list1.GetCount() - 1);
			m_listexcel.SetItemText(5, 3, "OK");
			bRight = true;
			pcba.ov1 = true;
		}
		else
		{
			valuestr.Format("HVH_WarnULoOutOfRang::%d", HVH_ECU_Frm2->HVH_WarnULoOutOfRang);
			m_list1.AddString(valuestr);
			m_list1.SetCurSel(m_list1.GetCount() - 1);

			m_listexcel.SetItemText(5, 3, "NOK");
			WriteErrMsg("chn1 低压过压故障记录异常", this, 5, 3);
			bRight = false;
			pcba.ov1 = false;
		}
	}
	
	//设置低压电源电压32.0v, 电流1.5A(过压恢复)
	LowPower_CTRL(POWER_SET, cfg[0].ov2, cfg[0].lowcur);
	//保持1000ms
	Sleep(6000);

	//ptc 0
	if (m_bLostPtc1 || m_bDevLost1 ||
		m_bDevLost2 || m_bLowPowerLost)
	{
		strOutstr.Format("%s", "NOK");
		m_listexcel.SetItemText(5, 3, strOutstr);
		if (m_bLostPtc1)
		{
			WriteErrMsg("chn1 通信丢失", this, 5, 3);
		}
		else if (m_bDevLost1)
		{
			WriteErrMsg("chn1 低压通信Rs232断开", this, 5, 3);
		}
		else if (m_bDevLost2)
		{
			WriteErrMsg("chn1 高压通信Rs485断开", this, 5, 3);
		}
		else if (m_bLowPowerLost)
		{
			WriteErrMsg("chn1 低压掉电", this, 5, 3);
		}

		StopFlag = 1;
		KillTimer(3);		
		_Log();

		pcba.ov1 = false;
		bRight = false;
		WriteTestReport();
		//将低压电压调到预设值(24v/12v, 1.2A)
		LowPower_CTRL(POWER_SET, cfg[0].lowpre, cfg[0].lowcur);
		return bRight;
	}
	else
	{
		if (bRight)
		{
			if (HVH_ECU_Frm2->HVH_WarnULoOutOfRang == 0x00)	//No issue
			{
				strOutstr.Format("ch1 低压过压故障正常恢复");
				m_list1.AddString(strOutstr);
				m_list1.SetCurSel(m_list1.GetCount() - 1);
				m_listexcel.SetItemText(5, 3, "OK");
				
				bRight &= true;
				pcba.ov1 &= true;
			}
			else
			{
				m_listexcel.SetItemText(5, 3, "NOK");
				WriteErrMsg("chn1 低压过压故障后，未正常恢复", this, 5, 3);
				bRight = false;
				pcba.ov1 = false;
			}
		}
		else
		{
			bRight = false;
			pcba.ov1 = false;
			strOutstr.Format("ch1 低压过压恢复,过压故障记录异常");
			m_list1.AddString(strOutstr);
		}
	}

	/*----------------------------------欠压故障------------------------------------------*/

	//设置低压电源电压17.0v, 电流1.2A(欠压)
	LowPower_CTRL(POWER_SET, cfg[0].uv1, cfg[0].lowcur);
	//保持1000ms
	Sleep(6000);

	//ptc 0
	if (m_bLostPtc1 || m_bDevLost1 || 
		m_bDevLost2 || m_bLowPowerLost)
	{
		strOutstr.Format("%s", "NOK");
		m_listexcel.SetItemText(5, 3, strOutstr);
		if (m_bLostPtc1)
		{
			WriteErrMsg("chn1 通信丢失", this, 5, 3);
		}
		else if (m_bDevLost1)
		{
			WriteErrMsg("chn1 低压通信Rs232断开", this, 5, 3);
		}
		else if (m_bDevLost2)
		{
			WriteErrMsg("chn1 高压通信Rs485断开", this, 5, 3);
		}
		else if (m_bLowPowerLost)
		{
			WriteErrMsg("chn1 低压掉电", this, 5, 3);
		}

		pcba.uv1 = false;
		bRight = false;

		StopFlag = 1;
		KillTimer(3);
		WriteTestReport();
		_Log();

		//将低压电压调到预设值(24v/12v, 1.2A)
		LowPower_CTRL(POWER_SET, cfg[0].lowpre, cfg[0].lowcur);
		return bRight;
	}
	else
	{
		if (HVH_ECU_Frm2->HVH_WarnULoOutOfRang == 0x01)	//Under voltage error
		{
			strOutstr.Format("ch1 低压欠压故障记录正常");
			m_list1.AddString(strOutstr);
			m_list1.SetCurSel(m_list1.GetCount() - 1);
			m_listexcel.SetItemText(5, 3, "OK");

			bRight &= true;
			pcba.uv1 = true;
		}
		else
		{
			valuestr.Format("HVH_WarnULoOutOfRang::%d", HVH_ECU_Frm2->HVH_WarnULoOutOfRang);
			m_list1.AddString(valuestr);

			m_listexcel.SetItemText(5, 3, "NOK");
			WriteErrMsg("chn1 低压欠压故障记录异常", this, 5, 3);
			bRight = false;
			pcba.uv1 = false;
		}
	}

	//设置低压电源电压18.0v, 电流1.2A(过压恢复)
	LowPower_CTRL(POWER_SET, cfg[0].uv2, cfg[0].lowcur);
	//保持1000ms
	Sleep(6000);

	//ptc 0
	if (m_bLostPtc1 || m_bDevLost1 ||
		m_bDevLost2 ||m_bLowPowerLost)
	{
		strOutstr.Format("%s", "NOK");
		m_listexcel.SetItemText(5, 3, strOutstr);
		if (m_bLostPtc1)
		{
			WriteErrMsg("chn1 通信丢失", this, 5, 3);
		}
		else if (m_bDevLost1)
		{
			WriteErrMsg("chn1 低压通信Rs232断开", this, 5, 3);
		}
		else if (m_bDevLost2)
		{
			WriteErrMsg("chn1 高压通信Rs485断开", this, 5, 3);
		}
		else if (m_bLowPowerLost)
		{
			WriteErrMsg("chn1 低压掉电", this, 5, 3);
		}

		pcba.uv1 = false;
		bRight = false;

		StopFlag = 1;
		KillTimer(3);
		WriteTestReport();
		_Log();

		//将低压电压调到预设值(24v/12v, 1.2A)
		LowPower_CTRL(POWER_SET, cfg[0].lowpre, cfg[0].lowcur);
		return bRight;
	}
	else
	{
		if (bRight)
		{
			if (HVH_ECU_Frm2->HVH_WarnULoOutOfRang == 0x00)	//No issue
			{
				strOutstr.Format("ch1 低压欠压故障正常恢复");
				m_list1.AddString(strOutstr);
				m_list1.SetCurSel(m_list1.GetCount() - 1);
				m_listexcel.SetItemText(5, 3, "OK");

				bRight &= true;
				pcba.uv1 &= true;
			}
			else
			{
				m_listexcel.SetItemText(5, 3, "NOK");
				WriteErrMsg("chn1 低压欠压故障后，未正常恢复", this, 5, 3);
				bRight = false;
				pcba.uv1 = false;
			}
		}
		else
		{
			bRight = false;
			pcba.uv1 = false;
			strOutstr.Format("ch1 低压欠压恢复,欠压故障记录异常");
			m_list1.AddString(strOutstr);
		}
	}

	if (!bRight)
	{
		m_listexcel.SetItemText(5, 3, "NOK");
		WriteErrMsg("chn1 低压故障", this, 5, 3);
	}

	//将低压电压调到预设值(24v/12v, 1.2A)
	LowPower_CTRL(POWER_SET, cfg[0].lowpre, cfg[0].lowcur);

	return bRight;
}

//检测IGBT传感器电路
bool CACPSDlg::Check_IGBTCircuit()
{
	bool bRight = false;
	CString strOutstr;

	//ptc 0
	if (m_bLostPtc1 ||m_bDevLost1 ||
		m_bDevLost2 || m_bLowPowerLost)
	{
		strOutstr.Format("%s", "NOK");
		m_listexcel.SetItemText(9, 1, strOutstr);
		WriteErrMsg("chn1 通信丢失", this, 9, 1);
		
		if (m_bLostPtc1)
		{
			WriteErrMsg("chn1 通信丢失", this, 9, 1);
		}
		else if (m_bDevLost1)
		{
			WriteErrMsg("chn1 低压通信Rs232断开", this, 9, 1);
		}
		else if (m_bDevLost2)
		{
			WriteErrMsg("chn1 高压通信Rs485断开", this, 9, 1);
		}
		else if (m_bLowPowerLost)
		{
			WriteErrMsg("chn1 低压掉电", this, 9, 1);
		}

		StopFlag = 1;
		KillTimer(3);
		
		_Log();
		pcba.igbt_temp1 = false;
		bRight = false;
		WriteTestReport();
	}
	else
	{
		if (abs(HVH_ECU_Frm1->HVH_Coolant_In_Temp - 0x19) <= 2)	//IGBT Fail Detected
		{
			strOutstr.Format("ch1 IGBT符合设定值");
			m_list1.AddString(strOutstr);
			m_listexcel.SetItemText(9, 1, "OK");
			bRight = true;
			pcba.igbt_temp1 = true;
		}
		else
		{
			m_listexcel.SetItemText(9, 1, "NOK");
			WriteErrMsg("ch1 IGBT温度异常", this, 9, 1);
			bRight = false;
			pcba.igbt_temp1 = false;
		}
	}

	return bRight;
}

//检测PTC传感器电路
bool CACPSDlg::Check_PTCCircuit()
{
	bool bRight = false;
	CString strOutstr;

	//ptc 0
	if (m_bLostPtc1 ||m_bDevLost1 ||
		m_bDevLost2|| m_bLowPowerLost)
	{
		strOutstr.Format("%s", "NOK");
		m_listexcel.SetItemText(9, 2, strOutstr);

		if (m_bLostPtc1)
		{
			WriteErrMsg("chn1 通信丢失", this, 9, 2);
		}
		else if (m_bDevLost1)
		{
			WriteErrMsg("chn1 低压通信Rs232断开", this, 9, 2);
		}
		else if (m_bDevLost2)
		{
			WriteErrMsg("chn1 高压通信Rs485断开", this, 9, 2);
		}
		else if (m_bLowPowerLost)
		{
			WriteErrMsg("chn1 低压掉电", this, 9, 2);
		}

		StopFlag = 1;
		KillTimer(3);		
		_Log();

		pcba.ptc_temp1 = false;
		bRight = false;
		WriteTestReport();
	}
	else
	{
		if (abs(HVH_ECU_Frm1->HVH_Coolant_Out_Temp - 0x19) <= 2)	//PTC 冷却液出口温度
		{
			strOutstr.Format("ch1 PTC符合设定值");
			m_list1.AddString(strOutstr);
			m_listexcel.SetItemText(9, 2, "OK");
			bRight = true;
			pcba.ptc_temp1 = true;
		}
		else
		{
			m_listexcel.SetItemText(9, 2, "NOK");
			WriteErrMsg("ch1 PTC温度异常", this, 9, 2);
			bRight = false;
			pcba.ptc_temp1 = false;
		}
	}

	return bRight;
}

//检测PCB温度传感器电路
bool CACPSDlg::Check_PCBTempCircuit()
{
	bool bRight = false;
	float fEnvTemp = 0.0f;//环境温度
	CString strOutstr;

	//ptc 0
	if (m_bLostPtc1 ||m_bDevLost1 || 
		m_bDevLost2 || m_bLowPowerLost)
	{
		strOutstr.Format("%s", "NOK");
		m_listexcel.SetItemText(9, 3, strOutstr);

		if (m_bLostPtc1)
		{
			WriteErrMsg("chn1 通信丢失", this, 9, 3);
		}
		else if (m_bDevLost1)
		{
			WriteErrMsg("chn1 低压通信Rs232断开", this, 9, 3);
		}
		else if (m_bDevLost2)
		{
			WriteErrMsg("chn1 高压通信Rs485断开", this, 9, 3);
		}
		else if (m_bLowPowerLost)
		{
			WriteErrMsg("chn1 低压掉电", this, 9, 3);
		}

		StopFlag = 1;
		KillTimer(3);
		_Log();

		pcba.ptc_env_temp1 = false;
		bRight = false;
		WriteTestReport();
	}
	else
	{
		m_Mediator->highPower->ReadNTCvalue(0, &fEnvTemp);
		if (abs(HVH_ECU_Frm1->HVH_Coolant_Out_Temp - fEnvTemp) <= 2)	//PTC 冷却液出口温度
		{
			strOutstr.Format("ch1 PCB温度与检测工装内环境温度差值符合要求");
			m_list1.AddString(strOutstr);
			m_listexcel.SetItemText(9, 3, "OK");
			bRight = true;
			pcba.ptc_env_temp1 = true;
		}
		else
		{
			m_listexcel.SetItemText(9, 3, "NOK");
			WriteErrMsg("ch1 PCB温度与检测工装内环境温度差值超限", this, 9, 3);
			bRight = false;
			pcba.ptc_env_temp1 = false;
		}
	}

	return bRight;
}

//开始/暂停报文发送/接收
void CACPSDlg::StartStopMessage(bool bPause)
{
	if (bPause)
	{
		CAN_Handle->SuspendThread();
		LIN_Handle->SuspendThread();
		KillTimer(2);
		m_bSuspendThread = true;
	}
	else
	{
		if (m_bSuspendThread)
		{
			CAN_Handle->ResumeThread();
			LIN_Handle->ResumeThread();
			SetTimer(2, 500, NULL);
			m_bSuspendThread = false;
		}
	}
}

//功率测试
bool CACPSDlg::Run_Selected_PowerMode(CACPSDlg* pDlg)
{
	CString valuestr;
	CString strInValue;
	CString strWaitTime;
	float fDestValue = 0.0f, fDestValue5Kw = 0.0f;;
	float fPower1 = 0.0f, fPower2 = 0.0f;
	int nWaitTime = 0, nWaitTime5Kw = 0;	//2Kw,5Kw运行时长
	byte dest_value = 0x00, dest_value2 = 0x00;	//目标功率
	//从参数配置中读取目标功率
	int cfgindex = 0;
	byte value = 0x00;

	//从参数配置中读取目标功率
	fDestValue = cfg[0].max_ptc_power;
	nWaitTime = cfg[0].waitting_time;
	value = static_cast<byte>(fDestValue);

	if (gSelectedBUSAdapter == 0)	//CAN请求功率测试
	{
		fDestValue = cfg[cfgindex].max_ptc_power;
		nWaitTime = cfg[cfgindex].waitting_time;

		fDestValue5Kw = cfg[cfgindex].max_ptc_power_5Kw;
		nWaitTime5Kw = cfg[cfgindex].waitting_time_5Kw;

		dest_value = static_cast<byte>(fDestValue);
		dest_value2 = static_cast<byte>(fDestValue5Kw);
		////上高压
		pDlg->m_Mediator->Set_Dev_Vol_Curr(pDlg->m_highPwr, 600.0f, cfg[0].run_current);
		Sleep(3000);
		return Power_CTRL_Mode(pDlg, portPre, dest_value, nWaitTime, dest_value2, nWaitTime5Kw, &fPower1, &fPower2);
	}
	else //LIN请求功率测试
	{
		byte realPower;
		realPower = value * 0x14;

		while (nWaitTime > 0)
		{
			SendMsg(realPower, 0x0, 0x0, 0x1, true);//1~7Kw运行 (需增加温度目标值)
			Sleep(1000);//默认等待30s后读取总线功率信号值
			nWaitTime--;
		}
		//pcba.high_power = LIN_Slave_MSG1->WPTC_Power;
		
		if (pcba.high_power < cfg[g_cfgindex].power_range_low || pcba.high_power > cfg[g_cfgindex].power_range_high)
		{
			pcba.PowerMode_power = pcba.high_power;
			valuestr.Format("%4.2f", pcba.high_power);
			pDlg->m_listexcel.SetItemText(13, 1, valuestr);
			pDlg->m_bErrMsg |= pDlg->WriteErrMsg("功率控制模式故障", pDlg, 13, 1);
		}
		else
		{
			pcba.PowerMode_power = pcba.high_power;
			valuestr.Format("%4.2f Kw", pcba.high_power);
			pDlg->m_listexcel.SetItemText(13, 1, valuestr);
			return true;
		}
	}
	return false;
}

//功率控制模式
bool CACPSDlg::Power_CTRL_Mode(CACPSDlg* pDlg, CSerial* port, byte nPower, int nWaitTime, byte nPower_5Kw, int nWaitTime_5Kw, float* value, float* value2)
{
	bool bRight = false;
	int nLoop = 0, nTryTimes = 0;
	byte limit;
	float volvalue = 0.0f, curvalue = 0.0f, fRealPower1 = 0.0f, fRealPower2 = 0.0f;
	float fPower = 0.0f, fPower2 = 0.0f, fPower3 = 0.0f, fPower4 = 0.0f;
	float fPower_err = 0.0f, fPower_err2 = 0.0f, fPower_err3 = 0.0f, fPower_err4 = 0.0f;
	CString valuestr;
	long lExpTime;

	/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$2Kw功率测试$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

		limit = 0x0A * nPower;
		ECU_HVH_Frm1->HVH_MaxPower = limit;
		ECU_HVH_Frm1->HVH_TargetCoolantTemp = 0x50;
		ECU_HVH_Frm1->HVH_HeaterEnable = 0x01;
		ECU_HVH_Frm1->HVH_ActiveDischarge = 0x00;

		//PTC1
		valuestr.Format("%2.2f Kw", cfg[0].max_ptc_power);
		m_list1.AddString("目标功率::" + valuestr);

#ifdef _AVERAGE_POWER_CALC
		//测试前清空功率值容器
		m_8710PowerLst1.clear();
		m_8710PowerLst2.clear();
#endif

		Cal_UnitTest_Time(true, 0);
		while (1)
		{
			Cal_UnitTest_Time(false, &lExpTime);
#ifdef _AVERAGE_POWER_CALC
			if (lExpTime == cfg[0].waitting_time - 5) //开始功率搜集
			{
				pCapturePowerOutThread->ResumeThread();
				m_bStartCollectPower = true;
			}
#endif
			if (lExpTime > nWaitTime)
			{
				m_bStartCollectPower = false;
				break;
			}
		}

		//PTC1
		if (m_bLostPtc1 || m_bDevLost1 || 
			m_bDevLost2 || m_bLowPowerLost)
		{
			if (m_bLostPtc1)
			{
				WriteErrMsg("chn1 通信丢失", this, 13, 2);
			}
			else if (m_bDevLost1)
			{
				WriteErrMsg("chn1 低压通信Rs232断开", this, 13, 2);
			}
			else if (m_bDevLost2)
			{
				WriteErrMsg("chn1 高压通信Rs485断开", this, 13, 2);
			}
			else if (m_bLowPowerLost)
			{
				WriteErrMsg("chn1 低压掉电", this, 13, 2);
			}

			StopFlag = 1;
			SuspendThread(pSendMsgThread);
			return bRight;
		}
		else
		{
#ifdef _AVERAGE_POWER_CALC
			for (std::vector<float>::iterator it = m_8710PowerLst1.begin(); it != m_8710PowerLst1.end(); ++it)
			{
				fPower += *it;
			}
			g_fPower1 = fPower / m_8710PowerLst1.size();
#else
			m_highPwr->Get_8710Dev_Vol_Curr(&volvalue, &curvalue);
			g_fPower1 = volvalue * curvalue;
#endif
			valuestr.Format("%3.2f V / %3.2f A / %3.2f W", volvalue, curvalue, g_fPower1);
			m_listexcel.SetItemText(17, 3, valuestr);

			//PTC1	输出功率计算
			*value = g_fPower1 / 1000.0f;

			valuestr.Format("%2.2f W", g_fPower1);
			m_list1.AddString("chn1 功率计输出功率::" + valuestr);

			/************************power error calculate*****************************/
			fPower = abs(cfg[0].max_ptc_power * 1000 - *value * 1000.0f) / (cfg[0].max_ptc_power * 1000);
			fPower_err = fPower * 100;
			/*********************************************************************/

			valuestr.Format("%2.2f", fPower_err);
			m_list1.AddString("chn1 功率误差::" + valuestr + "%");
			m_list1.SetCurSel(m_list1.GetCount() - 1);

			//PTC-1 输出功率误差判断
			if (*value < 0)
			{
				*value = 0.0f;
				fPower_err = 0;
			}

			pcba.PowerMode_power = *value;
			pcba.PowerError_1 = fPower_err;
			if (fPower < cfg[0].max_ptc_power_error)
			{
				valuestr.Format("%2.2f Kw", *value);
				pDlg->m_listexcel.SetItemText(13, 2, valuestr);
				bRight = true;
			}
			else
			{
				bRight = false;
				valuestr.Format("%2.2f  Kw", *value);
				pDlg->m_listexcel.SetItemText(13, 2, valuestr);
				pDlg->WriteErrMsg("chn1 功率控制模式故障", pDlg, 13, 2);
			}
		}
		

/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/
		//8Kw请求功率
		{
			lExpTime = 0;
			limit = 0x0A * nPower_5Kw;
			ECU_HVH_Frm1->HVH_MaxPower = limit;
			ECU_HVH_Frm1->HVH_TargetCoolantTemp = 0x50;
			ECU_HVH_Frm1->HVH_HeaterEnable = 0x01;
			ECU_HVH_Frm1->HVH_ActiveDischarge = 0x00;

			valuestr.Format("%2.2f Kw", cfg[0].max_ptc_power_5Kw);
			m_list1.AddString("目标功率::" + valuestr);

#ifdef _AVERAGE_POWER_CALC
			//测试前清空功率值容器
			m_8710PowerLst1.clear();
			m_8710PowerLst2.clear();
#endif

			Cal_UnitTest_Time(true, 0);
			while (1)
			{
				Cal_UnitTest_Time(false, &lExpTime);

#ifdef _AVERAGE_POWER_CALC
				if (lExpTime == cfg[0].waitting_time_5Kw - 5) //开始功率搜集
				{
					m_bStartCollectPower = true;

					pCapturePowerOutThread = AfxBeginThread(Capture_PowerTH, this,
						THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
					pCapturePowerOutThread->m_bAutoDelete = TRUE;//线程结束时自动删除
					pCapturePowerOutThread->ResumeThread();
				}
#endif
				if (lExpTime > nWaitTime_5Kw)
				{
					m_bStartCollectPower = false;
					break;
				}
			}

			//PTC1
			if (m_bLostPtc1 || m_bDevLost1 || 
				m_bDevLost2 || m_bLowPowerLost)
			{
				if (m_bLostPtc1)
				{
					WriteErrMsg("chn1 通信丢失", this, 13, 2);
				}
				else if (m_bDevLost1)
				{
					WriteErrMsg("chn1 低压通信Rs232断开", this, 13, 2);
				}
				else if (m_bDevLost2)
				{
					WriteErrMsg("chn1 高压通信Rs485断开", this, 13, 2);
				}
				else if (m_bLowPowerLost)
				{
					WriteErrMsg("chn1 低压掉电", this, 13, 2);
				}

				StopFlag = 1;
				SuspendThread(pSendMsgThread);
				return bRight;
			}
			else
			{
#ifdef _AVERAGE_POWER_CALC
				for (std::vector<float>::iterator it = m_8710PowerLst1.begin(); it != m_8710PowerLst1.end(); ++it)
				{
					fPower += *it;
				}
				g_fPower1 = fPower / m_8710PowerLst1.size();
#else
				m_highPwr->Get_8710Dev_Vol_Curr(&volvalue, &curvalue);
				g_fPower1 = volvalue * curvalue;
#endif
				valuestr.Format("%3.2f V / %3.2f A / %3.2f W", volvalue, curvalue, g_fPower1);
				m_listexcel.SetItemText(17, 3, valuestr);

				//PTC1	输出功率计算
				*value = g_fPower1 / 1000.0f;

				valuestr.Format("%2.2f W", g_fPower1);
				m_list1.AddString("chn1 功率计输出功率::" + valuestr);

				float uPower = HVH_ECU_Frm1->HVH_ActualPower / 1000.0f;

				/************************power error calculate*****************************/
				fPower3 = abs(*value - uPower) / *value;
				fPower_err3 = fPower3 * 100;
				/*********************************************************************/

				valuestr.Format("%2.2f", fPower_err3);
				m_list1.AddString("chn1 功率误差::" + valuestr + "%");
				m_list1.SetCurSel(m_list1.GetCount() - 1);

				//PTC-1 输出功率误差判断
				if (*value < 0)
				{
					*value = 0.0f;
					fPower_err3 = 0;
				}

				pcba.PowerMode_power_2 = *value;
				pcba.PowerError_3 = fPower_err3;
				if (fPower3 < cfg[0].max_ptc_power_5Kw_error)
				{
					valuestr.Format("%2.2f Kw", *value);
					pDlg->m_listexcel.SetItemText(13, 3, valuestr);
					bRight &= true;
				}
				else
				{
					bRight = false;
					valuestr.Format("%2.2f  Kw", *value);
					pDlg->m_listexcel.SetItemText(13, 3, valuestr);
					pDlg->WriteErrMsg("chn1 功率控制模式故障", pDlg, 13, 3);
				}
			}
		}	
	
	return bRight;
}

//功率控制模式(CAN)
bool CACPSDlg::Power_CTRL_Mode_CAN(CACPSDlg* pDlg, CSerial* port, byte nPower, int nWaitTime, byte nPower_5Kw, int nWaitTime_5Kw, float* value, float* value2)
{
	bool bRight = false;
	int nLoop = 0, nTryTimes = 0;
	byte limit;
	float volvalue = 0.0f, curvalue = 0.0f, fRealPower1 = 0.0f, fRealPower2 = 0.0f;
	float fPower = 0.0f, fPower2 = 0.0f, fPower3 = 0.0f, fPower4 = 0.0f;
	float fPower_err = 0.0f, fPower_err2 = 0.0f, fPower_err3 = 0.0f, fPower_err4 = 0.0f;
	CString valuestr;
	long lExpTime;

	/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$2Kw功率测试$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

	//if (pDlg->m_bEEPWrote == FALSE)
	//{
	//	m_listexcel.SetItemText(17, 1, "NOK");
	//	m_bErrMsg |= pDlg->WriteErrMsg("chn1 标志位写入失败，未执行测试", pDlg, 17, 1);
	//	pcba.PowerMode_power = 0;
	//	pcba.PowerError_1 = 0;
	//	m_listexcel.SetItemText(18, 1, "NOK");
	//	m_bErrMsg |= pDlg->WriteErrMsg("chn1 标志位写入失败，未执行测试", pDlg, 18, 1);
	//	pcba.PowerMode_power_1 = 0;
	//	pcba.PowerError_2 = 0;
	//	bRight = false;
	//}
	//else
	{
		limit = 0x0A * nPower;
		ECU_HVH_Frm1->HVH_MaxPower = limit;
		ECU_HVH_Frm1->HVH_TargetCoolantTemp = 0x50;
		ECU_HVH_Frm1->HVH_HeaterEnable = 0x01;
		ECU_HVH_Frm1->HVH_ActiveDischarge = 0x00;

		//PTC1
		valuestr.Format("%2.2f Kw", cfg[0].max_ptc_power);
		m_list1.AddString("目标功率::" + valuestr);

#ifdef _AVERAGE_POWER_CALC
		//测试前清空功率值容器
		m_8710PowerLst1.clear();
		m_8710PowerLst2.clear();
#endif

		Cal_UnitTest_Time(true, 0);
		while (1)
		{
			Cal_UnitTest_Time(false, &lExpTime);
#ifdef _AVERAGE_POWER_CALC
			if (lExpTime == cfg[0].waitting_time - 5) //开始功率搜集
			{
				pCapturePowerOutThread->ResumeThread();
				m_bStartCollectPower = true;
			}
#endif
			if (lExpTime > nWaitTime)
			{
				m_bStartCollectPower = false;
				break;
			}
		}

		//PTC1
		if (m_bLostPtc1 || m_bDevLost1 || m_bDevLost2)
		{
			if (m_bLostPtc1)
				WriteErrMsg("chn1 通信丢失", this, 13, 2);
			else if (m_bDevLost1)
				WriteErrMsg("chn1 低压通信Rs232断开", this, 13, 2);
			else if (m_bDevLost2)
				WriteErrMsg("chn1 高压通信Rs485断开", this, 13, 2);
			return bRight;
		}
		else
		{
#ifdef _AVERAGE_POWER_CALC
			for (std::vector<float>::iterator it = m_8710PowerLst1.begin(); it != m_8710PowerLst1.end(); ++it)
			{
				fPower += *it;
			}
			g_fPower1 = fPower / m_8710PowerLst1.size();
#else
			m_highPwr->Get_8710Dev_Vol_Curr(&volvalue, &curvalue);
			g_fPower1 = volvalue* curvalue;
#endif
			valuestr.Format("%3.2f V / %3.2f A / %3.2f W", volvalue, curvalue, g_fPower1);
			m_listexcel.SetItemText(17, 3, valuestr);

			//PTC1	输出功率计算
			*value = g_fPower1 / 1000.0f;

			valuestr.Format("%2.2f W", g_fPower1);
			m_list1.AddString("chn1 功率计输出功率::" + valuestr);

			fPower = abs(cfg[0].max_ptc_power * 1000 - *value * 1000.0f) / (cfg[0].max_ptc_power * 1000);
			fPower_err = fPower * 100;
			valuestr.Format("%2.2f", fPower_err);
			m_list1.AddString("chn1 功率误差::" + valuestr + "%");
			m_list1.SetCurSel(m_list1.GetCount() - 1);

			//PTC-1 输出功率误差判断
			if (*value < 0)
			{
				*value = 0.0f;
				fPower_err = 0;
			}
			pcba.PowerMode_power = *value;
			pcba.PowerError_1 = fPower_err;
			if (fPower < cfg[0].max_ptc_power_error)
			{
				valuestr.Format("%2.2f Kw", *value);
				pDlg->m_listexcel.SetItemText(13, 2, valuestr);
				bRight = true;
			}
			else
			{
				bRight = false;
				valuestr.Format("%2.2f  Kw", *value);
				pDlg->m_listexcel.SetItemText(13, 2, valuestr);
				pDlg->WriteErrMsg("chn1 功率控制模式故障", pDlg, 13, 2);
			}
		}


/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/
		//8Kw请求功率

		//if (pDlg->m_bEEPWrote2 == FALSE)
			//{
			//	m_listexcel.SetItemText(17, 2, "NOK");
			//	m_bErrMsg |= pDlg->WriteErrMsg("chn2 标志位写入失败，未执行测试", pDlg, 17, 2);
			//	pcba.PowerMode_power_3 = 0;
			//	pcba.PowerError_3 = 0;
			//	m_listexcel.SetItemText(18, 2, "NOK");
			//	m_bErrMsg |= pDlg->WriteErrMsg("chn2 标志位写入失败，未执行测试", pDlg, 18, 2);
			//	pcba.PowerMode_power_4 = 0;
			//	pcba.PowerError_4 = 0;
			//	bRight = false;
			//}
			//else	
		{
			lExpTime = 0;
			limit = /*0x1F*/ 0x0A * nPower_5Kw;
			ECU_HVH_Frm1->HVH_MaxPower = limit;
			ECU_HVH_Frm1->HVH_TargetCoolantTemp = 0x50;
			ECU_HVH_Frm1->HVH_HeaterEnable = 0x01;
			ECU_HVH_Frm1->HVH_ActiveDischarge = 0x00;

			valuestr.Format("%2.2f Kw", cfg[0].max_ptc_power_5Kw);
			m_list1.AddString("目标功率::" + valuestr);

#ifdef _AVERAGE_POWER_CALC
			//测试前清空功率值容器
			m_8710PowerLst1.clear();
			m_8710PowerLst2.clear();
#endif

			Cal_UnitTest_Time(true, 0);
			while (1)
			{
				Cal_UnitTest_Time(false, &lExpTime);

#ifdef _AVERAGE_POWER_CALC
				if (lExpTime == cfg[0].waitting_time_5Kw - 5) //开始功率搜集
				{
					m_bStartCollectPower = true;

					pCapturePowerOutThread = AfxBeginThread(Capture_PowerTH, this,
						THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
					pCapturePowerOutThread->m_bAutoDelete = TRUE;//线程结束时自动删除
					pCapturePowerOutThread->ResumeThread();
				}
#endif
				if (lExpTime > nWaitTime_5Kw)
				{
					m_bStartCollectPower = false;
					break;
				}
			}
			//PTC1

			if (m_bLostPtc1 || m_bDevLost1 || m_bDevLost2)
			{
				if (m_bLostPtc1)
					WriteErrMsg("chn1 通信丢失", this, 13, 3);
				else if (m_bDevLost1)
					WriteErrMsg("chn1 低压通信Rs232断开", this, 13, 3);
				else if (m_bDevLost2)
					WriteErrMsg("chn1 高压通信Rs485断开", this, 13, 3);
				return bRight;
			}
			else
			{
#ifdef _AVERAGE_POWER_CALC
				for (std::vector<float>::iterator it = m_8710PowerLst1.begin(); it != m_8710PowerLst1.end(); ++it)
				{
					fPower += *it;
				}
				g_fPower1 = fPower / m_8710PowerLst1.size();
#else
				m_highPwr->Get_8710Dev_Vol_Curr(&volvalue, &curvalue);
				g_fPower1 = volvalue* curvalue;
#endif
				valuestr.Format("%3.2f V / %3.2f A / %3.2f W", volvalue, curvalue, g_fPower1);
				m_listexcel.SetItemText(17, 3, valuestr);

				//PTC1	输出功率计算
				*value = g_fPower1 / 1000.0f;

				valuestr.Format("%2.2f W", g_fPower1);
				m_list1.AddString("chn1 功率计输出功率::" + valuestr);

				float uPower = HVH_ECU_Frm1->HVH_ActualPower;
				fPower3 = abs(*value - uPower) / *value;
				fPower_err3 = fPower3 * 100;

				valuestr.Format("%2.2f", fPower_err3);
				m_list1.AddString("chn1 功率误差::" + valuestr + "%");
				m_list1.SetCurSel(m_list1.GetCount() - 1);

				//PTC-1 输出功率误差判断
				if (*value < 0)
				{
					*value = 0.0f;
					fPower_err3 = 0;
				}

				pcba.PowerMode_power_2 = *value;
				pcba.PowerError_3 = fPower_err3;
				if (fPower3 < cfg[0].max_ptc_power_5Kw_error)
				{
					valuestr.Format("%2.2f Kw", *value / 1000.0f);
					pDlg->m_listexcel.SetItemText(13, 3, valuestr);
					bRight &= true;
				}
				else
				{
					bRight = false;
					valuestr.Format("%2.2f  Kw", *value / 1000.0f);
					pDlg->m_listexcel.SetItemText(13, 3, valuestr);
					pDlg->WriteErrMsg("chn1 功率控制模式故障", pDlg, 13, 3);
				}
			}
		}

		return bRight;
	}
}

//高压电压诊断
bool CACPSDlg::HighVol_Diagnostic()
{
	bool bRight = false, bStepTestResult= false;
	CString strValue, valuestr;
	float fVol1 = 0.0f, fVol2 = 0.0f;
	byte pKu[2] = { 0x00, 0x00 };
		 
	//高压过压(900v)
	bRight = HighVol_OverVoltage();
	if (!bRight)
	{
		if (m_bLostPtc1 || m_bDevLost1 ||
			m_bDevLost2 || m_bLowPowerLost)
			return bRight;
	}
	//高压欠压(380v)
	bRight &= HighVol_UnderVoltage();

	return bRight;
}

bool CACPSDlg::HighVol_OverVoltage()
{
	bool bRight = false;
	CString strValue, valuestr;
	float fVol1 = 0.0f, fVol2 = 0.0f;
	byte pKu[2] = { 0x00, 0x00 };

	//高压过压(900v)
	m_Mediator->Set_Dev_Vol_Curr(m_highPwr, cfg[0].highvol_over_vol, cfg[0].run_current);
	strValue.Format("高压电源电压设置为：vol->%f v, curr->%f A", cfg[0].highvol_over_vol, cfg[0].run_current);
	ShowStepMessage(strValue, 0, 0, pKu);

	//等待高压到达900v(3s后退出等待)
	//Waitfor_8710ArrivedDstVol(cfg[0].highvol_over_vol, CALI_POWRER_WAIT_TIME, &fVol1, &fVol2);
	Sleep(6000);

	//ptc0
	{
		if (m_bLostPtc1 || m_bDevLost1|| 
			m_bDevLost2 || m_bLowPowerLost)
		{
			valuestr.Format("%s", "NOK");
			m_listexcel.SetItemText(17, 1, valuestr);

			if (m_bLostPtc1)
			{
				WriteErrMsg("chn1 通信丢失", this, 17, 1);
			}
			else if (m_bDevLost1)
			{
				WriteErrMsg("chn1 低压通信Rs232断开", this, 17, 1);
			}
			else if (m_bDevLost2)
			{
				WriteErrMsg("chn1 高压通信Rs485断开", this, 17, 1);
			}
			else if (m_bLowPowerLost)
			{
				WriteErrMsg("chn1 低压掉电", this, 17, 1);
			}

			StopFlag = 1;
			KillTimer(3);
			pcba.highvol_overvol1 = false;
			bRight = false;
			return bRight;
		}
		else
		{
			if (HVH_ECU_Frm2->HVH_WarnHVOutOfRng == 0x02)		//OverVoltage
			{
				valuestr.Format("%s", "OK");
				m_listexcel.SetItemText(17, 1, valuestr);
				m_list1.AddString("chn1 PTC高压过压故障测试成功");
				m_list1.SetCurSel(m_list1.GetCount() - 1);

				m_listexcel.SetColor(17, 1, RGB(255, 255, 255));
				m_listexcel.Invalidate();

				pcba.highvol_overvol1 = true;
				bRight = true;
			}
			else
			{
				valuestr.Format("%s", "NOK");
				m_listexcel.SetItemText(17, 1, valuestr);
				WriteErrMsg("chn1 PTC高压过压测试失败", this, 17, 1);

				m_list1.AddString("Fault injection:");
				valuestr.Format("chn1 HVH_WarnHVOutOfRng:%d", HVH_ECU_Frm2->HVH_WarnHVOutOfRng);

				pcba.highvol_overvol1 = false;
				bRight = false;
			}
		}
	}

	//高压电压调到过压恢复值(880v)
	m_Mediator->Set_Dev_Vol_Curr(m_highPwr, cfg[0].highvol_over_restore_vol, cfg[0].run_current);
	strValue.Format("高压电源电压设置为：vol->%f v, curr->%f A", cfg[0].highvol_over_restore_vol, cfg[0].run_current);
	ShowStepMessage(strValue, 0, 0, pKu);

	//等待高压到达880v(3s后退出等待)
	//Waitfor_8710ArrivedDstVol(cfg[0].highvol_over_restore_vol, CALI_POWRER_WAIT_TIME, &fVol1, &fVol2);
	Sleep(6000);
	//ptc0
	{
		if (m_bLostPtc1 || m_bDevLost1 || 
			m_bDevLost2 || m_bLowPowerLost)
		{
			valuestr.Format("%s", "NOK");
			m_listexcel.SetItemText(17, 1, valuestr);

			if (m_bLostPtc1)
			{
				WriteErrMsg("chn1 通信丢失", this, 17, 1);
			}
			else if (m_bDevLost1)
			{
				WriteErrMsg("chn1 低压通信Rs232断开", this, 17, 1);
			}
			else if (m_bDevLost2)
			{
				WriteErrMsg("chn1 高压通信Rs485断开", this, 17, 1);
			}
			else if (m_bLowPowerLost)
			{
				WriteErrMsg("chn1 低压掉电", this, 17, 1);
			}

			StopFlag = 1;
			KillTimer(3);
			pcba.highvol_overvol1 = false;
			bRight = false;
			return bRight;
		}
		else
		{
			if (HVH_ECU_Frm2->HVH_WarnHVOutOfRng == 0x00)		//No Issue
			{
				valuestr.Format("%s", "OK");
				m_listexcel.SetItemText(17, 1, valuestr);
				m_list1.AddString("chn1 PTC高压过压故障测试成功");
				m_list1.SetCurSel(m_list1.GetCount() - 1);

				m_listexcel.SetColor(17, 1, RGB(255, 255, 255));
				m_listexcel.Invalidate();

				pcba.highvol_overvol1 = true;
				bRight &= true;
			}
			else
			{
				valuestr.Format("%s", "NOK");
				m_listexcel.SetItemText(17, 1, valuestr);
				WriteErrMsg("chn1 PTC高压过压测试失败", this, 17, 1);

				m_list1.AddString("Fault removed:");
				valuestr.Format("chn1 HVH_WarnHVOutOfRng:%d", HVH_ECU_Frm2->HVH_WarnHVOutOfRng);

				pcba.highvol_overvol1 = false;
				bRight = false;
			}
		}
	}

	return bRight;
}

bool CACPSDlg::HighVol_UnderVoltage()
{
	bool bRight = false;
	CString strValue, valuestr;
	float fVol1 = 0.0f, fVol2 = 0.0f;
	byte pKu[2] = { 0x00, 0x00 };

	//高压过压(380v)
	m_Mediator->Set_Dev_Vol_Curr(m_highPwr, cfg[0].highvol_under_vol, cfg[0].run_current);
	strValue.Format("高压电源电压设置为：vol->%f v, curr->%f A", cfg[0].highvol_under_vol, cfg[0].run_current);
	ShowStepMessage(strValue, 0, 0, pKu);

	//等待高压到达380v(3s后退出等待)
	//Waitfor_8710ArrivedDstVol(cfg[0].highvol_under_vol, CALI_POWRER_WAIT_TIME, &fVol1, &fVol2);
	Sleep(10000);

	//ptc0
	{
		if (m_bLostPtc1 ||m_bDevLost1 ||
			m_bDevLost2 || m_bLowPowerLost)
		{
			valuestr.Format("%s", "NOK");
			m_listexcel.SetItemText(17, 1, valuestr);
			if (m_bLostPtc1)
			{
				WriteErrMsg("chn1 通信丢失", this, 17, 1);
			}
			else if (m_bDevLost1)
			{
				WriteErrMsg("chn1 低压通信Rs232断开", this, 17, 1);
			}
			else if (m_bDevLost2)
			{
				WriteErrMsg("chn1 高压通信Rs485断开", this, 17, 1);
			}
			else if (m_bLowPowerLost)
			{
				WriteErrMsg("chn1 低压掉电", this, 17, 1);
			}

			StopFlag = 1;
			KillTimer(3);

			pcba.highvol_undervol1 = false;
			bRight = false;
			return bRight;
		}		
		else
		{
			if (HVH_ECU_Frm2->HVH_WarnHVOutOfRng == 0x01)		//UnderVoltage
			{
				valuestr.Format("%s", "OK");
				m_listexcel.SetItemText(17, 1, valuestr);
				m_list1.AddString("chn1 PTC高压欠压故障测试成功");
				m_list1.SetCurSel(m_list1.GetCount() - 1);

				m_listexcel.SetColor(17, 1, RGB(255, 255, 255));
				m_listexcel.Invalidate();

				pcba.highvol_undervol1 = true;
				bRight = true;
			}
			else
			{
				valuestr.Format("%s", "NOK");
				m_listexcel.SetItemText(17, 1, valuestr);
				WriteErrMsg("chn1 PTC高压欠压测试失败", this, 17, 1);

				m_list1.AddString("Fault injection:");
				valuestr.Format("chn1 HVH_WarnHVOutOfRng:%d", HVH_ECU_Frm2->HVH_WarnHVOutOfRng);

				pcba.highvol_undervol1 = false;
				bRight = false;
			}
		}
	}

	//高压电压调到过压恢复值(400v)
	m_Mediator->Set_Dev_Vol_Curr(m_highPwr, cfg[0].highvol_under_restore_vol, cfg[0].run_current);
	strValue.Format("高压电源电压设置为：vol->%f v, curr->%f A", cfg[0].highvol_under_restore_vol, cfg[0].run_current);
	ShowStepMessage(strValue, 0, 0, pKu);

	//等待高压到达400v(3s后退出等待)
	//Waitfor_8710ArrivedDstVol(cfg[0].highvol_under_restore_vol, CALI_POWRER_WAIT_TIME, &fVol1, &fVol2);
	Sleep(3000);

	//ptc0
	{
		if (m_bLostPtc1 || m_bDevLost1 || 
			m_bDevLost2 || m_bLowPowerLost)
		{
			valuestr.Format("%s", "NOK");
			m_listexcel.SetItemText(17, 1, valuestr);
			if (m_bLostPtc1)
				WriteErrMsg("chn1 通信丢失", this, 17, 1);
			else if (m_bDevLost1)
				WriteErrMsg("chn1 低压通信Rs232断开", this, 17, 1);
			else if (m_bDevLost2)
				WriteErrMsg("chn1 高压通信Rs485断开", this, 17, 1);
			else if (m_bLowPowerLost)
				WriteErrMsg("chn1 低压掉电", this, 17, 1);

			StopFlag = 1;
			KillTimer(3);

			pcba.highvol_undervol1 = false;
			bRight = false;
			return bRight;
		}
		else
		{
			if (HVH_ECU_Frm2->HVH_WarnHVOutOfRng == 0x00)		//No Issue
			{
				valuestr.Format("%s", "OK");
				m_listexcel.SetItemText(17, 1, valuestr);
				m_list1.AddString("chn1 PTC高压欠压故障测试成功");
				m_list1.SetCurSel(m_list1.GetCount() - 1);

				m_listexcel.SetColor(17, 1, RGB(255, 255, 255));
				m_listexcel.Invalidate();

				pcba.highvol_undervol1 = true;
				bRight &= true;
			}
			else
			{
				valuestr.Format("%s", "NOK");
				m_listexcel.SetItemText(17, 1, valuestr);
				WriteErrMsg("chn1 PTC高压欠压测试失败", this, 17, 1);

				m_list1.AddString("Fault removed:");
				valuestr.Format("chn1 HVH_WarnHVOutOfRng:%d", HVH_ECU_Frm2->HVH_WarnHVOutOfRng);

				pcba.highvol_undervol1 = false;
				bRight = false;
			}
		}
	}

	return bRight;
}

bool CACPSDlg::WriteErrMsg(CString msg, CACPSDlg *pDlg, int x, int y)
{
	pDlg->m_list1.AddString(msg);
	int nCnt = pDlg->m_list1.GetCount() - 1;
	pDlg->m_list1.SetCurSel(nCnt);

	if (x != 0 && y != 0)
	{
		m_listexcel.SetColor(x, y, RGB(255, 50, 50));
		m_listexcel.Invalidate();
	}

	return true;
}

void CACPSDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (nType == SIZE_RESTORED || nType == SIZE_MAXIMIZED)
	{
		ReSize();
	}
}

void CACPSDlg::ReSize()
{
	float FirstPos[2];
	POINT Newp; //获取现在对话框的大小  
	CRect recta;
	GetClientRect(&recta);     //取客户区大小    
	Newp.x = recta.right - recta.left;
	Newp.y = recta.bottom - recta.top;
	FirstPos[0] = (float)Newp.x / old.x;
	FirstPos[1] = (float)Newp.y / old.y;
	CRect Rect;
	int woc;
	CPoint OldTLPoint, TLPoint; //左上角  
	CPoint OldBRPoint, BRPoint; //右下角  
	HWND  hwndChild = ::GetWindow(m_hWnd, GW_CHILD);  //列出所有控件    
	while (hwndChild)
	{
		woc = ::GetDlgCtrlID(hwndChild);//取得ID 
		//if (woc == IDC_COMBOCFG)
		//{
		//	GetDlgItem(woc)->GetWindowRect(Rect);
		//	ScreenToClient(Rect);
		//	OldTLPoint = Rect.TopLeft();
		//	TLPoint.x = long(OldTLPoint.x * 1);
		//	TLPoint.y = long(OldTLPoint.y * 1);
		//	OldBRPoint = Rect.BottomRight();
		//	BRPoint.x = long(OldBRPoint.x * 1);
		//	BRPoint.y = long(OldBRPoint.y *1);
		//	Rect.SetRect(TLPoint, BRPoint);
		//	GetDlgItem(woc)->MoveWindow(Rect, TRUE);
		//	hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT);
		//}
		//else
		{
			GetDlgItem(woc)->GetWindowRect(Rect);
			ScreenToClient(Rect);
			OldTLPoint = Rect.TopLeft();
			TLPoint.x = long(OldTLPoint.x*FirstPos[0]);
			TLPoint.y = long(OldTLPoint.y*FirstPos[1]);
			OldBRPoint = Rect.BottomRight();//+needed
			BRPoint.x = long(OldBRPoint.x *FirstPos[0]);
			BRPoint.y = long(OldBRPoint.y *FirstPos[1]);
			Rect.SetRect(TLPoint, BRPoint);
			GetDlgItem(woc)->MoveWindow(Rect, TRUE);
			hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT);
		}
	}
	old = Newp;

	Invalidate();
}

void CACPSDlg::OnExit()
{
	// TODO: Add your control notification handler code here
	gHighVolTimes = 0;
}

void CACPSDlg::OnSelchangeCombo1()
{
	// TODO: Add your control notification handler code here
}

void CACPSDlg::WriteExcelData(CString strBookPath)
{
	CString tempstr;
	OleInitialize(NULL);
	COleVariant covOptional((long)DISP_E_PARAMNOTFOUND, VT_ERROR);
	if (!app.CreateDispatch("Excel.Application"))
	{
		this->MessageBox("无法创建Excel应用！");
	}

	/*得到工作簿容器*/
	books.AttachDispatch(app.GetWorkbooks());

	/*打开一个工作簿，如不存在，则新增一个工作簿*/
	try
	{
		/*打开一个工作簿*/
		lpDisp = books.Open(strBookPath,
			covOptional, covOptional, covOptional, covOptional, covOptional,
			covOptional, covOptional, covOptional, covOptional, covOptional,
			covOptional, covOptional, covOptional, covOptional);
		book.AttachDispatch(lpDisp);
	}
	catch (...)
	{
		CString temppath;
		temppath.Format("%s 文件不存在,请将对应Excel模板拷贝到程序根目录！", strBookPath);
		AfxMessageBox(temppath);

		return;
	}

	//得到工作簿中的Sheet的容器
	sheets.AttachDispatch(book.GetSheets());
	CString strSheetName;
	sheet=sheets.GetItem(COleVariant((short)1));

	strSheetName = sheet.GetName();
	try
	{
		/*打开一个已有的Sheet*/
		lpDisp = sheets.GetItem(COleVariant(strSheetName));
		sheet.AttachDispatch(lpDisp);
	}
	catch (...) //创建一个新的Sheet
	{
		AfxMessageBox("Sheet1文件不存在,请将对应Sheet1模板拷贝到excel中！");
		return;
	}

	erange.AttachDispatch(sheet.GetRange(COleVariant("A1"), COleVariant("A1")), TRUE);
	erange.Merge(COleVariant((long)0));

	tempstr.Format("二维码序列号\n（唯一性编号）\n");
	erange.SetValue2(COleVariant(tempstr));
	//设置列宽
	erange.AttachDispatch(erange.GetItem(COleVariant((long)1), vtMissing).pdispVal, TRUE);
	erange.SetColumnWidth(COleVariant((long)20)); 
	//设置行高
	erange.AttachDispatch(sheet.GetRows(), TRUE);
	erange.AttachDispatch(erange.GetItem(COleVariant((long)1), vtMissing).pdispVal);
	erange.SetRowHeight(COleVariant((long)90));

	//PTC1 序列号
	erange.AttachDispatch(sheet.GetRange(COleVariant("A2"), COleVariant("A2")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr.Format("%s\n", pcba.serise_num1.c_str());
	erange.SetValue2(COleVariant(tempstr));
	////PTC2 序列号
	//erange.AttachDispatch(sheet.GetRange(COleVariant("A3"), COleVariant("A3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//tempstr.Format("%s\n", pcba.serise_num2.c_str());
	//erange.SetValue2(COleVariant(tempstr));

	//测试日期
	erange.AttachDispatch(sheet.GetRange(COleVariant("B1"), COleVariant("B2")), TRUE);
	erange.Merge(COleVariant((long)0));
	CTime time = CTime::GetCurrentTime();
	CString curdata = time.Format("%Y-%m-%d");
	tempstr.Format("测试日期\n%s", curdata);
	erange.SetValue2(COleVariant(tempstr));

	//标志位
	erange.AttachDispatch(sheet.GetRange(COleVariant("C1"), COleVariant("C1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = " 标志位 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("C2"), COleVariant("C2")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.eep_mark_byte1)
		tempstr = "OK";
	else
		tempstr = "NOK";
	tempstr.Format("%s", tempstr);
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("C3"), COleVariant("C3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//if (pcba.eep_mark_byte2)
	//	tempstr = "OK";
	//else
	//	tempstr = "NOK";
	//tempstr.Format("%s", tempstr);
	//erange.SetValue2(COleVariant(tempstr));

	//低压电流
	erange.AttachDispatch(sheet.GetRange(COleVariant("D1"), COleVariant("D1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = " 低压电流 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("D2"), COleVariant("D2")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.lowpwr_curr1)
		tempstr = "OK";
	else
		tempstr = "NOK";
	tempstr.Format("%s", tempstr);
	erange.SetValue2(COleVariant(tempstr));

	////erange.AttachDispatch(sheet.GetRange(COleVariant("D3"), COleVariant("D3")), TRUE);
	////erange.Merge(COleVariant((long)0));
	////if (pcba.lowpwr_curr2)
	////	tempstr = "OK";
	////else
	////	tempstr = "NOK";
	////tempstr.Format("%s", tempstr);
	////erange.SetValue2(COleVariant(tempstr));

	//报文周期
	erange.AttachDispatch(sheet.GetRange(COleVariant("E1"), COleVariant("E1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = "  报文周期 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("E2"), COleVariant("E2")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.cycle_time1)
		tempstr = "OK";
	else
		tempstr = "NOK";
	tempstr.Format("%s", tempstr);
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("E3"), COleVariant("E3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//if (pcba.cycle_time2)
	//	tempstr = "OK";
	//else
	//	tempstr = "NOK";
	//tempstr.Format("%s", tempstr);
	//erange.SetValue2(COleVariant(tempstr));

	//项目信息
	erange.AttachDispatch(sheet.GetRange(COleVariant("F1"), COleVariant("F1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = " 项目信息 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("F2"), COleVariant("F2")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.project_info1)
		tempstr = "OK";
	else
		tempstr = "NOK";
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("F3"), COleVariant("F3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//if (pcba.uv1_1)
	//	tempstr = "OK";
	//else
	//	tempstr = "NOK";
	//erange.SetValue2(COleVariant(tempstr));

	//软件版本
	erange.AttachDispatch(sheet.GetRange(COleVariant("G1"), COleVariant("G1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = " 软件版本 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("G2"), COleVariant("G2")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.software_ver1)
		tempstr = "OK";
	else
		tempstr = "NOK";
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("G3"), COleVariant("G3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//if (pcba.software_ver1)
	//	tempstr = "OK";
	//else
	//	tempstr = "NOK";
	//erange.SetValue2(COleVariant(tempstr));

	//低压过压
	erange.AttachDispatch(sheet.GetRange(COleVariant("H1"), COleVariant("H1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = " 低压过压 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("H2"), COleVariant("H2")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.ov1)
		tempstr = "OK";
	else
		tempstr = "NOK";
	erange.SetValue2(COleVariant(tempstr));

	////erange.AttachDispatch(sheet.GetRange(COleVariant("H3"), COleVariant("H3")), TRUE);
	////erange.Merge(COleVariant((long)0));
	////if (pcba.ov2)
	////	tempstr = "OK";
	////else
	////	tempstr = "NOK";
	////erange.SetValue2(COleVariant(tempstr));

	//低压欠压
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("I1"), COleVariant("I1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = " 低压欠压 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("I2"), COleVariant("I2")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.uv1)
		tempstr = "OK";
	else
		tempstr = "NOK";
	tempstr.Format("%s", tempstr);
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("I3"), COleVariant("I3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//if (pcba.uv2)
	//	tempstr = "OK";
	//else
	//	tempstr = "NOK";
	//tempstr.Format("%s", tempstr);
	//erange.SetValue2(COleVariant(tempstr));

	//igbt温度传感器
	erange.AttachDispatch(sheet.GetRange(COleVariant("J1"), COleVariant("J1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = " IGBT温度传感器 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("J2"), COleVariant("J2")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.igbt_temp1)
		tempstr = "OK";
	else
		tempstr = "NOK";
	tempstr.Format("%s", tempstr);
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("J3"), COleVariant("J3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//if (pcba.igbt_temp2)
	//	tempstr = "OK";
	//else
	//	tempstr = "NOK";
	//tempstr.Format("%s", tempstr);
	//erange.SetValue2(COleVariant(tempstr));

	//ptc传感器
	erange.AttachDispatch(sheet.GetRange(COleVariant("K1"), COleVariant("K1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = " PTC传感器 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("K2"), COleVariant("K2")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.ptc_temp1)
		tempstr = "OK";
	else
		tempstr = "NOK";
	tempstr.Format("%s", tempstr);
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("K3"), COleVariant("K3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//if (pcba.ptc_temp2)
	//	tempstr = "OK";
	//else
	//	tempstr = "NOK";
	//tempstr.Format("%s", tempstr);
	//erange.SetValue2(COleVariant(tempstr));

	// PTC温度传感器
	/*erange.AttachDispatch(sheet.GetRange(COleVariant("L1"), COleVariant("L1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = " PTC温度传感器 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("L2"), COleVariant("L2")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.ptc_env_temp1)
		tempstr = "OK";
	else
		tempstr = "NOK";
	tempstr.Format("%s", tempstr);
	erange.SetValue2(COleVariant(tempstr));

	erange.AttachDispatch(sheet.GetRange(COleVariant("L3"), COleVariant("L3")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.ptc_env_temp2)
		tempstr = "OK";
	else
		tempstr = "NOK";
	tempstr.Format("%s", tempstr);
	erange.SetValue2(COleVariant(tempstr));*/

	//功率控制
	erange.AttachDispatch(sheet.GetRange(COleVariant("M1"), COleVariant("M1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = "PTC功率控制(4Kw)/ \t\n 误差(%)";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("M2"), COleVariant("M2")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr.Format("%2.2f / %2.2f", pcba.PowerMode_power, pcba.PowerError_1);
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("M3"), COleVariant("M3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//tempstr.Format("%2.2f / %2.2f", pcba.PowerMode_power_1, pcba.PowerError_2);
	//erange.SetValue2(COleVariant(tempstr));

	erange.AttachDispatch(sheet.GetRange(COleVariant("N1"), COleVariant("N1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = "PTC功率控制(8Kw) / \t\n 误差(%)";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("N2"), COleVariant("N2")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr.Format("%2.2f / %2.2f", pcba.PowerMode_power_2, pcba.PowerError_3);
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("N3"), COleVariant("N3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//tempstr.Format("%2.2f / %2.2f", pcba.PowerMode_power_3, pcba.PowerError_4);
	//erange.SetValue2(COleVariant(tempstr));

#ifdef _CALI_POWERSLOPE
	//功率校准后的功率值
	erange.AttachDispatch(sheet.GetRange(COleVariant("O1"), COleVariant("O1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = "PTC功率校准后功率值 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("O2"), COleVariant("O2")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr.Format("%2.2f ", pcba.fCalied_Power1);
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("O3"), COleVariant("O3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//tempstr.Format("%2.2f ", pcba.fCalied_Power2);
	//erange.SetValue2(COleVariant(tempstr));

	//功率校准后的功率误差
	erange.AttachDispatch(sheet.GetRange(COleVariant("P1"), COleVariant("P1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = "PTC功率校准后功率误差 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("P2"), COleVariant("P2")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr.Format("%2.2f ", pcba.fCalied_PowerErr1);
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("P3"), COleVariant("P3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//tempstr.Format("%2.2f ", pcba.fCalied_PowerErr2);
	//erange.SetValue2(COleVariant(tempstr));

	//功率校准斜率1
	erange.AttachDispatch(sheet.GetRange(COleVariant("Q1"), COleVariant("Q1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = "PTC功率校准斜率1 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("Q2"), COleVariant("Q2")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr.Format("%2.6f ", pcba.fCalied_50Slope1);
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("Q3"), COleVariant("Q3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//tempstr.Format("%2.6f ", pcba.fCalied_50Slope2);
	//erange.SetValue2(COleVariant(tempstr));

	//功率校准斜率2
	erange.AttachDispatch(sheet.GetRange(COleVariant("R1"), COleVariant("R1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = "PTC功率校准斜率2 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("R2"), COleVariant("R2")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr.Format("%2.6f ", pcba.fCalied_100Slope1);
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("R3"), COleVariant("R3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//tempstr.Format("%2.6f ", pcba.fCalied_100Slope2);
	//erange.SetValue2(COleVariant(tempstr));


#endif

	//高压过压
	erange.AttachDispatch(sheet.GetRange(COleVariant("S1"), COleVariant("S1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = " 高压过压 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("S2"), COleVariant("S2")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.highvol_overvol1)
		tempstr = "OK";
	else
		tempstr = "NOK";
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("S3"), COleVariant("S3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//if (pcba.highvol_overvol2)
	//	tempstr = "OK";
	//else
	//	tempstr = "NOK";
	//erange.SetValue2(COleVariant(tempstr));

	//高压欠压
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("T1"), COleVariant("T1")), TRUE);
	erange.Merge(COleVariant((long)0));
	tempstr = " 高压欠压 ";
	erange.SetValue2(COleVariant(tempstr));
	erange.AttachDispatch(sheet.GetRange(COleVariant("T2"), COleVariant("T2")), TRUE);
	erange.Merge(COleVariant((long)0));
	if (pcba.highvol_undervol1)
		tempstr = "OK";
	else
		tempstr = "NOK";
	tempstr.Format("%s", tempstr);
	erange.SetValue2(COleVariant(tempstr));

	//erange.AttachDispatch(sheet.GetRange(COleVariant("T3"), COleVariant("T3")), TRUE);
	//erange.Merge(COleVariant((long)0));
	//if (pcba.highvol_undervol2)
	//	tempstr = "OK";
	//else
	//	tempstr = "NOK";
	//tempstr.Format("%s", tempstr);
	//erange.SetValue2(COleVariant(tempstr));

	app.SetDisplayAlerts(FALSE);
	book.Save();
	ft.ReleaseDispatch();
	erange.ReleaseDispatch(); //退出
	sheet.ReleaseDispatch();
	sheets.ReleaseDispatch();
	book.ReleaseDispatch();
	books.Close();
	books.ReleaseDispatch();
	app.ReleaseDispatch();
	app.Quit();

	char *strTemp = new char[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, strTemp);

	tempstr.Format("excel测试报告生成成功! 测试报告位于目录:: \t\n %s%s ", strTemp, "\\excel\\");
	AfxMessageBox(tempstr);
	delete[] strTemp;
}

void CACPSDlg::OnDelete()
{
	// TODO: Add your control notification handler code here
	int Count = m_list1.GetCount();

	for (int i = Count; i >= 0; i--)
	{
		m_list1.DeleteString(i);
	}
}
/**************************************************************************************************************
* 功    能：数据包解密（工程安全监测协议V1.1）
* 入口参数：a：被解包数据
				 axor：随机值
* 出口参数：解密得到的数据
***************************************************************************************************************/
unsigned char CACPSDlg::DeCode(unsigned char a, unsigned char axor)
{
	unsigned char tmp, tempb, tempc;
	tmp = a;
	tempb = a << 6;
	tempc = a >> 2;
	tmp = tempb | tempc;
	tmp = tmp ^ axor;
	return tmp;
}

unsigned char CACPSDlg::EnCode(unsigned char a, unsigned char axor)
{
	unsigned char tmp, tempb, tempc;
	tmp = a ^ axor;
	tempb = tmp >> 6;
	tempc = tmp << 2;
	tmp = tempb | tempc;
	return tmp;
}
/////////////////////////////////////////////////////////
//功能：求权
//输入：	int base                    进制基数
//			int times                   权级数
//输出：
//返回：unsigned long               当前数据位的权
//////////////////////////////////////////////////////////
unsigned long CACPSDlg::power(int base, int times)
{
	int i;
	unsigned long rslt = 1;
	for (i = 0; i < times; i++)
		rslt *= base;
	return rslt;
}
/////////////////////////////////////////////////////////
//
//功能：十进制转BCD码
//
//输入：int Dec                      待转换的十进制数据
//      int length                   BCD码数据长度
//输出：unsigned char *Bcd           转换后的BCD码
//返回：0  success
//思路：原理同BCD码转十进制
//////////////////////////////////////////////////////////
int CACPSDlg::DectoBCD(int Dec, unsigned char *Bcd, int length)
{
	int i;
	int temp;
	for (i = length - 1; i >= 0; i--)
	{
		temp = Dec % 100;
		Bcd[i] = ((temp / 10) << 4) + ((temp % 10) & 0x0F);
		Dec /= 100;
	}
	return 0;
}
/////////////////////////////////////////////////////////
//
//功能：BCD转10进制
//输入：const unsigned char *bcd     待转换的BCD码
//      int length                   BCD码数据长度
//输出：
//返回：unsigned long               当前数据位的权
//思路：压缩BCD码一个字符所表示的十进制数据范围为0 ~ 99,进制为100
//      先求每个字符所表示的十进制值，然后乘以权
//////////////////////////////////////////////////////////
unsigned long CACPSDlg::BCDtoDec(const unsigned char *bcd, int length)
{
	int i, tmp;
	unsigned long dec = 0;
	for (i = 0; i < length; i++)
	{
		tmp = ((bcd[i] >> 4) & 0x0F) * 10 + (bcd[i] & 0x0F);
		dec += tmp * power(100, length - 1 - i);
	}
	return dec;
}

void CACPSDlg::CleanUpSRC()
{
	//关闭扫描抢
	//m_lowPwr->Open_ScannerGun(false);
	//Switch_ScannerGun_Port(false);

	//关闭电源
	ResetPower();

#ifdef PCAN_RECV_TH

	// Close stop handle thread event
	if (m_eventHandleToStopWaitingForObject != NULL)
	{
		// Set event to force close the "event thread"
		SetEvent(m_eventHandleToStopWaitingForObject);
		CloseHandle(m_eventHandleToStopWaitingForObject);
		m_eventHandleToStopWaitingForObject = NULL;
	}

	// Close handle for event
	if (m_eventHandleToReadCAN != NULL)
	{
		CloseHandle(m_eventHandleToReadCAN);
		m_eventHandleToReadCAN = NULL;
	}

#endif

	if (g_MsgCycleSignaled == NULL)
	{
		CloseHandle(g_MsgCycleSignaled);
		g_MsgCycleSignaled = NULL;
	}

	if (g_DiagRespSignaled == NULL)
	{
		CloseHandle(g_DiagRespSignaled);
		g_DiagRespSignaled = NULL;
	}

	//释放中介对象
	if (m_Mediator != NULL){
		delete m_Mediator; m_Mediator = NULL;
	}

	//释放低压电源对象
	if (m_lowPwr != NULL) {
		delete m_lowPwr; m_lowPwr = NULL;
	}

	//释放高压电源对象
	if (m_highPwr != NULL) {
		delete m_highPwr; m_highPwr = NULL;
	}

	//释放总线设备对象
	if (gSelectedBUSAdapter == 0)
	{
		if (m_Bus != NULL) {	//can
			m_Bus->DisConnectDevice();
			delete m_Bus; m_Bus = NULL;
		}
	}	
	else if (gSelectedBUSAdapter == 1) //LIN
	{
		if (m_pLinBus != NULL && m_pLinBus->m_DevHandle!=0) 
		{
			m_pLinBus->DisConnectDevice();
			delete m_pLinBus; m_pLinBus = NULL;
		}
	}

	if (m_exec != NULL) {
		delete m_exec; m_exec = NULL;
	}

	//释放 CAN 报文对象
	if (HVH_ECU_Frm1 != NULL)
	{
		delete HVH_ECU_Frm1; HVH_ECU_Frm1 = NULL;
	}
	if (HVH_ECU_Frm2 != NULL)
	{
		delete HVH_ECU_Frm2; HVH_ECU_Frm2 = NULL;
	}

	if (HVH_ECU_Frm21 != NULL)
	{
		delete HVH_ECU_Frm21; HVH_ECU_Frm21 = NULL;
	}
	if (HVH_ECU_Frm22 != NULL)
	{
		delete HVH_ECU_Frm22; HVH_ECU_Frm22 = NULL;
	}
	//释放LIN报文对象
	if (LINMSG_RECV != NULL) {
		delete LINMSG_RECV; LINMSG_RECV = NULL;
	}
}

void CACPSDlg::OnACPSCancel()
{
	CleanUpSRC();
	OnCancel();
}

TCHAR FirstDriveFromMask(ULONG unitmask)
{
	char i;
	for (i = 0; i < 26; ++i)
	{
		if (unitmask & 0x1) //看该驱动器的状态是否发生了变化
			break;
		unitmask = unitmask >> 1;
	}
	return (i + 'A');
}

int ChangeNum(CString str, int length)
{
	char revstr[16] = {0}; //根据十六进制字符串的长度，这里注意数组不要越界
	int num[16] = {0};
	int count = 1;
	int result = 0;
	strcpy(revstr, str);
	for (int i = length - 1; i >= 0; i--)
	{
		if ((revstr[i] >= '0') && (revstr[i] <= '9'))
			num[i] = revstr[i] - 48; //字符0的ASCII值为48
		else if ((revstr[i] >= 'a') && (revstr[i] <= 'f'))
			num[i] = revstr[i] - 'a' + 10;
		else if ((revstr[i] >= 'A') && (revstr[i] <= 'F'))
			num[i] = revstr[i] - 'A' + 10;
		else
			num[i] = 0;
		result = result + num[i] * count;
		count = count * 16; //十六进制(如果是八进制就在这里乘以8)
	}
	return result;
}

BOOL CACPSDlg::OnDeviceChange(UINT nEventType, DWORD dwData)
{
	CString portpath;
	switch (nEventType)
	{
		case DBT_DEVICEREMOVECOMPLETE: //移除设备，关闭串口
		{
			PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)dwData;
			PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;

			portpath = FirstDriveFromMask(lpdbv->dbcv_unitmask);
			portnumber = ChangeNum(portpath, portpath.GetLength());

			for (int i = 0; i < 2; i++)
			{			
				if (i + 10 +1 == portnumber)
				{
					m_bDevLost1 = true;
					m_list1.AddString("低压电源Rs232串口已关闭，请重连");
					m_lowPwr->Close();
				}
				if (i + 10 == portnumber)
				{
					m_bDevLost2 = true;
					m_list1.AddString("高压/功率计Rs485串口已关闭，请重连");
					m_highPwr->Close();
				}				
			}			
		}
		break;
		case DBT_DEVICEARRIVAL: //添加设备，打开串口
		{
			PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)dwData;
			PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
			
			portpath = FirstDriveFromMask(lpdbv->dbcv_unitmask);
			portnumber = ChangeNum(portpath, portpath.GetLength());
			
			for (int i = 0; i < 2; i++)
			{
				if (i + 10 == portnumber)
				{
					if (m_highPwr->Open(config[0].PORT, CommBaud[config[0].BUND]) == TRUE)
					{
						m_bDevLost2 = false;
						m_list1.AddString("高压/功率计Rs485串口已重连");
					}
				}

				if (i + 10 + 1 == portnumber)
				{
					if (m_lowPwr->Open(config[1].PORT, CommBaud[config[1].BUND]) == TRUE)
					{
						m_bDevLost1 = false;
						m_list1.AddString("低压电源Rs232串口已重连");
					}
				}
			}
		}
		break;

		default:
			break;
	}
	return TRUE;
}

BOOL CACPSDlg::PreTranslateMessage(MSG* pMsg)
{	
	CString strScanGunData;
	if (pMsg->message == WM_KEYDOWN)
	{
		//m_lowPwr->Get_ScannerGun_Data(&strScanGunData);
		//GetDlgItem(IDC_OVEREDIT)->SetWindowText(strScanGunData);

		//switch (pMsg->wParam)
		//{
		//case VK_RETURN:
		//	m_overedit.SetWindowText(m_strInput);
		//	m_strInput = "";
		//	break;
		////case VK_SHIFT:
		////	//m_strInput = "";
		////	//m_overedit.SetWindowText("");
		////	break;
		//default:
		//	CString str_ascii;
		//	char st = pMsg->wParam;
		//	if (st == VK_SHIFT)
		//		str_ascii = " ";
		//	else
		//		str_ascii = CString(st);
		//	m_strInput += str_ascii;
		//	break;
		//}

	}
	return CDialog::PreTranslateMessage(pMsg);
}

///////////////////////////////////////////////流程函数 start///////////////////////////////////////////////////////////////////////////////////////
bool CACPSDlg::OnHighPowerIn(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 11, 1);
}

bool CACPSDlg::OffHighPowerIn(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 11, 0);
}

bool CACPSDlg::OnHighPowerOut(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 10, 1);
}

bool CACPSDlg::OffHighPowerOut(CSerial *port)
{
	return  m_Mediator->WriteDO(m_highPwr, 10, 0);;
}

bool CACPSDlg::WaterInletNTCShortTest(CSerial *port)
{
	m_Mediator->WriteDO(m_highPwr, 2, 1);
	m_Mediator->WriteDO(m_highPwr, 3, 1);
	return 1;
}

bool CACPSDlg::WaterInletTemperTest(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 3, 0);
}

bool CACPSDlg::OffWaterInletTest(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 2, 0);
}

bool CACPSDlg::WaterOutletNTCShortTest(CSerial *port)
{
	m_Mediator->WriteDO(m_highPwr, 4, 1);
	m_Mediator->WriteDO(m_highPwr, 5, 1);
	return 1;
}

bool CACPSDlg::WaterOutletTemperTest(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 5, 0);
}

bool CACPSDlg::OffWaterOutletTest(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 4, 0);
}

bool CACPSDlg::IGBTNTCShortTest(CSerial *port)
{
	m_Mediator->WriteDO(m_highPwr, 6, 1);
	m_Mediator->WriteDO(m_highPwr, 7, 1);
	return 1;
}

bool CACPSDlg::IGBTTemperTest(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 7, 0);
}

bool CACPSDlg::OffIGBTTest(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 6, 0);
}

bool CACPSDlg::OnRedLed(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 1, 1);
}

bool CACPSDlg::OffRedLed(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 1, 0);
}

bool CACPSDlg::OnYellowLed(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 2, 1);
}

bool CACPSDlg::OffYellowLed(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 2, 0);
}

bool CACPSDlg::OnGreenLed(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 3, 1);
}

bool CACPSDlg::OffGreenLed(CSerial *port)
{
	return m_Mediator->WriteDO(m_highPwr, 3, 0);
}

//主流程操作函数
void CACPSDlg::OnBnClickedBtnstart()
{
	if (m_bSuspendThread)
	{
		if(pWinThread != NULL)
			pWinThread->ResumeThread();
		if(CAN_Handle!=NULL)
			CAN_Handle->ResumeThread();
		//if(pSendMsgThread!=NULL)
		//	pSendMsgThread->ResumeThread();
		if(LIN_Handle!=NULL)
			LIN_Handle->ResumeThread();

		SetTimer(1, 1000, NULL);
		SetTimer(3, 100, NULL);
		SetTimer(4, 1000, NULL);
		SetTimer(5, 1000, NULL);
		m_bSuspendThread = false;
		return;
	}
	GetDlgItem(IDC_BTNSTOP)->EnableWindow(TRUE);
	_InitVARs();
	ClearList();
	////////////////高低压电源、功率计和DO对象初始化
	int comsetport, comsetbaud, comdata, comstopbit, comcheck;
	CString strtemp = "";

	comsetport = config[0].PORT;
	comsetbaud = CommBaud[config[0].BUND];
	comdata = CommData[config[0].DATA];
	comstopbit = config[0].STOP;
	comcheck = config[0].CHECK;
	m_bportPreOpened = m_highPwr->Open(comsetport, comsetbaud, comdata, comstopbit, comcheck);
	if (!m_bportPreOpened)
	{
		AfxMessageBox(TEXT("RS485串口冲突"));
		return;
	}

	//让中介类 认识  485通信类(高压电源，功率计, 数字量输出)
	m_Mediator->highPower = m_highPwr;

	//高压电源使能
	m_Mediator->highPower->SetDWWState(true);

	comsetport = config[1].PORT;
	comsetbaud = CommBaud[config[1].BUND];
	comdata = CommData[config[1].DATA];
	comstopbit = config[1].STOP;
	comcheck = config[1].CHECK;
	m_bportTemOpened = m_lowPwr->Open(comsetport, comsetbaud, comdata, comstopbit, comcheck);
	if (!m_bportTemOpened)
	{
		AfxMessageBox(TEXT("低压电源设备串口冲突"));
		return;
	}

	//让中介类 认识  低压电源类
	m_Mediator->lowPower = m_lowPwr;

	if(gSelectedBUSAdapter == 0)
	{
		//开启CAN设备
		g_mCANDevOK = OpenDevice();
		if (!g_mCANDevOK)
		{
			MessageBox("Init-CAN device  failed!");
			return;
		}
	}
	else
	{
		//开启Toomoss LIN
		g_LINDevOK = (bool)Init_LIN();
		if (!g_LINDevOK)
		{
			MessageBox("Init-LIN device failed!");
			return;
		}
	}

	//开启主流程线程
	pWinThread = AfxBeginThread(PreThread, this,
		THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	pWinThread->m_bAutoDelete = FALSE;//线程结束时不自动删除
	pWinThread->ResumeThread();//恢复线程运行

	if (gSelectedBUSAdapter == 1)
	{
		//LIN报文接收线程
		LIN_Handle = AfxBeginThread(LINReceiveThread, 0,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		LIN_Handle->m_bAutoDelete = FALSE;//线程结束时不自动删除	
		LIN_Handle->ResumeThread();//恢复线程运行
	}
	else
	{
#ifdef PCAN_RECV_TH

		//CAN报文接收线程
		CAN_Handle = AfxBeginThread(CanReadThreadForEvent, 0,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		CAN_Handle->m_bAutoDelete = FALSE;//线程结束时不自动删除		
		CAN_Handle->ResumeThread();//恢复线程运行

#else
		//CAN报文接收线程
		CAN_Handle = AfxBeginThread(CANReceiveThread, 0,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		CAN_Handle->m_bAutoDelete = FALSE;//线程结束时不自动删除		
		CAN_Handle->ResumeThread();//恢复线程运行
#endif
	}

	//功率请求报文发送线程 pSendMsgThread
	pSendMsgThread = AfxBeginThread(PowerReqThread, this,
		THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	pSendMsgThread->m_bAutoDelete = TRUE;//线程结束时不自动删除
}

//关闭电源
void CACPSDlg::ResetPower()
{
	//存在真实故障，点亮红灯报警
	//OffYellowLed(portPre);
	if (!m_bErrMsg)
	{
		//OffYellowLed(portPre);
		//OffGreenLed(portPre);
		//OnRedLed(portPre);
		GetDlgItem(IDC_STATICPCBA)->SetWindowText("故障");
	}
	else
	{
		m_TestStatus.SetWindowText("测试结束");
		//OnGreenLed(portPre);
	}

	//关闭高/低压电源
	LowHigh_PowerDown();

	//停止线程
	StopThreads();

	//关闭总线设备
	if (gSelectedBUSAdapter == 0)
	{
		if (g_mCANDevOK)
		{
			g_mCANDevOK = false;
			//释放总线设备对象		
			if (m_Bus != NULL)  //can
			{
				m_Bus->DisConnectDevice();
			}		
		}
	}
	else if (gSelectedBUSAdapter == 1) //LIN
	{
		if (m_pLinBus != NULL && m_pLinBus->m_DevHandle != 0)
		{
			m_pLinBus->DisConnectDevice();
		}
	}

	//if (!m_bportPreOpened || !m_bportTemOpened || StopFlag)
	//	return;

	//关闭串口
	if (m_bportPreOpened)
	{
		m_highPwr->Close();
		m_bportPreOpened = FALSE;
	}

	if (m_bportTemOpened)
	{
		m_lowPwr->Close();
		m_bportTemOpened = FALSE;
	}

	//断开J3~J4
	PowerJ3Relay(false);
	PowerJ4Relay(false);
}
///////////////////////////////LIN通信专用接口////////////////////////////////////////
int CACPSDlg::Init_LIN()
{
	bool state = false;
	int ret = 0;
	m_Bus->gLINMasterIndex = gLINMasterIndex;
#ifdef _COMMAND_PATTERN
	ConnectDevCmd connect(m_pLinBus);
	Command* p_connect = &connect;
	//发送指令
	m_exec->SetCmd(p_connect);
	ret = m_exec->Notify();
#else
	ret = m_pLinBus->ConnectDevice();
#endif
	return ret;
}

bool CACPSDlg::Close_LIN_Device()
{
#ifdef _COMMAND_PATTERN
	DisConnectDevCmd disconnect(m_pLinBus);
	Command* p_disconnect = &disconnect;

	m_exec->SetCmd(p_disconnect);
	m_exec->Notify();
#else
	m_pLinBus->DisConnectDevice();
#endif
	return true;
}

int CACPSDlg::Send_Master_Data(BYTE* pData)
{
	int ret = -1;
	CString strSendMsg;

	SendCmd send(m_pLinBus);
	Command* p_send = &send;
	//发送指令
	m_exec->SetCmd(p_send);

	//A0 0C 00 00 C0 00 03 00 00
	pData[0] = 0xA0;
	pData[1] = 0x0C;
	pData[2] = 0x00; 
	pData[3] = 0x00;
	pData[4] = 0xC0;
	pData[5] = 0x00;
	pData[6] = 0x03;
	pData[7] = 0x00;

	m_exec->Send(0x26, 0, pData, &strSendMsg);
	ret = 0;

	//SetEvent(gEventLin);

	DispalyCurrentMsg(strSendMsg, false);	

	return ret;
}
///////////////////////////////////////////////////////////////////////CAN通信专用接口
//打开设备CAN通道
bool CACPSDlg::OpenDevice()
{
	m_nDevType = config[3].CHECK;
	m_nSendFrameType = config[3].PORT;
	m_nSendFrameFormat = config[3].BUND;
	m_nCanIndex = config[3].DATA;
	m_DevType = m_nDevType == 1 ?  VCI_USBCAN2 : VCI_USBCAN1 ;
	
	if (gCanFD.CANFD)
	{
		m_Bus->m_CanParams.PCANParam.m_IsFD = gCanFD.CANFD;
		m_Bus->m_CanParams.PCANParam.m_BRS = gCanFD.BRS;
	}
	m_Bus->m_CanParams.PCANParam.m_ReqAdr = g_DiagReq;
	m_Bus->m_CanParams.PCANParam.m_RespAdr[0] = ECUFeedback1;
	m_Bus->m_CanParams.PCANParam.m_RespAdr[1] = ECUFeedback2;

	m_Bus->m_CanParams.m_CANIndex = m_nCanIndex;
	m_Bus->m_CanParams.m_DevType = m_DevType;
	m_Bus->m_CanParams.m_SendFrameFormat = m_nSendFrameFormat; // remote frame  
	m_Bus->m_CanParams.m_SendFrameType = m_nSendFrameType;	//standard frame / extend frame
	
#ifdef _COMMAND_PATTERN
	ConnectDevCmd connect(m_Bus);
	Command* p_connect = &connect;
	//发送指令
	m_exec->SetCmd(p_connect);
	g_mCANDevOK = m_exec->Notify();
#else
	g_mCANDevOK = m_Bus->ConnectDevice();
#endif
	return g_mCANDevOK;
}
//关闭设备CAN通道
void CACPSDlg::CloseDevice()
{
#ifdef _COMMAND_PATTERN
	DisConnectDevCmd disconnect(m_Bus);
	Command* p_disconnect = &disconnect;
	m_exec->SetCmd(p_disconnect);
	m_exec->Notify();
#else

	m_Bus->DisConnectDevice();

#endif
}
//CAN通道发送信息
void CACPSDlg::SendCANMsg(u8 HVH_MaxPower, u8 HVH_TargetCoolantTemp, u8 HVH_HeaterEnable, u8 HVH_ActiveDischarge, int nCHN)
{	
	SendCmd send(m_Bus);
	Command* p_send = &send;
	//发送指令
	m_exec->SetCmd(p_send);

	BYTE pData[8];
	CString strmsg;
	memset(pData, 0, sizeof(pData));
	pData[0] = HVH_MaxPower;
	pData[1] = HVH_TargetCoolantTemp;
	pData[2] = 0x00;
	pData[3] = (HVH_HeaterEnable << 3) | (HVH_ActiveDischarge << 4);
	pData[4] = 0x00;
	pData[5] = 0x00;
	pData[6] = 0x00;
	pData[7] = 0x00;

	m_Bus->m_CanParams.m_SendFrameType = 1;//extend frame
	if (nCHN == 1)		//ptc 0
	{
		m_exec->Send(ECURequst, 0, 1, pData, &strmsg);
		DispalyCurrentMsg(strmsg, false);
	}
	else if (nCHN == 2)		//ptc 1
	{
		m_exec->Send(ECURequst, 1, 1, pData, &strmsg);
		DispalyCurrentMsg(strmsg, false);
	}
	//m_Bus->m_CanParams.m_SendFrameType = 0;
} 

void CACPSDlg::OnBnClickedOffmsg()
{
	m_listexcel.ClearColor();
	GetDlgItem(IDC_STATICPCBA)->SetWindowText("正常");
	//if(portPre!=NULL)
	//	OffRedLed(portPre);

	for (int k = m_list1.GetCount() - 1; k >= 0; k--)
		m_list1.DeleteString(k);

	Invalidate(TRUE);
}

void CACPSDlg::StopThreads()
{
	KillTimer(1);
	KillTimer(3);
	KillTimer(4);
	KillTimer(5);

	//等待报文接收线程结束 CAN
	if (CAN_Handle != NULL)
	{
		CloseHandle(CAN_Handle->m_hThread);
		CAN_Handle = NULL;
	}

	//结束功率请求线程
	if (pSendMsgThread != NULL)
	{
		CloseHandle(pSendMsgThread->m_hThread);
		pSendMsgThread = NULL;
	}

	//等待报文接收线程结束 LIN1
	if (LIN_Handle != NULL)
	{
		StopFlag = 1;
		Sleep(200);
		CloseHandle(LIN_Handle->m_hThread);
		LIN_Handle = NULL;
	}

	//等待主线程结束
	if (pWinThread != NULL)
	{
		CloseHandle(pWinThread->m_hThread);
		pWinThread = NULL;
	}
}

void CACPSDlg::OnBnClickedBtnstop()
{
	if (pWinThread != NULL)
		pWinThread->SuspendThread();
	//if (pSendMsgThread != NULL)
	//	pSendMsgThread->SuspendThread();
	if (CAN_Handle != NULL)
		CAN_Handle->SuspendThread();
	if (LIN_Handle != NULL)
		LIN_Handle->SuspendThread();

	KillTimer(1);
	KillTimer(2);
	KillTimer(3);
	KillTimer(4);
	KillTimer(5);

	m_bSuspendThread = true;
	GetDlgItem(IDC_BTNSTART)->EnableWindow(TRUE);
	Invalidate(TRUE);
}

void CACPSDlg::OnCbnDblclkCombocfg()
{
	TimerFlag = -1;
}

void CACPSDlg::OnCbnSelchangeCombocfg()
{
	TimerFlag = 0;
}

void CACPSDlg::OnCbnKillfocusCombocfg()
{
	// TODO: 在此添加控件通知处理程序代码
	TimerFlag = 0;
	setcfg_tolist(0);//TODO
}

void CACPSDlg::ClearList()
{
	for (int i = 1; i < 4; i++)
	{
		for (int j = 1; j <20; j += 4)
		{
			m_listexcel.SetItemText(j, i, "");
		}
		for (int j = 2; j < 20; j += 4)
		{
			m_listexcel.SetItemText(j, i, "");
		}
	}
	m_listexcel.ClearColor();
	m_exec->ClearCmds();

	for (int k= m_list1.GetCount() - 1; k>=0; k--)
		m_list1.DeleteString(k);
}

HBRUSH CACPSDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	switch (nCtlColor) //对所有同一类型的控件进行判断  
	{
	case CTLCOLOR_EDIT:
	case CTLCOLOR_STATIC:
		switch (pWnd->GetDlgCtrlID())			//对某一个特定控件进行判断  
		{
			// first CEdit control ID  
		case IDC_STATICSTATE:
			pDC->SetBkMode(TRANSPARENT);//设置背景透明    
			break;
		case IDC_STATICPCBA:
			pDC->SetBkColor(RGB(0, 255, 0));//设置背景颜色
			break;
		case IDC_LIST1:
			pDC->SetTextColor(RGB(0, 0, 255));//设置故障信息颜色
			break;
		case IDC_LIST5:
			pDC->SetBkColor(RGB(255, 255, 0));//设置背景颜色
			break;
		default:
			hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
			break;
		}
		break;
	
	}

	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}

void CACPSDlg::OnBnClickedBoxcfg()
{
}

void CACPSDlg::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();
}

void CACPSDlg::OnSelchangeCombolinmode()
{
	TimerFlag = 0;
}

void CACPSDlg::OnKillfocusCombolinmode()
{
	TimerFlag = 0;
}

void CACPSDlg::OnDropdownCombolinmode()
{
	TimerFlag = -1;
}

void Float2Byte(float f, byte* ByteToInt8)
{	
	float float_data = 0;
	unsigned long longdata = 0;
	
	longdata = *(unsigned long*)&f;          //注意，会丢失精度
	ByteToInt8[0] = (longdata & 0xFF000000) >> 24;
	ByteToInt8[1] = (longdata & 0x00FF0000) >> 16;
	ByteToInt8[2] = (longdata & 0x0000FF00) >> 8;
	ByteToInt8[3] = (longdata & 0x000000FF);
}

//测量功率线程
UINT Measure_Power_TH(LPVOID param)
{
	CACPSDlg* pDlg = (CACPSDlg*)param;
	pDlg->Run_Selected_PowerMode(pDlg);
	return 0;
}


void CACPSDlg::OnBnClickedRadioPower()
{
	m_modevalue.SetWindowText("7");
}


void CACPSDlg::OnBnClickedRadioGear()
{
	m_modevalue.SetWindowText("2");
}


void CACPSDlg::OnBnClickedRadioTemp()
{
	m_modevalue.SetWindowText("80");
}


void CACPSDlg::OnEnChangeEditMode()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	TimerFlag = 0;
}


void CACPSDlg::OnEnSetfocusEditMode()
{
	TimerFlag = -1;
}


void CACPSDlg::OnEnChangeEditWaittime()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	TimerFlag = 0;
}


void CACPSDlg::OnEnSetfocusEditWaittime()
{
	TimerFlag = -1;
}


void CACPSDlg::OnBnClickedCommsetting()
{
	CConfig ConfigFrame; //通讯参数配置界面
	ConfigFrame.DoModal();

	gDevIndex = linCfg[0].DEVNUM;
	m_Bus->gLINMasterIndex = linCfg[0].LINCHANNEL;

	//总线适配器类型变更
	if (gSelectedBUSAdapter != config[4].PORT)
	{
		if (config[4].PORT == 0)
		{
			if (m_Bus != NULL) {
				delete m_pLinBus; m_pLinBus = NULL;
			}
			
			m_Bus = new CAN_Bus();
		}
		else {
			if (m_Bus != NULL) {
				delete m_Bus; m_Bus = NULL;
			}
			
			m_Bus = new LIN_Bus();
		}
		gSelectedBUSAdapter = config[4].PORT;
	}
}

void CACPSDlg::WriteTestReport()
{
	//测试结果导出
	tm = CTime::GetCurrentTime();
	timestr = tm.Format("%Y%m%d%H%M");

	char* strTemp = new char[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, strTemp);

	strCurrentPath = strTemp;
	strCurrentPath = strCurrentPath + "\\excel\\";

	if (!IsDirExist(strCurrentPath))
		CreateDir(strCurrentPath);

	CString filepath;
	CString chtempstr, sheetstr;
	filepath.Format("%s%s.xls", strCurrentPath, g_Serialnumber);

	CSpreadSheet SS(filepath, sheetstr, false);
	CStringArray sampleArray;
	SS.BeginTransaction();
	sampleArray.RemoveAll();
	sampleArray.Add(timestr);
	SS.AddHeaders(sampleArray);
	SS.Commit();

	WriteExcelData(filepath);
	m_overedit.SetWindowText("");

	delete[] strTemp;
	strTemp = NULL;
}

void CACPSDlg::OnBnClickedReportExport()
{
	float f300Vol1=0.0f, f300Curr1 = 0.0f;
	CString info;
	m_highPwr->Get_8710Dev_Vol_Curr(&f300Vol1, &f300Curr1);
	info.Format("Get8710 vol::%2.2f, curr::%2.2f", f300Vol1, f300Curr1);
	m_list1.AddString(info);
}

void CACPSDlg::OnBnClickedTestParams()
{
	CRange dlg;

	dlg.cfgindex = 0;
	dlg.DoModal();
}

//打开/关闭扫描枪串口
void  CACPSDlg::Switch_ScannerGun_Port(bool bOpen)
{
	int comsetport, comsetbaud, comdata, comstopbit, comcheck;
	if (!bOpen)
	{
		//扫码枪
		comsetport = config[1].PORT;
		comsetbaud = CommBaud[config[1].BUND];
		comdata = CommData[config[1].DATA];
		comstopbit = config[1].STOP;
		comcheck = config[1].CHECK;
		m_bportTemOpened = m_lowPwr->Open(comsetport, comsetbaud, comdata, comstopbit, comcheck);
		if (!m_bportTemOpened)
		{
			AfxMessageBox(TEXT("扫码枪设备串口冲突"));
			return;
		}
		m_ScanGun.SetWindowText("关闭端口");
		g_bScanGunOpen = true;
		GetDlgItem(IDC_OVEREDIT)->SetFocus();
	}
	else
	{
		m_lowPwr->Close();
		m_ScanGun.SetWindowText("打开端口");
		g_bScanGunOpen = false;
	}
}

void CACPSDlg::OnClose()
{
	int comsetport, comsetbaud;
	CString strtemp = "";
	comsetport = config[0].PORT;
	comsetbaud = CommBaud[config[0].BUND];

	if (!m_bportPreOpened)
	{
		m_bportPreOpened = m_highPwr->Open(comsetport, comsetbaud);
		if (!m_bportPreOpened)
		{
			CDialog::OnClose();
		}
	}
	if (m_bportPreOpened)
	{
		m_Mediator->highPower->SetDWWState(true);
		m_Mediator->Set_Dev_Vol_Curr(m_highPwr, 0.0f, 0.0f);
		Sleep(1000);
		m_Mediator->highPower->SetDWWState(false);
		m_highPwr->Close();
	}


	CDialog::OnClose();
}
