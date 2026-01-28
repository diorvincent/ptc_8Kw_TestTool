#if !defined(_CBUS_H_)
#define _CBUS_H_

#pragma once

#include "ControlCAN.h"
#include "usb_device.h"
#include "usb2lin_ex.h"
#include "PCAN-ISO-TP_2004.h"
#include <vector>
#include <iostream>
#include <cstdlib>
using namespace std;

class MappingStatus;
//动作执行基类
class Action
{
protected:
	virtual bool Scan_Dev() { return true; };
	
public:
	Action() {};

	virtual bool ConnectDevice() { return true; };
	virtual bool DisConnectDevice() { return true; };

	virtual bool Send(DWORD ID, int nCHN, unsigned char* pData, CString* pSendMsg) { return true; };
	virtual bool Send(DWORD ID, int nCHN, int nSendFrameType, unsigned char* pData, CString* pSendMsg) { return true; };
	virtual int Receive(VCI_CAN_OBJ* pCanObj, int index, CString* pReceiveMsg, DWORD* ID) { return 0; };
	virtual int Receive(DWORD ID, LIN_EX_MSG* pOutMsgs) { return 0; }

	virtual DWORD SetValue(WORD hConHandle, HANDLE eHandle) { return 0; };
	virtual bool isConnected() { return true; }

public:
	

	//device connect parameters
	typedef struct _CANParams{
		int m_DevType;					//device type
		int m_CANIndex;				//CAN0 or CAN1 on device
		int	 m_SendFrameType;		//Standard frame or Extend frame
		int	 m_SendFrameFormat;		//Remote frame or not

		struct _PCANParams
		{
			UINT m_ReqAdr;				//request id
			UINT m_RespAdr[6];			//resposne id
			bool m_IsFD = false;			//CAN FD or not
			bool m_BRS = false;			//BRS enable
			int m_LEN = 8;					//message length
#ifdef _P_CAN_
			TPCANTPHandle m_pctpHandle = PCANTP_USBBUS1;
			TPCANTPBaudrate m_baudrate = PCANTP_BAUD_500K; 
			TPCANTPHWType m_hwType = 0;
#endif
			UINT32 m_ioPort = 0;
			BYTE m_interrupt = 0;
		}PCANParam;
	}CANParams;

	CANParams m_CanParams;

	//Toomoss device handle
	int m_DevHandle;
	int gLINMasterIndex;
};

class CAN_Bus : public Action		//ChuangXin CAN
{
protected:
	virtual bool Scan_Dev() { return true; }
	
public:
	CAN_Bus();

	virtual bool ConnectDevice();
	virtual bool DisConnectDevice();

	virtual bool Send(DWORD ID, int nCHN, unsigned char* pData, CString* pSendMsg);
	virtual bool Send(DWORD ID, int nCHN, int nSendFrameType, unsigned char* pData, CString* pSendMsg);
	virtual int Receive(VCI_CAN_OBJ* pCanObj, int index, CString* pReceiveMsg, DWORD* ID);
	virtual int Receive(DWORD ID, LIN_EX_MSG* pOutMsgs) { return 0; }
	virtual DWORD SetValue(WORD hConHandle,  HANDLE eHandle) { return 0; };

	virtual bool isConnected();
public:
	bool g_mCANDevOK;
};


#ifdef _P_CAN_

class PCAN_Bus : public Action		//P-CAN
{
protected:
	virtual bool Scan_Dev() { return true; }
	
public:
	PCAN_Bus();
	~PCAN_Bus();

	virtual bool ConnectDevice();
	virtual bool DisConnectDevice();

	virtual bool Send(DWORD ID, int nCHN, unsigned char* pData, CString* pSendMsg);
	virtual bool Send(DWORD ID, int nCHN, int nSendFrameType, unsigned char* pData, CString* pSendMsg);
	virtual int Receive(VCI_CAN_OBJ* pCanObj, int index, CString* pReceiveMsg, DWORD* ID);
	virtual int Receive(DWORD ID, LIN_EX_MSG* pOutMsgs) { return 0; }

	virtual bool isConnected();
	virtual DWORD SetValue(WORD hConHandle, HANDLE eHandle) ;
private:
	vector<MappingStatus*> m_mappings; // List of configured PCAN-ISO-TP mappings.
	void MappingsAdd(MappingStatus* mapping);
	void PostNcDestroy();

	// Tester Client address
	byte clientAddr[6] = { 0xF1, 0x2E, 0xDA, 0xB6, 0x3C, 0x51 };
	// ECU address
	byte ecuAddr[6] = { 0x10, 0xA1, 0x2B, 0xC3, 0x4D, 0xE5 };

public:
	bool g_mCANDevOK;

	TPCANTPMsg m_msg;
	//CANFD connect string	//f_clock_mhz=20, nom_brp=5, nom_tseg1=2, nom_tseg2=1, nom_sjw=1, data_brp=2, data_tseg1=3, data_tseg2=1, data_sjw=1
	const char* m_CANFD_ConStr = "f_clock=20000000, nom_brp=5, nom_tseg1=5, nom_tseg2=2, nom_sjw=1, data_brp=1, data_tseg1=2, data_tseg2=2, data_sjw=1";
};

/// <summary>
/// Class that stores information of a single ISO-TP mapping.
/// </summary>
class MappingStatus
{
public:
	MappingStatus(
		UINT32 canId,
		UINT32 canIdResponse,
		TPCANTPIdType canIdType,
		TPCANTPFormatType formatType,
		TPCANTPMessageType msgType,
		TPCANTPAddressingType targetType,
		byte sourceAddr,
		byte targetAddr,
		byte remoteAddr);

public:
	UINT32 m_canId;
	UINT32 m_canIdResponse;
	TPCANTPIdType m_canIdType;
	TPCANTPFormatType m_formatType;
	TPCANTPMessageType m_msgType;
	byte m_sourceAddr;
	byte m_targetAddr;
	TPCANTPAddressingType m_targetType;
	byte m_remoteAddr;
};

#endif


class LIN_Bus : public Action	//TOOMOSS LIN
{
protected:
	int gDevHandle[10];
	int gDevIndex;

	virtual bool Scan_Dev();

public:
	LIN_Bus();

	virtual bool ConnectDevice();
	virtual bool DisConnectDevice();

	virtual bool Send(DWORD ID, int nCHN, unsigned char* pData, CString* pSendMsg);
	virtual bool Send(DWORD ID, int nCHN, int nSendFrameType, unsigned char* pData, CString* pSendMsg);
	virtual int Receive(VCI_CAN_OBJ* pCanObj, int index, CString* pReceiveMsg, DWORD* ID) { return 0; }
	virtual int Receive(DWORD ID, LIN_EX_MSG* pOutMsgs);

	virtual bool isConnected() { return true; }
	virtual DWORD SetValue(WORD hConHandle,  HANDLE eHandle) { return 0; };
};


//抽象命令类
class Command {
protected:
	Action* receiver;
public:
	Command(Action* temp)
	{
		receiver = temp;
	}

	virtual bool ExecuteCmd() = 0;
	virtual void Send(DWORD ID, int nCHN, unsigned char* pData, CString* pSendMsg)=0;
	virtual bool Send(DWORD ID, int nCHN, int nSendFrameType, unsigned char* pData, CString* pSendMsg)=0;
	virtual int Receive(VCI_CAN_OBJ* pCanObj, int index, CString* pReceiveMsg, DWORD* ID) = 0;
	virtual int Receive(DWORD ID, LIN_EX_MSG* pOutMsgs) = 0;
};


//连接设备命令
class ConnectDevCmd : public Command
{
public:
	ConnectDevCmd(Action* temp) : Command(temp) {}
	virtual bool ExecuteCmd()
	{
		return receiver->ConnectDevice();
	}
	virtual void Send(DWORD ID, int nCHN, unsigned char* pData, CString* pSendMsg)
	{
			
	}
	virtual bool Send(DWORD ID, int nCHN, int nSendFrameType, unsigned char* pData, CString* pSendMsg)
	{
		return true;
	}
	virtual int Receive(VCI_CAN_OBJ* pCanObj, int index, CString* pReceiveMsg, DWORD* ID)
	{
		return 0;
	}
	virtual int Receive(DWORD ID, LIN_EX_MSG* pOutMsgs)
	{
		return 0;
	}
};

//断开连接命令
class DisConnectDevCmd : public Command
{
public:
	DisConnectDevCmd(Action* temp) : Command(temp) {}

	virtual bool ExecuteCmd()
	{
		return receiver->DisConnectDevice();
	}
	virtual void Send(DWORD ID,int nCHN, unsigned char* pData, CString* pSendMsg)
	{

	}
	virtual bool Send(DWORD ID, int nCHN, int nSendFrameType, unsigned char* pData, CString* pSendMsg)
	{
		return true;
	}
	virtual int Receive(VCI_CAN_OBJ* pCanObj, int index, CString* pReceiveMsg, DWORD* ID)
	{
		return 0;
	}
	virtual int Receive(DWORD ID, LIN_EX_MSG* pOutMsgs)
	{
		return 0;
	}
};

//发送命令
class SendCmd :public Command
{
public:
	SendCmd(Action* temp):Command(temp){}
	virtual bool ExecuteCmd() { return true; }
	virtual void Send(DWORD ID, int nCHN, unsigned char* pData, CString* pSendMsg)
	{
		receiver->Send(ID, nCHN, pData, pSendMsg);
	}
	virtual bool Send(DWORD ID, int nCHN, int nSendFrameType, unsigned char* pData, CString* pSendMsg)
	{
		return receiver->Send(ID, nCHN, nSendFrameType, pData, pSendMsg);
	}
	virtual int Receive(VCI_CAN_OBJ* pCanObj, int index, CString* pReceiveMsg, DWORD* ID) { return 0; }
	virtual int Receive(DWORD ID, LIN_EX_MSG* pOutMsgs) { return 0; }
};

//接收命令
class ReceiveCmd :public Command
{
public:
	ReceiveCmd(Action* temp) :Command(temp) {}
	virtual bool ExecuteCmd() { return true; }
	virtual void Send(DWORD ID, int nCHN, unsigned char* pData, CString* pSendMsg){}
	virtual int Receive(VCI_CAN_OBJ* pCanObj, int index, CString* pReceiveMsg, DWORD* ID)
	{
		return receiver->Receive(pCanObj, index, pReceiveMsg, ID);
	}
	virtual int Receive(DWORD ID, LIN_EX_MSG* pOutMsgs)
	{
		return receiver->Receive(ID, pOutMsgs);
	}
};

//执行类
class Executer
{
protected:
	vector<Command*> m_commandList;
public:
	~Executer()
	{
		ClearCmds();
	}

	void ClearCmds()
	{
		m_commandList.clear();//清除旧的命令		
	}
	
	Command* Undo()
	{
		vector<Command*>::reverse_iterator p = m_commandList.rbegin();
		Command* pCommand = *p;
		m_commandList.pop_back();
		return pCommand;
	}

	void SetCmd(Command* temp)
	{
		m_commandList.push_back(temp);
		cout << "增加操作" << endl;
	}

	//通知执行
	bool Notify()
	{
		bool bRight = false;

		vector<Command*>::reverse_iterator p = m_commandList.rbegin();
		if (!m_commandList.empty())
			bRight = (*p)->ExecuteCmd();

		return bRight;
	}

	void Send(DWORD ID, int nCHN, unsigned char* pData, CString* pSendMsg)
	{
		vector<Command*>::reverse_iterator p = m_commandList.rbegin();
		if (!m_commandList.empty())
		{
			(*p)->Send(ID, nCHN, pData, pSendMsg);
			m_commandList.clear();
		}
	}

	void Send(DWORD ID, int nCHN, int nSendFrameType, unsigned char* pData, CString* pSendMsg)
	{
		vector<Command*>::reverse_iterator p = m_commandList.rbegin();
		if (!m_commandList.empty())
		{
			(*p)->Send(ID, nCHN, nSendFrameType, pData, pSendMsg);
			m_commandList.clear();
		}
	}

	int Receive(VCI_CAN_OBJ* pCanObj, int index, CString* pReceiveMsg, DWORD* ID)
	{
		int nReceiveNum = 0;

		vector<Command*>::reverse_iterator p = m_commandList.rbegin();
		if (!m_commandList.empty())
		{
			nReceiveNum = (*p)->Receive(pCanObj, index, pReceiveMsg, ID);
			m_commandList.clear();
		}
		return nReceiveNum;
	}

	int Receive(DWORD ID, LIN_EX_MSG* pOutMsgs)
	{
		int nReceiveNum = 0;

		vector<Command*>::reverse_iterator p = m_commandList.rbegin();
		if (!m_commandList.empty())
			nReceiveNum = nReceiveNum = (*p)->Receive(ID, pOutMsgs);

		return nReceiveNum;
	}
};


#endif