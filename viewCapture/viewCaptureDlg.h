
// viewCaptureDlg.h : header file
//

#pragma once


// CviewCaptureDlg dialog
class CviewCaptureDlg : public CDialogEx
{
// Construction
public:
	CviewCaptureDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_VIEWCAPTURE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedChk80col();
	afx_msg void OnBnClickedChkbg();
	afx_msg void OnBnClickedChksprite();
	afx_msg void OnBnClickedChkblank();
	afx_msg void OnBnClickedChkflicker();
	afx_msg void OnBnClickedBtnsave();
	afx_msg void OnBnClickedBtnload();
	
	void FixSpriteChecks();
	void FixScreenColors();
	int countbits(int x);
	void setDefaults(int which);
	void updateByReg();
	void updateDropDowns();
	void UpdateList(int ctrl, char *str, int which);
	void UpdateReg(int idx);
	void updateDropdown(int ctrl, int reg);

	afx_msg void OnCbnSelchangeLstdefaults();
	afx_msg void OnEnChangeTxtr0();
	afx_msg void OnEnChangeTxtr1();
	afx_msg void OnEnChangeTxtr2();
	afx_msg void OnEnChangeTxtr3();
	afx_msg void OnEnChangeTxtr4();
	afx_msg void OnEnChangeTxtr5();
	afx_msg void OnEnChangeTxtr6();
	afx_msg void OnEnChangeTxtr7();
	afx_msg void OnCbnSelchangeLstmode();
	afx_msg void OnCbnSelchangeLstsit();
	afx_msg void OnCbnSelchangeLstct();
	afx_msg void OnCbnSelchangeLstpdt();
	afx_msg void OnCbnSelchangeLstsal();
	afx_msg void OnCbnSelchangeLstsdt();
	afx_msg void OnCbnSelchangeLstctsize();
	afx_msg void OnCbnSelchangeLstpatsize();
	afx_msg void OnBnClickedChk2x();
	afx_msg void OnBnClickedChkmag();
	afx_msg void OnCbnSelchangeLsttxtcol();
	afx_msg void OnCbnSelchangeLstscrncol();
};
