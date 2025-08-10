#define uchar   UCHAR
#define ushort  USHORT
#define ulong   ULONG
#define word    ushort
#define WORD    ushort
#define integer short
#define INTEGER short
#define dword   ulong
#define DWORD   ulong

#define TRUE  1
#define FALSE 0
#define Null  0

typedef VOID (* _Optlink THREADFUNC)(void *);

#define CvChar(x)    *((unsigned char *)(x))
#define CvByt(x)     *((unsigned char *)(x))
#define CvUshort(x)  *((unsigned short *)(x))
#define CvWrd(x)     *((unsigned short *)(x))
#define CvUlong(x)   *((unsigned long *)(x))
#define CvDwd(x)     *((unsigned long *)(x))
#define CvWrdSwap(x) (int)(x)[1] | ((int)(x)[0] << 8)
#define CvDwdSwap(x) (ulong)(x)[3] | ((ulong)(x)[2] << 8) | ((ulong)(x)[1] << 16) | ((ulong)(x)[0] << 8)

#define until(expr) while(!(expr))
