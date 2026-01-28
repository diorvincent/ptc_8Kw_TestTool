#include "stdafx.h"
#include "CSnapWnd.h"
#include "resource.h"

//

HWND cutWnd;
CRect cutRc;
CString  savePath;

CSnapWnd::CSnapWnd()
{
}

/// <summary>
/// 获取控件截图，保存成图片
/// </summary>
/// <param name="hwnd">控件窗口句柄</param>
/// <param name="strSavePath">截图保存路径</param>
void CSnapWnd::SnapWnd(HWND hwnd, CString strSavePath, CString strIssueName)
{
	CString savaFullPath;
	RECT rc;

	//截取控件截图
	HWND DeskHwnd = ::GetWindow(hwnd, GW_HWNDFIRST);//::GetDesktopWindow(); //取得桌面句柄
	HDC DeskDC = ::GetWindowDC(DeskHwnd); //取得桌面设备场景
	int oldRop2 = SetROP2(DeskDC, R2_NOTXORPEN);
		
	::GetWindowRect(hwnd, &rc); //获得窗口矩形
	cutWnd = hwnd;
	cutRc = rc;
	if (rc.left < 0) rc.left = 0;
	if (rc.top < 0) rc.top = 0;
	HPEN newPen = ::CreatePen(PS_SOLID, 3, RGB(255, 0, 0)); //建立新画笔,载入DeskDC
	HGDIOBJ oldPen = ::SelectObject(DeskDC, newPen);
	::Rectangle(DeskDC, rc.left, rc.top, rc.right, rc.bottom); //在窗口周围显示闪烁矩形
	Sleep(50); //设置闪烁时间间隔
	::Rectangle(DeskDC, rc.left, rc.top, rc.right, rc.bottom);
	::SetROP2(DeskDC, oldRop2);
	::SelectObject(DeskDC, oldPen);
	::DeleteObject(newPen);
	::ReleaseDC(DeskHwnd, DeskDC);
	DeskDC = NULL;
	
	//CopyBitmapToClipboard(FromHandle(cutWnd), TRUE);
	//CopyBitmapToClipboard(CopyScreenToBitmap((LPRECT)&cutRc));
	// 
	//保存控件截图
	if(strIssueName=="")
		savaFullPath = strSavePath + GetNowTime(_T("%Y%m%d%H%M%S")) + _T(".bmp");
	else
		savaFullPath = strSavePath + strIssueName + _T(".bmp");
	CSnapWnd::SaveBitmapToFile(CopyScreenToBitmap((LPRECT)&cutRc), savaFullPath);
	
}


void CSnapWnd::CopyBitmapToClipboard(CWnd* wnd, BOOL FullWnd)
{
	CDC* dc;
	if (FullWnd)
	{
		/* 抓取整个窗口*/
		dc = new CWindowDC(wnd);
	}
	else
	{
		/* 仅抓取客户区时*/
		dc = new CClientDC(wnd);
	}

	CDC memDC;
	memDC.CreateCompatibleDC(dc);

	CBitmap bm;
	CRect r;
	if (FullWnd)
		wnd->GetWindowRect(&r);
	else
		wnd->GetClientRect(&r);

	CString s;
	wnd->GetWindowText(s);
	CSize sz(r.Width(), r.Height());
	bm.CreateCompatibleBitmap(dc, sz.cx, sz.cy);

	CBitmap* oldbm = memDC.SelectObject(&bm);
	memDC.BitBlt(0, 0, sz.cx, sz.cy, dc, 0, 0, SRCCOPY);
	//直接调用OpenClipboard()，而不用wnd->GetParent()->OpenClipboard();
	wnd->OpenClipboard();

	::EmptyClipboard();
	::SetClipboardData(CF_BITMAP, bm.m_hObject);
	CloseClipboard();
	//恢复原始环境
	memDC.SelectObject(oldbm);
	bm.Detach();
	delete dc;

	// 加一句提示
	//::MessageBox(NULL, _T("复制完成"), "info", IDOK);

}

/// <summary>
/// 将HBITMAP对象复制到剪切板
/// </summary>
/// <param name="hBitmap">要复制的HBITMAP对象</param>
void CSnapWnd::CopyBitmapToClipboard(HBITMAP hBitmap)
{
	::OpenClipboard(cutWnd);
	EmptyClipboard();
	SetClipboardData(CF_BITMAP, hBitmap);
	CloseClipboard();

	// 加一句提示
	//::MessageBox(NULL, _T("复制完成"), "info", IDOK);

}

/// <summary>
/// 将屏幕的某个矩形区域的图像复制到一个位图对象中
/// </summary>
/// <param name="lpRect">输入一个确定矩形范围的LPRECT型数据</param>
/// <returns>一个存储了截图的bitmap句柄</returns>
HBITMAP CSnapWnd::CopyScreenToBitmap(LPRECT lpRect)
{
	HDC hScrDC, hMemDC;
	// 屏幕和内存设备描述表 
	HBITMAP hBitmap, hOldBitmap;
	// 位图句柄 
	int nX, nY, nX2, nY2;
	// 选定区域坐标 
	int nWidth, nHeight;
	// 位图宽度和高度 
	int xScrn, yScrn;
	// 屏幕分辨率

	// 确保选定区域不为空矩形 
	if (IsRectEmpty(lpRect))
		return NULL;

	//为屏幕创建设备描述表 
	hScrDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	//为屏幕设备描述表创建兼容的内存设备描述表 
	hMemDC = CreateCompatibleDC(hScrDC);
	// 获得选定区域坐标 
	nX = lpRect->left;
	nY = lpRect->top;
	nX2 = lpRect->right;
	nY2 = lpRect->bottom;
	// 获得屏幕分辨率 
	xScrn = GetDeviceCaps(hScrDC, HORZRES);
	yScrn = GetDeviceCaps(hScrDC, VERTRES);
	//确保选定区域是可见的 
	if (nX < 0)
		nX = 0;
	if (nY < 0)
		nY = 0;
	if (nX2 > xScrn)
		nX2 = xScrn;
	if (nY2 > yScrn)
		nY2 = yScrn;
	nWidth = nX2 - nX;
	nHeight = nY2 - nY;
	// 创建一个与屏幕设备描述表兼容的位图 
	hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);
	// 把新位图选到内存设备描述表中 
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
	// 把屏幕设备描述表拷贝到内存设备描述表中 
	BitBlt(hMemDC, 0, 0, nWidth, nHeight,hScrDC, nX, nY, SRCCOPY);
	//得到屏幕位图的句柄 
	hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);
	//清除 
	DeleteDC(hScrDC);
	DeleteDC(hMemDC);

	// 返回位图句柄 
	return hBitmap;
}

/// <summary>
/// 将截取的图像存储为文件
/// </summary>
/// <param name="hBitmap">HBITMAP对象</param>
/// <param name="lpFileName">文件名（路径+xx.bmp）</param>
/// <returns></returns>
BOOL CSnapWnd::SaveBitmapToFile(HBITMAP hBitmap, LPCSTR lpFileName)
{
	HDC hDC; //设备描述表 
	int iBits; //当前显示分辨率下每个像素所占字节数 
	WORD wBitCount; //位图中每个像素所占字节数 
	DWORD dwPaletteSize = 0, //定义调色板大小， 位图中像素字节大小 ，位图文件大小 ， 写入文件字节数 
		dwBmBitsSize,
		dwDIBSize, dwWritten;
	BITMAP Bitmap; //位图属性结构 
	BITMAPFILEHEADER bmfHdr; //位图文件头结构 
	BITMAPINFOHEADER bi; //位图信息头结构 
	LPBITMAPINFOHEADER lpbi; //指向位图信息头结构 

	HANDLE fh, hDib, hPal, hOldPal = NULL; //定义文件，分配内存句柄，调色板句柄 

	//计算位图文件每个像素所占字节数 
	HDC hWndDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	hDC = ::CreateCompatibleDC(hWndDC);
	iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
	DeleteDC(hDC);

	if (iBits <= 1)
		wBitCount = 1;
	else if (iBits <= 4)
		wBitCount = 4;
	else if (iBits <= 8)
		wBitCount = 8;
	else if (iBits <= 24)
		wBitCount = 24;
	else
		wBitCount = 24;

	//计算调色板大小 
	if (wBitCount <= 8)
		dwPaletteSize = (1 << wBitCount) * sizeof(RGBQUAD);

	//设置位图信息头结构 
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = Bitmap.bmWidth;
	bi.biHeight = Bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = wBitCount;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	dwBmBitsSize = ((Bitmap.bmWidth * wBitCount + 31) / 32) * 4 * Bitmap.bmHeight;

	//为位图内容分配内存 
	hDib = GlobalAlloc(GMEM_MOVEABLE, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	if (lpbi == NULL)
	{
		DWORD dwError = GetLastError();
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dwError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // 默认语言
			(LPTSTR)&lpMsgBuf,
			0, NULL);

		printf("GlobalLock error:%s", dwError);
		return FALSE;
	}
	*lpbi = bi;

	// 处理调色板 
	hPal = GetStockObject(DEFAULT_PALETTE);
	if (hPal)
	{
		hDC = ::GetDC(NULL);
		hOldPal = ::SelectPalette(hDC, (HPALETTE)hPal, FALSE);
		RealizePalette(hDC);
	}

	// 获取该调色板下新的像素值 
	GetDIBits(hDC, hBitmap, 0, (UINT)Bitmap.bmHeight,
		(LPSTR)lpbi + sizeof(BITMAPINFOHEADER)
		+ dwPaletteSize,
		(LPBITMAPINFO)
		lpbi, DIB_RGB_COLORS);

	//恢复调色板 
	if (hOldPal)
	{
		SelectPalette(hDC, (HPALETTE)hOldPal, TRUE);
		RealizePalette(hDC);
		::ReleaseDC(NULL, hDC);
	}

	//创建位图文件 
	fh = CreateFile(lpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;

	// 设置位图文件头 
	bmfHdr.bfType = 0x4D42; // "BM" 
	dwDIBSize = sizeof(BITMAPFILEHEADER)
		+ sizeof(BITMAPINFOHEADER)
		+ dwPaletteSize + dwBmBitsSize;
	bmfHdr.bfSize = dwDIBSize;
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER)
		+ (DWORD)sizeof(BITMAPINFOHEADER)
		+ dwPaletteSize;

	// 写入位图文件头 
	WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);

	// 写入位图文件其余内容 
	WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);

	//清除 
	GlobalUnlock(hDib);
	GlobalFree(hDib);
	CloseHandle(fh);

	return TRUE;
}

void CSnapWnd::OnBtnSavepath(CString strSavePath)
{
	LPCSTR tempPath[256];
	GetSavePath(tempPath);
	savePath.Format("%s", tempPath);

	//GetNowTime(_T("%Y年%m月%d日%H时%M分%S秒"));
}

/// <summary>
/// 弹出一个文件夹选择对话框
/// </summary>
/// <returns>用户选择的文件夹路径</returns>
void CSnapWnd::GetSavePath(LPCSTR* tempPath)
{
#define BIF_NEWDIALOGSTYLE 0x00000040 // 因为vc6.0不支持BIF_NEWDIALOGSTYLE,vs2003以上无此问题（此句可删掉）

	TCHAR Buffer[MAX_PATH];
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(BROWSEINFO));
	bi.hwndOwner = NULL;
	bi.ulFlags = BIF_RETURNONLYFSDIRS;    //要求返回文件系统的目

	bi.ulFlags = BIF_NEWDIALOGSTYLE;        //窗口可以调整大小，有新建文件夹按钮

	bi.pszDisplayName = Buffer;           //此参数如为NULL则不能显示对话框 
	bi.lpszTitle = _T("请选择文件夹");
	bi.lpfn = NULL;
	bi.iImage = IDR_MAINFRAME;

	LPITEMIDLIST pIDList = SHBrowseForFolder(&bi);//调用显示选择对话框
	if (pIDList)
	{
		SHGetPathFromIDList(pIDList, Buffer);
		//取得文件夹路径到Buffer里

		*tempPath = Buffer;		
	}
	else
	{
		return;  // 用户点了取消
	}

	LPMALLOC lpMalloc;
	if (FAILED(SHGetMalloc(&lpMalloc)))
		return;

	//释放内存
	lpMalloc->Free(pIDList);
	lpMalloc->Release();
}

/// <summary>
/// 通过CTime类获取当前时间
/// </summary>
/// <param name="formatStr">格式化样式参数</param>
/// <returns>相应的格式化后的日期</returns>
CString CSnapWnd::GetNowTime(CString formatStr)
{
	CTime m_time;

	m_time = CTime::GetCurrentTime();   //获取当前时间日期
	return m_time.Format(formatStr);
}
