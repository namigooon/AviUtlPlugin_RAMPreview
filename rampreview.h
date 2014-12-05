//----------------------------------------------------------------------------------------------------
// RAM Preview
// 	Ver. 0.02
// 
//	2014/02/28 - 2014/11/24
//			hksy
//----------------------------------------------------------------------------------------------------

#pragma	once
#ifndef	_H_APF_RAMPREVIEW
#define	_H_APF_RAMPREVIEW

//--------------------------------------------------

#include	<windows.h>
#include	<tchar.h>
#include	<stdio.h>
#include	<math.h>
#include	<mmsystem.h>
#include	<process.h>

#include	"filter.h"
#include	"resource.h"

#pragma		comment(lib, "winmm.lib")

//--------------------------------------------------

TCHAR	*FilterName	= "RAMプレビュー";
TCHAR	*FilterInfo	= "RAMプレビュー version 0.02 by 白水";

//--------------------------------------------------

#define	WINDOW_W		320
#define	WINDOW_H		240

#define	WINDOW_BOTTOM		32
#define	BUTTON_SIZE		24
#define	BUTTON_SIZE_MINI	18
#define	READFRAME_HEIGHT	3

#define	DEF_USEMEMORY		4096

//--------------------------------------------------

#define	WM_READDATA_END		WM_USER+0

//--------------------------------------------------

typedef enum{
	SIZE_PERCENT025,
	SIZE_PERCENT050,
	SIZE_PERCENT100,
	SIZE_PERCENT200,
	SIZE_PX640,
	SIZE_PX320,
	SIZE_PX160,
	SIZE_WINDOW
}VIEWSIZE;

typedef	enum{
	PLAY_ALL,
	PLAY_VIDEO,
	PLAY_AUDIO
}PLAYMODE;

//--------------------------------------------------

const int	DW_SIZE	= sizeof(DWORD);

#define	SETARRAYDATA(dst,src)	memcpy_s((void*)dst,sizeof(dst),(void*)src,sizeof(dst))

const BYTE	HeaderChunkID[]		= {'R','I','F','F'};
const BYTE	HeaderFormType[]	= {'W','A','V','E'};
const BYTE	FormatChunkID[]		= {'f','m','t',' '};
const BYTE	DataChunkID[]		= {'d','a','t','a'};

#pragma pack(push,1)	// アライメントを詰める

typedef struct tagWAVEFILEHEADER{
	BYTE		FileSignature[4];
	DWORD		FileSize;
	BYTE		FileType[4];
	BYTE		FormatChunkID[4];
	DWORD		FormatChunkSize;
	WAVEFORMATEX	Format;
	BYTE		DataChunkID[4];
	DWORD		DataChunkSize;
}WAVEFILEHEADER;
typedef struct tagINTSIZE{
	int	cx;
	int	cy;
}INTSIZE;

#pragma pack(pop)

//--------------------------------------------------

#ifndef	MAKEDWORD
#define	MAKEDWORD(a,b)	(((a)&0xFFFF)|(((b)&0xFFFF)<<16))
#endif

#define	RANGE(v,r1,r2)			(((v)<(r1))?(r1):((r2)<(v)?r2:v))
#define	NUMDATATORESID_SIZE(num)	(IDM_VIEW_SIZE_PERCENT025	+(num))
#define	NUMDATATORESID_PLAY(num)	(IDM_PLAY_PLAY_ALL		+(num))

// Debug
//--------------------------------------------------
#ifdef _DEBUG
#define debug(format,...)	printf("DEBUG : " format, __VA_ARGS__);
#define DebugAlert(format,...)	{char str[512];sprintf_s(str,sizeof(str),format,__VA_ARGS__);MessageBox(NULL,str,"DEBUG",MB_OK);}
#define DebugLog(format,...)	{char str[512];sprintf_s(str,sizeof(str),format,__VA_ARGS__);OutputDebugString(str);}
#else
#define debug
#define DebugAlert
#define	DebugLog
#endif



//--------------------------------------------------

#endif	// _H_APF_RAMPREVIEW




