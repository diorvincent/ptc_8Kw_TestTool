// Serial.cpp
/*
* AUTHOR:Jingchi.He
* DATE: 9/13/2025
* DESIGN PATTERN: Mediator
* PURPOSE:
* Encapsulate a series of object interactions with a mediator object. 
* The mediator enables objects to avoid explicitly referencing each other, 
* thus reducing their coupling and allowing their interactions to be changed independently.
*/
#include "stdafx.h"
#include "Serial.h"
#include "Winbase.h"
#include "methord.h"


CSerial::CSerial(HardwareComMediator* hw_mediator)
{
	m_hIDComDev = NULL;
	m_bOpened = FALSE;

	HW_Mediator = hw_mediator;
}

CSerial::CSerial()
{

}

CSerial::~CSerial()
{
	Close();
}

BOOL CSerial::Open(int nPort, int nBaud, int nData, int nStopBit, int nCheck)//´ò¿ªÍ¨ĞÅ¶Ë¿ÚµÄ³ÉÔ±º¯Êı£¬´øÁ½¸ö²ÎÊı£¬Ò»¸öÊÇ´®ĞĞ¶Ë¿ÚºÅ£¬ÁíÒ»¸öÊÇ²¨ÌØÂÊ
{
	if( m_bOpened ) 
		return( TRUE );

	char szPort[15];
	char szComParams[50];
	DCB dcb;

	sprintf( szPort, "COM%d:", nPort );
	CString comn;
	comn.Format(TEXT("\\\\.\\COM%d"),nPort);

	m_hIDComDev = CreateFile(comn, GENERIC_READ | GENERIC_WRITE, 0,
							 NULL, OPEN_EXISTING,0, NULL );

	if( m_hIDComDev == NULL ) 
	{
		return FALSE;
	}

	COMMTIMEOUTS CommTimeOuts;
	CommTimeOuts.ReadIntervalTimeout =0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 5000;
	SetCommTimeouts( m_hIDComDev, &CommTimeOuts );

	sprintf( szComParams, "COM%d:%d,n,8,1", nPort, nBaud );

	dcb.DCBlength = sizeof( DCB );
	GetCommState( m_hIDComDev, &dcb );
	dcb.BaudRate = nBaud;

	dcb.ByteSize = nData;
	dcb.StopBits = nStopBit;
	dcb.Parity = nCheck;

/*	
	dcb.ByteSize = 8;
	dcb.Parity=NOPARITY;
	dcb.StopBits=ONESTOPBIT;

	dcb.fOutxCtsFlow=FALSE;
	dcb.fRtsControl=RTS_CONTROL_ENABLE;
	dcb.fInX = FALSE;
	dcb.XonChar=XON;
	dcb.XoffChar=XOFF;
*/
	if(!SetCommState( m_hIDComDev, &dcb ))
	{
		return FALSE;
	}
    PurgeComm(m_hIDComDev,PURGE_TXCLEAR);
    PurgeComm(m_hIDComDev,PURGE_RXCLEAR);

	m_bOpened = TRUE;

	return( m_bOpened );

}

BOOL CSerial::Close( void )//¹Ø±ÕÍ¨ĞÅ¶Ë¿ÚµÄ³ÉÔ±º¯Êı¡£ÀàÎö¹¹º¯Êıµ÷ÓÃÕâ¸öº¯Êı£¬Òò´Ë¿É²»ÓÃÏÔÊ½µ÷ÓÃÕâ¸öº¯Êı
{
	if( !m_bOpened || m_hIDComDev == NULL ) return( TRUE );

	CloseHandle( m_hIDComDev );
	m_bOpened = FALSE;
	m_hIDComDev = NULL;

	return( TRUE );
}

BOOL CSerial::WriteCommByte( unsigned char ucByte )
{
	BOOL bWriteStat;
	DWORD dwBytesWritten;
    bWriteStat = WriteFile( m_hIDComDev, (LPSTR) &ucByte, 1, &dwBytesWritten,NULL);

    if(!bWriteStat)
	{
		return(FALSE);
	}
	return( TRUE );
}

//º¯Êı°ÑÊı¾İ´ÓÒ»¸ö»º³åÇøĞ´µ½´®ĞĞ¶Ë¿Ú¡£µÚÒ»¸ö²ÎÊıÊÇ»º³åÇøÖ¸Õë£¬ÆäÖĞ°üº¬Òª±»·¢ËÍµÄ×ÊÁÏ¡£Õâ¸öº¯Êı·µ»ØÒ»Ğ©µ½¶Ë¿ÚµÄÊµ¼Ê×Ö½ÚÊı
int CSerial::SendData(void *buffer, int size )
{
	if( !m_bOpened || m_hIDComDev == NULL ) return( 0 );

	DWORD dwError;
	DWORD dwBytesWritten = 0;

	if (::ClearCommError(m_hIDComDev, &dwError, NULL) && dwError > 0)
		::PurgeComm(m_hIDComDev, PURGE_TXABORT);

	WriteFile(m_hIDComDev, buffer, size, &dwBytesWritten,NULL);

	return( (int) dwBytesWritten );
}

int CSerial::ReadDataWaiting( void )//º¯Êı·µ»ØµÈ´ıÔÚÍ¨ĞÅ¶Ë¿Ú»º³åÇøÖĞµÄÊı¾İ£¬²»´ø²ÎÊı
{
	if( !m_bOpened || m_hIDComDev == NULL ) return( 0 );

	DWORD dwErrorFlags;
	COMSTAT ComStat;

	ClearCommError( m_hIDComDev, &dwErrorFlags, &ComStat );

	return( (int) ComStat.cbInQue );
}

int CSerial::ReadData( void *buffer, int limit )//º¯Êı´Ó¶Ë¿Ú½ÓÊÕ»º³åÇø¶ÁÈëÊı¾İ¡£µÚÒ»¸ö²ÎÊıÊÇ»º³åÇøÖ¸Õë£¬µÚ¶ş¸ö²ÎÊıÊÇ¸öÕûÊıÖµ£¬¸ø³ö»º³åÇøµÄ´óĞ¡
{
	if( !m_bOpened || m_hIDComDev == NULL ) return( 0 );
	
	BOOL bReadStatus;
	DWORD dwBytesRead;
	bReadStatus = ReadFile( m_hIDComDev, buffer, limit, &dwBytesRead,NULL);
	
	return( (int) dwBytesRead );
}

int CSerial::WaitAllDataArrial(CSerial* serial, int waittime)
{
	int tempnumber, NumberInTheBuffer;
	int i = 0;
	BOOL bReading = TRUE;
	//º¯Êı·µ»ØµÈ´ıÔÚÍ¨ĞÅ¶Ë¿Ú»º³åÇøÖĞµÄÊı¾İ
	NumberInTheBuffer = serial->ReadDataWaiting();
	tempnumber = NumberInTheBuffer;
	while (bReading)
	{
		Sleep(20);
		NumberInTheBuffer = serial->ReadDataWaiting();
		if (NumberInTheBuffer <= 2)
		{
			if (i < waittime)
			{
				i++;
				continue;
			}
			else
			{
				return 0;
			}
		}
		if (tempnumber < NumberInTheBuffer)
		{
			tempnumber = NumberInTheBuffer;
			continue;
		}
		bReading = FALSE;
	}
	return NumberInTheBuffer;
}

unsigned short CSerial::CRC_116(unsigned char* puchMsg, unsigned short usDataLen, bool b8710)
//uchar *puchMsg ;  Òª½øĞĞCRCĞ£ÑéµÄÏûÏ¢
//uchar usDataLen ;  ÏûÏ¢ÖĞ×Ö½ÚÊı
{
	unsigned char uchCRCHi = 0xFF; //¸ßCRC×Ö½Ú³õÊ¼»¯
	unsigned char uchCRCLo = 0xFF; //µÍCRC ×Ö½Ú³õÊ¼»¯
	unsigned char uIndex;		   // CRCÑ­»·ÖĞµÄË÷Òı
	while (usDataLen--)			   // ´«ÊäÏûÏ¢»º³åÇø
	{
		uIndex = uchCRCHi ^ *puchMsg++; // ¼ÆËãCRC
		uchCRCHi = uchCRCLo ^ auch1CRCHi[uIndex];
		uchCRCLo = auch1CRCLo[uIndex];
	}
	
	// ×¢ÒâÕâÀïµÄË³Ğò	
	if (!b8710)
	{
		return (uchCRCLo << 8 | uchCRCHi);
	}
	else
	{
		return (uchCRCHi << 8 | uchCRCLo);
	}
}

//$$$$$$$$$$$$$$$$$$$$$$$$Éè±¸Í¨ĞÅÀà$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

//######################Rs232 com####################################################
//µÍÑ¹µçÔ´
LowVolPowerCom::LowVolPowerCom(HardwareComMediator* serial) : CSerial(serial) 
{
}

LowVolPowerCom::~LowVolPowerCom()
{
}

bool LowVolPowerCom::PackCmdWrite(unsigned char* strOutMsg, float value, int* length, PowerQueryType powerquerytype)
{
	char tempstr[30];
	if (powerquerytype == SET_VOL)
		//sprintf(tempstr, "APP:VOLT %2.1f,%2.1f,%2.1f\r\n", value, value, 0);
		sprintf(tempstr, "VOLTage %2.1fV\r\n", value);
	else if (powerquerytype == SET_CURR)
		//sprintf(tempstr, "APP:CURR %2.1f,%2.1f,%2.1f\r\n", value, value, 0);
		sprintf(tempstr, "CURR %1.2fA\r\n", value);
	else if (powerquerytype == SET_CHANNEL)
		sprintf(tempstr, "INSTrument:NSELect %d\r\n", (int)value);
	//{	
	//	if(value == 1)
	//		strcpy((char*)strOutMsg, "INST FIRST\r\n");
	//	else if(value == 2)
	//		strcpy((char*)strOutMsg, "INST SECOND\r\n");
	//	else if (value == 3)
	//		strcpy((char*)strOutMsg, "INST THIRD\r\n");
	//}
	else if (powerquerytype == OUTP)
		sprintf(tempstr, "OUTP %d\r\n",  (int)value);
	else if (powerquerytype == REMOTE_ENABLE)
	{
		if (value == 1.0f)
			sprintf(tempstr, "SYSTem:REM\r\n");
		else
			sprintf(tempstr, "SYSTem:LOC\r\n");
	}

	strcpy((char*)strOutMsg, tempstr);
	*length = strlen(tempstr);

	return true;
}

bool LowVolPowerCom::PackCmdRead(unsigned char* strOutMsg, int* length, PowerQueryType powerquerytype)
{
	char tempstr[30];

	if(powerquerytype == GET_VOL)
		sprintf(tempstr, "VOLTage?\r\n");
	else 	if (powerquerytype == GET_CURR)
		sprintf(tempstr, "CURR?\r\n");
	else if(powerquerytype == MEAS_VOL)
		sprintf(tempstr, "MEAS:VOLT?\r\n");
	else if (powerquerytype == MEAS_CURR)
		sprintf(tempstr, "MEAS:CURR?\r\n"); 
	strcpy((char*)strOutMsg, tempstr);
	*length = strlen(tempstr);
	return true;
}

bool LowVolPowerCom::PackSCPICmd(unsigned char* strInMsg, unsigned char* strOutMsg, int* length)
{
	return true;
}

signed long LowVolPowerCom::MBMasterSnd(unsigned char Addr, unsigned char RegCode, signed long RegStart, signed long RegNum, void* src, signed long srclen, unsigned char* dst)
{
	return 0;
}

bool LowVolPowerCom::EnableRemoteCtrl(bool enable)
{
	unsigned char value = 0;
	BOOL HaveError = false;
	int lengthtemp = 0;
	int CycNumber = 0;
	char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);
	
	do
	{
		CycNumber++;
		memset((unsigned char*)PackageOut, 0, PACKAGELENGTH);

		PackCmdWrite((unsigned char*)PackageOut, (float)enable, &lengthtemp, REMOTE_ENABLE);
		this->SendData(PackageOut, lengthtemp);

		Sleep(100);
	} while (CycNumber < 3);

	if (CycNumber > 3)
		HaveError = true;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	return HaveError;
}

bool LowVolPowerCom::SelectChannelOutput(int nChannel, bool bOutput)
{
	unsigned char value = 0;
	int CycNumber = 0;
	int lengthtemp = 0;
	BOOL HaveError = FALSE;
	char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	memset((unsigned char*)PackageOut, 0, PACKAGELENGTH);
	memset(strRecvData, 0, PACKAGELENGTH);

	PackCmdWrite((unsigned char*)PackageOut, nChannel, &lengthtemp, SET_CHANNEL);
	this->SendData(PackageOut, strlen((char*)PackageOut));

	Sleep(100);
	memset((unsigned char*)PackageOut, 0, PACKAGELENGTH);
	memset(strRecvData, 0, PACKAGELENGTH);

	PackCmdWrite((unsigned char*)PackageOut, (float)bOutput, &lengthtemp, OUTP);
	this->SendData(PackageOut, strlen((char*)PackageOut));

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);
	Sleep(100);

	return true;
}

float LowVolPowerCom::ReadSetVLOT()
{
	float value = 0.0f;
	BOOL HaveError;
	unsigned char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];
	int NumberInTheBuffer = 8;
	int num;
	int lengthtemp;
	CString valuestr;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	HaveError = FALSE;
	int CycNumber = 0;
	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		PackCmdRead(PackageOut, &lengthtemp, GET_VOL);
		this->SendData(PackageOut, lengthtemp);

		NumberInTheBuffer = WaitAllDataArrial(this, 100);
		if (NumberInTheBuffer > 10)
		{
			continue;
		}
		else
		{
			num = this->ReadData(strRecvData, NumberInTheBuffer);
			
			valuestr = strRecvData;
			value = atof(valuestr);
			break;
		}

	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);
	Sleep(100);

	return value;
}

float LowVolPowerCom::ReadSetCURR()
{
	float value = 0.0f;
	BOOL HaveError;
	unsigned char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];
	int NumberInTheBuffer = 8;
	int num;
	int lengthtemp;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	HaveError = FALSE;
	int CycNumber = 0;
	CString valuestr;

	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		PackCmdRead(PackageOut, &lengthtemp, GET_CURR);
		this->SendData(PackageOut, lengthtemp);

		NumberInTheBuffer = WaitAllDataArrial(this, 100);
		if (NumberInTheBuffer > 10)
		{
			continue;
		}
		else
		{
			this->ReadData(strRecvData, NumberInTheBuffer);
			
			valuestr = strRecvData;
			value = atof(valuestr);
			break;
		}

	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);
	Sleep(100);

	return value;
}

bool LowVolPowerCom::Set_Dev_Vol_Curr(float voltage, float current)
{
	BOOL HaveError;
	unsigned char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];

	int lengthtemp;
	int CycNumber;
	float setvalue, checkvalue;

	//set low power voltage
	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	CycNumber = 0;
	HaveError = FALSE;

	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		PackCmdWrite(PackageOut, voltage, &lengthtemp, SET_VOL);
		this->SendData(PackageOut, lengthtemp);

		setvalue = ReadSetVLOT();
		checkvalue = fabsf(setvalue - voltage);

		if (checkvalue > 0.2)
		{
			continue;
		}
		else
		{
			break;
		}

	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}
	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);
	
	Sleep(100);

	//set low power current
	HaveError = FALSE;
	CycNumber = 0;
	int NumberInTheBuffer = 0;
	CString valuestr;

	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		PackCmdWrite(PackageOut, current, &lengthtemp, SET_CURR);
		this->SendData(PackageOut, lengthtemp);

		setvalue = ReadSetCURR();

		if (fabsf(setvalue - current) > 0.02)
		{
			continue;
		}
		else
		{
			HaveError = FALSE;
			break;
		}

	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	return HaveError;
}

bool LowVolPowerCom::Get_Dev_Vol_Curr(float* voltage, float* current)
{
	BOOL HaveError;
	unsigned char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];
	int NumberInTheBuffer = 8;
	int num;
	int lengthtemp;
	int CycNumber;
	CString valuestr;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	//measure voltage on low power 
	HaveError = FALSE;
	CycNumber = 0;

	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		PackCmdRead(PackageOut, &lengthtemp, MEAS_VOL);
		this->SendData(PackageOut, lengthtemp);

		NumberInTheBuffer = WaitAllDataArrial(this, 100);
		if (NumberInTheBuffer > 20)
		{
			continue;
		}
		else
		{
			this->ReadData(strRecvData, NumberInTheBuffer);
			
			valuestr = strRecvData;
			*voltage = atof(valuestr);
			break;
		}

	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	//measure current on low power 
	HaveError = FALSE;
	CycNumber = 0;

	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		PackCmdRead(PackageOut, &lengthtemp, MEAS_CURR);
		this->SendData(PackageOut, lengthtemp);

		NumberInTheBuffer = WaitAllDataArrial(this, 100);
		if (NumberInTheBuffer > 20)
		{
			continue;
		}
		else
		{
			this->ReadData(strRecvData, NumberInTheBuffer);

			valuestr = strRecvData;
			*current = atof(valuestr);
			break;
		}

	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	return HaveError;
}

bool LowVolPowerCom::Get_Dev_Vol(float* voltage)
{
	BOOL HaveError;
	unsigned char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];
	int NumberInTheBuffer = 8;
	int num;
	int lengthtemp;
	int CycNumber;
	CString valuestr;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	//measure voltage on low power 
	HaveError = FALSE;
	CycNumber = 0;

	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		PackCmdRead(PackageOut, &lengthtemp, GET_VOL); 	//   MEAS_VOL);	
		this->SendData(PackageOut, lengthtemp);

		NumberInTheBuffer = WaitAllDataArrial(this, 50);
		if (NumberInTheBuffer > 20)
		{
			continue;
		}
		else
		{
			this->ReadData(strRecvData, NumberInTheBuffer);

			valuestr = strRecvData;
			*voltage = atof(valuestr);
			break;
		}

	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	return HaveError;
}

//»ñÈ¡Á½¸öÍ¨µÀµÄµçÑ¹
bool LowVolPowerCom::Get_Dev_Vol(float* voltage, float* voltage2)
{
	BOOL HaveError;
	unsigned char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];
	int NumberInTheBuffer = 8;
	int num;
	int lengthtemp;
	int CycNumber;
	CString valuestr;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	//measure voltage on low power 
	HaveError = FALSE;
	CycNumber = 0;

	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		PackCmdRead(PackageOut, &lengthtemp, MEAS_VOL);
		this->SendData(PackageOut, lengthtemp);

		NumberInTheBuffer = WaitAllDataArrial(this, 100);
		if (NumberInTheBuffer > 20)
		{
			continue;
		}
		else
		{
			this->ReadData(strRecvData, NumberInTheBuffer);
			valuestr = strRecvData;
			*voltage = atof(valuestr);
			// 
			//*voltage = ((strRecvData[0] - '0') * 10 + (strRecvData[1] - '0') + (strRecvData[3] - '0') / 10.0f + (strRecvData[4] - '0') / 100.0f + (strRecvData[5] - '0') / 1000.0f);
			//*voltage2 = ((strRecvData[7] - '0') * 10 + (strRecvData[8] - '0') + (strRecvData[10] - '0') / 10.0f + (strRecvData[11] - '0') / 100.0f + (strRecvData[12] - '0') / 1000.0f);
			break;
		}

	} while (CycNumber < 5);

	if (CycNumber == 5)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	return HaveError;
}

//»ñÈ¡Á½¸öÍ¨µÀµÄµçÑ¹
bool LowVolPowerCom::Get_Dev_Curr(float* current, float* current2)
{
	BOOL HaveError;
	unsigned char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];
	int NumberInTheBuffer = 8;
	int num;
	int lengthtemp;
	int CycNumber;
	CString valuestr;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	//measure current on low power 
	HaveError = FALSE;
	CycNumber = 0;

	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		PackCmdRead(PackageOut, &lengthtemp, MEAS_CURR);
		this->SendData(PackageOut, lengthtemp);

		NumberInTheBuffer = WaitAllDataArrial(this, 50);
		if (NumberInTheBuffer > 20 || NumberInTheBuffer==0)
		{
			continue;
		}
		else
		{
			this->ReadData(strRecvData, NumberInTheBuffer);
			valuestr = strRecvData;
			*current = atof(valuestr);

			//*current = ((strRecvData[0] - '0') + (strRecvData[2] - '0') / 10.0f + (strRecvData[3] - '0') / 100.0f + (strRecvData[4] - '0') / 1000.0f);
			//*current2 = ((strRecvData[6] - '0') + (strRecvData[8] - '0') / 10.0f + (strRecvData[9] - '0') / 100.0f + (strRecvData[10] - '0') / 1000.0f);
			break;
		}

	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	return HaveError;


}

//×éÖ¯É¨ÂëÇ¹´ò¿ªÃüÁî
void LowVolPowerCom::PackCmd_OpenScannerGun(unsigned char* strOutMsg, int* length)
{
	//04 E4 04 00 FF 14
	strOutMsg[0] = 0x04;
	strOutMsg[1] = 0xE4;
	strOutMsg[2] = 0x04;
	strOutMsg[3] = 0x00;
	strOutMsg[4] = 0xFF;
	strOutMsg[5] = 0x14;
	*length = 6;
}
//×éÖ¯É¨ÂëÇ¹¹Ø±ÕÃüÁî
void LowVolPowerCom::PackCmd_CloseScannerGun(unsigned char* strOutMsg, int* length)
{
	//04 E5 04 00 FF 13
	strOutMsg[0] = 0x04;
	strOutMsg[1] = 0xE5;
	strOutMsg[2] = 0x04;
	strOutMsg[3] = 0x00;
	strOutMsg[4] = 0xFF;
	strOutMsg[5] = 0x13;
	*length = 6;
}

//Ö´ĞĞ´ò¿ª/¹Ø±ÕÉ¨ÂëÇ¹
bool LowVolPowerCom::Open_ScannerGun(bool bOpen)
{
	BOOL bCmdSucceed = FALSE;
	u8 PackageOut[PACKAGELENGTH];
	u8 strRecvData[PACKAGELENGTH];

	int lengthtemp = 11;
	int CycNumber = 0;
	int NumberInTheBuffer = 8;
	u8 uResValue = 0;
	CString strCmd;
	CString strTmp;
	CString strRecv;
	CString strTestTime;
	CTime time;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		if (bOpen)
			PackCmd_OpenScannerGun(PackageOut, &lengthtemp);
		else
			PackCmd_CloseScannerGun(PackageOut, &lengthtemp);

		strCmd = methord::u8tostr(PackageOut, lengthtemp);
		this->SendData(PackageOut, lengthtemp);

		NumberInTheBuffer = WaitAllDataArrial(this, 10);
		if (NumberInTheBuffer == 0)
		{
			continue;
		}
		else
		{
			this->ReadData(strRecvData, lengthtemp);

			//paste received data into UI controls	
			if (strRecvData[0] == 0x05 && strRecvData[1] == 0xD1 &&
				strRecvData[4] == 0x06 && strRecvData[5] == 0xFF)
			{
				bCmdSucceed = TRUE;
				break;
			}
		}

	} while (CycNumber < 2); //5s

	if (CycNumber == 2)
	{
		bCmdSucceed = FALSE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);
	return bCmdSucceed;
}

//»ñÈ¡É¨ÃèÇ¹Êı¾İ(ĞòÁĞÂë)
bool LowVolPowerCom::Get_ScannerGun_Data(CString* pRecvData)
{
	BOOL bCmdSucceed = FALSE;
	u8 PackageOut[PACKAGELENGTH];
	u8 strRecvData[PACKAGELENGTH];

	int lengthtemp = 64;
	int CycNumber = 0;
	int NumberInTheBuffer = 8;
	CString strRecv("");
	CString strRecv2("");
	CString strTmp("");

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		NumberInTheBuffer = WaitAllDataArrial(this, 50); //waitting 2.5s
		if (NumberInTheBuffer == 0)
		{
			continue;
		}
		else
		{
			this->ReadData(strRecvData, lengthtemp);

			//paste received data into UI controls		
			strRecv = methord::u8tostr(strRecvData, lengthtemp);
			if (!strRecv.IsEmpty())
			{
				//½ÓÊÕµ½¹Ø±ÕÉ¨ÃèÇ¹Ö¸ÁîÏìÓ¦£¬²»ÏÔÊ¾ÓÚĞòÁĞºÅÀ¸ÖĞ
				if (strRecvData[0] == 0x05 &&	 // '\x5'	unsigned char
					strRecvData[1] == 0xd1 &&	 // '?'		unsigned char
					strRecvData[2] == 0x00 &&	 // '\0'	unsigned char
					strRecvData[3] == 0x00 &&	 // '\0'	unsigned char
					strRecvData[4] == 0x06 &&	 // '\x6'	unsigned char
					strRecvData[5] == 0xff &&		// 'ÿ'		unsigned char
					strRecvData[6] == 0x24)		// '$'		unsigned char
					break;

				//ÏÔÊ¾´ÓÉ¨ÂëÇ¹½ÓÊÕµ½µÄ×Ö·û´®
				*pRecvData = strRecvData;
				bCmdSucceed = TRUE;
				break;
			}
		}
	} while (CycNumber < 4);

	if (CycNumber == 4)
	{
		bCmdSucceed = FALSE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);
	return bCmdSucceed;
}





//######################Rs485 com####################################################
//¸ßÑ¹µçÔ´
HighVolPowerCom::~HighVolPowerCom()
{
}

bool HighVolPowerCom::PackCmdWrite(unsigned char* strOutMsg, float value, int* length, PowerQueryType powerquerytype)
{
	unsigned short checksum;
	checksum = 0;
	if (powerquerytype == REMOTE_ENABLE)
	{
		strOutMsg[0] = 0x02;
		strOutMsg[1] = 0x10;
		strOutMsg[2] = 0x04;
		strOutMsg[3] = 0xEC;
		strOutMsg[4] = 0x00;
		strOutMsg[5] = 0x01;
		strOutMsg[6] = 0x02;
		strOutMsg[7] = 0xFF;
		strOutMsg[8] = 0xFF;

		checksum = CRC_116(strOutMsg, 9);
		strOutMsg[9] = checksum & 0x000000ff;
		strOutMsg[10] = (checksum >> 8) & 0x000000ff;
		*length = 11;
	}
	else
	{
		strOutMsg[0] = 0x02;
		strOutMsg[1] = 0x10;
		strOutMsg[2] = 0x04;
		strOutMsg[3] = 0xEC;
		strOutMsg[4] = 0x00;
		strOutMsg[5] = 0x01;
		strOutMsg[6] = 0x02;
		strOutMsg[7] = 0x00;
		strOutMsg[8] = 0x00;

		checksum = CRC_116(strOutMsg, 9);
		strOutMsg[9] = checksum & 0x000000ff;
		strOutMsg[10] = (checksum >> 8) & 0x000000ff;
		*length = 11;
	}

	return true;
}

bool HighVolPowerCom::PackCmdRead(unsigned char* strOutMsg, int* length, PowerQueryType powerquerytype)
{
	unsigned short checksum;
	checksum = 0;
	strOutMsg[0] = 0x02;
	strOutMsg[1] = 0x03;
	strOutMsg[2] = 0x04;
	strOutMsg[3] = 0xE7;
	strOutMsg[4] = 0x00;
	strOutMsg[5] = 0x02;
	checksum = CRC_116(strOutMsg, 6);
	strOutMsg[6] = checksum & 0x000000ff;
	strOutMsg[7] = (checksum >> 8) & 0x000000ff;
	*length = 8;
	return true;
}

void HighVolPowerCom::PackCmdWrite2(unsigned int voltage, unsigned int current, unsigned char* strOutMsg, int* length)
{
	unsigned short checksum;
	checksum = 0;
	strOutMsg[0] = 0x02;
	strOutMsg[1] = 0x10;
	strOutMsg[2] = 0x04;
	strOutMsg[3] = 0xEA;
	strOutMsg[4] = 0x00;
	strOutMsg[5] = 0x02;
	strOutMsg[6] = 0x04;
	strOutMsg[7] = voltage / 256;
	strOutMsg[8] = voltage % 256;
	strOutMsg[9] = current / 256;
	strOutMsg[10] = current % 256;
	checksum = CRC_116(strOutMsg, 11);
	strOutMsg[11] = checksum & 0x000000ff;
	strOutMsg[12] = (checksum >> 8) & 0x000000ff;
	*length = 13;
	return;
}

bool HighVolPowerCom::PackSCPICmd(unsigned char* strInMsg, unsigned char* strOutMsg, int* length)
{
	unsigned short checksum;
	checksum = 0;
	//41 54 2B 43 4D 53 59 53 43 54 52 4C 3D 33
	memcpy(strOutMsg, strInMsg, 14);
	*length = 14;
	//strOutMsg[0] = strInMsg[0];
	//strOutMsg[1] = strInMsg[1];
	//strOutMsg[2] = strInMsg[2];
	//strOutMsg[3] = strInMsg[3];
	//strOutMsg[4] = strInMsg[4];
	//strOutMsg[5] = strInMsg[5];
	//strOutMsg[6] = strInMsg[6];
	////checksum = CRC_116(strOutMsg, 7);
	//strOutMsg[7] = strInMsg[7];//checksum & 0x000000ff;
	//strOutMsg[8] = strInMsg[8];//(checksum >> 8) & 0x000000ff;
	//*length = 9;
	return true;
}

signed long HighVolPowerCom::MBMasterSnd(unsigned char Addr, unsigned char RegCode, signed long RegStart, signed long RegNum, void* src, signed long srclen, unsigned char* dst)
{
	u8* srcu8 = (u8*)src;
	u16* srcu16 = (u16*)src;
	u8 tempu8;
	u16 tempu16;
	int len;

	u8 tmp[300];
	s16 tmpindex = 0;
	//µØÖ·
	tmp[tmpindex++] = Addr;
	//Ö¸ÁîÂë
	tmp[tmpindex++] = RegCode;
	switch (RegCode)
	{
	case 1: //1£º¶ÁÏßÈ¦¼Ä´æÆ÷
			//¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegStart);
		//¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegNum);
		break;
	case 2: //2£º¶Á¹âñî×´Ì¬
			//¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegStart);
		//¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegNum);
		break;
	case 3://Ğ´¶à¸ö±£³Ö¼Ä´æÆ÷
		   //¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegStart);
		//¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegNum);
		break;
	case 4: //4£º¶ÁÖ»¶Á¼Ä´æÆ÷×´Ì¬
			//¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegStart);
		//¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegNum);
		break;
	case 5: //Ğ´µ¥¸öÏßÈ¦
			//¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegStart);
		//¶ÁÈ¡ÏßÈ¦×´Ì¬
		tempu8 = srcu8[0];
		if (tempu8 & 0x01) tempu16 = 0xff00;
		else tempu16 = 0x0000;
		tmpindex += methord::u16tou8(tmp + tmpindex, tempu16);
		break;
	case 15: //Ğ´¶à¸öÏßÈ¦¼Ä´æÆ÷
			 //¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegStart);
		//¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegNum);

		len = (RegNum % 8) ? (RegNum / 8 + 1) : (RegNum / 8);
		tmp[tmpindex++] = len;
		if (len <= srclen)
		{
			for (int i = 0; i < len; i++)
				tmp[tmpindex++] = srcu8[i];
		}
		break;
	case 16://Ğ´¶à¸ö±£³Ö¼Ä´æÆ÷
			//¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegStart);
		//¼Ä´æÆ÷ÆğÊ¼µØÖ·
		tmpindex += methord::u16tou8(tmp + tmpindex, RegNum);

		tmp[tmpindex++] = RegNum * 2;
		for (int i = 0; i < RegNum; i++)
			tmpindex += methord::u16tou8(tmp + tmpindex, srcu16[i]);
		break;

	default:
		break;
	}
	if (tmpindex < 3)
	{
		return (-2);
	}

	/* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
	u16  usCRC16;
	s16 sdev = 0;
	usCRC16 = CRC_116(tmp, srclen);
	for (int i = 0; i < srclen; i++)
		dst[sdev++] = tmp[i];
	dst[sdev++] = usCRC16 & 0xFF;
	dst[sdev++] = (usCRC16 >> 8) & 0xFF;
	dst[sdev] = 0X00;

	return sdev;
}

bool HighVolPowerCom::Set_Dev_Vol_Curr(float voltage, float current)
{
	BOOL HaveError;
	unsigned char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];

	int lengthtemp;
	int NumberInTheBuffer = 11;
	int num;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	int CycNumber = 0;
	HaveError = FALSE;
	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		PackCmdWrite2(voltage * 10, current * 10, PackageOut, &lengthtemp);
		this->SendData(PackageOut, lengthtemp);

		NumberInTheBuffer = WaitAllDataArrial(this, 50);
		if (NumberInTheBuffer < 5 || NumberInTheBuffer > 1000)
		{
			continue;
		}
		else
		{
			NumberInTheBuffer = WaitAllDataArrial(this, 30);
			num = this->ReadData(strRecvData, NumberInTheBuffer);
			if (strRecvData[0] == 0x02)											//ÅĞ¶ÏÉè±¸µØÖ·ÂëÊÇ·ñÕıÈ·
			{
				if ((CRC_116(strRecvData, num)) == 0x0000)				//ÅĞ¶ÏCRCĞ£ÑéÊÇ·ñÕıÈ·
				{
					HaveError = FALSE;
					break;
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}
	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);
	Sleep(100);

	return HaveError;
}

bool HighVolPowerCom::Get_Dev_Vol_Curr(float* voltage, float* current)
{
	BOOL HaveError;
	unsigned char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];

	int lengthtemp;
	int NumberInTheBuffer = 11;
	int num;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	int CycNumber = 0;
	HaveError = FALSE;
	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		PackCmdRead(PackageOut, &lengthtemp, MEAS_VOL);
		this->SendData(PackageOut, lengthtemp);

		NumberInTheBuffer = WaitAllDataArrial(this, 50);
		if (NumberInTheBuffer < 5 || NumberInTheBuffer > 1000)
		{
			continue;
		}
		else
		{
			num = this->ReadData(strRecvData, NumberInTheBuffer);
			if (strRecvData[0] == 0x02)								//ÅĞ¶ÏÉè±¸µØÖ·ÂëÊÇ·ñÕıÈ·
			{
				if ((CRC_116(strRecvData, num)) == 0x0000)    //ÅĞ¶ÏCRCĞ£ÑéÊÇ·ñÕıÈ·
				{
					unsigned long lVol = strRecvData[3] << 8 | strRecvData[4];
					unsigned long lCurr = strRecvData[5] << 8 | strRecvData[6];

					if ((lVol & 0xFF00) > 0)
						*voltage = lVol * 0.1f;
					else
						*voltage = lVol;

					if ((lCurr & 0xFF00) > 0)
						*current = lCurr * 0.1f;
					else
						*current = lCurr;

					HaveError = FALSE;
					break;
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}
	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);
	Sleep(100);

	return HaveError;
}

bool HighVolPowerCom::Pack8710CmdRead(unsigned char address, unsigned char* strOutMsg, int* length, PowerQueryType powerquerytype)
{
	unsigned short checksum;
	checksum = 0;
	strOutMsg[0] = address;
	strOutMsg[1] = 0x03;
	strOutMsg[2] = 0x01;
	strOutMsg[3] = 0x00;
	strOutMsg[4] = 0x00;
	strOutMsg[5] = 0x06;
	checksum = CRC_116(strOutMsg, 6, true);

	strOutMsg[6] = (checksum >> 8) & 0x000000ff;
	strOutMsg[7] = checksum & 0x000000ff;
	*length = 8;
	return true;
}

bool HighVolPowerCom::Get_8710Dev_Vol_Curr(float* voltage, float* current, int nCHN)
{
	BOOL HaveError;
	unsigned char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];

	int lengthtemp;
	int NumberInTheBuffer = 11;
	int num;
	CString strSendCMD, strRecvCmd;
	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	int CycNumber = 0;
	HaveError = FALSE;
	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		if (nCHN == 1)
			Pack8710CmdRead(03, PackageOut, &lengthtemp, MEAS_VOL);
		else
			Pack8710CmdRead(04, PackageOut, &lengthtemp, MEAS_VOL);

		this->SendData(PackageOut, lengthtemp);

		NumberInTheBuffer = WaitAllDataArrial(this, 5);
		if (NumberInTheBuffer < 5 || NumberInTheBuffer > 1000)
		{
			continue;
		}
		else
		{
			num = this->ReadData(strRecvData, NumberInTheBuffer);
			if (strRecvData[0] == 3)													//ÅĞ¶ÏÉè±¸µØÖ·ÂëÊÇ·ñÕıÈ·
			{
				if ((CRC_116(strRecvData, num, true)) == 0x0000)		//ÅĞ¶ÏCRCĞ£ÑéÊÇ·ñÕıÈ·
				{
					float fVolValue = 0.0f;
					float fCurrValue = 0.0f;
					char error[30] = { 0 };

					unsigned long recvVol = (strRecvData[3] << 24) + (strRecvData[4] << 16) + (strRecvData[5] << 8) + (strRecvData[6]);
					methord::Calcute8710Val(&recvVol, &fVolValue, error);
					*voltage = fVolValue;

					unsigned long recvCurr = (strRecvData[7] << 24) + (strRecvData[8] << 16) + (strRecvData[9] << 8) + (strRecvData[10]);
					methord::Calcute8710Val(&recvCurr, &fCurrValue, error);
					*current = fCurrValue;

					HaveError = FALSE;
					break;
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}
	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);
	return HaveError;
}

bool HighVolPowerCom::SetSCPICommand(unsigned char* pData)
{
	BOOL HaveError;
	unsigned char PackageOut[PACKAGE485];

	int lengthtemp;
	int NumberInTheBuffer = 11;
	int num;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	int CycNumber = 0;
	HaveError = FALSE;
	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGE485);

		PackSCPICmd(pData, PackageOut, &lengthtemp);
		this->SendData(PackageOut, lengthtemp);

		Sleep(100);
	} while (CycNumber < 3);

	HaveError = TRUE;
	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	return HaveError;
}

bool HighVolPowerCom::GetSCPICommand(unsigned char* pData)
{
	BOOL bRight = FALSE;
	unsigned char strRecvData[SCPICOMMAND];

	int lengthtemp;
	int NumberInTheBuffer = 11;
	int num;
	int CycNumber = 0;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	do
	{
		CycNumber++;
		memset(strRecvData, 0, SCPICOMMAND);

		NumberInTheBuffer = WaitAllDataArrial(this, 50);
		if (NumberInTheBuffer < 5 || NumberInTheBuffer > 1000)
		{
			continue;
		}
		else
		{
			num = this->ReadData(strRecvData, NumberInTheBuffer);
			//if (strRecvData[0] == 0x01)									//ÅĞ¶ÏÉè±¸µØÖ·ÂëÊÇ·ñÕıÈ·
			{
				//if ((CRC_116(strRecvData, num)) == 0x0000)    //ÅĞ¶ÏCRCĞ£ÑéÊÇ·ñÕıÈ·
				{
					memcpy(pData, strRecvData, 8/*NumberInTheBuffer*/);

					bRight = TRUE;
					break;
				}
				//else
				//{
				//	continue;
				//}
			}
			//else
			//{
			//	continue;
			//}
		}
	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		bRight = FALSE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	return bRight;
}

bool HighVolPowerCom::SetDWWState(bool state)
{
	BOOL HaveError;
	unsigned char PackageOut[PACKAGELENGTH];
	unsigned char strRecvData[PACKAGELENGTH];

	int lengthtemp=11;
	int NumberInTheBuffer = 11;
	int num;

	if (this == NULL)
		return TRUE;
	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	int CycNumber = 0;
	HaveError = FALSE;
	do
	{
		CycNumber++;
		memset(PackageOut, 0, PACKAGELENGTH);
		memset(strRecvData, 0, PACKAGELENGTH);

		if (state)
		{
			PackCmdWrite(PackageOut, 1, &lengthtemp, REMOTE_ENABLE);
		}
		else
		{
			PackCmdWrite(PackageOut, 0, &lengthtemp, SET_VOL);
		}
		this->SendData(PackageOut, lengthtemp);

		NumberInTheBuffer = WaitAllDataArrial(this, 50);
		if (NumberInTheBuffer < 5 || NumberInTheBuffer > 1000)
		{
			continue;
		}
		else
		{
			NumberInTheBuffer = WaitAllDataArrial(this, 30);
			num = this->ReadData(strRecvData, NumberInTheBuffer);
			if (strRecvData[0] == 0x02)								//ÅĞ¶ÏÉè±¸µØÖ·ÂëÊÇ·ñÕıÈ·
			{
				if ((CRC_116(strRecvData, num)) == 0x0000)    //ÅĞ¶ÏCRCĞ£ÑéÊÇ·ñÕıÈ·
				{
					HaveError = FALSE;
					break;
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}
	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);
	Sleep(100);

	return HaveError;
}

bool HighVolPowerCom::WriteDO(signed long ioindex, bool onoff)
{
	u8  src[10];
	u8  dst[MBRTULENGTH];
	s16 dstlen = 0;
	u8 strRecvData[MBRTULENGTH];
	int NumberInTheBuffer = 11;
	int num;
	bool HaveError = TRUE;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	int CycNumber = 0;
	do
	{
		CycNumber++;
		memset(dst, 0, MBRTULENGTH);
		memset(strRecvData, 0, MBRTULENGTH);

		if (onoff)
		{
			src[0] = 0x01;
		}
		else
		{
			src[0] = 0x00;
		}

		dstlen = MBMasterSnd(0x01, 0x05, 0 + ioindex, 1, src, 1, dst);
		this->SendData(dst, dstlen);

		NumberInTheBuffer = this->WaitAllDataArrial(this, 100);
		if (NumberInTheBuffer < 5 || NumberInTheBuffer > 1000)
		{
			continue;
		}
		else
		{
			num = this->ReadData(strRecvData, NumberInTheBuffer);
			if (strRecvData[0] == 0x01)       //ÅĞ¶ÏÉè±¸µØÖ·ÂëÊÇ·ñÕıÈ·
			{
				if ((this->CRC_116(strRecvData, num)) == 0x0000)    //ÅĞ¶ÏCRCĞ£ÑéÊÇ·ñÕıÈ·
				{
					HaveError = FALSE;
					break;
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}
	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	return HaveError;
}

//»ñÈ¡»·¾³ÎÂ¶È´«¸ĞÆ÷Öµ
bool HighVolPowerCom::ReadNTCvalue(int nCHN, float* NTCvalue)
{
	u8  src[10];
	u8  dst[512];
	u8 strRecvData[100];
	s16 dstlen = 0;
	int num;
	int NumberInTheBuffer = 11;
	bool HaveError = TRUE;
	int CycNumber = 0;

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);

	do
	{
		CycNumber++;
		memset(dst, 0, sizeof(dst));
		memset(strRecvData, 0, sizeof(strRecvData));

		dstlen = MBMasterSnd(0xFE, 0x04, 0, nCHN, src, 1, dst);
		this->SendData(dst, dstlen);

		NumberInTheBuffer = WaitAllDataArrial(this, 50);
		if (NumberInTheBuffer < 5 || NumberInTheBuffer > 1000)
		{
			continue;
		}
		else
		{
			num = this->ReadData(strRecvData, NumberInTheBuffer);
			if (strRecvData[0] == 0xFE)       //ÅĞ¶ÏÉè±¸µØÖ·ÂëÊÇ·ñÕıÈ·
			{
				if ((this->CRC_116(strRecvData, num)) == 0x0000)    //ÅĞ¶ÏCRCĞ£ÑéÊÇ·ñÕıÈ·
				{
					u16 hiResByte = 0;
					//u16 NTCvaluetemp0, NTCvaluetemp;
					hiResByte = (strRecvData[3] << 8 || strRecvData[4]);
					//methord::u8tou16(&hiResByte, &NTCvaluetemp0);
					//methord::u8tou16(&strRecvData[4], &NTCvaluetemp);
					//*NTCvalue = (float)(NTCvaluetemp0 * 0.01 + NTCvaluetemp * 0.01);
					*NTCvalue = hiResByte * 0.01f;
					HaveError = FALSE;
					break;
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}
	} while (CycNumber < 3);

	if (CycNumber == 3)
	{
		HaveError = TRUE;
	}

	PurgeComm(this->m_hIDComDev, PURGE_RXCLEAR);
	PurgeComm(this->m_hIDComDev, PURGE_TXCLEAR);
	return HaveError;
}

//ÖĞ½éÀàÊµÏÖ
bool ConcreteHardwareCom::Set_Dev_Vol_Curr(CSerial* power, float voltage, float current)
{
	bool bRight = false;
	if (power == lowPower)
		bRight = lowPower->Set_Dev_Vol_Curr(voltage, current);
	else if (power == highPower)
		bRight = highPower->Set_Dev_Vol_Curr(voltage, current);

	return !bRight;
}

bool ConcreteHardwareCom::Get_Dev_Vol_Curr(CSerial* power, float* voltage, float* current)
{
	bool bRight = false;
	if (power == lowPower)
		bRight = lowPower->Get_Dev_Vol_Curr(voltage, current);
	else if (power == highPower)
		bRight = highPower->Get_Dev_Vol_Curr(voltage, current);

	return !bRight;
}

bool ConcreteHardwareCom::SetSCPICommand(CSerial* power, unsigned char* pData)
{
	bool bRight = false;
	if (power == highPower)
		bRight = highPower->SetSCPICommand(pData);
	return bRight;
}

bool ConcreteHardwareCom::GetSCPICommand(CSerial* power, unsigned char* pData)
{
	bool bRight = false;
	if (power == highPower)
		bRight = highPower->GetSCPICommand(pData);
	return bRight;
}

bool ConcreteHardwareCom::PackCmdWrite(unsigned char* strOutMsg, float value, int* length, PowerQueryType powerquerytype)
{
	return true;
}
bool ConcreteHardwareCom::PackCmdRead(unsigned char* strOutMsg, int* length, PowerQueryType powerquerytype)
{
	return true;
}
bool  ConcreteHardwareCom::PackSCPICmd(unsigned char* strInMsg, unsigned char* strOutMsg, int* length)
{
	return true;
}
float ConcreteHardwareCom::ReadSetVLOT()
{
	return true;
}
signed long ConcreteHardwareCom::MBMasterSnd(unsigned char Addr, unsigned char RegCode, signed long RegStart, signed long RegNum, void* src, signed long srclen, unsigned char* dst)
{
	return 0;
}
bool ConcreteHardwareCom::WriteDO(CSerial* power, signed long ioindex, bool onoff)
{
	bool bRight = false;
	if (power == highPower)
		bRight = highPower->WriteDO(ioindex, onoff);

	return bRight;
};
