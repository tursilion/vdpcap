typedef unsigned char Byte;
typedef unsigned short Word;

extern Byte VDP[16*1024+128];					// Video RAM (16k)
extern int bEnable80Columns;				// Enable the beginnings of the 80 column mode - to replace someday with F18A
extern Byte VDPREG[8];						// VDP read-only registers
extern int bUse5SpriteLimit;				// whether the sprite flicker is on
extern bool bDisableBlank, bDisableSprite, bDisableBackground;	// other layers :)
extern int modeDrawn;						// which mode we selected
extern int SIT;								// Screen Image Table
extern int CT;								// Color Table
extern int PDT;								// Pattern Descriptor Table
extern int SAL;								// Sprite Allocation Table
extern int SDT;								// Sprite Descriptor Table
extern int CTsize;							// Color Table size in Bitmap Mode
extern int PDTsize;							// Pattern Descriptor Table size in Bitmap Mode
extern Byte VDPREG[8];						// VDP read-only registers

void VDPinit(HWND inWnd);
void UpdateDisplay();
void VDPshutdown();
void SaveScreenshot();
bool LoadBuffer();
