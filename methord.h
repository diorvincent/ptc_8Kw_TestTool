// methord.h: interface for the methord class.
//
//////////////////////////////////////////////////////////////////////
#include "numtype.h"
#include <iostream>
#include <cmath> // for std::round

#if !defined(AFX_METHORD_H__7858A136_CA0F_4711_AAED_568B1B9085F6__INCLUDED_)
#define AFX_METHORD_H__7858A136_CA0F_4711_AAED_568B1B9085F6__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class methord  
{
public:

	static u8 u16tou8(u8 *tm,u16 data);
	static u8 u32tou8(u8 *tm,u32 data);		
	static u8 u8tou16(const u8 *tm,u16 *data);
	static u8 u8tou32(const u8 *tm,u32 *data);
	
	static u8 strtou32(u8 *str,u32 *data);
	static u8 strtou16(u8 *str,u16 *data);

	static u8 u16toPu8(u16 data, u8* pU8);

	static u8 u8tostr(u8 data, u8 *str);
	static CString u8tostr(const u8 *data, u16 len);
	static u8 u16tostr(u16 data,u8 *str);

	static u8 u32tostr(u32 data,u8 *str); //20251230
	static std::string Int2Hex(int val);//20251230

	static s16 strlen_1(uc8 *src,s16 maxstrlen=4096);
	static s16 FindASCII(u8 *src,u8 ascii,s16 srclen=0x00);
	static int autocpy(u8 *dst,const u8 *src,int dstlen=0,int srclen=0,u8 ascii=0);

	static s16 gsmBytes2String(u8*pSrc, u8* pDst, s16 nSrcLength);
	static s16 gsmString2Bytes(u8* pSrc, u8* pDst, s16 nSrcLength);

	static int methord::FindCommPort(CString *serial);
	static void StrSort(CString *StrList, int Comnum);
	static CString GetAppPath();//获取应用程序路径

	static void CalcFractionalPart(u8* pSrc, double* fValue, int nSrcLen);
	static void Calcute8710Val(u32* src, float* str, char* err);

	static void UDS_Seed2Key(u8* pInSeed, u8* pKey);

	methord();
	virtual ~methord();

};


static class FixedPoint
{
public:
	FixedPoint() {};
	int makeRaw(float f, int scale) // 获取原始的定点数表示
	{
		return  static_cast<int>(std::round(f * std::pow(2, scale)));
	}

	float toFloat(int value, int scale) const	// 转换为浮点数时的值
	{
		return static_cast<float>(value / std::pow(2, scale));
	}
};

#endif // !defined(AFX_METHORD_H__7858A136_CA0F_4711_AAED_568B1B9085F6__INCLUDED_)
