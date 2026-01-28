#pragma once
#include <vector>

// CListCtrlP
//字体颜色结构体
struct ItemColor
{
	int m_iRow;
	int m_iCol;
	COLORREF m_TextColor;
};

class CListCtrlP : public CListCtrl
{
	DECLARE_DYNAMIC(CListCtrlP)

public:
	CListCtrlP();
	virtual ~CListCtrlP();

protected:
	DECLARE_MESSAGE_MAP()
public:
	//重新绘制的方法
	//afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
	//设置颜色
	void SetColor(int iRow, int iCol, COLORREF color);
	//清空
	void ClearColor();
private:
	//用于保存某一行某一列的字体颜色
	std::vector<ItemColor> m_itemColors;
	COLORREF m_TextColor;
public:
	int m_iRow ; 
	int m_iCol;
	CFont *m_Font;
	COLORREF m_Color;
public:
	afx_msg void OnNMCustomdrawListExcel(NMHDR *pNMHDR, LRESULT *pResult);
};


