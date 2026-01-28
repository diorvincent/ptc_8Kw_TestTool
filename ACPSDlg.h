// ACPSDlg.h : header file
//

#if !defined(AFX_ACPSDLG_H__413D8310_FEF9_4710_859D_1793FC5EF671__INCLUDED_)
#define AFX_ACPSDLG_H__413D8310_FEF9_4710_859D_1793FC5EF671__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "Config.h"
#include "Range.h"
#include "CXPButton.h"
#include "serial.h"
#include "numtype.h"
#include "afxwin.h"
#include "ControlCAN.h"
#include "ListCtrlP.h"
#include "NumberEdit.h"
#include "CBus.h"

#include <stdarg.h>
#include <Dbt.h>

#include <vector>
#include <codecvt>
#include <locale>
#include <string>
#include <iostream>

/////////////////////////////////////////////////////////////////////////////
// CACPSDlg dialog

#define WM_UPDATEDATA WM_USER + 1 //自定义消息，更新界面中接收到的报文
#define	WM_TEST_ABNORMAL WM_USER + 10

const int CommBaud[18]={75,110,134,150,300,600,1200,1800,2400,4800,
7200,9600,14400,19200,38400,57600,115200,128000};
const int CommData[4] = { 5,6,7,8 };
const CString CommCheck[5]={"None","Odd","Even","Mark","Space"};


#define       LINNUM              4
#define       COMNUM            5
#define       SENSORNUM     10
#define       CHNNUM           10
#define       MAXPACKAGELENGTH   100
#define WM_PRE_CHANGE (WM_USER)
#define WM_TEM_CHANGE (WM_USER+100)
#define WM_SEN_CHANGE (WM_USER+200)
#define  MAX_SIZE 1 //16

#define SOFTWARE_VERSION		4
#define POWER_CALIBRATION	2
#define CALI_POWRER_WAIT_TIME0 10000
#define CALI_POWRER_WAIT_TIME1 40000
#define CALI_POWRER_WAIT_TIME 60000

typedef struct { int HighVolPower_Addr; }HIGHVOLPOWER_ADDR;

union LIN_PID
{
	unsigned char PID;
	struct
	{
		unsigned char  bit0 : 1;
		unsigned char  bit1 : 1;
		unsigned char  bit2 : 1;
		unsigned char  bit3 : 1;
		unsigned char  bit4 : 1;
		unsigned char  bit5 : 1;
		unsigned char  bit6 : 1;
		unsigned char  bit7 : 1;
	}bit;
};

typedef struct
{
	int CANFD;
	int BRS;
}CANFD;

typedef struct
{
	int DEVNUM;
	int LINCHANNEL;
}LINConfig; //PCBALIN

typedef struct 
{
	int PORT;
	int BUND;
    int DATA;
	int	 STOP;
    int CHECK;
}Config;//PCBACAN

typedef struct
{
	float high_vol;//高压电源输出电压
	float high_cur;//高压电源输出电流
	float high_power;//高压电源输出功率

	string serise_num1, serise_num2;//产品序列码

	float GearMode_power; //挡位控制模式功率
	float TempMode_power; //温度控制模式功率

	bool Check_CAN_Recv1;		//2. 校验CAN接收数据
	bool Check_Rs485_Cmd; //3.	校验485接收数据
	bool Check_LIN_Data;	//4.	校验LIN接收数据
	bool Send_Rs485_Cmd_CheckLIN; //5.	发送485指令接收LIN数据
	bool Set_Pump_Power;	//校验接收信号Flow_Switch_1 == HIGH
	bool Set_FAN_Power;		//校验接收信号Flow_Switch_2 == HIGH；
	bool Check_CAN_Recv2; //6.3.2.	校验CAN接收信号2
	bool Set_FAN_Type;		//校验信号Pwm_Dutycycle_FAN1介于 [70%, 80%]，Pwm_Freq_ FAN1介于 [9900Hz, 1100Hz]；

	bool ov1, ov2, uv1, uv2, uv3, uv4;
	bool ov1_1, ov2_1, uv1_1, uv2_1, uv3_1, uv4_1;

	bool highvol_overvol1, highvol_overvol2;	//高压过压
	bool highvol_undervol1, highvol_undervol2;	//高压欠压

	bool lowpwr_curr1, lowpwr_curr2; //低压电流
	bool eep_mark_byte1, eep_mark_byte2;//标志位写入
	bool cycle_time1, cycle_time2; //报文周期
	bool project_info1, project_info2; //项目信息
	bool software_ver1, software_ver2;//软件版本

	bool igbt_temp1, igbt_temp2;	//冷却液入口温度 
	bool ptc_temp1, ptc_temp2;	//冷却液出口温度
	bool ptc_env_temp1, ptc_env_temp2;	//PTC & 环境温度 差小于2

	float fCalied_50Slope1, fCalied_50Slope2;		//功率校正后的斜率50
	float fCalied_100Slope1, fCalied_100Slope2;	//功率校正后的斜率100
	float fCalied_Power1, fCalied_Power2;				//功率校正后的功率
	float fCalied_PowerErr1, fCalied_PowerErr2;		//功率校正后的功率误差

	float PowerMode_power, PowerMode_power_1, PowerMode_power_2, PowerMode_power_3;//功率控制模式功率
	float PowerError_1, PowerError_2, PowerError_3, PowerError_4; //功率误差

}PcbaStru;//PCBA返回值

typedef struct
{
	float lowpre, lowcur; //额定低压, 额定电流
	float low_pdelay;//上电延时
	float low_checkcur, high_checkcur;//低压电流检查
	float low_tmp, high_tmp;
	float low_5v_low, low_5v_high;
	float high_5v_low, high_5v_high;
	float drv_vol_low, drv_vol_high;
	float in_tmpcail_low, in_tmpcail_high;
	float out_tmpcail_low, out_tmpcail_high;
	float IGBT_tmpcail_low, IGBT_tmpcail_high;
	float ov1, ov2, uv1, uv2, uv3, uv4;
	float high_vol;//高压电源输出电压
	float high_cur;//高压电源输出电流
	float high_power;//高压电源输出功率
	float curerror_low, curerror_high;//电流误差设置
	float curerror1_low, curerror1_high;//电流误差设置
	float hv1, hv1error_low, hv1error_high, hv2, hv2error_low, hv2error_high, hv3, hv3error_low, hv3error_high, hv4,hv5;
	float curtime;//被动放电检测设定值
	int msg_error_low, msg_error_high;//报文误差范围
	float power_range_low, power_range_high; //功率控制模式范围
	float gear_power;	//挡位控制模式功率范围
	float temp_power;	//温度控制模式功率范围

	float ptc_overtemp_low, ptc_overtemp_high; //ptc过温
	float igbt_overtemp_low, igbt_overtemp_high; //igbt过温

	float highvol_over_vol; //高压过压电压
	float highvol_over_restore_vol; //高压过压恢复电压
	float highvol_under_vol;//高压欠压电压
	float highvol_under_restore_vol;//高压欠压恢复电压

	float max_ptc_power; //ptc最大功率
	float max_ptc_power_error; //ptc最大功率误差
	int waitting_time; //功率测试等待时间
	float max_ptc_power_5Kw_error; //ptc最大功率误差(5Kw)
	float max_ptc_power_5Kw; //ptc最大功率(5Kw)
	int waitting_time_5Kw; //功率测试等待时间(5Kw
	float run_current; //测试电流

	//功率校准
	double slope_base1;	//斜率基准值1
	double slope_base2;	//斜率基准值2
	double slope_error1;	//斜率误差值21
	double slope_error2;  //斜率误差值22

	float duty_cycle_value1; //占空比1值
	float duty_cycle_value2; //占空比2值
	float duty_cycle_value3; //占空比3值

	int per50_wait_time; //50%占空比等待时间
	int per100_wait_time; //100%占空比等待时间
	int per90_wait_time; //90%占空比等待时间

	std::string project_info;	//项目信息

	int msg_lost_monitor_cycle; //报文丢失监控周期

}CfgSet;//标定量程配置

//定义LIN信息帧
typedef  struct _LIN_MSG_RECV {
	UINT byte0;
	UINT byte1;
	UINT byte2;
	UINT byte3;
	UINT byte4;
	UINT byte5;
	UINT byte6;
	UINT byte7;
}LIN_MSG_RECV, *PLIN_MSG_RECV;

//模块CAN报文
typedef  struct _ECU_HVH_Frame1 {
	UINT HVH_MaxPower;
	int HVH_TargetCoolantTemp;
	u8 HVH_HeaterEnable;
	u8 HVH_ActiveDischarge;
}ECU_HVH_Frame1, * PECU_HVH_Frame1;

typedef  struct _HVH_ECU_Frame1 {
	u8 HVH_Status;
	u8 HVH_HVIL_Status;
	u8 HVH_ActiveDis_Status;	
	int HVH_Coolant_In_Temp;
	int HVH_Coolant_Out_Temp;
	float HVH_HV_Current;
	float HVH_HV_Voltage;
	UINT HVH_ActualPower;
}HVH_ECU_Frame1, *PHVH_ECU_Frame1;

typedef  struct _HVH_ECU_Frame2 {
	u8 HVH_ProtocolError;
	u8 HVH_ComponentProtection;
	u8 HVH_WarnHVOutOfRng;
	u8 HVH_WarnULoOutOfRang;
	u8 HVH_WarnOverheat;
	u8 HVH_FailHotspot;
	u8 HVH_FailTempSensor;
	u8 HVH_FailDriver;
	u8 HVH_FailHighCurrent;
	u8 HVH_FailMemory_InterCom;
	u8 HVH_FailDCDCConverter;
}HVH_ECU_Frame2, *PHVH_ECU_Frame2;

typedef enum _LowPower_CTRL {
	POWER_ON = 0,
	POWER_OFF,
	POWER_SET,
}LowPowerCtrl;

//_


class CACPSDlg : public CDialog
{
// Construction
public:
	bool OnHighPowerIn(CSerial *port); //开启高压电源进口
	bool OffHighPowerIn(CSerial *port);//关闭高压电源进口
	bool OnHighPowerOut(CSerial *port);//开启高压电源出口
	bool OffHighPowerOut(CSerial *port);//关闭高压电源出口
	bool WaterInletNTCShortTest(CSerial *port);//开启进水口短路测试
	bool WaterInletTemperTest(CSerial *port);//开启进水口温度测试
	bool OffWaterInletTest(CSerial *port);//关闭进水口温度测试
	bool WaterOutletNTCShortTest(CSerial *port);//开启出水口短路测试
	bool WaterOutletTemperTest(CSerial *port);//开启出水口温度测试
	bool OffWaterOutletTest(CSerial *port);//关闭进水口温度测试
	bool IGBTNTCShortTest(CSerial *port);//开启IGBT短路测试
	bool IGBTTemperTest(CSerial *port); //开启IGBT温度测试
	bool OffIGBTTest(CSerial *port);//关闭进水口温度测试
	bool OnRedLed(CSerial *port);//开启红灯
	bool OffRedLed(CSerial *port); //关闭红灯
	bool OnYellowLed(CSerial *port);//开启黄灯
	bool OffYellowLed(CSerial *port); //关闭黄灯
	bool OnGreenLed(CSerial *port); //开启绿灯
	bool OffGreenLed(CSerial *port); //关闭绿灯

	//通用
	unsigned char DeCode(unsigned char a,unsigned char axor);
	unsigned char EnCode(unsigned char a,unsigned char axor);
	unsigned long power(int base, int times);
	int DectoBCD(int Dec, unsigned char *Bcd, int length);
	unsigned long BCDtoDec(const unsigned char *bcd, int length);

	void ReSize();
    POINT old;
	CACPSDlg(CWnd* pParent = NULL);	// standard constructor
	void AutoAdjustColumnWidth(CListCtrl *pListCtrl);

	CConfig	*m_configdlg;
	CRange *m_rangedlg;

	DWORD prevSendtime;//上次发送时间
	BOOL m_bportPreOpened, m_bportTemOpened;
	CString m_strRevMsg;//接收到的报文

// Dialog Data
	//{{AFX_DATA(CACPSDlg)
	enum { IDD = IDD_ACPS_DIALOG };
	CCXPButton	m_delete;
	CCXPButton	m_exit;
	CListBox	m_list1;
	CListCtrlP	m_listexcel;
	CCXPButton	m_buttonmsg;

	CBrush m_brush;
	HBRUSH m_brushEdit;
	HBRUSH m_brushList;
	CStatic	m_static1;
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL
private:    
     CFont m_Font;  
	 LOGFONT logfont;


public:
	CString timestr;
	CTime tm;
	CString strCurrentPath;
	int portnumber;//拔插的串口号

	int m_DevType;
	bool OpenDevice();
	void CloseDevice();
	void SendCANMsg(u8 HVH_MaxPower, u8 HVH_TargetCoolantTemp, u8 HVH_HeaterEnable, u8 HVH_ActiveDischarge, int nCHN = 1);
	void setcfg_tolist(int cfgnum);
	void ClearList();
	void output_debug_info(const char* pStr);
	void DispalyCurrentMsg(CString strMsg, bool bRecv=true, int nChannel = 1);

	//+ LIN報文
	int Init_LIN();
	bool Close_LIN_Device();
	int Send_Master_Data(BYTE* pData);
public:
	int		m_nSendFrameFormat;
	int		m_nSendFrameType;
	CString	m_strSendData;
	CString	m_strSendID;
	int		m_radioIDFormat;
	bool	m_bCanRxEn;
	int		m_nCanIndex;
	int		m_nDevType;
	int		m_nDevIndex;
	UINT	m_nRollingCounter;
	int		m_nRollingCnt_RandomVal;
	int		m_nPowerON_CANTime;
	int		m_nLINMode;

	bool g_mCANDevOK = false; //CAN device connected
	bool g_LINDevOK = false;  //Lin device connected


	 /// <summary>
 /// Handle on event when a CAN message is coming.
 /// </summary>
	HANDLE m_eventHandleToReadCAN;
	/// <summary>
	/// Handle on event to stop thread when a different mode is selected (manual or timer).
	/// </summary>
	HANDLE m_eventHandleToStopWaitingForObject;


// Implementation
private:
	void  ReadConfig();
	void  ReadRange(int cfgnum);
	void  ReadBoxCfg();
	void _InitVARs();
protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExit();
	afx_msg void OnSelchangeCombo1();
	afx_msg void OnDelete();
	afx_msg void OnACPSCancel();
	afx_msg LRESULT OnAbnormalTest(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateData(WPARAM wParam, LPARAM lParam);
	void WriteExcelData(CString strBookPath);
	
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	bool m_bErrMsg; //总测试结果标志
	BOOL m_bMeasureHighVolCurr;
	int m_nHighVolTimeCnt;
	// 扫描枪输入// 扫描枪输入
	CEdit m_overedit;
	CString m_strInput;
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	//高/低压电源、功率计、数字量输出设备
	ConcreteHardwareCom* m_Mediator;
	LowVolPowerCom* m_lowPwr;
	HighVolPowerCom* m_highPwr;

	//总线设备
	Action* m_Bus, *m_pLinBus;
	Executer* m_exec;
	bool m_stopReadThread;

	void readMessages();	
	bool WriteErrMsg(CString msg, CACPSDlg *pDlg, int x=0, int y = 0);

	void CleanUpSRC();
	void SendMsg(byte power, byte gear, byte temp, byte ac_setptcsts = 0x01, bool bPowerTest = false);

	//UDS 0x10 03 request
	int UDS_0x1003Request();
	//UDS 0x27 01 request seed
	int UDS_0x2701Request();
	//UDS 0x27 02 send key
	int UDS_0x2702Request();

	//UDS访问0x31服务前流程
	int UDS_RequestBefore0x31Svr();
	//禁止APP报文
	int UDS_DisableAppMsg();
	//使能APP报文
	int UDS_EnableAppMsg();

	//清空标志位
	void ResetUDSRespMark();

	//写标志位2
	bool WriteMarkByte(byte* pData, int nWaitTimes);

	//项目信息验证
	bool VerifyProjectInfo(int nPTC);

	//软件版本验证
	bool VerifySoftwareVer(int nPTC);
	
	//创建测试项目
	void BuildTestItems();

	//验证软件版本号
	bool CheckSeriseNum(CACPSDlg* pDlg);

	//点亮黄灯
	bool YellowLed_On(CACPSDlg* pDlg);

	//清除故障码
	bool ClearDTC(CACPSDlg* pDlg);

	//功率校准
	bool ptc_Power_Calibration(CACPSDlg* pDlg);

	//发送CAN请求报文
	bool SendMsgOnCAN(DWORD ID, PHVH_ECU_Frame1 pData);
	//写标志位
	bool WriteMarkByte(int nWaitTimes);
	//重启动
	void ResetLowPower();
	//写日志
	/*static */void _Log(CString strInfo="");
	//高精度时钟任务
	float timerFunction(bool bStartTimer, int nExpireTime = 0);


	
	/***********************8Kw测试用例***************************/

	//总线报文容器，用于测试结束写日志
	std::vector<CString> m_can_message_lst;

	 //8710功率值搜集
	std::vector<float> m_8710PowerLst1;
	std::vector<float> m_8710PowerLst2;

	//测试结果集合
	std::vector< PcbaStru> m_test_result_lst;

	bool m_bEEPWrote, m_bEEPWrote2; //写App跳转标志位成功?
	bool m_bReadProjectInfo1, m_bReadProjectInfo2; //读取项目信息
	bool m_bSoftwareVerVerified, m_bSoftwareVerVerified2;//软件版本验证

	//功率校准涉及的诊断访问变量
	volatile bool m_bSplit_ReqADCCounterFail;	//拆分读取ADCCounter失败标志(失败后整合命令再次请求)

	volatile bool m_UDSREQ1, m_UDSREQ2; //UDS诊断请求
	volatile bool m_UDS1REQ1003, m_UDS2REQ1003; // 0x10 03请求
	volatile bool m_UDS1REQ2801, m_UDS2REQ2801; // 0x28 01 01请求
	volatile bool m_UDS1REQ2800, m_UDS2REQ2800; // 0x28 00 01请求
	volatile bool m_UDS1REQ2701, m_UDS2REQ2701; // 0x27 01请求
	volatile bool m_UDS_KEYSEND1, m_UDS_KEYSEND2; //UDS诊断安全访问请求

	bool m_bStartCaliPowerOK1, m_bStartCaliPowerOK2; //功率校准开始	//step1
	bool m_bStartCaliPower100vOK1, m_bStartCaliPower100vOK2; //100v功率校准开始		//step2
	bool m_bStartCaliPower400vOK1, m_bStartCaliPower400vOK2; //400v功率校准开始		//step3
	bool m_bStartKuOK1, m_bStartKuOK2; //斜率计算开始		//step4
	bool m_bStart50KuOK1, m_bStart50KuOK2; //占空比为50%开始		//step5
	bool m_bStartGetADCValue1, m_bStartGetADCValue2; //检测高压电源电流值开始	//step6
	bool m_bStart100KuOK1, m_bStart100KuOK2; //占空比为100%开始		//step7
	bool m_bStartGetADCValue3, m_bStartGetADCValue4; //检测高压电源电流值开始	//step8
	bool m_bStartCaliPower11OK1, m_bStartCaliPower11OK2; //确认复位成功后，通过31服务请求进入校正确认开始	//step11
	bool m_bReadCaliedVolOK1, m_bReadCaliedVolOK2; //通过31服务读取对应校正后的电压值	//step14
	bool m_bReadCaliedCurrOK1, m_bReadCaliedCurrOK2;// 通过31服务读取校正后的电流值  //step15
	bool m_bStartCaliPowerExist1, m_bStartCaliPowerExist2; //功率校准退出开始	//step17

	volatile bool m_UDS1RESP1003, m_UDS2RESP1003; // 0x10 03响应
	volatile bool m_UDS1RESP2801, m_UDS2RESP2801; // 0x28 01 01响应
	volatile bool m_UDS1RESP2800, m_UDS2RESP2800; // 0x28 00 01响应
	volatile bool m_UDS1RESP2701, m_UDS2RESP2701; // 0x27 01响应
	volatile bool m_UDSRESP1, m_UDSRESP2; //UDS seed诊断响应
	volatile bool m_UDS_KEYRESP1, m_UDS_KEYRESP2; //UDS key诊断安全访问响应

	bool m_bEndCaliPowerOK1, m_bEndCaliPowerOK2; //功率校准结束
	bool m_bEndCaliPower100vOK1, m_bEndCaliPower100vOK2; //100v功率校准结束
	bool m_bEndCaliPower400vOK1, m_bEndCaliPower400vOK2; //400v功率校准结束
	bool m_bEndKuOK1, m_bEndKuOK2; //斜率计算结束
	bool m_bEnd50KuOK1, m_bEnd50KuOK2; //占空比为50%结束
	bool m_bEndGetADCValue1, m_bEndGetADCValue2; //检测高压电源电流值结束
	bool m_bEnd100KuOK1, m_bEnd100KuOK2; //占空比为100%结束		
	bool m_bEndGetADCValue3, m_bEndGetADCValue4; //检测高压电源电流值结束	
	bool m_bEndCaliPower11OK1, m_bEndCaliPower11OK2; //确认复位成功后，通过31服务请求进入校正确认结束	//step11
	bool m_bEndReadCaliedVolOK1, m_bEndReadCaliedVolOK2; //确认通过31服务读取对应校正后的电压值	//step14
	bool m_bEndReadCaliedCurrOK1, m_bEndReadCaliedCurrOK2;// 确认通过31服务读取校正后的电流值  //step15
	bool m_bEndCaliPowerExist1, m_bEndCaliPowerExist2; //功率校准退出

	//_________end___________//

	bool m_bClearDTC1, m_bClearDTC2;	//清除DTC
	volatile bool m_bTested_Msg_Cycle; //报文周期检测开始?

	//保存接收到的报文，用于计算报文周期误差
	std::vector<PHVH_ECU_Frame1> m_recv_msglst1;
	std::vector<PHVH_ECU_Frame2>  m_recv_msglst2;
	//std::vector<long long> m_recv_msglst1;
	//std::vector<long long>  m_recv_msglst2;

	std::vector<float> m_DutyCycle_Current;	//保存功率计读取到的电流值HI1 / HI2，连续读取求均值	

	//高压电压设置
	void AdjustHighPowerVol(float fVol, float fCurr);
	//10%占空比请求(下电)
	bool _per10DutyCycleReq(float fDutyCycle);

	//等待功率计到达目标电压
	bool Waitfor_8710ArrivedDstVol(float fDstVol, int nWaitTime, float* fVol1, float* fVol2, bool bSend0x3E=false);

	//等待功率计到达目标电流
	bool Waitfor_8710ArrivedDstCurr(float fDstCurr, int nWaitTime, float* fCurr, int nCHN, bool bSend0x3E = false);

	//功率斜率计算
	bool Cali_PowerSlope(int* nPTC);
	//功率校准期间监控设备异常
	bool Moniter_DevLostInCali(int* nPTC);
	

	//界面提示信息(功率校准)
	void ShowStepMessage(const char* pMsg, u16 ADCCounter = 0, double resVal = 0.0f, u8* pKu = NULL);

	//功率校准请求
	bool CaliPowerReq(byte* pData, int nPTCIndex, int nWaitTimes);

	//保持诊断会话状态
	void Cycyle_0x3E();

	//监测通信丢失
	bool Monitoring_ComLost();

	//运行时间计算
	void  Cal_UnitTest_Time(bool bStart, long* lExpenditureTime, int nIntervalFrm = 0);

	//产品序列号有效性
	bool Check_SerialNumber();

	//闭合断开继电器J3(接通/断开低压回路)
	bool PowerJ3Relay(bool bOn = true);

	//闭合/断开继电器J3(接通/断开高压回路)
	bool PowerJ4Relay(bool bOn = true);

	//低压电源控制
	bool LowPower_CTRL(LowPowerCtrl lowPwrCtrl, float fVol=0.0f, float fCurr = 0.0f);

	//输出测试报告
	void WriteTestReport();

	//低压电流(检测低压电路情况)
	bool lowPower_CurrentTest();

	//调用写标志位
	bool Invoke_WriteMarkByte();

	//诊断响应处理
	bool UDS_Respose(byte* pData, int nCanIdx);

	//报文周期检查
	bool Check_MsgCycle();

	//项目信息检测
	bool Check_ProjectInfo();

	//软件版本验证
	bool Check_SoftwareVerion();

	//检测低压电压诊断电路
	bool Check_LowPowerCircuit();

	//检测IGBT传感器电路
	bool Check_IGBTCircuit();

	//检测PTC传感器电路
	bool Check_PTCCircuit();

	//检测PCB温度传感器电路
	bool Check_PCBTempCircuit();

	//功率测试
	bool Run_Selected_PowerMode(CACPSDlg* pDlg);
	bool Power_CTRL_Mode(CACPSDlg* pDlg, CSerial* port, byte nPower, int nWaitTime, byte nPower_5Kw, int nWaitTime_5Kw, float* value, float* value2);//功率控制模式
	bool Power_CTRL_Mode_CAN(CACPSDlg* pDlg, CSerial* port, byte nPower, int nWaitTime, byte nPower_5Kw, int nWaitTime_5Kw, float* value, float* value2);//功率控制模式
	
	//高压电压诊断
	bool HighVol_Diagnostic();
	bool HighVol_OverVoltage();
	bool HighVol_UnderVoltage();
				
	//运行结束
	void TestEndProcess();

	//高压/低压下电
	bool LowHigh_PowerDown();
								
	//普通报文发送打开/关闭
	void SendAppMsg_OnTimer(int nStep = 0);

																																					 
	/************************************************************/
	
	
	void ResetPower();
	void StopThreads();

	afx_msg void OnBnClickedBtnstart();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedOffmsg();
	afx_msg void OnBnClickedBtnstop();
	
	afx_msg void OnCbnDblclkCombocfg();
	afx_msg void OnCbnSelchangeCombocfg();

	afx_msg void OnCbnKillfocusCombocfg();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	
	afx_msg void OnBnClickedBoxcfg();
	virtual void OnOK();
	afx_msg void OnSelchangeCombolinmode();
	afx_msg void OnKillfocusCombolinmode();
	afx_msg void OnDropdownCombolinmode();
private:
	CStatic m_ManHuiLogo;
	CStatic m_ENVTEMP;


public:
	//总线通信监控
	ULONG m_ulTicktCount1, m_ulTicktCount2;
	bool m_bLostPtc1, m_bLostPtc2; //CAN报文通信丢失
	bool m_bDevLost1, m_bDevLost2;//低压电源通信丢失， 高压电源通信丢失
	bool m_bLowPowerLost; //低压电源断电
	bool m_bStartCollectPower;


	CCXPButton m_boxcfg;
	CListBox m_msglst;
	CNumberEdit m_modevalue;
	CNumberEdit m_WaitTime;
	
	afx_msg void OnBnClickedRadioPower();
	afx_msg void OnBnClickedRadioGear();
	afx_msg void OnBnClickedRadioTemp();
	
	afx_msg void OnEnChangeEditMode();
	afx_msg void OnEnSetfocusEditMode();
	afx_msg void OnEnChangeEditWaittime();
	afx_msg void OnEnSetfocusEditWaittime();
	afx_msg void OnBnClickedCommsetting();
	afx_msg void OnBnClickedReportExport();
	afx_msg void OnBnClickedTestParams();

	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD dwData);

	//打开/关闭扫描枪串口
	void Switch_ScannerGun_Port(bool bOpen);

	//开始/暂停报文发送/接收
	void StartStopMessage(bool bPause = true);

	CCXPButton m_StartTestTH;
	CCXPButton m_PauseTestTH;
	CCXPButton m_CommSetting;
	CCXPButton m_ReportExport;
	CCXPButton m_ParamsSetting;
	
	CCXPButton m_ScanGun;
	CEdit m_serise;
	CEdit m_overedit2;
	CStatic m_TestStatus;
	afx_msg void OnClose();
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACPSDLG_H__413D8310_FEF9_4710_859D_1793FC5EF671__INCLUDED_)
