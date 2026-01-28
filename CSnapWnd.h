#pragma once
class CSnapWnd:CWnd
{
public:
	CSnapWnd();

	static void SnapWnd(HWND hwnd, CString strSavePath, CString strIssueName = "");
	
private:
	static void CopyBitmapToClipboard(CWnd* wnd, BOOL FullWnd);
	static void CopyBitmapToClipboard(HBITMAP hBitmap);
	static HBITMAP CopyScreenToBitmap(LPRECT lpRect);
	static BOOL SaveBitmapToFile(HBITMAP hBitmap, LPCSTR lpFileName);
	static void OnBtnSavepath(CString strSavePath);
	static void GetSavePath(LPCSTR* tempPath);
	static CString GetNowTime(CString formatStr);
};

