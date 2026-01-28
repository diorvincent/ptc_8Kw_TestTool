// Range.cpp : implementation file
//

#include "stdafx.h"
#include "acps.h"
#include "Range.h"
#include "ACPSDlg.h"
#include "NumberEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CfgSet cfg[MAX_SIZE];
/////////////////////////////////////////////////////////////////////////////
// CRange dialog

CRange::CRange(CWnd* pParent /*=NULL*/)
	: CDialog(CRange::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRange)
	//}}AFX_DATA_INIT
}


void CRange::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRange)
	DDX_Control(pDX, IDCANCEL, m_cancel);
	DDX_Control(pDX, IDC_BUTTON1, m_save);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_LISTCFG, m_cfglist);
	DDX_Control(pDX, IDC_NUMEDIT, m_numedit);
}


BEGIN_MESSAGE_MAP(CRange, CDialog)
	//{{AFX_MSG_MAP(CRange)
	ON_BN_CLICKED(IDC_BUTTON1, OnSave)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_DBLCLK, IDC_LISTCFG, &CRange::OnNMDblclkListcfg)
	ON_EN_KILLFOCUS(IDC_NUMEDIT, &CRange::OnEnKillfocusCfgedit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRange message handlers

void CRange::OnSave() 
{
	// TODO: Add your control notification handler code here
	CString sFile;
	
	TCHAR ch[100];
	memset(ch,0,sizeof(ch));
	
	GetModuleFileName(NULL,sFile.GetBuffer(MAX_PATH),MAX_PATH);
	sFile.ReleaseBuffer(MAX_PATH);
	sFile = sFile.Left(sFile.ReverseFind('\\') + 1);
	CString filename;
	filename.Format("range%d.ini", cfgindex);
	filename.Replace(" ", "");
	sFile += filename;
	
	CFileFind finder;
	
	if (!finder.FindFile(sFile))
	{
		//AfxMessageBox("当前目录没有发现range.ini配置文件");
	}

	UpdateData(TRUE);
	
	CString str;
	str = m_cfglist.GetItemText(0, 2);
	cfg[cfgindex].ov1 = atof(str);
	::WritePrivateProfileString("RANGE", "OVONE", str, sFile);
	str = m_cfglist.GetItemText(1, 2);
	cfg[cfgindex].ov2 = atof(str);
	::WritePrivateProfileString("RANGE", "OVTWO", str, sFile);
	str = m_cfglist.GetItemText(2, 2);
	cfg[cfgindex].uv1 = atof(str);
	::WritePrivateProfileString("RANGE", "UVONE", str, sFile);
	str = m_cfglist.GetItemText(3, 2);
	cfg[cfgindex].uv2 = atof(str);
	::WritePrivateProfileString("RANGE", "UVTWO", str, sFile);

	str = m_cfglist.GetItemText(4, 2);
	cfg[cfgindex].highvol_over_vol = atof(str);
	::WritePrivateProfileString("RANGE", "HI_OVER_VOL", str, sFile);
	str = m_cfglist.GetItemText(5, 2);
	cfg[cfgindex].highvol_over_restore_vol = atof(str);
	::WritePrivateProfileString("RANGE", "HI_OVER_RESTORE_VOL", str, sFile);
	
	str = m_cfglist.GetItemText(6, 2);
	cfg[cfgindex].highvol_under_vol = atof(str);
	::WritePrivateProfileString("RANGE", "HI_UNDER_VOL", str, sFile);
	str = m_cfglist.GetItemText(7, 2);
	cfg[cfgindex].highvol_under_restore_vol = atof(str);
	::WritePrivateProfileString("RANGE", "HI_UNDER_RESTORE_VOL", str, sFile);

	//str = m_cfglist.GetItemText(8, 2);
	//cfg[cfgindex].ptc_overtemp_low = atof(str);
	//::WritePrivateProfileString("RANGE", "PTC_OVER_TEMP", str, sFile);
	//str = m_cfglist.GetItemText(9, 2);
	//cfg[cfgindex].ptc_overtemp_high = atof(str);
	//::WritePrivateProfileString("RANGE", "PTC_RESTORE_TEMP", str, sFile);

	//str = m_cfglist.GetItemText(10, 2);
	//cfg[cfgindex].igbt_overtemp_low = atof(str);
	//::WritePrivateProfileString("RANGE", "IGBT_OVER_TEMP", str, sFile);
	//str = m_cfglist.GetItemText(11, 2);
	//cfg[cfgindex].igbt_overtemp_high = atof(str);
	//::WritePrivateProfileString("RANGE", "IGBT_RESTORE_TEMP", str, sFile);

	//功率测试参数（2Kw）
	str = m_cfglist.GetItemText(8, 2);
	cfg[cfgindex].max_ptc_power = atof(str);
	::WritePrivateProfileString("RANGE", "MAX_PTC_POWER", str, sFile);

	str = m_cfglist.GetItemText(9, 2);
	cfg[cfgindex].max_ptc_power_error = atof(str);
	::WritePrivateProfileString("RANGE", "MAX_PTC_POWER_ERROR", str, sFile);

	str = m_cfglist.GetItemText(10, 2);
	cfg[cfgindex].waitting_time = atoi(str);
	::WritePrivateProfileString("RANGE", "WAITTING_TIME", str, sFile);

	//功率测试参数（5Kw）
	str = m_cfglist.GetItemText(11, 2);
	cfg[cfgindex].max_ptc_power_5Kw = atof(str);
	::WritePrivateProfileString("RANGE", "MAX_PTC_POWER_5Kw", str, sFile);

	str = m_cfglist.GetItemText(12, 2);
	cfg[cfgindex].max_ptc_power_5Kw_error = atof(str);
	::WritePrivateProfileString("RANGE", "MAX_PTC_POWER_5Kw_ERROR", str, sFile);

	str = m_cfglist.GetItemText(13, 2);
	cfg[cfgindex].waitting_time_5Kw = atoi(str);
	::WritePrivateProfileString("RANGE", "WAITTING_TIME_5Kw", str, sFile);

	str = m_cfglist.GetItemText(14, 2);
	cfg[cfgindex].run_current = atof(str);
	::WritePrivateProfileString("RANGE", "PTC_CURRENT", str, sFile);

	//功率校准斜率参数
	str = m_cfglist.GetItemText(15, 2);
	cfg[cfgindex].slope_base1 = atof(str);
	::WritePrivateProfileString("RANGE", "SLOPE_BASE1", str, sFile);

	str = m_cfglist.GetItemText(16, 2);
	cfg[cfgindex].slope_base2 = atof(str);
	::WritePrivateProfileString("RANGE", "SLOPE_BASE2", str, sFile);

	str = m_cfglist.GetItemText(17, 2);
	cfg[cfgindex].slope_error1 = atof(str);
	::WritePrivateProfileString("RANGE", "SLOPE_ERROR1", str, sFile);

	str = m_cfglist.GetItemText(18, 2);
	cfg[cfgindex].slope_error2 = atof(str);
	::WritePrivateProfileString("RANGE", "SLOPE_ERROR2", str, sFile);

	//dutycycle1 (dutycycle1 value)
	str = m_cfglist.GetItemText(19, 2);
	cfg[cfgindex].duty_cycle_value1 = atof(str);
	::WritePrivateProfileString("RANGE", "DUTY_CYCLE_1_VALUE", str, sFile);

	//dutycycle1 wait time(dutycycle1 waitting time)
	str = m_cfglist.GetItemText(20, 2);
	cfg[cfgindex].per50_wait_time = atof(str);
	::WritePrivateProfileString("RANGE", "PER50_WAIT_TIME", str, sFile);

	//dutycycle2 (dutycycle2 value)
	str = m_cfglist.GetItemText(21, 2);
	cfg[cfgindex].duty_cycle_value2 = atof(str);
	::WritePrivateProfileString("RANGE", "DUTY_CYCLE_2_VALUE", str, sFile);

	//dutycycle2 wait time(dutycycle2 waitting time)
	str = m_cfglist.GetItemText(22, 2);
	cfg[cfgindex].per100_wait_time = atof(str);
	::WritePrivateProfileString("RANGE", "PER100_WAIT_TIME", str, sFile);

	//dutycycle3 (dutycycle3 value)
	str = m_cfglist.GetItemText(23, 2);
	cfg[cfgindex].duty_cycle_value3 = atof(str);
	::WritePrivateProfileString("RANGE", "DUTY_CYCLE_2_VALUE", str, sFile);

	//dutycycle3 wait time(dutycycle3 waitting time)
	str = m_cfglist.GetItemText(24, 2);
	cfg[cfgindex].per90_wait_time = atof(str);
	::WritePrivateProfileString("RANGE", "PER90_WAIT_TIME", str, sFile);

	str = m_cfglist.GetItemText(25, 2);
	if (str.GetLength() != 4)
	{
		MessageBox("项目信息字节长度必须是4个字节", "提示", MB_OK);
		return;
	}

	cfg[cfgindex].project_info = str.GetBuffer();
	::WritePrivateProfileString("RANGE", "PROJECT_INFO", str, sFile);
	str.ReleaseBuffer();

	//低压电源
	str = m_cfglist.GetItemText(26, 2);
	cfg[cfgindex].lowpre = atof(str);
	::WritePrivateProfileString("RANGE", "LOWPRE", str, sFile);

	str = m_cfglist.GetItemText(27, 2);
	cfg[cfgindex].lowcur = atof(str);
	::WritePrivateProfileString("RANGE", "LOWCUR", str, sFile);

	//报文丢失监控周期
	str = m_cfglist.GetItemText(28, 2);
	cfg[cfgindex].msg_lost_monitor_cycle = atoi(str);
	::WritePrivateProfileString("RANGE", "MSG_LOST_MONITOR_CYCLE", str, sFile);

	//更新界面参数
	CACPSDlg* pFather = (CACPSDlg*)GetParent();
	pFather->setcfg_tolist(cfgindex);

	AfxMessageBox("配置文件保存成功");
	UpdateData(FALSE);
}

BOOL CRange::OnInitDialog() 
{
	int i;
	CString str;
	CString valuestr;
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	UpdateData(true);
	m_numedit.ShowWindow(SW_HIDE);

	DWORD dwStyle = m_cfglist.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;//选中某行使整行高亮（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_GRIDLINES;//网格线（只适用与report风格的listctrl）  
	m_cfglist.SetExtendedStyle(dwStyle); //设置扩展风格

	m_cfglist.InsertColumn(0, "序号", LVCFMT_LEFT, 60); //插入列
	m_cfglist.InsertColumn(1, "参数名称", LVCFMT_LEFT, 200);
	m_cfglist.InsertColumn(2, "参数值", LVCFMT_LEFT, 300);

	for (i = 0; i <= 27; i++)
	{
		str.Format("%d", i + 1);
		int nRow = m_cfglist.InsertItem(i + 1, str); //插入行
	}

	m_cfglist.SetItemText(0, 1, "低压过压故障");
	m_cfglist.SetItemText(1, 1, "低压过压恢复");
	m_cfglist.SetItemText(2, 1, "低压欠压故障");
	m_cfglist.SetItemText(3, 1, "低压欠压恢复");

	m_cfglist.SetItemText(4, 1, "高压过压故障");
	m_cfglist.SetItemText(5, 1, "高压过压恢复");
	m_cfglist.SetItemText(6, 1, "高压欠压故障");
	m_cfglist.SetItemText(7, 1, "高压欠压恢复");

	m_cfglist.SetItemText(8, 1, "PTC 4Kw最大功率(Kw)");
	m_cfglist.SetItemText(9, 1, "PTC 4Kw最大功率误差(Kw)");
	m_cfglist.SetItemText(10, 1, "PTC 4Kw功率控制时间(s)");

	m_cfglist.SetItemText(11, 1, "PTC 8Kw最大功率(Kw)");
	m_cfglist.SetItemText(12, 1, "PTC 8Kw最大功率误差(Kw)");
	m_cfglist.SetItemText(13, 1, "PTC 8Kw功率控制时间(s)");
	m_cfglist.SetItemText(14, 1, "PTC测试电流(A)");

	m_cfglist.SetItemText(15, 1, "斜率基准值1");
	m_cfglist.SetItemText(16, 1, "斜率基准值2");
	m_cfglist.SetItemText(17, 1, "斜率误差值1");
	m_cfglist.SetItemText(18, 1, "斜率误差值2");

	m_cfglist.SetItemText(19, 1, "占空比1值");
	m_cfglist.SetItemText(20, 1, "占空比1运行时间");
	m_cfglist.SetItemText(21, 1, "占空比2值");
	m_cfglist.SetItemText(22, 1, "占空比2运行时间");
	m_cfglist.SetItemText(23, 1, "占空比3值");
	m_cfglist.SetItemText(24, 1, "占空比3运行时间");

	m_cfglist.SetItemText(25, 1, "项目信息");
	m_cfglist.SetItemText(26, 1, "低压电源电压(V)");
	m_cfglist.SetItemText(27, 1, "低压电源电流(A)");
	m_cfglist.SetItemText(28, 1, "报文丢失监控周期");

	
	//低压故障参数
	valuestr.Format("%7.1f", cfg[cfgindex].ov1);
	m_cfglist.SetItemText(0, 2, valuestr);
	valuestr.Format("%7.1f", cfg[cfgindex].ov2);
	m_cfglist.SetItemText(1, 2, valuestr);
	valuestr.Format("%7.1f", cfg[cfgindex].uv1);
	m_cfglist.SetItemText(2, 2, valuestr);
	valuestr.Format("%7.1f", cfg[cfgindex].uv2);
	m_cfglist.SetItemText(3, 2, valuestr);
	//高压故障参数
	valuestr.Format("%7.1f", cfg[cfgindex].highvol_over_vol);
	m_cfglist.SetItemText(4, 2, valuestr);
	valuestr.Format("%7.1f", cfg[cfgindex].highvol_over_restore_vol);
	m_cfglist.SetItemText(5, 2, valuestr);
	valuestr.Format("%7.1f", cfg[cfgindex].highvol_under_vol);
	m_cfglist.SetItemText(6, 2, valuestr);
	valuestr.Format("%7.1f", cfg[cfgindex].highvol_under_restore_vol);
	m_cfglist.SetItemText(7, 2, valuestr);

	//功率测试参数
	valuestr.Format("%7.2f", cfg[cfgindex].max_ptc_power);
	m_cfglist.SetItemText(8, 2, valuestr);
	valuestr.Format("%7.2f", cfg[cfgindex].max_ptc_power_error);
	m_cfglist.SetItemText(9, 2, valuestr);
	valuestr.Format("%7d", cfg[cfgindex].waitting_time);
	m_cfglist.SetItemText(10, 2, valuestr);

	valuestr.Format("%7.2f", cfg[cfgindex].max_ptc_power_5Kw);
	m_cfglist.SetItemText(11, 2, valuestr);
	valuestr.Format("%7.2f", cfg[cfgindex].max_ptc_power_5Kw_error);
	m_cfglist.SetItemText(12, 2, valuestr);
	valuestr.Format("%7d", cfg[cfgindex].waitting_time_5Kw);
	m_cfglist.SetItemText(13, 2, valuestr);
	valuestr.Format("%7.1f", cfg[cfgindex].run_current);
	m_cfglist.SetItemText(14, 2, valuestr);

	//功率校准参数
	valuestr.Format("%7.15f", cfg[cfgindex].slope_base1);
	m_cfglist.SetItemText(15, 2, valuestr);
	valuestr.Format("%7.16f", cfg[cfgindex].slope_base2);
	m_cfglist.SetItemText(16, 2, valuestr);
	valuestr.Format("%7.15f", cfg[cfgindex].slope_error1);
	m_cfglist.SetItemText(17, 2, valuestr);
	valuestr.Format("%7.15f", cfg[cfgindex].slope_error2);
	m_cfglist.SetItemText(18, 2, valuestr);

	//占空比请求等待时间和值
	valuestr.Format("%7.2f", cfg[cfgindex].duty_cycle_value1);
	m_cfglist.SetItemText(19, 2, valuestr);
	valuestr.Format("%7d", cfg[cfgindex].per50_wait_time);
	m_cfglist.SetItemText(20, 2, valuestr);

	valuestr.Format("%7.2f", cfg[cfgindex].duty_cycle_value2);
	m_cfglist.SetItemText(21, 2, valuestr);
	valuestr.Format("%7d", cfg[cfgindex].per100_wait_time);
	m_cfglist.SetItemText(22, 2, valuestr);

	valuestr.Format("%7.2f", cfg[cfgindex].duty_cycle_value3);
	m_cfglist.SetItemText(23, 2, valuestr);
	valuestr.Format("%7d", cfg[cfgindex].per90_wait_time);
	m_cfglist.SetItemText(24, 2, valuestr);

	//项目信息
	valuestr.Format("%s", cfg[cfgindex].project_info.c_str());
	m_cfglist.SetItemText(25, 2, valuestr);

	//低压电源供电值
	valuestr.Format("%7.2f", cfg[cfgindex].lowpre);
	m_cfglist.SetItemText(26, 2, valuestr);
	valuestr.Format("%7.2f", cfg[cfgindex].lowcur);
	m_cfglist.SetItemText(27, 2, valuestr);

	//报文丢失监控周期
	valuestr.Format("%7d", cfg[cfgindex].msg_lost_monitor_cycle);
	m_cfglist.SetItemText(28, 2, valuestr);

	m_cfglist.ShowWindow(SW_SHOW);
	UpdateData(false);	
	return TRUE;  
}


void CRange::OnNMDblclkListcfg(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CRect rc;
	CString strTemp;
	NM_LISTVIEW *pNMListView = (NM_LISTVIEW *)pNMHDR;
	m_Row = pNMListView->iItem;
	m_Col = pNMListView->iSubItem;

	if (pNMListView->iItem == -1)	//选择空白处，添加一行，并设置焦点为最后一行，第二列
	{
		m_Row = m_cfglist.GetItemCount();
		strTemp.Format(_T("%d"), m_Row + 1);
		m_cfglist.InsertItem(m_Row, strTemp);
		m_cfglist.SetItemState(m_Row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		m_cfglist.EnsureVisible(m_Row, FALSE);
		//m_Col = 1;
	}

	if (m_Col != 0)	// 选择子项
	{
		m_cfglist.GetSubItemRect(m_Row, m_Col, LVIR_LABEL, rc);
		m_numedit.SetParent(&m_cfglist);
		m_numedit.MoveWindow(rc);
		m_numedit.SetWindowText(m_cfglist.GetItemText(m_Row, m_Col));
		m_numedit.ShowWindow(SW_SHOW);
		m_numedit.SetFocus();//设置Edit焦点
		m_numedit.ShowCaret();//显示光标
		m_numedit.SetSel(0, -1);//全选
	}

	*pResult = 0;
}


void CRange::OnEnKillfocusCfgedit()
{
	// TODO: 在此添加控件通知处理程序代码

	CString str;
	m_numedit.GetWindowText(str);
	m_cfglist.SetItemText(m_Row, m_Col, str);
	m_numedit.ShowWindow(SW_HIDE);
}
