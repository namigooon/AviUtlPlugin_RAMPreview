//----------------------------------------------------------------------------------------------------
// RAM Preview
// 	Ver. 0.03
// 
//	2014/02/28 - 2015/02/19
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
#include	"RAMImage.h"
#include	"View.h"
#include	"resource.h"

#pragma		comment(lib, "winmm.lib")

//--------------------------------------------------

#define	WM_READDATA_END		WM_USER+0
#define	WM_READDATA_FRAME	WM_USER+1

//--------------------------------------------------

#define	WINDOW_W		320
#define	WINDOW_H		240

#define	WINDOW_BOTTOM		32
#define	BUTTON_SIZE		24
#define	BUTTON_SIZE_MINI	18
#define	BUTTON_PADDING		4
#define	READFRAME_HEIGHT	3

#define	DEF_USEMEMORY		4
#define	USEMEMORY_UNIT		20	// Žg—pƒƒ‚ƒŠ‚Ì’PˆÊ[bit] 20->MB



//--------------------------------------------------

enum VIEWSIZE{
	SIZE_PERCENT025,
	SIZE_PERCENT050,
	SIZE_PERCENT100,
	SIZE_PERCENT200,
	SIZE_PX640,
	SIZE_PX320,
	SIZE_PX160,
	SIZE_WINDOW
};
enum PLAYMODE{
	PLAY_ALL,
	PLAY_VIDEO,
	PLAY_AUDIO
};
enum PREVIEWQUALITY{
	PQ_ORIGINAL	= 100,
	PQ_HALF		=  50,
	PQ_QUARTER	=  25,
	PQ_USER		=   0
};

struct RP_CONFIG{
	VIEWSIZE	ViewSize;
	PLAYMODE	PlayMode;
	bool		isPlaySelect;
	bool		isPlayLoop;
	int		PreviewQuality;
	int		UseMemory;
};

//--------------------------------------------------

template<class T>inline bool	isRange(T v, T s, T e, bool seq=true, bool eeq=false){
	if(seq==true){	if(v<s){	return false;	}}
	else{		if(v<=s){	return false;	}}
	if(eeq==true){	if(e<v){	return false;	}}
	else{		if(e<=v){	return false;	}}
	return true;
}
template<class T>inline T	Range(T v, T s, T e){
	return (s<v)?(e<v)?e:v:s;
}

#ifndef	MAKEDWORD
#define	MAKEDWORD(a,b)	(((a)&0xFFFF)|(((b)&0xFFFF)<<16))
#endif

#define	NUMDATATORESID_SIZE(num)	(IDM_VIEW_SIZE_PERCENT025	+(num))
#define	NUMDATATORESID_PLAY(num)	(IDM_PLAY_PLAY_ALL		+(num))

#define	SETARRAYDATA(dst,src)	memcpy_s((void*)dst,sizeof(dst),(void*)src,sizeof(dst))

double	GetFPS(FILTER *fp, void* editp);

//--------------------------------------------------

extern FILTER_DLL	filter;
extern const int	SizePerList[];
extern const int	SizePxList[];
extern const DWORD	COLOR_READFRAME;
extern const DWORD	COLOR_VIEWFRAME;



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


