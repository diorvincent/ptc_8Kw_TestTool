// methord.cpp: implementation of the methord class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "methord.h"
#include "numtype.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cctype>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

uc8 ASIItab[]="0123456789ABCDEF";
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


#include <iostream>
#include <cctype>

using namespace std;

void methord::UDS_Seed2Key(u8* pInSeed, u8* pOutKey)
{
    u32 MASK = 0x7D3EFD82;
    u32 dwordkey = 0;
    u32 seed = 0;
    byte Key[4];

    seed = (u32)((pInSeed[0] << 24) + (pInSeed[1] << 16) + (pInSeed[2] << 8) + pInSeed[3]);
    if (seed != 0)
    {
        dwordkey = (seed >> 7) | (seed << 16);
        dwordkey *= 2;
        dwordkey ^= MASK;
        dwordkey = (dwordkey << 5) | (dwordkey >> 12);
    }

    memset(Key, 0, sizeof(Key));
    Key[0] = (byte)((dwordkey & 0xFF000000) >> 24);
    Key[1] = (byte)((dwordkey & 0xFF0000) >> 16);
    Key[2] = (byte)((dwordkey & 0xFF00) >> 8);
    Key[3] = (byte)(dwordkey & 0xFF);

    memcpy(pOutKey, Key, sizeof(Key));
}

void methord::CalcFractionalPart(u8* pSrc, double* fValue, int nSrcLen)
{
    string hexDigit, binaryDigit;//设两个字符数组储存16进制输入和2进制输出。

    for (int i = 0; i < nSrcLen; ++i) { //循环到数组结尾
        char e = *pSrc++;               //设个e方便阅读，不写直接用hexDigit[i]也行
        
        if (e >= 'a' && e <= 'f') {
            e = e - ('a' - 'A');
        }
        if (e >= 'A' && e <= 'F') {
            int a = static_cast<int>(e - 'A');//不转换成int 的话，case 0变成'0'应该也行
            switch (a) {
            case 0:
                binaryDigit += "1010";
                break;
            case 1:
                binaryDigit += "1011";
                break;
            case 2:
                binaryDigit += "1100";
                break;
            case 3:
                binaryDigit += "1101";
                break;
            case 4:
                binaryDigit += "1110";
                break;
            case 5:
                binaryDigit += "1111";
                break;
            }
        }
        else if (isdigit(e)) {
            int b = static_cast<int>(e - '0');
            switch (b) {
            case 1:
                binaryDigit += "0001";
                break;
            case 2:
                binaryDigit += "0010";
                break;
            case 3:
                binaryDigit += "0011";
                break;
            case 4:
                binaryDigit += "0100";
                break;
            case 5:
                binaryDigit += "0101";
                break;
            case 6:
                binaryDigit += "0110";
                break;
            case 7:
                binaryDigit += "0111";
                break;
            case 8:
                binaryDigit += "1000";
                break;
            case 9:
                binaryDigit += "1001";
                break;
            }
        }
    }

    for (unsigned int j = 0; j < binaryDigit.length(); ++j) {
        if (binaryDigit[j] != '0') {
            *fValue += (1 / pow(2, j));
        }
    }
}

methord::methord()
{

}

methord::~methord()
{

}

void methord::Calcute8710Val(u32* src, float* fValue, char* err)
{
	u8 S, fs; 
	u16 E;
	u32 F0, F1;
    double F = 0.00f;

    try {
        F0 = *src;

        S = (F0 >> 31) & 0xf;                   //符号位 bit32 
        E = ((F0 & 0x7FFFFFFF) >> 23) & 0xFF;   //幂部分 bit24 ~ bit31
        F1 = F0 & 0x007FFFFF;                   //纯小数部分 bit1 ~ bit23

        u32tostr(F1, &fs);
        CalcFractionalPart(&fs, &F, 8);

        *fValue = (float)(pow((-1), S) * (1 + F) * pow(2, (E - 127)));
    }
    catch(exception ex){
        err = const_cast<char*>(ex.what());
    }

}

u8 methord::u32tostr(u32 data,u8 *str)
{
	str[0]=ASIItab[(data>>28)&0x0f];
    str[1]=ASIItab[(data>>24)&0x0f];
    str[2]=ASIItab[(data>>20)&0x0f];
    str[3]=ASIItab[(data>>16)&0x0f];
    str[4]=ASIItab[(data>>12)&0x0f];
    str[5]=ASIItab[(data>>8)&0x0f];
    str[6]=ASIItab[(data>>4)&0x0f];
    str[7]=ASIItab[(data>>0)&0x0f];    
    return 8;
}

u8 methord::u16toPu8(u16 data, u8* pU8)
{
    pU8[0] = (data >> 8) & 0x00ff;
    pU8[1] = data & 0x00ff;
    return 2;
}

std::string methord::Int2Hex(int val)
{
    std::stringstream ss;
    // 整数转换为大写的十六进制字符串，且每个字节占用两个字符的宽度
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << val;
    return ss.str();
}

u8 methord::u16tostr(u16 data, u8 *str)
{
	str[0] = ASIItab[(data>>12)&0x0f];
    str[1] = ASIItab[(data>>8)&0x0f];
    str[2] = ASIItab[(data>>4)&0x0f];
    str[3] = ASIItab[(data>>0)&0x0f];
    return 4;
}

u8 methord::u8tostr(const u8 data, u8 *str)
{
    str[0] = ASIItab[(data>>4)&0x0f];
    str[1] = ASIItab[(data>>0)&0x0f];
    return 2;
}

CString methord::u8tostr(const u8 *data, u16 len)
{
	CString str="";
	CString temp="";
	for (int i = 0; i < len; i++)
	{
		temp.Format(_T("%02X "), data[i]);
		str += temp;
	}
	return str;
}

u8 methord::strtou16(u8 *str, u16 *data)
{
	u8 tm[5];
    if(gsmString2Bytes(str,tm,4)==0)return 0;
    u8tou16(tm,data);
    return 4; 
}

u8 methord::strtou32(u8 *str, u32 *data)
{
	u8 tm[20];
    if(gsmString2Bytes(str,tm,8))return 0;
    u8tou32(tm,data);
    return 8;
}

u8 methord::u8tou16(const u8 *tm, u16 *data)
{
	*data = tm[0];
    *data<<=8;
    *data |= tm[1];
    return 2;   
}

u8 methord::u16tou8(u8 *tm, u16 data)
{
	tm[0] = (data>>8)&0xff;
	tm[1] = data&0xff;
    return 2;  
}

u8 methord::u8tou32(const u8 *tm, u32 *data)
{
	*data = tm[0];*data<<=8;
    *data |= tm[1];*data<<=8;
    *data |= tm[2];*data<<=8;
    *data |= tm[3];
    return 4;   
}

u8 methord::u32tou8(u8 *tm, u32 data)
{
	tm[0] = (u8)((data>>24)&0xff);
	tm[1] = (u8)((data>>16)&0xff);
	tm[2] = (u8)((data>>8)&0xff);
	tm[3] = (u8)(data&0xff);
    return 4;   
}


int methord::autocpy(u8 *dst,const u8 *src,int srclen,int dstlen,u8 ascii)
{
	if(srclen==0) srclen = strlen_1(src);
	if(dstlen==0) dstlen = srclen;
	
	s16 i;
	for( i=0;i<dstlen&&i<srclen;i++)
		dst[i]=src[i];
	for(;i<dstlen;i++)
		dst[i] = ascii;
	
	dst[i] = 0;
	return dstlen;
}

s16 methord::strlen_1(uc8 *src,s16 maxstrlen)
{
	s16 n;
    for ( n = 0; (*src != '\0') && (n < maxstrlen); src++ ) ++n;
    return ( n );
}

s16 methord::FindASCII(u8 *src, u8 ascii, s16 srclen)
{
	s16 i;
    if( srclen <= 0 )
    {
        srclen = strlen_1(src);
    }
	if( ascii == 0 )
    {
        return ( srclen );
    }
    for ( i = 0; i < srclen; i++ )
    {
        if( src[i] == ascii ) return ( i );
    }    
    return ( -1 );
}

s16 methord::gsmString2Bytes(u8 *pSrc, u8 *pDst, s16 nSrcLength)
{
	s16 i;
    for (i = 0; i < nSrcLength; i+=2)
    {
        if ((*pSrc>= '0') && (*pSrc<= '9'))
		{
            *pDst = *pSrc- '0'; 
		}
        else if ((*pSrc>= 'A') && (*pSrc<= 'F'))  /* A....F  */
		{
            *pDst = *pSrc- 'A'+0x0A; 
		}
        else if ((*pSrc>= 'a') && (*pSrc<= 'f'))  /* a....f  */
		{
            *pDst = *pSrc- 'a'+0x0a; 
		}
        else
		{
            return 0;
		} 
		
        *pDst<<=4;
		
        pSrc++;
		
        if ((*pSrc>= '0') && (*pSrc<= '9'))
		{
            *pDst |= *pSrc- '0'; 
		}
        else if ((*pSrc>= 'A') && (*pSrc<= 'F'))  /* A....F  */
		{
            *pDst |= *pSrc- 'A'+0x0A; 
		}
        else if ((*pSrc>= 'a') && (*pSrc<= 'f'))  /* a....f  */
		{
            *pDst |= *pSrc- 'a'+0x0a; 
		}
        else
		{
            return 0;
		} 
		
        pSrc++;
        pDst++;
    } 
    *pDst=0;
    return(nSrcLength / 2);
}

s16 methord::gsmBytes2String(u8 *pSrc, u8 *pDst, s16 nSrcLength)
{
	s16 i;
    for (i = 0; i < nSrcLength; i++)
    {
        *pDst++ = ASIItab[*pSrc >> 4];      // 输出高4位
        *pDst++ = ASIItab[*pSrc & 0x0f];    // 输出低4位
        pSrc++;
    }
    // 输出字符串加个结束符
    *pDst = '\0';
    // 返回目标字符串长度
    return(nSrcLength * 2);
}

void methord::StrSort(CString *StrList, int Comnum)
{
	int i=0,j=0;
    CString StrTemp;
    
	for (i=0; i<Comnum; i++)		  
	{         
		for (j=Comnum-2; j>=i;j--)    
		{                    
			if (StrList[j]>StrList[j+1])       
			{    
				StrTemp=StrList[j];        
				StrList[j]=StrList[j+1];        
				StrList[j+1]=StrTemp;
			}
		} 
	}  
}

int methord::FindCommPort(CString *serial)
{
	HKEY hKey;  
    int rtn;
	int   i=0,Comnum=0;
	
	CString StrTemp;
	
	
    //m_cmbComm.ResetContent();
    rtn = RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T("Hardware\\DeviceMap\\SerialComm"),   
		NULL, KEY_READ, &hKey);  //   打开串口注册表 
    if( rtn == ERROR_SUCCESS)     
    {   
		char  portName[255], commName[255];
		CString Com, serial_Name;
        DWORD   dwLong,dwSize;   
        while(1)   
        {   
			dwSize = sizeof(portName);
            dwLong   =   dwSize;
            rtn = RegEnumValue( hKey, i, portName, &dwLong,   
				NULL, NULL, (PUCHAR)commName, &dwSize );
            if( rtn == ERROR_NO_MORE_ITEMS )   //   枚举串口   
                break;      
            i++;   
			
			Com = (LPTSTR)commName;
			serial_Name = (CString)portName;
			
			if(Com.Left(3)=="COM")
			{
				//m_ComBoSerialSet.InsertString(Comnum,Com);
                //查出虚拟串口，并且标识出来
				if (serial_Name.Left(15)=="\\Device\\VSerial")
				{
					serial[Comnum]=Com+"(V)";
				}
				else
				{
					serial[Comnum]=Com;
				}
				
				Comnum++;
			}
        }   
        RegCloseKey(hKey); 
    }   
	
	//获取的串口字符串排序
	StrSort(serial,Comnum);
	return Comnum;
}

CString methord::GetAppPath()
{
	TCHAR modulePath[MAX_PATH];
    GetModuleFileName(NULL, modulePath, MAX_PATH);
    CString strModulePath(modulePath);
    strModulePath = strModulePath.Left(strModulePath.ReverseFind(_T('\\')));
    return strModulePath;
}