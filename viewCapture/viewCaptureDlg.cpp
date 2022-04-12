
// viewCaptureDlg.cpp : implementation file
//

#include "stdafx.h"
#include "viewCapture.h"
#include "viewCaptureDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "tivdp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define DEFAULT_LIST 7
static const unsigned char VDPDefaults[][8] = {
	/* 0 = EA */	{	0x00, 0xe0, 0x00, 0x0e, 0x01, 0x06, 0x00, 0xf3	},
	/* 1 = text */	{	0x00, 0xf0, 0x00, 0x0e, 0x01, 0x06, 0x00, 0xf4	},	// TI Writer
	/* 2 = Title */	{	0x00, 0xe0, 0xf0, 0x0e, 0xf9, 0x86, 0xf8, 0xf7	},	
	/* 3 = BASIC */	{	0x00, 0xe0, 0xf0, 0x0c, 0xf8, 0x86, 0xf8, 0x03	},
	/* 4 = XB */	{	0x00, 0xe0, 0x00, 0x20, 0x00, 0x06, 0x00, 0x07	},
	/* 5 = bitmap */{	0x02, 0xe2, 0x06, 0xff, 0x03, 0x36, 0x03, 0x11	},	// Parsec
	/* 6 = alt bmp*/{	0x02, 0xe2, 0x06, 0x7f, 0x07, 0x36, 0x03, 0x11	},	// manually swapped tables
	/* 7 = halfbmp*/{	0x02, 0xe0, 0x02, 0x9f, 0x00, 0x06, 0x00, 0x01	}	// Rock Runner
};

// buffers for long strings
static char colorAdrList[256*4+1];
static char spriteAdrList[128*4+1];

// CviewCaptureDlg dialog
CviewCaptureDlg::CviewCaptureDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CviewCaptureDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CviewCaptureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CviewCaptureDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CviewCaptureDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CviewCaptureDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_CHK80COL, &CviewCaptureDlg::OnBnClickedChk80col)
	ON_BN_CLICKED(IDC_CHKBG, &CviewCaptureDlg::OnBnClickedChkbg)
	ON_BN_CLICKED(IDC_CHKSPRITE, &CviewCaptureDlg::OnBnClickedChksprite)
	ON_BN_CLICKED(IDC_CHKBLANK, &CviewCaptureDlg::OnBnClickedChkblank)
	ON_BN_CLICKED(IDC_CHKFLICKER, &CviewCaptureDlg::OnBnClickedChkflicker)
	ON_BN_CLICKED(IDC_BTNSAVE, &CviewCaptureDlg::OnBnClickedBtnsave)
	ON_BN_CLICKED(IDC_BTNLOAD, &CviewCaptureDlg::OnBnClickedBtnload)
	ON_CBN_SELCHANGE(IDC_LSTDEFAULTS, &CviewCaptureDlg::OnCbnSelchangeLstdefaults)
	ON_EN_KILLFOCUS(IDC_TXTR0, &CviewCaptureDlg::OnEnChangeTxtr0)
	ON_EN_KILLFOCUS(IDC_TXTR1, &CviewCaptureDlg::OnEnChangeTxtr1)
	ON_EN_KILLFOCUS(IDC_TXTR2, &CviewCaptureDlg::OnEnChangeTxtr2)
	ON_EN_KILLFOCUS(IDC_TXTR3, &CviewCaptureDlg::OnEnChangeTxtr3)
	ON_EN_KILLFOCUS(IDC_TXTR4, &CviewCaptureDlg::OnEnChangeTxtr4)
	ON_EN_KILLFOCUS(IDC_TXTR5, &CviewCaptureDlg::OnEnChangeTxtr5)
	ON_EN_KILLFOCUS(IDC_TXTR6, &CviewCaptureDlg::OnEnChangeTxtr6)
	ON_EN_KILLFOCUS(IDC_TXTR7, &CviewCaptureDlg::OnEnChangeTxtr7)
	ON_CBN_SELCHANGE(IDC_LSTMODE, &CviewCaptureDlg::OnCbnSelchangeLstmode)
	ON_CBN_SELCHANGE(IDC_LSTSIT, &CviewCaptureDlg::OnCbnSelchangeLstsit)
	ON_CBN_SELCHANGE(IDC_LSTCT, &CviewCaptureDlg::OnCbnSelchangeLstct)
	ON_CBN_SELCHANGE(IDC_LSTPDT, &CviewCaptureDlg::OnCbnSelchangeLstpdt)
	ON_CBN_SELCHANGE(IDC_LSTSAL, &CviewCaptureDlg::OnCbnSelchangeLstsal)
	ON_CBN_SELCHANGE(IDC_LSTSDT, &CviewCaptureDlg::OnCbnSelchangeLstsdt)
	ON_CBN_SELCHANGE(IDC_LSTCTSIZE, &CviewCaptureDlg::OnCbnSelchangeLstctsize)
	ON_CBN_SELCHANGE(IDC_LSTPATSIZE, &CviewCaptureDlg::OnCbnSelchangeLstpatsize)
	ON_BN_CLICKED(IDC_CHK2X, &CviewCaptureDlg::OnBnClickedChk2x)
	ON_BN_CLICKED(IDC_CHKMAG, &CviewCaptureDlg::OnBnClickedChkmag)
	ON_CBN_SELCHANGE(IDC_LSTTXTCOL, &CviewCaptureDlg::OnCbnSelchangeLsttxtcol)
	ON_CBN_SELCHANGE(IDC_LSTSCRNCOL, &CviewCaptureDlg::OnCbnSelchangeLstscrncol)
END_MESSAGE_MAP()


// CviewCaptureDlg message handlers

BOOL CviewCaptureDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// set up the display
	VDPinit(GetSafeHwnd());
	((CButton*)GetDlgItem(IDC_CHK80COL))->SetCheck(BST_CHECKED);

	// set up some helper strings
	colorAdrList[0]='\0';
	for (int idx=0; idx<256; idx++) {
		sprintf(colorAdrList, "%s%04X", colorAdrList, idx<<6);
	}
	spriteAdrList[0]='\0';
	for (int idx=0; idx<128; idx++) {
		sprintf(spriteAdrList, "%s%04X", spriteAdrList, idx<<7);
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CviewCaptureDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		UpdateDisplay();
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CviewCaptureDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Check boxes - just update the local option and redraw
void CviewCaptureDlg::OnBnClickedChk80col()
{
	if (((CButton*)GetDlgItem(IDC_CHK80COL))->GetCheck() == BST_CHECKED) {
		bEnable80Columns = true;
	} else {
		bEnable80Columns = false;
	}
	UpdateDisplay();
}

void CviewCaptureDlg::OnBnClickedChkbg()
{
	if (((CButton*)GetDlgItem(IDC_CHKBG))->GetCheck() == BST_CHECKED) {
		bDisableBackground = true;
	} else {
		bDisableBackground = false;
	}
	UpdateDisplay();
}

void CviewCaptureDlg::OnBnClickedChksprite()
{
	if (((CButton*)GetDlgItem(IDC_CHKSPRITE))->GetCheck() == BST_CHECKED) {
		bDisableSprite = true;
	} else {
		bDisableSprite = false;
	}
	UpdateDisplay();
}

void CviewCaptureDlg::OnBnClickedChkblank()
{
	if (((CButton*)GetDlgItem(IDC_CHKBLANK))->GetCheck() == BST_CHECKED) {
		bDisableBlank = true;
	} else {
		bDisableBlank = false;
	}
	UpdateDisplay();
}

void CviewCaptureDlg::OnBnClickedChkflicker()
{
	if (((CButton*)GetDlgItem(IDC_CHKFLICKER))->GetCheck() == BST_CHECKED) {
		bUse5SpriteLimit = true;
	} else {
		bUse5SpriteLimit = false;
	}
	UpdateDisplay();
}

void CviewCaptureDlg::OnBnClickedChk2x()
{
	if (((CButton*)GetDlgItem(IDC_CHK2X))->GetCheck() == BST_CHECKED) {
		VDPREG[1] |= 0x02;
	} else {
		VDPREG[1] &= ~0x02;
	}
	char buf[128];
	sprintf(buf, "%02X", VDPREG[1]);
	((CEdit*)GetDlgItem(IDC_TXTR0+1))->SetWindowTextA(buf);
	UpdateDisplay();
	((CComboBox*)GetDlgItem(IDC_LSTDEFAULTS))->SetCurSel(-1);
}

void CviewCaptureDlg::OnBnClickedChkmag()
{
	if (((CButton*)GetDlgItem(IDC_CHKMAG))->GetCheck() == BST_CHECKED) {
		VDPREG[1] |= 0x01;
	} else {
		VDPREG[1] &= ~0x01;
	}
	char buf[128];
	sprintf(buf, "%02X", VDPREG[1]);
	((CEdit*)GetDlgItem(IDC_TXTR0+1))->SetWindowTextA(buf);
	UpdateDisplay();
	((CComboBox*)GetDlgItem(IDC_LSTDEFAULTS))->SetCurSel(-1);
}

void CviewCaptureDlg::FixSpriteChecks()
{
	// update the check boxes for sprites when reg 1 is updated
	if (VDPREG[1] & 0x02) {
		((CButton*)GetDlgItem(IDC_CHK2X))->SetCheck(BST_CHECKED);
	} else {
		((CButton*)GetDlgItem(IDC_CHK2X))->SetCheck(BST_UNCHECKED);
	}
	if (VDPREG[1] & 0x01) {
		((CButton*)GetDlgItem(IDC_CHKMAG))->SetCheck(BST_CHECKED);
	} else {
		((CButton*)GetDlgItem(IDC_CHKMAG))->SetCheck(BST_UNCHECKED);
	}
	// since we're already called for reg 1, we can also check for
	// text mode and update the foreground mode drop box
	CComboBox *pCtrl = (CComboBox*)GetDlgItem(IDC_LSTTXTCOL);
	if (NULL != pCtrl) {
		pCtrl->ResetContent();
		if (VDPREG[1] & 0x10) {
			// text mode requires the foreground color
			pCtrl->AddString("Transparent");
			pCtrl->AddString("Black");
			pCtrl->AddString("Med.Green");
			pCtrl->AddString("Lt.Green");
			pCtrl->AddString("Dk.Blue");
			pCtrl->AddString("Lt.Blue");
			pCtrl->AddString("Dk.Red");
			pCtrl->AddString("Cyan");
			pCtrl->AddString("Med.Red");
			pCtrl->AddString("Lt.Red");
			pCtrl->AddString("Dk.Yellow");
			pCtrl->AddString("Lt.Yellow");
			pCtrl->AddString("Dk.Green");
			pCtrl->AddString("Magenta");
			pCtrl->AddString("Gray");
			pCtrl->AddString("White");
			pCtrl->SetCurSel(VDPREG[7]>>4);
		} else {
			// not in text mode
			pCtrl->AddString("xxxx");
			pCtrl->SetCurSel(0);
		}
	}
}

void CviewCaptureDlg::FixScreenColors()
{
	if (VDPREG[1] & 0x10) {
		// text mode
		((CComboBox*)GetDlgItem(IDC_LSTTXTCOL))->SetCurSel(VDPREG[7]>>4);
	} else {
		// not text mode
		((CComboBox*)GetDlgItem(IDC_LSTTXTCOL))->SetCurSel(0);
	}

	((CComboBox*)GetDlgItem(IDC_LSTSCRNCOL))->SetCurSel(VDPREG[7]&0x0f);
}

// Buttons - do stuff ;) (Or not in the case of OK)
void CviewCaptureDlg::OnBnClickedOk()
{
	// ensures pressing ENTER doesn't close the dialog
}

void CviewCaptureDlg::OnBnClickedCancel()
{
	// also triggered on Escape
	VDPshutdown();
	CDialogEx::OnCancel();
}

void CviewCaptureDlg::OnBnClickedBtnsave()
{
	SaveScreenshot();
}

void CviewCaptureDlg::OnBnClickedBtnload()
{
	if (LoadBuffer()) {
		// we loaded data, so set the E/A default and redraw!
		setDefaults(0);
	}
}

void CviewCaptureDlg::UpdateList(int ctrl, char *str, int which) {
	CComboBox *pCtrl = (CComboBox*)GetDlgItem(ctrl);
	if (NULL == pCtrl) return;

	pCtrl->ResetContent();
	for (unsigned int idx=0; idx<strlen(str)-3; idx+=4) {
		char buf[5];
		memcpy(buf, &str[idx], 4);
		buf[4]='\0';
		pCtrl->AddString(buf);
	}
	pCtrl->SetCurSel(which);
}

// count number of set bits in a byte
int CviewCaptureDlg::countbits(int x) {
	int cnt = 0;
	int mask = 1;

	for (int idx = 0; idx<8; idx++) {
		if (x&mask) ++cnt;
		mask <<= 1;
	}

	return cnt;
}

// based on current settings, update the drop lists
void CviewCaptureDlg::updateDropDowns() {
	((CComboBox*)GetDlgItem(IDC_LSTMODE))->SetCurSel(modeDrawn);

	// no tables have any effect in ILLEGAL modes
	if (modeDrawn == 0) {
		UpdateList(IDC_LSTSIT, "xxxx", 0);
		UpdateList(IDC_LSTCT, "xxxx", 0);
		UpdateList(IDC_LSTPDT, "xxxx", 0);
		UpdateList(IDC_LSTSAL, "xxxx", 0);
		UpdateList(IDC_LSTSDT, "xxxx", 0);
		UpdateList(IDC_LSTCTSIZE, "xxxx", 0);
		UpdateList(IDC_LSTPATSIZE, "xxxx", 0);
		return;
	}

	// bitmap modes have different settings for CT/PT (and sizes)
	if (VDPREG[0] & 0x02) {
		// this is a bitmap mode
		if (VDPREG[1]&0x10) {
			// text mode has no color table
			UpdateList(IDC_LSTCT, "xxxx", 0);
		} else {
			UpdateList(IDC_LSTCT, "00002000", CT/0x2000);
		}
		UpdateList(IDC_LSTPDT, "00002000", PDT/0x2000);
		UpdateList(IDC_LSTCTSIZE, "003f007f00ff01ff03ff07ff0fff1fff", countbits(CTsize>>6));
		UpdateList(IDC_LSTPATSIZE, "00xx08xx10xx18xx", PDTsize>>11);
	} else {
		// non-bitmap mode
		if (VDPREG[1]&0x10) {
			// text mode has no color table
			UpdateList(IDC_LSTCT, "xxxx", 0);
		} else {
			UpdateList(IDC_LSTCT, colorAdrList, CT>>6);
		}
		UpdateList(IDC_LSTPDT, "00000800100018002000280030003800", PDT>>11);
		UpdateList(IDC_LSTCTSIZE, "001F", 0);
		UpdateList(IDC_LSTPATSIZE, "07FF", 0);
	}

	UpdateList(IDC_LSTSIT, "0000040008000c001000140018001c002000240028002c003000340038003c00", SIT>>10);

	if (modeDrawn<4) {
		// no sprites
		UpdateList(IDC_LSTSAL, "xxxx", 0);
		UpdateList(IDC_LSTSDT, "xxxx", 0);
	} else {
		UpdateList(IDC_LSTSAL, spriteAdrList, SAL>>7);
		UpdateList(IDC_LSTSDT, "00000800100018002000280030003800", SDT>>11);
	}
}

// easifier options!
void CviewCaptureDlg::setDefaults(int which)
{
	if (which > DEFAULT_LIST) return;
	for (int idx=0; idx<8; idx++) {
		VDPREG[idx] = VDPDefaults[which][idx];
	}

	// change happened at the register level - need to update all the other controls
	// if we redraw first, then we can just steal all the info from the VDP variables
	UpdateDisplay();

	// direct access like this does not trigger the onChange methods
	((CComboBox*)GetDlgItem(IDC_LSTDEFAULTS))->SetCurSel(which);

	// fill in the drop downs
	updateDropDowns();

	// and now update the registers
	for (int idx=0; idx<8; idx++) {
		char buf[128];
		sprintf(buf, "%02X", VDPREG[idx]);
		// Register text boxes MUST be sequential resources in resource.h!
		((CEdit*)GetDlgItem(IDC_TXTR0+idx))->SetWindowTextA(buf);
	}
	FixSpriteChecks();
	FixScreenColors();
}

void CviewCaptureDlg::updateByReg()
{
	// change happened at the register level - need to update all the other controls
	// if we redraw first, then we can just steal all the info from the VDP variables
	UpdateDisplay();

	((CComboBox*)GetDlgItem(IDC_LSTDEFAULTS))->SetCurSel(-1);

	updateDropDowns();
}

void CviewCaptureDlg::OnCbnSelchangeLstdefaults()
{
	int which = ((CComboBox*)GetDlgItem(IDC_LSTDEFAULTS))->GetCurSel();
	setDefaults(which);
}

void CviewCaptureDlg::UpdateReg(int idx)
{
	char buf[128];
	buf[0]='\0';

	((CEdit*)GetDlgItem(IDC_TXTR0+idx))->GetWindowTextA(buf,128);
	if (isxdigit(buf[0])) {
		int n;
		if (1 == sscanf(buf, "%x", &n)) {
			VDPREG[idx] = n;
		}
	}

	// print it back to confirm entry
	sprintf(buf, "%02X", VDPREG[idx]);
	((CEdit*)GetDlgItem(IDC_TXTR0+idx))->SetWindowTextA(buf);

	if (idx == 1) {
		FixSpriteChecks();
	}
	if (idx == 7) {
		FixScreenColors();
	}

	updateByReg();
}

// despite the name, these are all now KILL FOCUS events
void CviewCaptureDlg::OnEnChangeTxtr0()
{
	UpdateReg(0);
}

void CviewCaptureDlg::OnEnChangeTxtr1()
{
	UpdateReg(1);
}

void CviewCaptureDlg::OnEnChangeTxtr2()
{
	UpdateReg(2);
}

void CviewCaptureDlg::OnEnChangeTxtr3()
{
	UpdateReg(3);
}

void CviewCaptureDlg::OnEnChangeTxtr4()
{
	UpdateReg(4);
}

void CviewCaptureDlg::OnEnChangeTxtr5()
{
	UpdateReg(5);
}

void CviewCaptureDlg::OnEnChangeTxtr6()
{
	UpdateReg(6);
}

void CviewCaptureDlg::OnEnChangeTxtr7()
{
	UpdateReg(7);
}

void CviewCaptureDlg::OnCbnSelchangeLstmode()
{
	// zero out the relevant bits
	VDPREG[0] &= ~0x02;		// bitmap modifier
	VDPREG[0] &= ~0x04;		// 80-column text modifier
	VDPREG[1] &= ~0x10;		// text
	VDPREG[1] &= ~0x08;		// multicolor

	// update the registers as appropriate then redraw
	int mode = ((CComboBox*)GetDlgItem(IDC_LSTMODE))->GetCurSel();
	switch (mode) {
	case 0:		// illegal - we just set a dummy mode for that
		VDPREG[1]|=0x18;
		break;

	case 1:		// VDP Text 2 (bitmap text)
		VDPREG[1] |= 0x10;
		VDPREG[0] |= 0x02;
		break;

	case 2:		// 80 column text
		VDPREG[1] |= 0x10;
		VDPREG[0] |= 0x04;
		break;

	case 3:		// 40 column text
		VDPREG[1] |= 0x10;
		break;

	case 4:		// multicolor 2 (bitmap multicolor)
		VDPREG[1] |= 0x08;
		VDPREG[0] |= 0x02;
		break;

	case 5:		// multicolor
		VDPREG[1] |= 0x08;
		break;

	case 6:		// bitmap
		VDPREG[0] |= 0x02;
		break;

	case 7:		// multicolor (all off)
		break;

	default:
		// not a new mode, do nothing
		return;
	}

	// update UI for Regs 0 and 1
	for (int idx=0; idx<2; idx++) {
		char buf[32];
		sprintf(buf, "%02X", VDPREG[idx]);
		((CEdit*)GetDlgItem(IDC_TXTR0+idx))->SetWindowTextA(buf);
	}
	FixSpriteChecks();

	updateByReg();
}

// these ones are simpler - just convert the number back into a register
// and redraw
void CviewCaptureDlg::updateDropdown(int ctrl, int reg) {
	char buf[128];
	buf[0]='\0';

	((CComboBox*)GetDlgItem(ctrl))->GetWindowTextA(buf,128);
	if (isxdigit(buf[0])) {
		VDPREG[reg] = ((CComboBox*)GetDlgItem(ctrl))->GetCurSel();

		sprintf(buf, "%02X", VDPREG[reg]);
		((CEdit*)GetDlgItem(IDC_TXTR0+reg))->SetWindowTextA(buf);

		if (reg == 1) {
			// should never be needed, though...
			FixSpriteChecks();
		}
		if (reg == 7) {
			FixScreenColors();
		}
		UpdateDisplay();
		((CComboBox*)GetDlgItem(IDC_LSTDEFAULTS))->SetCurSel(-1);
	}
}

void CviewCaptureDlg::OnCbnSelchangeLstsit()
{
	updateDropdown(IDC_LSTSIT, 2);
}

void CviewCaptureDlg::OnCbnSelchangeLstct()
{
	if (VDPREG[0] & 0x02) {
		// bitmap
		// it's 0000 or 2000
		char buf[128];
		buf[0]='\0';

		((CComboBox*)GetDlgItem(IDC_LSTCT))->GetWindowTextA(buf,128);
		if (isxdigit(buf[0])) {
			if (buf[0] == '0') {
				VDPREG[3] &= ~0x80;
			} else {
				VDPREG[3] |= 0x80;
			}
			sprintf(buf, "%02X", VDPREG[3]);
			((CEdit*)GetDlgItem(IDC_TXTR0+3))->SetWindowTextA(buf);
			UpdateDisplay();
			((CComboBox*)GetDlgItem(IDC_LSTDEFAULTS))->SetCurSel(-1);
		}
	} else {
		updateDropdown(IDC_LSTCT, 3);
	}
}


void CviewCaptureDlg::OnCbnSelchangeLstpdt()
{
	if (VDPREG[0] & 0x02) {
		// bitmap
		// it's 0000 or 2000
		char buf[128];
		buf[0]='\0';

		((CComboBox*)GetDlgItem(IDC_LSTPDT))->GetWindowTextA(buf,128);
		if (isxdigit(buf[0])) {
			if (buf[0] == '0') {
				VDPREG[4] &= ~0x04;
			} else {
				VDPREG[4] |= 0x04;
			}
			sprintf(buf, "%02X", VDPREG[4]);
			((CEdit*)GetDlgItem(IDC_TXTR0+4))->SetWindowTextA(buf);
			UpdateDisplay();
			((CComboBox*)GetDlgItem(IDC_LSTDEFAULTS))->SetCurSel(-1);
		}
	} else {
		updateDropdown(IDC_LSTPDT, 4);
	}
}


void CviewCaptureDlg::OnCbnSelchangeLstsal()
{
	updateDropdown(IDC_LSTSAL, 5);
}

void CviewCaptureDlg::OnCbnSelchangeLstsdt()
{
	updateDropdown(IDC_LSTSDT, 6);
}

// the masks are a bit of a pain in the ass in any event
// but, they are only meaningful in bitmap mode
void CviewCaptureDlg::OnCbnSelchangeLstctsize()
{
	// if not in bitmap mode, return
	if ((VDPREG[0] & 0x02) == 0) {
		return;
	}
	// color table mask can't be changed in text mode
	if ((VDPREG[1] & 0x10) == 0x10) {
		return;
	}

	// all right, update the mask
	char buf[128];
	buf[0]='\0';

	int n = ((CComboBox*)GetDlgItem(IDC_LSTCTSIZE))->GetCurSel();
	// This one is a bitshifted mask, rather than an index, so fix it up
	if (n>0) {
		int t=n-1;
		n=1;
		while (t-->0) {
			n|=n<<1;
		}
	}
	VDPREG[3] &= ~0x7f;
	VDPREG[3] |= n&0x7f;

	sprintf(buf, "%02X", VDPREG[3]);
	((CEdit*)GetDlgItem(IDC_TXTR0+3))->SetWindowTextA(buf);

	UpdateDisplay();
	((CComboBox*)GetDlgItem(IDC_LSTDEFAULTS))->SetCurSel(-1);
}

void CviewCaptureDlg::OnCbnSelchangeLstpatsize()
{
	// if not in bitmap mode, return
	if ((VDPREG[0] & 0x02) == 0) {
		return;
	}

	// all right, update the mask
	char buf[128];
	buf[0]='\0';

	int n = ((CComboBox*)GetDlgItem(IDC_LSTPATSIZE))->GetCurSel();
	VDPREG[4] &= ~0x03;
	VDPREG[4] |= n&0x03;

	sprintf(buf, "%02X", VDPREG[4]);
	((CEdit*)GetDlgItem(IDC_TXTR0+4))->SetWindowTextA(buf);

	UpdateDisplay();
	((CComboBox*)GetDlgItem(IDC_LSTDEFAULTS))->SetCurSel(-1);
}

void CviewCaptureDlg::OnCbnSelchangeLsttxtcol()
{
	if ((VDPREG[1] & 0x10) == 0) {
		// not in text mode
		return;
	}

	// update register 7
	VDPREG[7] &= ~0xf0;
	VDPREG[7] |= (((CComboBox*)GetDlgItem(IDC_LSTTXTCOL))->GetCurSel())<<4;

	char buf[128];
	sprintf(buf, "%02X", VDPREG[7]);
	((CEdit*)GetDlgItem(IDC_TXTR0+7))->SetWindowTextA(buf);

	UpdateDisplay();
	((CComboBox*)GetDlgItem(IDC_LSTDEFAULTS))->SetCurSel(-1);
}


void CviewCaptureDlg::OnCbnSelchangeLstscrncol()
{
	// update register 7
	VDPREG[7] &= ~0x0f;
	VDPREG[7] |= (((CComboBox*)GetDlgItem(IDC_LSTSCRNCOL))->GetCurSel());

	char buf[128];
	sprintf(buf, "%02X", VDPREG[7]);
	((CEdit*)GetDlgItem(IDC_TXTR0+7))->SetWindowTextA(buf);

	UpdateDisplay();
	((CComboBox*)GetDlgItem(IDC_LSTDEFAULTS))->SetCurSel(-1);
}
