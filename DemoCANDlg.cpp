// DemoCANDlg.cpp : implementation file
//


#include "stdafx.h"
#include "DemoCAN.h"
#include "DemoCANDlg.h"
#include "ControlCAN.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CString sFile;
unsigned long nextrow;
int StopFlag=0;
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
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

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDemoCANDlg dialog

CDemoCANDlg::CDemoCANDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDemoCANDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDemoCANDlg)
	m_nSendFrameFormat = 0;
	m_nSendFrameType = 0;
	m_strSendData = _T("11 22 33 44 55 66 77 88");
	m_strSendID = _T("00 00 00 88");
	m_radioIDFormat = 1;
	m_bCanRxEn = FALSE;
	m_nCanIndex = 0;
	m_nDevType = 1;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDemoCANDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDemoCANDlg)
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_CBIndex(pDX, IDC_COMBO_SENDFRAMEFORMAT, m_nSendFrameFormat);
	DDX_CBIndex(pDX, IDC_COMBO_SENDFRAMETYPE, m_nSendFrameType);
	DDX_Text(pDX, IDC_EDIT_SEND_DATA, m_strSendData);
	DDX_Text(pDX, IDC_EDIT_SEND_ID, m_strSendID);
	DDX_Radio(pDX, IDC_RADIO_ID_FORMAT, m_radioIDFormat);
	DDX_Check(pDX, IDC_CHECK_CANRX_EN, m_bCanRxEn);
	DDX_CBIndex(pDX, IDC_COMBO_CAN_INDEX, m_nCanIndex);
	DDX_CBIndex(pDX, IDC_COMBO_DEVTYPE, m_nDevType);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDemoCANDlg, CDialog)
	//{{AFX_MSG_MAP(CDemoCANDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN_DEVICE, OnButtonOpenDevice)
	ON_BN_CLICKED(IDC_BUTTON_SEND, OnButtonSend)
	ON_BN_CLICKED(IDC_CHECK_CANRX_EN, OnCheckCanrxEn)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, OnButtonClear)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE_DEVICE, OnButtonCloseDevice)
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_COMBO_DEVTYPE, &CDemoCANDlg::OnCbnSelchangeComboDevtype)
	ON_CBN_SELCHANGE(IDC_COMBO_CAN_INDEX, &CDemoCANDlg::OnCbnSelchangeComboCanIndex)
	ON_CBN_SELCHANGE(IDC_COMBO_SENDFRAMETYPE, &CDemoCANDlg::OnCbnSelchangeComboSendframetype)
	ON_EN_CHANGE(IDC_EDIT_SEND_DATA, &CDemoCANDlg::OnEnChangeEditSendData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDemoCANDlg message handlers

BOOL CDemoCANDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
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
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
    //信息显示列表初始化
	m_list.InsertColumn(0,"Seq");
	m_list.SetColumnWidth(0,40);
	m_list.InsertColumn(1,"Time");
	m_list.SetColumnWidth(1,85);
	m_list.InsertColumn(2,"CANIndex");
	m_list.SetColumnWidth(2,60);
	m_list.InsertColumn(3,"Tx-Rx");
	m_list.SetColumnWidth(3,60);
	m_list.InsertColumn(4," ID ");
	m_list.SetColumnWidth(4,60);
	m_list.InsertColumn(5,"Frame");
	m_list.SetColumnWidth(5,50);
	m_list.InsertColumn(6,"Type");
	m_list.SetColumnWidth(6,70);
	m_list.InsertColumn(7,"DLC");
	m_list.SetColumnWidth(7,30);
	m_list.InsertColumn(8,"Data");
	m_list.SetColumnWidth(8,160);
	//信息显示列表初始化完毕
	m_strSendID = "12 34 56 78";
	// TODO: Add extra initialization here
	UpdateData(FALSE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDemoCANDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDemoCANDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

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
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDemoCANDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

//一位十六进制转换为十进制
int HexChar(char c)
{
	if((c>='0') && (c<='9'))
		return c-0x30;
	else if((c>='A') && (c<='F'))
		return c-'A'+10;
	else if((c>='a') && (c<='f'))
		return c-'a'+10;
	else
		return 0x10;
}
//两位十六进制数转换为十进制
int Str2Hex(CString str)
{	
	int len = str.GetLength();
	if(len == 2)
	{
		int a= HexChar(str[0]);
		int b =HexChar(str[1]);
		if(a==16 || b==16 )
		{
			AfxMessageBox("Format error");
			return 256;
		}
		else
		{
			return a*16+b;
			
		}
		
	}
	else
	{
		AfxMessageBox("input length must be 2");
		return 256;
	}
}
//打开设备
void CDemoCANDlg::OnButtonOpenDevice() 
{

	UpdateData(TRUE);
	m_DevIndex=0;	
	m_DevType= m_nDevType == 1 ?  VCI_USBCAN2 :  VCI_USBCAN1;
	DWORD Reserved=0;
	//打开设备
	if(VCI_OpenDevice(m_DevType,m_DevIndex,Reserved)!=1)
	{
		MessageBox("open failed");
		return;
	}
	VCI_INIT_CONFIG InitInfo[1];
	InitInfo->Timing0=0x00;
	InitInfo->Timing1=0x14;
	InitInfo->Filter=0;
	InitInfo->AccCode=0x80000008;
	InitInfo->AccMask=0xFFFFFFFF;
	InitInfo->Mode=2;
    //初始化通道0
	if(VCI_InitCAN(m_DevType,m_DevIndex, 0,InitInfo)!=1)	//can-0
	{
		MessageBox("Init-CAN failed!");
		return;
	}
	Sleep(100);
    //初始化通道0
	if(VCI_StartCAN(m_DevType,m_DevIndex, 0)!=1)	//can-0
	{
		MessageBox("Start-CAN failed!");
		return;
	}
    //初始化通道1
	if(m_nDevType == 1)
	{
		if(VCI_InitCAN(m_DevType,m_DevIndex, 1,InitInfo)!=1)	//can-1
		{
			MessageBox("Init-CAN failed!");
			return;
		}
	}
	Sleep(100);
    //初始化通道1
	if(VCI_StartCAN(m_DevType,m_DevIndex, 1)!=1)	//can-0
	{
		MessageBox("Start-CAN failed!");
		return;
	}
	
	MessageBox("Open successfule!\n Start CAN OK!");	

}
//关闭设备
void CDemoCANDlg::OnButtonCloseDevice() 
{
	if(VCI_CloseDevice(m_DevType,m_DevIndex)!=1)
	{
		return;
	}
	
	MessageBox("Close successful!");		
}
//发送信息
void CDemoCANDlg::OnButtonSend(BYTE PWM1EN, BYTE PWM2EN, BYTE PWM3EN, BYTE PWM4EN, BYTE HVEN)
{
   //从界面获取发送信息
	VCI_CAN_OBJ sendbuf[1];

	UpdateData(TRUE);
	BYTE buf[50] = {0};
	BYTE SendID[10];
	int len,datanum=9,IDnum= 0x200,newflag=1,i;
	m_nSendFrameFormat = 0;
	m_nSendFrameType = 0;
	sendbuf->ExternFlag=m_nSendFrameType;
	sendbuf->DataLen=datanum;
	sendbuf->RemoteFlag=m_nSendFrameFormat;
	buf[0] = (PWM1EN & 0x03) | ((PWM2EN & 0x03) << 2) | ((PWM3EN & 0x03) << 4) | ((PWM4EN & 0x03) << 6);
	buf[1] = (HVEN & 0x01);
	if (m_nSendFrameFormat == 1)//if remote frame, data area is invalid
	{
		for (i = 0; i < datanum; i++)
		{
			buf[i] = 0;
		}
	}
	for (i = 0; i < datanum; i++)
	{
		sendbuf->Data[i] = buf[i];
	}
	sendbuf->ID = IDnum;


/****************************************************************************/	
/******************************从界面获取发送信息完毕***********************/
/****************************************************************************/
	int flag;

	if((m_nCanIndex==1)&&(m_DevType!= VCI_USBCAN2))
	{
		MessageBox("the device only support CAN index 0");
		m_nCanIndex=0;
	}
    //调用动态链接库发送函数
	flag=VCI_Transmit(m_DevType,m_DevIndex,m_nCanIndex,sendbuf,1);//CAN message send
	if(flag<1)
	{
		if(flag==-1)
			MessageBox("failed- device not open\n");
		else if(flag==0)
			MessageBox("send error\n");
		return;
	
	}
}

//2.定义CAN信息帧的数据类型。
typedef  struct  _CAN_MSG_100 {
	INT	    PCBTemp;
	INT	    InletTemp;
	INT	    OutletTemp;
	INT	    IGBTTemp;
	UINT	HighVoltage;
	float	DriveVoltage;
	float	Current;//是否是远程帧
	UINT	PassiveDischargeTime;//是否是扩展帧
}CAN_MSG_100, *PCAN_MSG_100;

//2.定义CAN信息帧的数据类型。
typedef  struct  _CAN_MSG_101 {
	UINT	PCBACode;
	UINT	PWM1;
	UINT	PWM2;
	UINT	PWM3;
	UINT	PWM4;
	BOOL	LVUndervoltage;
	BOOL	LVOvervoltage;//是否是远程帧
	BOOL	DVUndervoltage;//是否是扩展帧
	BOOL    InletTempSensorOpenCircuit;
	BOOL    InletTempSensorShortCircuit;
	BOOL    OutletTempSensorOpenCircuit;
	BOOL    OutletTempSensorShortCircuit;
	BOOL    IGBTTempSensorOpenCircuit;
	BOOL    IGBTTempSensorShortCircuit;
	BOOL    PTCShortCircuit;
	BOOL    IGBTShortCircuit;
	BOOL    ProtectReset;
}CAN_MSG_101, *PCAN_MSG_101;

UINT CDemoCANDlg::ReceiveThread(PCAN_MSG_100 CAN_RECV_MSG1, PCAN_MSG_101 CAN_RECV_MSG2)
{
	CDemoCANDlg *dlg=(CDemoCANDlg*) AfxGetApp()->GetMainWnd();	
	int k=0;
	while(1)
	{
		int NumValue;
		int i;
		VCI_CAN_OBJ pCanObj[200];
		CString strbuf[200],str1;
		int num=0;

		CSize size;
		unsigned int JustnowItem;
		DWORD ReceivedID;

		size.cx=0;
		size.cy=50;
		CString str;
		int Len=0;
		for(int kCanIndex=0;kCanIndex<2;kCanIndex++)
		{
			//调用动态链接看接收函数
			NumValue=VCI_Receive(dlg->m_DevType,dlg->m_DevIndex,kCanIndex,pCanObj,200,0);
			//接收信息列表显示
			k++;
			for(num=0;num<NumValue;num++)
			{
				if (pCanObj[num].ID == 0x100)
				{
					CAN_RECV_MSG1->PCBTemp = (INT)pCanObj[num].Data[0] - 40;
					CAN_RECV_MSG1->InletTemp = (INT)pCanObj[num].Data[1] - 40;
					CAN_RECV_MSG1->OutletTemp = (INT)pCanObj[num].Data[2] - 40;
					CAN_RECV_MSG1->IGBTTemp = (INT)pCanObj[num].Data[3] - 40;
					CAN_RECV_MSG1->HighVoltage = pCanObj[num].Data[4] * 2;
					CAN_RECV_MSG1->DriveVoltage = (float)pCanObj[num].Data[5] * 0.1;
					CAN_RECV_MSG1->Current = (float)pCanObj[num].Data[6] * 0.1;
					CAN_RECV_MSG1->PassiveDischargeTime = pCanObj[num].Data[7];
				}
				else if (pCanObj[num].ID == 0x101)
				{
					CAN_RECV_MSG2->PCBACode = pCanObj[num].Data[0];
					CAN_RECV_MSG2->PWM1 = pCanObj[num].Data[1];
					CAN_RECV_MSG2->PWM2 = pCanObj[num].Data[2];
					CAN_RECV_MSG2->PWM3 = pCanObj[num].Data[3];
					CAN_RECV_MSG2->PWM4 = pCanObj[num].Data[4];
					CAN_RECV_MSG2->LVUndervoltage = pCanObj[num].Data[5] & 0x01;
					CAN_RECV_MSG2->LVOvervoltage = (pCanObj[num].Data[5]>>1) & 0x01;
					CAN_RECV_MSG2->DVUndervoltage = (pCanObj[num].Data[5] >> 2) & 0x01;
					CAN_RECV_MSG2->InletTempSensorOpenCircuit = (pCanObj[num].Data[5] >> 3) & 0x01;
					CAN_RECV_MSG2->InletTempSensorShortCircuit = (pCanObj[num].Data[5] >> 4) & 0x01;
					CAN_RECV_MSG2->OutletTempSensorOpenCircuit = (pCanObj[num].Data[5] >> 5) & 0x01;
					CAN_RECV_MSG2->OutletTempSensorShortCircuit = (pCanObj[num].Data[5] >> 6) & 0x01;
					CAN_RECV_MSG2->IGBTTempSensorOpenCircuit = (pCanObj[num].Data[5] >> 7) & 0x01;
					CAN_RECV_MSG2->IGBTTempSensorShortCircuit = pCanObj[num].Data[6] & 0x01;
					CAN_RECV_MSG2->PTCShortCircuit = (pCanObj[num].Data[6] >> 1) & 0x01;
					CAN_RECV_MSG2->IGBTShortCircuit = (pCanObj[num].Data[6] >> 2) & 0x01;
					CAN_RECV_MSG2->ProtectReset = (pCanObj[num].Data[6] >> 3) & 0x01;
				}
				else
				{
					return 0;
				}
			}
		}
		Sleep(10);	

		if(StopFlag==1)
			return 0;	
	}

	return 1;
}

void CDemoCANDlg::OnCheckCanrxEn() 
{
	UpdateData(TRUE);
	if(m_bCanRxEn)
	{
		StopFlag=0;
		//开启接收线程
		AfxBeginThread(ReceiveThread,0);	
	}
	else
		StopFlag=1;
}
//清空信息显示列表
void CDemoCANDlg::OnButtonClear() 
{
	m_list.DeleteAllItems();	
}

/////////////////////////////////////////////////////////////////////////////







void CDemoCANDlg::OnCbnSelchangeComboDevtype()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CDemoCANDlg::OnCbnSelchangeComboCanIndex()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CDemoCANDlg::OnCbnSelchangeComboSendframetype()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CDemoCANDlg::OnEnChangeEditSendData()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}
