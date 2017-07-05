#define     B4800			0
#define     B9600			1
#define     B19200			2
#define     B32800			3
#define     B57600			4
#define     B115200			5

//#define     Main_XTAL       16000000L

extern void Com1_Init(char _rate);
extern unsigned char Com1_rd_char(void);
extern void Com1_TXDataWR(char *str);
extern void Com1_TXDataChar(char _ch);

extern void Com2_Init(char _rate);
extern unsigned char Com2_rd_char(void);
extern void Com2_TXDataWR(char *str);
extern void Com2_TXDataChar(char _ch);

extern unsigned char Com2_TxBuff_Ck(void);
extern unsigned char Com2_RXBuff_Ck(void);

extern unsigned char Com2_rd_string(char *str,unsigned char St_add);
extern unsigned char Com2_rd_char(void);
extern void Com2_StringTX(unsigned char *str);
extern void Com2_CharTX(unsigned char Data);

