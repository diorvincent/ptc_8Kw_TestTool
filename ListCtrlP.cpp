// ListCtrlP.cpp : 实现文件
//

#include "stdafx.h"
#include "ListCtrlP.h"


// CListCtrlP

IMPLEMENT_DYNAMIC(CListCtrlP, CListCtrl)

CListCtrlP::CListCtrlP()
{
	m_iRow = -1;  //这里我们定义了三个成员变量，分别用于表示单元格的“行号”，“列号”，“字体” 在构造函数中初始化他们。
	m_iCol = -1;
	m_Font = NULL;
}

CListCtrlP::~CListCtrlP()
{
}

BEGIN_MESSAGE_MAP(CListCtrlP, CListCtrl)
	//{{AFX_MSG_MAP(CColorListCtrl)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdrawListExcel)
END_MESSAGE_MAP()

// CListCtrlP 消息处理程序

void CListCtrlP::SetColor(int iRow, int iCol, COLORREF color)
{
	ItemColor itemColor;
	itemColor.m_iRow = iRow;
	itemColor.m_iCol = iCol;
	itemColor.m_TextColor = color;
	m_itemColors.push_back(itemColor);
	//m_iRow = iRow;
	//m_iCol = iCol;
	//m_Color = color;
}

void CListCtrlP::ClearColor()
{
	m_itemColors.clear();
}


void CListCtrlP::OnNMCustomdrawListExcel(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR); //首先声明一个NMLVCUSTOMDRAW结构体的指针pLVCD，关联pNMHDR，为了下面的操作。

																	   // Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		// This is the notification message for an item. We'll request
		// notifications before each subitem's prepaint stage.
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	}
	else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage) //仅当pLVCD结构体中nmcd成员的dwDrawStage状态为CDDS_ITEMPREPAINT | CDDS_SUBITEM时
	{     
		pLVCD->clrTextBk = 16777215;//如果不是选择的“行”和“列”就设置成系统默认的那种颜色。
		pLVCD->clrText = 0;
		for (unsigned int i=0;i<m_itemColors.size();i++)
		{
			if (m_itemColors[i].m_iCol == pLVCD->iSubItem && m_itemColors[i].m_iRow == pLVCD->nmcd.dwItemSpec)
			{
				pLVCD->clrTextBk = m_itemColors[i].m_TextColor;
			}
		}

		//SetFont(m_Font, false);
		// Store the colors back in the NMLVCUSTOMDRAW struct.
		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;
	}
}
