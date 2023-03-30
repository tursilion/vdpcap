//
// (C) 2015 Mike Brent aka Tursi aka HarmlessLion.com
// This software is provided AS-IS. No warranty
// express or implied is provided.
//
// This notice defines the entire license for this code.
// All rights not explicity granted here are reserved by the
// author.
//
// You may redistribute this software provided the original
// archive is UNCHANGED and a link back to my web page,
// http://harmlesslion.com, is provided as the author's site.
// It is acceptable to link directly to a subpage at harmlesslion.com
// provided that page offers a URL for that purpose
//
// Source code, if available, is provided for educational purposes
// only. You are welcome to read it, learn from it, mock
// it, and hack it up - for your own use only.
//
// Please contact me before distributing derived works or
// ports so that we may work out terms. I don't mind people
// using my code but it's been outright stolen before. In all
// cases the code must maintain credit to the original author(s).
//
// -COMMERCIAL USE- Contact me first. I didn't make
// any money off it - why should you? ;) If you just learned
// something from this, then go ahead. If you just pinched
// a routine or two, let me know, I'll probably just ask
// for credit. If you want to derive a commercial tool
// or use large portions, we need to talk. ;)
//
// If this, itself, is a derived work from someone else's code,
// then their original copyrights and licenses are left intact
// and in full force.
//
// http://harmlesslion.com - visit the web page for contact info
//
///////////////////////////////////////////////////////
// Classic99 VDP Routines (stripped down)
// M.Brent
///////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500

#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <atlstr.h>
#include "tivdp.h"

// 32-bit 0RGB colors
unsigned int TIPALETTE[16] = {
	0x00000000,0x00000000,0x0020C840,0x0058D878,0x005050E8,0x007870F8,0x00D05048,
	0x0040E8F0,0x00F85050,0x00F87878,0x00D0C050,0x00E0C880,0x0020B038,0x00C858B8,
	0x00C8C8C8,0x00F8F8F8
};

int SIT;									// Screen Image Table
int CT;										// Color Table
int PDT;									// Pattern Descriptor Table
int SAL;									// Sprite Allocation Table
int SDT;									// Sprite Descriptor Table
int CTsize;									// Color Table size in Bitmap Mode
int PDTsize;								// Pattern Descriptor Table size in Bitmap Mode
int VDPREG[8];								// VDP read-only registers (negative means hard-coded offset)
int modeDrawn;

Byte VDP[16*1024+1024];						// Video RAM (16k), + padding for load and default SIT

int bEnable80Columns=1;						// Enable the beginnings of the 80 column mode - to replace someday with F18A

unsigned int *framedata;					// The actual pixel data
BITMAPINFO myInfo;							// Bitmapinfo header for the DIB functions
BITMAPINFO myInfo80Col;						// Bitmapinfo header for the DIB functions
HWND myWnd;

int bUse5SpriteLimit=1;						// whether the sprite flicker is on
bool	bDisableBlank=false, 
		bDisableSprite=false, 
		bDisableBackground=false;			// other layers :)

// temp hack for no F18A
const int F18ASpritePaletteSize = 0;
const int F18AECModeSprite = 0;
const int F18APalette[1] =  {0};

// prototypes
void gettables(int reg0);
void VDPdisplay();
void VDPgraphics();
void VDPgraphicsII();
void VDPtext();
void VDPtextII();
void VDPtext80();
void VDPillegal();
void VDPmulticolor();
void VDPmulticolorII(); 
void doBlit();
void DrawSprites();
void pixel(int x, int y, int c);
void pixel80(int x, int y, int c);
void spritepixel(int x, int y, int c);
void bigpixel(int x, int y, int c);
int pixelMask(int addr, int F18ASpriteColorLine[]);

//////////////////////////////////////////////////////////
// Get table addresses from Registers
// We pass in VDP reg 0 so the bitmap filtering can be external
//////////////////////////////////////////////////////////
void gettables(int reg0)
{
	/* Screen Image Table */
	if (VDPREG[2] < 0) {
		SIT = -VDPREG[2];
	} else {
		if ((bEnable80Columns) && (reg0 & 0x04)) {
			// in 80-column text mode, the two LSB are some kind of mask that we here ignore - the rest of the register is larger
			// The 9938 requires that those bits be set to 11, therefore, the F18A treats 11 and 00 both as 00, but treats
			// 01 and 10 as their actual values. (Okay, that is a bit weird.) That said, the F18A still only honours the least
			// significant 4 bits and ignores the rest (the 9938 reads 7 bits instead of 4, masking as above).
			// So anyway, the goal is F18A support, but the 9938 mask would be 0x7C instead of 0x0C, and the shift was only 8?
			// TODO: check the 9938 datasheet - did Matthew get it THAT wrong? Or does the math work out anyway?
			// Anyway, this works for table at >0000, which is most of them.
			SIT=(VDPREG[2]&0x0F);
			if ((SIT&0x03)==0x03) SIT&=0x0C;	// mask off a 0x03 pattern, 0x00,0x01,0x02 left alone
			SIT<<=10;
		} else {
			SIT=((VDPREG[2]&0x0f)<<10);
		}
	}

	/* Sprite Attribute List */
	if (VDPREG[5] < 0) {
		SAL = -VDPREG[5];
	} else {
		SAL=((VDPREG[5]&0x7f)<<7);
	}

	/* Sprite Descriptor Table */
	if (VDPREG[6] < 0) {
		SDT = -VDPREG[6];
	} else {
		SDT=((VDPREG[6]&0x07)<<11);
	}

	// The normal math for table addresses isn't quite right in bitmap mode
	// The PDT and CT have different math and a size setting
	if ((VDPREG[3] > 0) && (VDPREG[4] >= 0)) {
		if (reg0&0x02) {
			// this is for bitmap modes
			CT=(VDPREG[3]&0x80) ? 0x2000 : 0;
			PDT=(VDPREG[4]&0x04) ? 0x2000 : 0;
			CTsize=((VDPREG[3]&0x7f)<<6)|0x3f;
			PDTsize=((VDPREG[4]&0x03)<<11);
			if (VDPREG[1]&0x10) {			// in Bitmap text, we fill bits with 1, as there is no color table
				PDTsize|=0x7ff;
			} else {
				PDTsize|=(CTsize&0x7ff);	// In other bitmap modes we get bits from the color table mask
			}
		} else {
			// this is for non-bitmap modes
			/* Colour Table */
			CT=VDPREG[3]<<6;
			/* Pattern Descriptor Table */
			PDT=((VDPREG[4]&0x07)<<11);
			CTsize=32;
			PDTsize=2048;
		}
	} else {
		// don't touch sizes
		if (VDPREG[3] < 0) {
			CT = -VDPREG[3];
		}
		if (VDPREG[4] < 0) {
			PDT = -VDPREG[4];
		}
	}
}

////////////////////////////////////////////////////////////
// Startup VDP graphics interface
////////////////////////////////////////////////////////////
void VDPinit(HWND inWnd)
{	
	myWnd = inWnd;
	framedata=(unsigned int*)malloc((512+16)*(192+16)*4);	// This is where we draw everything - 8 pixel border - extra room left for 80 column mode

	myInfo.bmiHeader.biSize=sizeof(myInfo.bmiHeader);
	myInfo.bmiHeader.biWidth=256+16;
	myInfo.bmiHeader.biHeight=192+16;
	myInfo.bmiHeader.biPlanes=1;
	myInfo.bmiHeader.biBitCount=32;
	myInfo.bmiHeader.biCompression=BI_RGB;
	myInfo.bmiHeader.biSizeImage=0;
	myInfo.bmiHeader.biXPelsPerMeter=1;
	myInfo.bmiHeader.biYPelsPerMeter=1;
	myInfo.bmiHeader.biClrUsed=0;
	myInfo.bmiHeader.biClrImportant=0;

	memcpy(&myInfo80Col, &myInfo, sizeof(myInfo));
	myInfo80Col.bmiHeader.biWidth=512+16;

	// layers all enabled at start
	bDisableBlank=false;
	bDisableSprite=false;
	bDisableBackground=false;
}

void UpdateDisplay() {
	VDPdisplay();
	doBlit();
}

void VDPshutdown() {
	free(framedata);
}

//////////////////////////////////////////////////////////
// Perform drawing of a single frame
// Determines which screen mode to draw
//////////////////////////////////////////////////////////
void VDPdisplay()
{
	int idx;
	DWORD longcol;
	DWORD *plong;
	int reg0 = VDPREG[0];
	int nMax;
	bool showDisplay = true;

	if (!bEnable80Columns) {
		// disable 80 columns if not enabled
		reg0&=~0x04;
	}
	gettables(reg0);

	// blank screen
	plong=(DWORD*)framedata;
	longcol=TIPALETTE[VDPREG[7]&0xf];
	if ((reg0&0x04)&&(VDPREG[1]&0x10)) {
		// 80 column text
		nMax = ((512+16)*(192+16))/4;
	} else {
		// all other modes
		nMax = ((256+16)*(192+16))/4;
	}
	for (idx=0; idx<nMax; idx++) {
		*(plong++)=longcol;
		*(plong++)=longcol;
		*(plong++)=longcol;
		*(plong++)=longcol;
	}

	// check for blank display separately, so we can get a mode back to the GUI
	if (!bDisableBlank) {
		if (!(VDPREG[1] & 0x40)) {	// Disable display
			showDisplay = false;
		}
	}
	if (bDisableBackground) {
		showDisplay = false;
	}

	if ((VDPREG[1] & 0x18)==0x18)	// MODE BITS 2 and 1
	{
		modeDrawn = 0;
		if (showDisplay) VDPillegal();
		return;
	}

	if (VDPREG[1] & 0x10)		// MODE BIT 2
	{
		if (reg0 & 0x02) {		// BITMAP MODE BIT
			modeDrawn = 1;
			if (showDisplay) VDPtextII();		// undocumented bitmap text mode
		} else if (reg0 & 0x04) {	// MODE BIT 4 (9938)
			modeDrawn = 2;
			if (showDisplay) VDPtext80();		// 80-column text, similar to 9938/F18A
		} else {
			modeDrawn = 3;
			if (showDisplay) VDPtext();			// regular 40-column text
		}
		return;
	}

	if (VDPREG[1] & 0x08)		// MODE BIT 1
	{
		if (reg0 & 0x02) {		// BITMAP MODE BIT
			modeDrawn = 4;
			if (showDisplay) VDPmulticolorII();	// undocumented bitmap multicolor mode
		} else {
			modeDrawn = 5;
			if (showDisplay) VDPmulticolor();
		}
		return;
	}

	if (reg0 & 0x02) {			// BITMAP MODE BIT
		modeDrawn = 6;
		if (showDisplay) VDPgraphicsII();		// documented bitmap graphics mode
	} else {
		modeDrawn = 7;
		if (showDisplay) VDPgraphics();
	}

	if (!showDisplay) {
		// as long as mode bit 2 is not set, sprites are okay
		if ((VDPREG[1] & 0x10) == 0) {
			DrawSprites();
		}
	}
}

/////////////////////////////////////////////////////////
// Draw graphics mode
/////////////////////////////////////////////////////////
void VDPgraphics()
{
	int t,o;						// temp variables
	int i1,i2,i3;					// temp variables
	int p_add;
	int fgc, bgc, c;
	unsigned char ch=0xff;

	o=0;							// offset in SIT

	for (i1=0; i1<192; i1+=8)		// y loop
	{ 
		for (i2=0; i2<256; i2+=8)	// x loop
		{ 
			ch=VDP[SIT+o];
			p_add=PDT+(ch<<3);
			c = ch>>3;
			fgc=VDP[CT+c];
			bgc=fgc&0x0f;
			fgc>>=4;
			o++;

			for (i3=0; i3<8; i3++)
			{	
				t=VDP[p_add++];
	
				pixel(i2,i1+i3,(t&0x80 ? fgc : bgc ));
				pixel(i2+1,i1+i3,(t&0x40 ? fgc : bgc ));
				pixel(i2+2,i1+i3,(t&0x20 ? fgc : bgc ));
				pixel(i2+3,i1+i3,(t&0x10 ? fgc : bgc ));
				pixel(i2+4,i1+i3,(t&0x08 ? fgc : bgc ));
				pixel(i2+5,i1+i3,(t&0x04 ? fgc : bgc ));
				pixel(i2+6,i1+i3,(t&0x02 ? fgc : bgc ));
				pixel(i2+7,i1+i3,(t&0x01 ? fgc : bgc ));
			}
		}
	}

	DrawSprites();

}

/////////////////////////////////////////////////////////
// Draw bitmap graphics mode
/////////////////////////////////////////////////////////
void VDPgraphicsII()
{
	int t,o;						// temp variables
	int i1,i2,i3;					// temp variables
	int p_add, c_add;
	int fgc, bgc;
	int table, Poffset, Coffset;
	unsigned char ch=0xff;

	o=0;							// offset in SIT
	table=0; Poffset=0; Coffset=0;

	for (i1=0; i1<192; i1+=8)		// y loop
	{ 
		if ((i1==64)||(i1==128)) {
			table++;
			Poffset=table*0x800;
			Coffset=table*0x800;
		}

		for (i2=0; i2<256; i2+=8)	// x loop
		{ 
			ch=VDP[SIT+o];
			p_add=PDT+(((ch<<3)+Poffset)&PDTsize);
			c_add=CT+(((ch<<3)+Coffset)&CTsize);
			o++;

			for (i3=0; i3<8; i3++)
			{	
				t=VDP[p_add++];
				fgc=VDP[c_add++];
				bgc=fgc&0x0f;
				fgc>>=4;
				{
					pixel(i2,i1+i3,(t&0x80 ? fgc : bgc ));
					pixel(i2+1,i1+i3,(t&0x40 ? fgc : bgc ));
					pixel(i2+2,i1+i3,(t&0x20 ? fgc : bgc ));
					pixel(i2+3,i1+i3,(t&0x10 ? fgc : bgc ));
					pixel(i2+4,i1+i3,(t&0x08 ? fgc : bgc ));
					pixel(i2+5,i1+i3,(t&0x04 ? fgc : bgc ));
					pixel(i2+6,i1+i3,(t&0x02 ? fgc : bgc ));
					pixel(i2+7,i1+i3,(t&0x01 ? fgc : bgc ));
				}
			}
		}
	}

	DrawSprites();

}

////////////////////////////////////////////////////////////////////////
// Draw text mode 40x24
////////////////////////////////////////////////////////////////////////
void VDPtext()
{ 
	int t,o;
	int i1,i2,i3;
	int c1, c2, p_add;
	unsigned char ch=0xff;

	t=VDPREG[7];
	c1=t&0xf;
	c2=t>>4;

	// erase border area
	for (i1=0; i1<8; i1++) {
		for (i2=0; i2<192; i2++) {
			pixel(i1, i2, c1);
			pixel(i1+248, i2, c1);
		}
	}

	o=0;										// offset in SIT
	for (i1=0; i1<192; i1+=8)					// y loop
	{ 
		for (i2=8; i2<248; i2+=6)				// x loop
		{ 
			ch=VDP[SIT+o];
			p_add=PDT+(ch<<3);
			o++;

			for (i3=0; i3<8; i3++)		// 6 pixels wide
			{	
				t=VDP[p_add++];
				pixel(i2,i1+i3,(t&0x80 ? c2 : c1 ));
				pixel(i2+1,i1+i3,(t&0x40 ? c2 : c1 ));
				pixel(i2+2,i1+i3,(t&0x20 ? c2 : c1 ));
				pixel(i2+3,i1+i3,(t&0x10 ? c2 : c1 ));
				pixel(i2+4,i1+i3,(t&0x08 ? c2 : c1 ));
				pixel(i2+5,i1+i3,(t&0x04 ? c2 : c1 ));
			}
		}
	}
	// no sprites in text mode
}

////////////////////////////////////////////////////////////////////////
// Draw bitmap text mode 40x24
////////////////////////////////////////////////////////////////////////
void VDPtextII()
{ 
	int t,o;
	int i1,i2,i3;
	int c1, c2, p_add;
	int table, Poffset;
	unsigned char ch=0xff;

	t=VDPREG[7];
	c1=t&0xf;
	c2=t>>4;

	// erase border area
	for (i1=0; i1<8; i1++) {
		for (i2=0; i2<192; i2++) {
			pixel(i1, i2, c1);
			pixel(i1+248, i2, c1);
		}
	}

	o=0;							// offset in SIT
	table=0; Poffset=0;

	for (i1=0; i1<192; i1+=8)					// y loop
	{ 
		if ((i1==64)||(i1==128)) {
			table++;
			Poffset=table*0x800;
		}

		for (i2=8; i2<248; i2+=6)				// x loop
		{ 
			ch=VDP[SIT+o];
			p_add=PDT+(((ch<<3)+Poffset)&PDTsize);
			o++;

			for (i3=0; i3<8; i3++)		// 6 pixels wide
			{	
				t=VDP[p_add++];
				pixel(i2,i1+i3,(t&0x80 ? c2 : c1 ));
				pixel(i2+1,i1+i3,(t&0x40 ? c2 : c1 ));
				pixel(i2+2,i1+i3,(t&0x20 ? c2 : c1 ));
				pixel(i2+3,i1+i3,(t&0x10 ? c2 : c1 ));
				pixel(i2+4,i1+i3,(t&0x08 ? c2 : c1 ));
				pixel(i2+5,i1+i3,(t&0x04 ? c2 : c1 ));
			}
		}
	}
	// no sprites in text mode
}

////////////////////////////////////////////////////////////////////////
// Draw text mode 80x24 (note: 80x26.5 mode not supported, blink not supported)
////////////////////////////////////////////////////////////////////////
void VDPtext80()
{ 
	int t,o;
	int i1,i2,i3;
	int c1, c2, p_add;
	unsigned char ch=0xff;

	t=VDPREG[7];
	c1=t&0xf;
	c2=t>>4;

	// erase border area
	for (i1=0; i1<8; i1++) {
		for (i2=0; i2<192; i2++) {
			pixel80(i1, i2, c1);
			pixel80(i1+488, i2, c1);
		}
	}

	o=0;										// offset in SIT
	for (i1=0; i1<192; i1+=8)					// y loop
	{ 
		for (i2=8; i2<488; i2+=6)				// x loop
		{ 
			ch=VDP[SIT+o];
			p_add=PDT+(ch<<3);
			o++;

			for (i3=0; i3<8; i3++)		// 6 pixels wide
			{	
				t=VDP[p_add++];
				pixel80(i2,i1+i3,(t&0x80 ? c2 : c1 ));
				pixel80(i2+1,i1+i3,(t&0x40 ? c2 : c1 ));
				pixel80(i2+2,i1+i3,(t&0x20 ? c2 : c1 ));
				pixel80(i2+3,i1+i3,(t&0x10 ? c2 : c1 ));
				pixel80(i2+4,i1+i3,(t&0x08 ? c2 : c1 ));
				pixel80(i2+5,i1+i3,(t&0x04 ? c2 : c1 ));
			}
		}
	}
	// no sprites in text mode
	// TODO: except on the F18A
}

////////////////////////////////////////////////////////////////////////
// Draw Illegal mode (similar to text mode)
////////////////////////////////////////////////////////////////////////
void VDPillegal()
{ 
	int t;
	int i1,i2,i3;
	int c1, c2;

	t=VDPREG[7];
	c1=t&0xf;
	c2=t>>4;

	// erase border area
	for (i1=0; i1<8; i1++) {
		for (i2=0; i2<192; i2++) {
			pixel(i1, i2, c1);
			pixel(i1+248, i2, c1);
		}
	}

	// Each character is made up of rows of 4 pixels foreground, 2 pixels background

	for (i1=0; i1<192; i1+=8)					// y loop
	{ 
		for (i2=8; i2<248; i2+=6)				// x loop
		{ 
			for (i3=0; i3<8; i3++)				// 6 pixels wide
			{	
				pixel(i2,i1+i3,c2);
				pixel(i2+1,i1+i3,c2);
				pixel(i2+2,i1+i3,c2);
				pixel(i2+3,i1+i3,c2);
				pixel(i2+4,i1+i3,c1);
				pixel(i2+5,i1+i3,c1);
			}
		}
	}
	// no sprites in this mode
}

/////////////////////////////////////////////////////
// Draw Multicolor Mode
/////////////////////////////////////////////////////
void VDPmulticolor() 
{
	int o;								// temp variables
	int i1,i2,i3, i4;					// temp variables
	int p_add;
	int fgc, bgc;
	int off;
	unsigned char ch=0xff;

	o=0;							// offset in SIT
	off=0;							// offset in pattern

	for (i1=0; i1<192; i1+=8)									// y loop
	{ 
		for (i2=0; i2<256; i2+=8)								// x loop
		{ 
			ch=VDP[SIT+o];
			p_add=PDT+(ch<<3)+off;
			o++;

			for (i3=0; i3<7; i3+=4)
			{	
				fgc=VDP[p_add++];
				bgc=fgc&0x0f;
				fgc>>=4;
	
				for (i4=0; i4<4; i4++) {
					pixel(i2,i1+i3+i4,fgc);
					pixel(i2+1,i1+i3+i4,fgc);
					pixel(i2+2,i1+i3+i4,fgc);
					pixel(i2+3,i1+i3+i4,fgc);
					pixel(i2+4,i1+i3+i4,bgc);
					pixel(i2+5,i1+i3+i4,bgc);
					pixel(i2+6,i1+i3+i4,bgc);
					pixel(i2+7,i1+i3+i4,bgc);
				}
			}
		}
		off+=2;
		if (off>7) off=0;
	}

	DrawSprites();

	return;
}

/////////////////////////////////////////////////////
// Draw Bitmap Multicolor Mode
/////////////////////////////////////////////////////
void VDPmulticolorII() 
{
	int o;								// temp variables
	int i1,i2,i3, i4;					// temp variables
	int p_add;
	int fgc, bgc;
	int off;
	int table, Poffset;
	unsigned char ch=0xff;

	o=0;							// offset in SIT
	off=0;							// offset in pattern
	table=0; Poffset=0;

	for (i1=0; i1<192; i1+=8)									// y loop
	{ 
		if ((i1==64)||(i1==128)) {
			table++;
			Poffset=table*0x800;
		}

		for (i2=0; i2<256; i2+=8)								// x loop
		{ 
			ch=VDP[SIT+o];
			p_add=PDT+(((ch<<3)+Poffset)&PDTsize);
			o++;

			for (i3=0; i3<7; i3+=4)
			{	
				fgc=VDP[p_add++];
				bgc=fgc&0x0f;
				fgc>>=4;
	
				for (i4=0; i4<4; i4++) {
					pixel(i2,i1+i3+i4,fgc);
					pixel(i2+1,i1+i3+i4,fgc);
					pixel(i2+2,i1+i3+i4,fgc);
					pixel(i2+3,i1+i3+i4,fgc);
					pixel(i2+4,i1+i3+i4,bgc);
					pixel(i2+5,i1+i3+i4,bgc);
					pixel(i2+6,i1+i3+i4,bgc);
					pixel(i2+7,i1+i3+i4,bgc);
				}
			}
		}
		off+=2;
		if (off>7) off=0;
	}

	DrawSprites();

	return;
}

////////////////////////////////////////////////////////////////
// Stretch-blit the buffer into the active window
//
// NOTES: Graphics modes we have (and some we need)
// 272x208 -- the standard default pixel mode of the 9918A plus a fixed (incorrect) border
// 528x208 -- 80-column mode
// These are all with 24 rows -- the F18A adds a 26.5 row mode (212 pixels) (todo: or was it 240?)
////////////////////////////////////////////////////////////////
void doBlit()
{
	RECT rect1;
	HDC myDC;

	GetClientRect(myWnd, &rect1);
	rect1.bottom = rect1.top + 384+32;
	rect1.right = rect1.left + 512+32;
	myDC=GetDC(myWnd);
	SetStretchBltMode(myDC, COLORONCOLOR);

	// TODO: hacky city - 80-column mode doesn't filter or anything, cause we'd have to change ALL the stuff below.
	if ((bEnable80Columns)&&(VDPREG[0]&0x04)&&(VDPREG[1]&0x10)) {
		StretchDIBits(myDC, 0, 0, rect1.right-rect1.left, rect1.bottom-rect1.top, 0, 0, 512+16, 192+16, framedata, &myInfo80Col, 0, SRCCOPY);
	} else {
		StretchDIBits(myDC, 0, 0, rect1.right-rect1.left, rect1.bottom-rect1.top, 0, 0, 256+16, 192+16, framedata, &myInfo, 0, SRCCOPY);
	}

	ReleaseDC(myWnd, myDC);
}

//////////////////////////////////////////////////////////
// Draw Sprites into the backbuffer
//////////////////////////////////////////////////////////
void DrawSprites()
{
	int i1, i2, i3, xx, yy, pat, col, p_add, t, sc;
	int highest;
	int curSAL;

	// a hacky, but effective 4-sprite-per-line limitation emulation
	// We can do this right when we have scanline based VDP
	char nLines[192];
	char bSkipScanLine[32][32];		// 32 sprites, 32 lines max
	int b5OnLine=0;					// first sprite >4 on scanline

	if (bDisableSprite) {
		return;
	}

	memset(nLines, 0, sizeof(nLines));
	memset(bSkipScanLine, 0, sizeof(bSkipScanLine));

	// set up the draw
	highest=31;

	// find the highest active sprite
	for (i1=0; i1<32; i1++)			// 32 sprites 
	{
		yy=VDP[SAL+(i1<<2)];
		if (yy==0xd0)
		{
			highest=i1-1;
			break;
		}
	}
	
	if (bUse5SpriteLimit) {
		// go through the sprite table and check if any scanlines are obliterated by 4-per-line
		i3=8;							// number of sprite scanlines
		if (VDPREG[1] & 0x2) {			 // TODO: Handle F18A ECM where sprites are doubled individually
			// double-sized
			i3*=2;
		}
		if (VDPREG[1]&0x01)	{
			// magnified sprites
			i3*=2;
		}
		for (i1=0; i1<=highest; i1++) {
			curSAL=SAL+(i1<<2);
			yy=VDP[curSAL]+1;				// sprite Y, it's stupid, cause 255 is line 0 
			if (yy>225) yy-=256;			// fade in from top
			t=yy;
			for (i2=0; i2<i3; i2++,t++) {
				if ((t>=0) && (t<=191)) {
					nLines[t]++;
					if (nLines[t]>4) {
						b5OnLine=i1;
						bSkipScanLine[i1][i2]=1;
					}
				}
			}
		}
	}

	// now draw
	for (i1=highest; i1>=0; i1--)	
	{	
		curSAL=SAL+(i1<<2);
		yy=VDP[curSAL++]+1;				// sprite Y, it's stupid, cause 255 is line 0 
		if (yy>225) yy-=256;			// fade in from top: TODO: is this right??
		xx=VDP[curSAL++];				// sprite X 
		pat=VDP[curSAL++];				// sprite pattern
		int dblSize = F18AECModeSprite ? VDP[curSAL] & 0x10 : VDPREG[1] & 0x2;
		if (dblSize) {
			pat=pat&0xfc;				// if double-sized, it must be a multiple of 4
		}
		col=VDP[curSAL]&0xf;			// sprite color 
	
		if (VDP[curSAL++]&0x80)	{		// early clock
			xx-=32;
		}

		// Even transparent sprites get drawn into the collision buffer
		p_add=SDT+(pat<<3);
		sc=0;						// current scanline
		
		// Added by Rasmus M
		// TODO: For ECM 1 we need one more bit from R24
//		int paletteBase = F18AECModeSprite ? (col >> (F18AECModeSprite - 2)) * F18ASpritePaletteSize : 0;
		int paletteBase = 0;
		int F18ASpriteColorLine[8]; // Colors indices for each of the 8 pixels in a sprite scan line

		if (VDPREG[1]&0x01)	{		// magnified sprites
			for (i3=0; i3<16; i3+=2)
			{	
				t = pixelMask(p_add++, F18ASpriteColorLine);	// Modified by RasmusM. Sets up the F18ASpriteColorLine[] array.

				if (!bSkipScanLine[i1][sc]) {
					if (t&0x80) 
						bigpixel(xx, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
					if (t&0x40)
						bigpixel(xx+2, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
					if (t&0x20)
						bigpixel(xx+4, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
					if (t&0x10)
						bigpixel(xx+6, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
					if (t&0x08)
						bigpixel(xx+8, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
					if (t&0x04)
						bigpixel(xx+10, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
					if (t&0x02)
						bigpixel(xx+12, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
					if (t&0x01)
						bigpixel(xx+14, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);
				}

				if (dblSize)		// double-size sprites, need to draw 3 more chars 
				{	
					t = pixelMask(p_add + 7, F18ASpriteColorLine);	// Modified by RasmusM
	
					if (!bSkipScanLine[i1][sc+16]) {
						if (t&0x80)
							bigpixel(xx, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
						if (t&0x40)
							bigpixel(xx+2, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
						if (t&0x20)
							bigpixel(xx+4, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
						if (t&0x10)
							bigpixel(xx+6, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
						if (t&0x08)
							bigpixel(xx+8, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
						if (t&0x04)
							bigpixel(xx+10, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
						if (t&0x02)
							bigpixel(xx+12, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
						if (t&0x01)
							bigpixel(xx+14, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);

						t = pixelMask(p_add + 23, F18ASpriteColorLine);	// Modified by RasmusM
						if (t&0x80)
							bigpixel(xx+16, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
						if (t&0x40)
							bigpixel(xx+18, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
						if (t&0x20)
							bigpixel(xx+20, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
						if (t&0x10)
							bigpixel(xx+22, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
						if (t&0x08)
							bigpixel(xx+24, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
						if (t&0x04)
							bigpixel(xx+26, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
						if (t&0x02)
							bigpixel(xx+28, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
						if (t&0x01)
							bigpixel(xx+30, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);
					}

					if (!bSkipScanLine[i1][sc]) {
						t = pixelMask(p_add + 15, F18ASpriteColorLine);	// Modified by RasmusM
						if (t&0x80)
							bigpixel(xx+16, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
						if (t&0x40)
							bigpixel(xx+18, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
						if (t&0x20)
							bigpixel(xx+20, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
						if (t&0x10)	
							bigpixel(xx+22, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
						if (t&0x08)
							bigpixel(xx+24, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
						if (t&0x04)
							bigpixel(xx+26, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
						if (t&0x02)
							bigpixel(xx+28, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
						if (t&0x01)
							bigpixel(xx+30, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);
					}
				}
				sc+=2;
			}
		} else {
			for (i3=0; i3<8; i3++)
			{	
				t = pixelMask(p_add++, F18ASpriteColorLine);	// Modified by RasmusM

				if (!bSkipScanLine[i1][sc]) {
					if (t&0x80)
						spritepixel(xx, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
					if (t&0x40)
						spritepixel(xx+1, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
					if (t&0x20)
						spritepixel(xx+2, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
					if (t&0x10)
						spritepixel(xx+3, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
					if (t&0x08)
						spritepixel(xx+4, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
					if (t&0x04)
						spritepixel(xx+5, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
					if (t&0x02)
						spritepixel(xx+6, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
					if (t&0x01)
						spritepixel(xx+7, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);
				}

				if (dblSize)		// double-size sprites, need to draw 3 more chars 
				{	
					t = pixelMask(p_add + 7, F18ASpriteColorLine);	// Modified by RasmusM

					if (!bSkipScanLine[i1][sc+8]) {
						if (t&0x80)
							spritepixel(xx, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
						if (t&0x40)
							spritepixel(xx+1, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
						if (t&0x20)
							spritepixel(xx+2, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
						if (t&0x10)
							spritepixel(xx+3, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
						if (t&0x08)
							spritepixel(xx+4, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
						if (t&0x04)
							spritepixel(xx+5, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
						if (t&0x02)
							spritepixel(xx+6, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
						if (t&0x01)
							spritepixel(xx+7, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);

						t = pixelMask(p_add + 23, F18ASpriteColorLine);	// Modified by RasmusM
						if (t&0x80)
							spritepixel(xx+8, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
						if (t&0x40)
							spritepixel(xx+9, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
						if (t&0x20)
							spritepixel(xx+10, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
						if (t&0x10)
							spritepixel(xx+11, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
						if (t&0x08)
							spritepixel(xx+12, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
						if (t&0x04)
							spritepixel(xx+13, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
						if (t&0x02)
							spritepixel(xx+14, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
						if (t&0x01)
							spritepixel(xx+15, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);
					}

					if (!bSkipScanLine[i1][sc]) {
						t = pixelMask(p_add + 15, F18ASpriteColorLine);	// Modified by RasmusM
						if (t&0x80)
							spritepixel(xx+8, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
						if (t&0x40)
							spritepixel(xx+9, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
						if (t&0x20)
							spritepixel(xx+10, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
						if (t&0x10)	
							spritepixel(xx+11, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
						if (t&0x08)
							spritepixel(xx+12, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
						if (t&0x04)
							spritepixel(xx+13, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
						if (t&0x02)
							spritepixel(xx+14, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
						if (t&0x01)
							spritepixel(xx+15, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);
					}
				}
				sc++;
			}
		}
	}
}

////////////////////////////////////////////////////////////
// Draw a pixel onto the backbuffer surface
////////////////////////////////////////////////////////////
void pixel(int x, int y, int c)
{
	framedata[((199-y)<<8)+((199-y)<<4)+x+8]=TIPALETTE[c];
}

////////////////////////////////////////////////////////////
// Draw a pixel onto the backbuffer surface in 80 column mode
////////////////////////////////////////////////////////////
void pixel80(int x, int y, int c)
{
	framedata[((199-y)<<9)+((199-y)<<4)+x+8]=TIPALETTE[c];
}

////////////////////////////////////////////////////////////
// Draw a range-checked pixel onto the backbuffer surface
////////////////////////////////////////////////////////////
void spritepixel(int x, int y, int c)
{
	if ((y>191)||(y<0)) return;
	if ((x>255)||(x<0)) return;
	
//	if (!(F18AECModeSprite ? c % F18ASpritePaletteSize : c)) return;		// don't DRAW transparent, Modified by RasmusM
	if (c == 0) return;		// 'c' is NOT a Boolean. '!' doesn't mean "equal to zero".
	framedata[((199-y)<<8)+((199-y)<<4)+x+8] = F18AECModeSprite ? F18APalette[c] : TIPALETTE[c];	// Modified by RasmusM
	return;
}

////////////////////////////////////////////////////////////
// Draw a magnified pixel onto the backbuffer surface
////////////////////////////////////////////////////////////
void bigpixel(int x, int y, int c)
{
	spritepixel(x,y,c);
	spritepixel(x+1,y,c);
	spritepixel(x,y+1,c);
	spritepixel(x+1,y+1,c);
}

////////////////////////////////////////////////////////////
// Pixel mask
////////////////////////////////////////////////////////////
int pixelMask(int addr, int F18ASpriteColorLine[])
{
	int t = VDP[addr];
	if (F18AECModeSprite > 0) {
		for (int pix = 0; pix < 8; pix++) {
			F18ASpriteColorLine[pix] = ((t >> (7 - pix)) & 0x01);
		}		
		if (F18AECModeSprite > 1) {
			int t1 = VDP[addr + 0x0800]; 
			for (int pix = 0; pix < 8; pix++) {
				F18ASpriteColorLine[pix] |= ((t1 >> (7 - pix)) & 0x01) << 1;
			}		
			t |= t1;
			if (F18AECModeSprite > 2) {
				int t2 = VDP[addr + 0x1000]; 
				for (int pix = 0; pix < 8; pix++) {
					F18ASpriteColorLine[pix] |= ((t2 >> (7 - pix)) & 0x01) << 2;
				}		
				t |= t2;
			}
		}
	}
	return t;
}

//////////////////////////////////////
// Save a screenshot - just BMP for now
// there are lots of nice helpers for others in
// 2000 and higher, but that's ok 
//////////////////////////////////////
void SaveScreenshot() {
	CString csFile;
	CString csTmp;
	OPENFILENAME ofn;
	char buf[256], buf2[256];
	BOOL bRet;

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize=sizeof(OPENFILENAME);
	ofn.hwndOwner=myWnd;
	ofn.lpstrFilter="BMP Files\0*.bmp\0\0";
	strcpy(buf, "");
	ofn.lpstrFile=buf;
	ofn.nMaxFile=256;
	strcpy(buf2, "");
	ofn.lpstrFileTitle=buf2;
	ofn.nMaxFileTitle=256;
	ofn.Flags=OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

	bRet = GetSaveFileName(&ofn);

	csTmp = ofn.lpstrFile;				// save the file we are opening now
	if (ofn.nFileExtension > 1) {
		csFile = csTmp.Left(ofn.nFileExtension-1);
	} else {
		csFile = csTmp;
		csTmp+=".bmp";
	}

	if (bRet) {
		// we just create a 24-bit BMP file
		int nX, nY, nBits;
		unsigned char *pBuf;

		nX=256+16;
		nY=192+16;
		pBuf=(unsigned char*)framedata;
		nBits=32;

		// 80 column override
		if ((bEnable80Columns)&&(VDPREG[0]&0x04)&&(VDPREG[1]&0x10)) {
			nX=512+16;
		}

		FILE *fp=fopen(csTmp, "wb");
		if (NULL == fp) {
			MessageBox(myWnd, "Failed to open output file", "Error saving", MB_OK);
			return;
		}

		int tmp;
		fputc('B', fp);				// signature, BM
		fputc('M', fp);
		tmp=nX*nY*3+54;
		fwrite(&tmp, 4, 1, fp);		// size of file
		tmp=0;
		fwrite(&tmp, 4, 1, fp);		// four reserved bytes (2x 2 byte fields)
		tmp=26;
		fwrite(&tmp, 4, 1, fp);		// offset to data
		tmp=12;
		fwrite(&tmp, 4, 1, fp);		// size of the header (v1)
		fwrite(&nX, 2, 1, fp);		// width in pixels
		fwrite(&nY, 2, 1, fp);		// height in pixels
		tmp=1;
		fwrite(&tmp, 2, 1, fp);		// number of planes (1)
		tmp=24;
		fwrite(&tmp, 2, 1, fp);		// bits per pixel (0x18=24)

		// 32-bit 0BGR
		for (int idx=0; idx<nX*nY; idx++) {
			int r,g,b;
				
			// extract colors
			b=*pBuf++;
			g=*pBuf++;
			r=*pBuf++;
			pBuf++;					// skip	0

			// write out to file
			fputc(b, fp);
			fputc(g, fp);
			fputc(r, fp);
		}

		fclose(fp);
	}
}

bool LoadBuffer() {
	CString csFile;
	OPENFILENAME ofn;
	char buf[256], buf2[256];
	BOOL bRet;

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize=sizeof(OPENFILENAME);
	ofn.hwndOwner=myWnd;
	ofn.lpstrFilter="All files\0*.*\0\0";
	strcpy(buf, "");
	ofn.lpstrFile=buf;
	ofn.nMaxFile=256;
	strcpy(buf2, "");
	ofn.lpstrFileTitle=buf2;
	ofn.nMaxFileTitle=256;
	ofn.Flags=OFN_FILEMUSTEXIST;

	bRet = GetOpenFileName(&ofn);

	if (bRet) {
		FILE *fp = fopen(buf, "rb");
		if (NULL == fp) {
			MessageBox(myWnd, "Failed to open file", "Error loading", MB_OK);
			return 0;
		}

		bRet = fread(VDP, 1, sizeof(VDP), fp) > 0;
		fclose(fp);

		// check for TIFILES header
		if (memcmp(VDP, "\x7TIFILES", 8) == 0) {
			memmove(VDP, &VDP[128], 16*1024);
		} else {
			// check (poorly) for V9T9 header
			char *p = strrchr(buf, '\\');
			if (p) memmove(buf, p+1, strlen(p+1)+1);
			int idx;
			for (idx=0; idx<10; idx++) {
				if (VDP[idx]==buf[idx]) {
					continue;
				}
				if ((buf[idx] == 0)&&(VDP[idx] == ' ')&&(idx>0)) {
					idx=11;
					break;
				}
				break;
			}
			if (idx>=10) {
				// probably is
				memmove(VDP, &VDP[128], 16*1024);
			}
		}

		// after load, put a default bitmap SIT at 0x4000 for testing files
		for (int i=0; i<768; ++i) {
			VDP[i+0x4000]=i&0xff;
		}
	}

	return (bRet ? true : false);
}