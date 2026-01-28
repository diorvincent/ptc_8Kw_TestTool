/*
* AUTHOR: Jingchi.He
* DATE:9/16/2025
* UPDATE DATE: 11/7/2025
* Add P-CAN bus adapter support for CAN
* 
* DESIGN PATTERN:Command
* PURPOSE: 
* Encapsulate a request as an object, thereby allowing you to parameterize clients with different requests; 
* queue or log requests, and support undoable operations.
*/

#include "stdafx.h"
#include "CBus.h"

#pragma comment(lib,"ControlCAN.lib")
#pragma comment(lib,"USB2XXX.lib")

#ifdef _P_CAN_
#pragma comment(lib,"PCAN-ISO-TP.lib")
#endif


CAN_Bus::CAN_Bus()
{
	g_mCANDevOK = false;
}

bool CAN_Bus::ConnectDevice()
{
	if (VCI_OpenDevice(m_CanParams.m_DevType, 0, 0) != 1)
	{
		return g_mCANDevOK;
	}

	VCI_INIT_CONFIG InitInfo[1];
	//Timing0:0x00, Timing1:0x1C  = 500k
	//Timing0:0x10, Timing1:0x1C  = 250k
	InitInfo->Timing0 = 0x01;
	InitInfo->Timing1 = 0x1C;
	InitInfo->Filter = 0;
	InitInfo->AccCode = 0x80000000;
	InitInfo->AccMask = 0xFFFFFFFF;
	InitInfo->Mode = 0;

	//初始化通道0
	if (VCI_InitCAN(m_CanParams.m_DevType, 0, m_CanParams.m_CANIndex, InitInfo) != 1)	//can-0
	{
		return g_mCANDevOK;
	}
	Sleep(100);

	////智能滤波设置，需要的时候启用以下代码
	//VCI_FILTER_RECORD FILTER[1];	
	//FILTER[0].ExtFrame = 1; FILTER[0].Start = gSlaveDM1; FILTER[0].End = gSlavePID;//接收扩展帧ID范围, ID=0x18FECA7D - 0x18FF767D
	////如果原来配置了智能滤波，可以先清除原来的滤波列表，
	//VCI_SetReference(m_DevType, m_DevIndex, 0, 3, 0);
	//VCI_SetReference(m_DevType, m_DevIndex, 0, 1, &FILTER[0]);//添加智能滤波列表
	//VCI_SetReference(m_DevType, m_DevIndex, 0, 2, 0);

	//初始化通道0
	if (VCI_StartCAN(m_CanParams.m_DevType, 0, m_CanParams.m_CANIndex) != 1)	//can-0
	{
		printf("Start-CAN0 failed!");
		return g_mCANDevOK;
	}
	//初始化通道1
	if (m_CanParams.m_CANIndex == 1)
	{	
		if (VCI_InitCAN(m_CanParams.m_DevType, 0, m_CanParams.m_CANIndex, InitInfo) != 1)	//can-1
		{
			printf("Init-CAN1 failed!");
			return g_mCANDevOK;
		}
		Sleep(100);
		//初始化通道1
		if (VCI_StartCAN(m_CanParams.m_DevType, 0, m_CanParams.m_CANIndex) != 1)	//can-1
		{
			printf("Start-CAN1 failed!");
			return g_mCANDevOK;
		}
	}

	g_mCANDevOK = true;
	return g_mCANDevOK;
}

bool CAN_Bus::DisConnectDevice()
{
	if (VCI_CloseDevice(m_CanParams.m_DevType, 0) != 1)
	{
		return false;
	}
	return true;
}

bool CAN_Bus::Send(DWORD ID,int nCHN, unsigned char* pData, CString* pSendMsg)
{
	int i;
	int datanum = 8;	
	int newflag = 1;
	int flag = 0;
	int nCount = 0;
	VCI_CAN_OBJ sendbuf[1];
	CString strMsg, strMT, strID, strOutStr;

	sendbuf->ExternFlag = m_CanParams.m_SendFrameType;
	sendbuf->DataLen = datanum;
	sendbuf->RemoteFlag = m_CanParams.m_SendFrameFormat;

	if (m_CanParams.m_SendFrameFormat == 1)//if remote frame, data area is invalid
	{
		for (i = 0; i < datanum; i++)
		{
			pData[i] = 0;
		}
	}

	for (i = 0; i < datanum; i++)
	{
		sendbuf->Data[i] = pData[i];
		strMT.Format("0x%02X ", pData[i]);
		strMsg += strMT;
	}
	
	strID.Format("0x%X::", ID);
	strOutStr = strID + strMsg;
	*pSendMsg = strOutStr;
	
	sendbuf->ID = ID;
	sendbuf->SendType = 1;
	sendbuf->TimeFlag = 0;
	sendbuf->TimeStamp = 0;
	
	if ((m_CanParams.m_CANIndex == 1) && (m_CanParams.m_DevType != VCI_USBCAN2))
	{
		printf("the device only support CAN index 0");
		m_CanParams.m_CANIndex = 0;
	}
	//调用动态链接库发送函数
	flag = VCI_Transmit(m_CanParams.m_DevType, 0, nCHN, sendbuf, 1);//CAN message send
	if (flag < 1)
	{
		if (flag == -1)
			printf("failed- device not open\n");
		return false;
	}
	return true;
}

bool CAN_Bus::Send(DWORD ID, int nCHN, int nSendFrameType, unsigned char* pData, CString* pSendMsg)
{
	int i;
	int datanum = 8;
	int newflag = 1;
	int flag = 0;
	int nCount = 0;
	CString strMsg, strMT, strID, strOutStr;
	VCI_CAN_OBJ sendbuf[1];

	//if (m_CanParams.m_SendFrameFormat == 1)//if remote frame, data area is invalid
	//{
	//	for (i = 0; i < datanum; i++)
	//	{
	//		pData[i] = 0;
	//	}
	//}

	for (i = 0; i < datanum; i++)
	{
		sendbuf->Data[i] = pData[i];
		strMT.Format("0x%02X ", pData[i]);
		strMsg += strMT;
	}

	strID.Format("0x%X::", ID);
	strOutStr = strID + strMsg;
	*pSendMsg = strOutStr;

	sendbuf->ID = ID;
	sendbuf->SendType = 0x01;
	sendbuf->TimeFlag = 0x00;
	sendbuf->TimeStamp = 0;
	sendbuf->ExternFlag = (BYTE)nSendFrameType;
	sendbuf->DataLen = (BYTE)datanum;
	sendbuf->RemoteFlag =(BYTE)m_CanParams.m_SendFrameFormat;

	//if ((m_CanParams.m_CANIndex == 1) && (m_CanParams.m_DevType != VCI_USBCAN2))
	//{
	//	printf("the device only support CAN index 0");
	//	m_CanParams.m_CANIndex = 0;
	//}
	//调用动态链接库发送函数
	flag = VCI_Transmit(m_CanParams.m_DevType, 0, nCHN, sendbuf, 1);//CAN message send
	if (flag < 1)
	{
		if (flag == -1)
			printf("failed- device not open\n");
		return false;
	}
	return true;
}

int CAN_Bus::Receive(VCI_CAN_OBJ* pCanObj, int index, CString* pReceiveMsg, DWORD* ID)
{
	int nRecvNum = 0;
	CString str, strID, strRevMsg;
	
	nRecvNum = VCI_Receive(m_CanParams.m_DevType, 0, index, pCanObj, 200, 0);
	if(nRecvNum > 0)
	{
		*ID = pCanObj[index].ID;
		strID.Format("0x%X::", *ID);
		for (int j = 0; j < pCanObj[index].DataLen; j++) {
			str.Format("0x%02X ", pCanObj[index].Data[j]);
			strRevMsg += str;
		}
		*pReceiveMsg = strID + strRevMsg;

		return nRecvNum;
	}
	return -1;
}

bool CAN_Bus::isConnected()
{
	bool bRight = false;
	PVCI_BOARD_INFO2 pInfo = new VCI_BOARD_INFO2();
	DWORD dReturn= VCI_FindUsbDevice2(pInfo);
	if (pInfo->can_Num != 0 && pInfo->fw_Version != 0 && pInfo->hw_Version != 0)
		bRight = true;
	else
		bRight = false;
	if(pInfo!=NULL)
	{
		delete pInfo;
		pInfo = NULL;
	}
	return bRight;
}

#ifdef _P_CAN_
PCAN_Bus::PCAN_Bus()
{
	g_mCANDevOK = false;
}

PCAN_Bus::~PCAN_Bus()
{
	PostNcDestroy();
}

bool PCAN_Bus::isConnected()
{
	return PCANTP_ERROR_NOT_INITIALIZED != CANTP_GetStatus(m_CanParams.PCANParam.m_pctpHandle);
}

DWORD PCAN_Bus::SetValue(WORD hConHandle,  HANDLE eHandle)
{
	TPCANTPStatus sts = PCANTP_ERROR_OK;
	sts = CANTP_SetValue(hConHandle, PCANTP_PARAM_RECEIVE_EVENT, &eHandle, sizeof(eHandle));
	return sts;
}

bool PCAN_Bus::ConnectDevice()
{
	TPCANTPStatus sts;
	if (m_CanParams.PCANParam.m_IsFD)
	{
		unsigned int buflen = 512;
		char bufferTextCanFd[4096] = { '\0' }; // char buffer
		memcpy(bufferTextCanFd, m_CANFD_ConStr, buflen);
		sts = CANTP_InitializeFD(m_CanParams.PCANParam.m_pctpHandle, bufferTextCanFd);
	}
	else
	{
		sts = CANTP_Initialize(m_CanParams.PCANParam.m_pctpHandle,
			m_CanParams.PCANParam.m_baudrate, 
			m_CanParams.PCANParam.m_hwType, 
			m_CanParams.PCANParam.m_ioPort, 
			m_CanParams.PCANParam.m_interrupt);
	}
	if (sts == PCANTP_ERROR_OK)
	{
		g_mCANDevOK = true;
		UINT32 canIdCli2Ecu = m_CanParams.PCANParam.m_ReqAdr;			// CAN ID used to communicate from Client to ECU
		UINT32 canIdEcu2Cli;		// CAN ID used to communicate from ECU to Client

		if (m_CanParams.m_SendFrameFormat == 0) //Standard Frame
		{
			canIdEcu2Cli = m_CanParams.PCANParam.m_RespAdr[0];
			// Defines a p_mapping to allow communication from client to ECU (a.k.a request).
			MappingsAdd(new MappingStatus(canIdCli2Ecu, canIdEcu2Cli,
				PCANTP_ID_CAN_11BIT,
				PCANTP_FORMAT_NORMAL,
				PCANTP_MESSAGE_DIAGNOSTIC,
				PCANTP_ADDRESSING_PHYSICAL,
				clientAddr[0], ecuAddr[0], m_CanParams.m_SendFrameType));

			//// Defines a p_mapping to allow communication from ECU to client (a.k.a response).
			//MappingsAdd(new MappingStatus(canIdEcu2Cli, canIdCli2Ecu,
			//	PCANTP_ID_CAN_11BIT,
			//	PCANTP_FORMAT_NORMAL,
			//	PCANTP_MESSAGE_DIAGNOSTIC,
			//	PCANTP_ADDRESSING_PHYSICAL,
			//	ecuAddr[0], clientAddr[0],  m_CanParams.m_SendFrameType));
		}
		else if (m_CanParams.m_SendFrameFormat == 1) //29bit Extend frame
		{
			// Defines a p_mapping to allow communication from client to ECU (a.k.a request).		
			canIdEcu2Cli = m_CanParams.PCANParam.m_RespAdr[0];
			MappingsAdd(new MappingStatus(canIdCli2Ecu, canIdEcu2Cli,
				PCANTP_ID_CAN_29BIT,
				PCANTP_FORMAT_NORMAL,
				PCANTP_MESSAGE_DIAGNOSTIC,
				PCANTP_ADDRESSING_PHYSICAL,
				clientAddr[0], ecuAddr[0], m_CanParams.m_SendFrameType));

			//// Defines a p_mapping to allow communication from ECU to client (a.k.a response).
			//MappingsAdd(new MappingStatus(canIdEcu2Cli, canIdCli2Ecu,
			//	PCANTP_ID_CAN_29BIT,
			//	PCANTP_FORMAT_NORMAL,
			//	PCANTP_MESSAGE_DIAGNOSTIC,
			//	PCANTP_ADDRESSING_PHYSICAL,
			//	ecuAddr[0], clientAddr[0], m_CanParams.m_SendFrameType));
		}
	}

	return g_mCANDevOK;
}

bool PCAN_Bus::DisConnectDevice()
{
	TPCANTPStatus sts;

	// Disconnects from selected PCAN-ISO-TP channel
	sts = CANTP_Uninitialize(m_CanParams.PCANParam.m_pctpHandle);
	if (sts == PCANTP_ERROR_OK)
		g_mCANDevOK = false;

	return g_mCANDevOK;
}

bool PCAN_Bus::Send(DWORD ID, int nCHN, unsigned char* pData, CString* pSendMsg)
{
	TPCANTPMsg msg;
	TPCANTPStatus sts = PCANTP_N_ERROR;
	CString strByte = L"";
	int index = 0;
	bool bRight = false;

	// Initialization of Data field before using
	memset(msg.DATA, '\0', 4095);

	// Asserts if corresponding mapping is NULL
	if (NULL == m_mappings[index])
		return false;

	// Initializes the TPCANTPMsg to send with the selected mapping.
	if (m_CanParams.m_SendFrameFormat == 0)
	{
		m_mappings[index]->m_canIdType = 0x01;
		m_mappings[index]->m_formatType = 0x01;
		m_mappings[index]->m_msgType = 0x01;
		m_mappings[index]->m_targetType = 0x01;
	}
	else
	{
		m_mappings[index]->m_canIdType = 0x02;
		m_mappings[index]->m_formatType = 0x02;
		m_mappings[index]->m_msgType = 0x01;
		m_mappings[index]->m_targetType = 0x01;
	}

	msg.IDTYPE = m_mappings[index]->m_canIdType;
	msg.FORMAT = m_mappings[index]->m_formatType;
	msg.MSGTYPE = m_mappings[index]->m_msgType;
	msg.TA_TYPE = m_mappings[index]->m_targetType;

	// The mapping defines the addresses.
	msg.SA = m_mappings[index]->m_sourceAddr;
	msg.TA = m_mappings[index]->m_targetAddr;

	m_mappings[index]->m_remoteAddr = m_mappings[index]->m_remoteAddr==0x01? 0x01: 0x00;
	msg.RA = m_mappings[index]->m_remoteAddr;

	// Initializes RESULT now to avoid a strange behaviour within Visual Studio while debugging (C++.NET issue).
	msg.RESULT = PCANTP_N_OK;

	// Sets length and data.
	msg.LEN = m_CanParams.PCANParam.m_LEN;
	for (int i = 0; i < msg.LEN; i++)
	{
		msg.DATA[i] =pData[i]; 
	}
	// CAN FD support
	if (m_CanParams.PCANParam.m_IsFD)
	{
		msg.IDTYPE |= PCANTP_ID_CAN_FD;
		// if BRS support
		if (m_CanParams.PCANParam.m_BRS)
		{
			msg.IDTYPE |= PCANTP_ID_CAN_BRS;
		}
	}
	
	sts = CANTP_Write(m_CanParams.PCANParam.m_pctpHandle, &msg);
	if (sts == PCANTP_ERROR_OK)
	{
		CString strMT, strMsg, strOutStr, strID;
		for (int i = 0; i < msg.LEN; i++)
		{
			strMT.Format("0x%02X ", pData[i]);
			strMsg += strMT;
		}

		strID.Format("%X::", m_mappings[index]->m_canId);
		strOutStr = strID + strMsg;
		*pSendMsg = strOutStr;

		bRight = true;
	}

	return bRight;
}

bool PCAN_Bus::Send(DWORD ID, int nCHN, int nSendFrameType, unsigned char* pData, CString* pSendMsg)
{
	TPCANTPMsg msg;
	TPCANTPStatus sts = PCANTP_N_ERROR;
	CString strByte = L"";
	int index = 0;
	bool bRight = false;

	// Initialization of Data field before using
	memset(msg.DATA, '\0', 4095);

	// Asserts if corresponding mapping is NULL
	if (NULL == m_mappings[index])
		return false;

	// Initializes the TPCANTPMsg to send with the selected mapping.
	if (nSendFrameType == 0)
	{
		m_mappings[index]->m_canIdType = 0x01;
		m_mappings[index]->m_formatType = 0x01;
		m_mappings[index]->m_msgType = 0x01;
		m_mappings[index]->m_targetType = 0x01;
	}
	else
	{
		m_mappings[index]->m_canIdType = 0x02;
		m_mappings[index]->m_formatType = 0x02;
		m_mappings[index]->m_msgType = 0x01;
		m_mappings[index]->m_targetType = 0x01;
	}

	msg.IDTYPE = m_mappings[index]->m_canIdType;
	msg.FORMAT = m_mappings[index]->m_formatType;
	msg.MSGTYPE = m_mappings[index]->m_msgType;
	msg.TA_TYPE = m_mappings[index]->m_targetType;

	// The mapping defines the addresses.
	msg.SA = m_mappings[index]->m_sourceAddr;
	msg.TA = m_mappings[index]->m_targetAddr;

	m_mappings[index]->m_remoteAddr = m_mappings[index]->m_remoteAddr == 0x01 ? 0x01 : 0x00;
	msg.RA = m_mappings[index]->m_remoteAddr;

	// Initializes RESULT now to avoid a strange behaviour within Visual Studio while debugging (C++.NET issue).
	msg.RESULT = PCANTP_N_OK;

	// Sets length and data.
	msg.LEN = m_CanParams.PCANParam.m_LEN;
	for (int i = 0; i < msg.LEN; i++)
	{
		msg.DATA[i] = pData[i];
	}
	// CAN FD support
	if (m_CanParams.PCANParam.m_IsFD)
	{
		msg.IDTYPE |= PCANTP_ID_CAN_FD;
		// if BRS support
		if (m_CanParams.PCANParam.m_BRS)
		{
			msg.IDTYPE |= PCANTP_ID_CAN_BRS;
		}
	}

	sts = CANTP_Write(m_CanParams.PCANParam.m_pctpHandle, &msg);
	if (sts == PCANTP_ERROR_OK)
	{
		CString strMT, strMsg, strOutStr, strID;
		for (int i = 0; i < msg.LEN; i++)
		{
			strMT.Format("0x%02X ", pData[i]);
			strMsg += strMT;
		}

		strID.Format("%X::", m_mappings[index]->m_canId);
		strOutStr = strID + strMsg;
		*pSendMsg = strOutStr;

		bRight = true;
	}

	return bRight;
}

int PCAN_Bus::Receive(VCI_CAN_OBJ* pCanObj, int index, CString* pReceiveMsg, DWORD* ID)
{
	int nRecvNum = 0;
	CString str, strID, strMT, strMsg, strOutStr;

	TPCANTPMsg msg;
	TPCANTPTimestamp ts;
	TPCANTPStatus sts;

	// Reads and process a single ISO-TP message
	sts = CANTP_Read(m_CanParams.PCANParam.m_pctpHandle, &msg, &ts);
	if (sts == PCANTP_ERROR_OK)
	{
		pCanObj->DataLen = msg.LEN;
		*ID = m_mappings[0]->m_canIdResponse;

		for (int i = 0; i < msg.LEN; i++)
		{
			pCanObj->Data[i] = msg.DATA[i];

			strMT.Format("0x%02X ", msg.DATA[i]);
			strMsg += strMT;
		}
		strID.Format("%X::", m_mappings[index]->m_canIdResponse);
		strOutStr = strID + strMsg;
		*pReceiveMsg = strOutStr;
	}
	
	return sts;
}

void PCAN_Bus::MappingsAdd(MappingStatus* p_mapping)
{
	TPCANTPStatus sts;

	// Adds the p_mapping inside the API
	sts = CANTP_AddMapping(m_CanParams.PCANParam.m_pctpHandle, p_mapping->m_canId, p_mapping->m_canIdResponse,
		p_mapping->m_canIdType, p_mapping->m_formatType, p_mapping->m_msgType,
		p_mapping->m_sourceAddr, p_mapping->m_targetAddr, p_mapping->m_targetType, p_mapping->m_remoteAddr);

	if (sts == PCANTP_ERROR_OK)
	{
		// Adds the p_mapping to the internal list of configured mappings.
		m_mappings.push_back(p_mapping);
	}
	else
	{
		// If it is not added to the list of mappings it will never deleted
		// Free memory here
		if (p_mapping != NULL)
		{
			delete p_mapping;
			p_mapping = NULL;
		}
	}
}

void PCAN_Bus::PostNcDestroy()
{
	for (UINT i = 0; i < m_mappings.size(); i++)
	{
		if (m_mappings[i] != NULL)
		{
			// Removes the p_mapping from the API (via the CAN ID that defines it uniquely)
			CANTP_RemoveMapping(m_CanParams.PCANParam.m_pctpHandle, m_mappings[i]->m_canId);

			delete m_mappings[i];
			m_mappings[i] = NULL;

			m_mappings.erase(m_mappings.begin() + i);
		}
	}
}

/// <summary>
/// Default constructor
/// </summary>
/// <param name="p_canId">CAN ID to map to CAN-ISO-TP network addressing information</param>
/// <param name="p_canIdResponse">CAN ID used by the other side to internally respond to the CAN-ISO-TP segmented frames
/// (i.e. the Flow Control frames will use this ID)</param>
/// <param name="p_canIdType">The CAN ID type used by the mapping (11 bits or 29 bits CAN ID)</param>
/// <param name="p_formatType">The ISO-TP network addressing format.</param>
/// <param name="p_msgType">Type of CAN-ISO-TP message (diagnostic or remote disgnostic message)</param>
/// <param name="p_sourceAddr">Source address</param>
/// <param name="p_targetAddr">Target address</param>
/// <param name="p_targetType">Type of addressing (physical: -node to node-, or functional: -node to all-)</param>
/// <param name="p_remoteAddr">Remote address (used only with remote disgnostic message)</param>
MappingStatus::MappingStatus(
	UINT32 p_canId = 0x00,
	UINT32 p_canIdResponse = 0x00,
	TPCANTPIdType p_canIdType = PCANTP_ID_CAN_11BIT,
	TPCANTPFormatType p_formatType = PCANTP_FORMAT_UNKNOWN,
	TPCANTPMessageType p_msgType = PCANTP_MESSAGE_UNKNOWN,
	TPCANTPAddressingType p_targetType = PCANTP_ADDRESSING_UNKNOWN,
	byte p_sourceAddr = 0x00,
	byte p_targetAddr = 0x00,
	byte p_remoteAddr = 0x00) 
{
	m_canId = p_canId;
	m_canIdResponse = p_canIdResponse;
	m_canIdType = p_canIdType;
	m_formatType = p_formatType;
	m_msgType = p_msgType;
	m_sourceAddr = p_sourceAddr;
	m_targetAddr = p_targetAddr;
	m_targetType = p_targetType;
	m_remoteAddr = p_remoteAddr;
}

#endif

bool LIN_Bus::Scan_Dev()
{
	int ret = 0;
	ret = USB_ScanDevice(gDevHandle);
	if (ret <= 0) {
		printf("No lin device connected!\n");
		return ret;
	}
	else
	{
		printf("当前连接的设备数为：%d\n", ret);
		printf("每个设备的句柄为：\n");
		for (int i = 0; i < ret; i++) {
			printf("DevHandle[%d] = %d\n", i, gDevHandle[i]);
		}
	}
	return ret;
}

LIN_Bus::LIN_Bus()
{
	gDevIndex = 0;
	gLINMasterIndex = 0;

	memset(gDevHandle, 0, sizeof(gDevHandle));
}

bool LIN_Bus::ConnectDevice()
{
	bool state = false;
	int ret = 0;

	//扫描已连接的设备
	if (Scan_Dev()>0)
	{
		//打开设备
		state = USB_OpenDevice(gDevHandle[gDevIndex]);
		if (!state) {
			printf("Open lin port0 error!\n");

			gDevIndex += 1;
			state = USB_OpenDevice(gDevHandle[gDevIndex]);
			if (!state) {
				printf("Open lin port1 error!\n");
				return ret;
			}
		}

		//初始化配置LIN
		unsigned char szLinIndex;
		szLinIndex = gLINMasterIndex == 0 ? 0 : 1;
		ret = LIN_EX_Init(gDevHandle[gDevIndex], szLinIndex, 19200, LIN_EX_SLAVE);//初始化为从机模式

		if (ret != LIN_EX_SUCCESS) {
			printf("Config LIN failed!\n");
			return ret;
		}
		else {
			ret = 1;
			printf("Config LIN Success!\n");
		}
	}
	ret == LIN_EX_SUCCESS ? true : false;
	return ret;
}

bool LIN_Bus::DisConnectDevice()
{
	bool state = false;
	state = USB_CloseDevice(gDevHandle[gDevIndex]);
	if (state) {
		printf("关闭设备成功!\n");
		state = true;
	}
	else {
		printf("关闭设备失败！\n");
	}
	return state;
}

bool LIN_Bus::Send(DWORD ID, int nCHN, unsigned char* pData, CString* pSendMsg)
{
	int ret = -1;
	LIN_EX_MSG LINMsg[2] = { 0 };
	unsigned int MsgNum = 8;
	BYTE buf[8] = { 0 };
	//unsigned char szChannelIndex;

	//设置ID为LIN_EX_MSG_TYPE_SW模式，这样主机就可以读取到数据
	LIN_EX_MSG LINSlaveMsg[1];
	LINSlaveMsg[0].PID = ID;
	LINSlaveMsg[0].CheckType = LIN_EX_CHECK_EXT;
	LINSlaveMsg[0].DataLen = 8;

	for (int j = 0; j < LINSlaveMsg[0].DataLen; j++) {
		LINSlaveMsg[0].Data[j] = pData[j];
	}
	LINSlaveMsg[0].MsgType = LIN_EX_MSG_TYPE_SW;//从机发送数据模式
	
	//szChannelIndex = gLINMasterIndex == 0 ? 0 : 1;
	ret = LIN_EX_SlaveSetIDMode(gDevHandle[gDevIndex], nCHN, LINSlaveMsg, 5);
	if (ret != LIN_EX_SUCCESS) {
		printf("Config LIN ID Mode failed!\n");
		return 0;
	}
	else {
		printf("Config LIN ID Mode Success!\n");
	}

	{
		CString strTmp, strID, strSendMsg;
		strTmp = "";
		strSendMsg = "";
		printf("MsgLen = %d\n", ret);

		for (int j = 0; j < LINSlaveMsg[0].DataLen; j++) {
			strTmp.Format("0x%02X ", LINSlaveMsg[0].Data[j]);
			strSendMsg += strTmp;
		}			
		
		strID.Format("0x%X::", ID);
		*pSendMsg = strID + strSendMsg;

		ret = 0;
	}

	return ret;
}

bool LIN_Bus::Send(DWORD ID, int nCHN, int nSendFrameType, unsigned char* pData, CString* pSendMsg)
{
	int ret = -1;
	LIN_EX_MSG LINMsg[2] = { 0 };
	unsigned int MsgNum = 8;
	BYTE buf[8] = { 0 };

	//设置ID为LIN_EX_MSG_TYPE_SW模式，这样主机就可以读取到数据
	LIN_EX_MSG LINSlaveMsg[1];
	LINSlaveMsg[0].PID = ID;
	LINSlaveMsg[0].CheckType = LIN_EX_CHECK_EXT;
	LINSlaveMsg[0].DataLen = 8;

	for (int j = 0; j < LINSlaveMsg[0].DataLen; j++) {
		LINSlaveMsg[0].Data[j] = pData[j];
	}
	LINSlaveMsg[0].MsgType = LIN_EX_MSG_TYPE_SW;//从机发送数据模式

	ret = LIN_EX_SlaveSetIDMode(gDevHandle[gDevIndex], nCHN, LINSlaveMsg, 5);
	if (ret != LIN_EX_SUCCESS) {
		printf("Config LIN ID Mode failed!\n");
		return 0;
	}
	else {
		printf("Config LIN ID Mode Success!\n");
	}

	{
		CString strTmp, strID, strSendMsg;
		strTmp = "";
		strSendMsg = "";
		printf("MsgLen = %d\n", ret);

		for (int j = 0; j < LINSlaveMsg[0].DataLen; j++) {
			strTmp.Format("0x%02X ", LINSlaveMsg[0].Data[j]);
			strSendMsg += strTmp;
		}

		strID.Format("0x%X::", ID);
		*pSendMsg = strID + strSendMsg;

		ret = 0;
	}

	return ret;
}

int LIN_Bus::Receive(DWORD ID, LIN_EX_MSG* pOutMsgs)
{
	int ret = 0;
	CString str, strID, strRevMsg;
	unsigned char szChannelIndex;
	const int nRecvMsgNum = 10;
	LIN_EX_MSG LINOutMsg[nRecvMsgNum];

	szChannelIndex = gLINMasterIndex == 0 ? 0 : 1;
	memset(LINOutMsg, 0, nRecvMsgNum);
	ret = LIN_EX_SlaveGetData(gDevHandle[gDevIndex], szChannelIndex, LINOutMsg);	//从机读取

	if (ret < LIN_EX_SUCCESS) {
		printf("Slave receive LIN failed!\n");
		return ret;
	}
	else if (ret>0 && ret<= nRecvMsgNum)
	{
		for(int k = 0; k< ret; k++)
			pOutMsgs[k] = LINOutMsg[k];
	}
	if (ret > nRecvMsgNum)
	{
		for (int k = 0; k < nRecvMsgNum; k++)
			pOutMsgs[k] = LINOutMsg[k];
		ret = nRecvMsgNum;

	}

	return ret;
}