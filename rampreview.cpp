//----------------------------------------------------------------------------------------------------
// RAM Preview
// 	Ver. 0.02
// 
//	2014/02/28 - 2014/11/24
//			hksy
//----------------------------------------------------------------------------------------------------


#include	"rampreview.h"

VIEWSIZE	ViewSize;
PLAYMODE	PlayMode;

const int SizePerList[]	= {25,50,100,200};
const int SizePxList[]	= {640,320,160};

bool	isPlaySelect;
bool	isPlayLoop;


BYTE	*VideoMemory;
short	*AudioMemory;
UINT	UseMemory;
int	FrameVideoSize;
int	FrameAudioSize;
int	AudioChannel;
UINT	AllocateVideoMemory;
UINT	AllocateAudioMemory;
bool	CanPlayMemory;
UINT	PlayStartFrame;
INTSIZE	ReadScreenSize;
UINT	ReadFrame;
UINT	ReadCompleteFrame;
bool	isPrepareMemory;
bool	isReading;
bool	isPlaying;

WAVEFILEHEADER	WaveFileHeader;

HANDLE	hReadThread;

const DWORD	WindowStyle	= 0x14CC0000;
const DWORD	WindowExStyle	= 0x00000180;
const DWORD	COLOR_READFRAME	= RGB(0x00,0xFF,0x00);
const DWORD	COLOR_VIEWFRAME	= RGB(0xFF,0x00,0x00);



//---------------------------------------------------------------------
//		フィルタ構造体定義
//---------------------------------------------------------------------
FILTER_DLL filter = {
	FILTER_FLAG_DISP_FILTER|FILTER_FLAG_WINDOW_THICKFRAME|FILTER_FLAG_ALWAYS_ACTIVE|FILTER_FLAG_WINDOW_SIZE|FILTER_FLAG_EX_INFORMATION,
	WINDOW_W,WINDOW_H,			// size
	FilterName,				// name
	NULL,NULL,NULL,NULL,NULL,		// track
	NULL,NULL,NULL,				// check
	NULL,					// Func	: filter
	NULL,					// Func : init
	NULL,					// Func	: end
	NULL,					// Func : update
	func_WndProc,				// Func : window
	NULL,NULL,				// track, check data
	NULL,NULL,				// exdata, exsize
	FilterInfo,				// info
	NULL,NULL,				// save
	NULL,					// exfunc
	NULL,					// hWnd
	NULL,					// hInst
	NULL,					// exdata
	NULL,					// 
	NULL,					// read	aup
	NULL,					// save aup
	NULL,					// path
	NULL					// reserve
};


//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable( void ){
	return &filter;
}



//---------------------------------------------------------------------
//		Utility
//---------------------------------------------------------------------
double	GetFPS(FILTER *fp, void* editp){
	FILE_INFO	fileInfo;
	fp->exfunc->get_file_info(editp,&fileInfo);
	return (double)fileInfo.video_rate/fileInfo.video_scale;
}

inline void	GetScreenSize(FILTER *fp, void *editp, int frame, int *w, int *h){
	fp->exfunc->get_pixel_filtered(editp, frame, NULL, w, h);
}
void	UpdateSize(FILTER *fp, void *editp, int frame, int *w, int *h){
	GetScreenSize(fp, editp, frame, &ReadScreenSize.cx, &ReadScreenSize.cy);
	if(w!=NULL){	*w	= ReadScreenSize.cx;	}
	if(h!=NULL){	*h	= ReadScreenSize.cy;	}
	//ReadFrame		= AllocateVideoMemory/FrameVideoSize;
}



//---------------------------------------------------------------------
//		メモリ管理
//---------------------------------------------------------------------
void	UpdateMemoryInfo(FILTER *fp, void *editp){
	int		Sample	= fp->exfunc->get_audio_filtered(editp,0,NULL);
	FILE_INFO	fi;
	int		w,h;
	int		playVideoFrame	= ReadFrame;
	int		playAudioFrame	= ReadFrame;

	GetScreenSize(fp, editp, 0, &w, &h);
	fp->exfunc->get_file_info(editp,&fi);

	AudioChannel		= fi.audio_ch;
	FrameVideoSize		= w*h*sizeof(PIXEL);
	FrameAudioSize		= Sample*AudioChannel;

	// メモリ確保時
	if(AllocateVideoMemory>0){
		playVideoFrame	= AllocateVideoMemory / FrameVideoSize;
	}
	if(AllocateAudioMemory>0){
		playAudioFrame	= (AllocateAudioMemory - sizeof(WaveFileHeader)/sizeof(short)) / FrameAudioSize;
	}

	ReadFrame	= min(playVideoFrame, playAudioFrame);
	DebugLog("ReadFrame = min(%d, %d) = %d",playVideoFrame, playAudioFrame, ReadFrame);
}

template<class T> bool	AllocatePreviewMemory(HWND hWnd, T **buf, UINT size, UINT *allocsize, TCHAR* name){
	T	*prev	= *buf;	// 失敗時の継続用に旧メモリは確定するまで退避
	*buf		= new T[size];

	if(!buf){
		delete[]	*buf;	// nullだけど一応
		*buf	= prev;

		TCHAR	strbuf[256];
		_stprintf_s(strbuf, sizeof(strbuf), _T("%sメモリの確保に失敗しました"), name);
		MessageBox(hWnd, strbuf, _T(filter.name), MB_OK|MB_ICONERROR);		
		return false;
	}
	else{
		DebugLog("delete	: Memory = 0x%p",prev);
		delete[] prev;
		*allocsize	= size;
		prev	= NULL;
	}
	return true;
}

bool	UpdateMemory(FILTER *fp, void *editp){
	const UINT	MulVideo	= 1<<10;
	bool		result		= true;

#ifdef	_DEBUG
	int		w,h;
	GetScreenSize(fp, editp, 0, &w, &h);
	int		Sample	= fp->exfunc->get_audio_filtered(editp,0,NULL);
#endif

	UpdateMemoryInfo(fp, editp);


	if(FrameVideoSize){
		ReadFrame		= (UseMemory*MulVideo)/FrameVideoSize;

		UINT	newvideosize	= max(UseMemory*MulVideo,(UINT)FrameVideoSize);
		UINT	newaudiosize	= sizeof(WaveFileHeader)/sizeof(short) + FrameAudioSize*ReadFrame;

		if(
			!AllocatePreviewMemory<BYTE>	(fp->hwnd, &VideoMemory, newvideosize, &AllocateVideoMemory, _T("映像")) ||
			!AllocatePreviewMemory<short>	(fp->hwnd, &AudioMemory, newaudiosize, &AllocateAudioMemory, _T("音声"))
		){
			result	= false;
		}

#ifdef	_DEBUG
		DebugLog("(%d, %d) * %dbyte    = %d [byte /frame]",w,h,sizeof(PIXEL),FrameVideoSize);
		DebugLog("%dsample * %dchannel = %d [short/frame]",Sample,AudioChannel,FrameAudioSize);
		DebugLog("-> RAM : %d frame",ReadFrame);
		DebugLog("new		: VideoMemory = 0x%p / 0x%08x byte [ %dKB * %dbyte = %d ]", VideoMemory,AllocateVideoMemory,UseMemory,MulVideo,AllocateVideoMemory);
		DebugLog("new		: AudioMemory = 0x%p / 0x%08x byte [ %dbyte + %dbyte * %dframe = %d ]", AudioMemory,AllocateAudioMemory, sizeof(WaveFileHeader),FrameAudioSize,ReadFrame,AllocateAudioMemory);
#endif

	}

	isPrepareMemory		= (VideoMemory!=NULL && AudioMemory!=NULL);
	CanPlayMemory		= false;
	ReadCompleteFrame	= 0;

	UpdateMemoryInfo(fp, editp);

	return result;
}

void	ReleaseMemory(void){
	DebugLog("delete	: VideoMemory = 0x%p",VideoMemory);
	DebugLog("delete	: AudioMemory = 0x%p",AudioMemory);

	delete[]	VideoMemory;
	delete[]	AudioMemory;

	VideoMemory		= NULL;
	AudioMemory		= NULL;
	//UseMemory		= 0;
	AllocateVideoMemory	= 0;
	AllocateAudioMemory	= 0;
	CanPlayMemory		= false;
	ReadFrame		= 0;
	ReadCompleteFrame	= 0;
	isPrepareMemory		= false;
}

inline int	GetFrameIndex(int frame){
	UINT	i	= frame-PlayStartFrame;
	return	RANGE(i,0,ReadFrame)*FrameVideoSize;
}



//---------------------------------------------------------------------
//		表示
//---------------------------------------------------------------------

RECT	GetViewPosition(FILTER *fp, void *editp){
	int		screen_w	= ReadScreenSize.cx;
	int		screen_h	= ReadScreenSize.cy;
	RECT		rc;

	GetClientRect(fp->hwnd,&rc);
	rc.bottom	-=WINDOW_BOTTOM;

	// 画面サイズの計算
	double	wx	= (double)rc.right /screen_w;
	double	hx	= (double)rc.bottom/screen_h;
	double	p	= 1.0;

	if(ViewSize==SIZE_WINDOW){
		p	= min(wx,hx);
	}
	else if(SIZE_PERCENT025<=ViewSize && ViewSize<=SIZE_PERCENT200){
		p	= RANGE(min(wx,hx),0,SizePerList[ViewSize-SIZE_PERCENT025]/100.0);
	}
	else if(SIZE_PX640<=ViewSize && ViewSize<=SIZE_PX160){
		p	= RANGE(min(wx,hx),0,(double)SizePxList[ViewSize-SIZE_PX640]/screen_w);
	}
	rc.left		= (int)((rc.right -screen_w*p)/2);
	rc.top		= (int)((rc.bottom-screen_h*p)/2);
	rc.right	= (int)(screen_w*p);
	rc.bottom	= (int)(screen_h*p);

	return rc;
}

void	RefreshBG(HWND hWnd){
	RECT	r;
	GetClientRect(hWnd, &r);

	HDC	hDC		= GetDC(hWnd);
	HPEN	hPenNull	= (HPEN)GetStockObject(NULL_PEN);
	HBRUSH	hBrushBG	= (HBRUSH)CreateSolidBrush( GetSysColor( COLOR_3DFACE ) );

	SelectObject(hDC, hPenNull);
	SelectObject(hDC, hBrushBG);
	Rectangle(hDC, r.left, r.top, r.right+1, r.bottom-WINDOW_BOTTOM+1);

	// 描画終了
	DeleteObject(hPenNull);
	DeleteObject(hBrushBG);
	ReleaseDC(hWnd,hDC);
}

void disp( HWND hwnd,FILTER *fp,void *editp,int frame,int totalframe,int screen_w,int screen_h ){
	BITMAPINFO	bmi;
	TCHAR		b[MAX_PATH];

	if( !fp->exfunc->is_filter_window_disp(fp) ) return;
	RECT		rc	= GetViewPosition(fp, editp);

	//	フレームの表示
	ZeroMemory(&bmi,sizeof(bmi));
	bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth       = screen_w;
	bmi.bmiHeader.biHeight      = screen_h;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = sizeof(PIXEL)*8;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biPlanes      = 1;


	HDC	hDC		= GetDC(hwnd);
	HPEN	hPenNull	= (HPEN)GetStockObject(NULL_PEN);
	HBRUSH	hBrushBG	= (HBRUSH)CreateSolidBrush( GetSysColor( COLOR_3DFACE ) );
	SetStretchBltMode(hDC,COLORONCOLOR);


	if( fp->exfunc->is_editing(editp) && totalframe ) {	// 編集中
		// フレーム画像
		void	*pixelp		= NULL;
		bool	isAllocate	= false;
		bool	isDraw		= PlayMode==PLAY_ALL || PlayMode==PLAY_VIDEO;

		//if(CanPlayMemory && isPlaying && isDraw){
		if(CanPlayMemory && (isPlaying || isReading) && isDraw){	// 映像
			pixelp		= (void*)&VideoMemory[GetFrameIndex(frame)];
			isDraw		= true;
			//DebugLog("View : VideoMemory[%08d] 0x%p",GetFrameIndex(frame),pixelp);
		}
		else if(CanPlayMemory && isPlaying){				// 音声のみ
			pixelp		= (void*)&VideoMemory[GetFrameIndex(PlayStartFrame)];
			isDraw		= true;
		}
		else{								// プレビュー以外
			int	size	= screen_w * screen_h * sizeof(PIXEL);
			//DebugLog("View : frame[%08d] / 0x%08Xbyte",frame,size);
			pixelp		= new BYTE[size];
			isDraw		= fp->exfunc->get_pixel_filtered(editp,frame,pixelp,NULL,NULL)!=0;
			isAllocate	= true;
		}

		if(isDraw){
			StretchDIBits(hDC,
				rc.left,rc.top,rc.right,rc.bottom,
				0,0,screen_w,screen_h,
				pixelp,&bmi,DIB_RGB_COLORS,SRCCOPY
			);
		}
		if(isAllocate){
			delete	pixelp;
		}


		// 読み込んだフレーム情報
		RECT	RectRead;
		GetClientRect(fp->hwnd,&rc);
		rc.right++;
		rc.bottom++;
		RectRead.left	= (LONG)(((PlayStartFrame                  )/(double)totalframe)*rc.right);
		RectRead.right	= (LONG)(((PlayStartFrame+ReadCompleteFrame)/(double)totalframe)*rc.right);
		RectRead.top	= rc.bottom-READFRAME_HEIGHT;
		RectRead.bottom	= rc.bottom;

		LONG	CurrentPos	= (LONG)(((frame                   )/(double)totalframe)*rc.right);

		HPEN	hPenView	= CreatePen(PS_SOLID,1,COLOR_VIEWFRAME);
		HBRUSH	hBrushRead	= CreateSolidBrush(COLOR_READFRAME);

		SelectObject(hDC, hPenNull);
		SelectObject(hDC, hBrushBG);	Rectangle(hDC, rc.left,		RectRead.top,	rc.right,	rc.bottom);
		SelectObject(hDC, hBrushRead);	Rectangle(hDC, RectRead.left,	RectRead.top,	RectRead.right,	RectRead.bottom);
		SelectObject(hDC, hPenView);	MoveToEx(hDC, CurrentPos, RectRead.top, NULL);	LineTo(hDC, CurrentPos, RectRead.bottom);
		DeleteObject(hBrushRead);
		DeleteObject(hPenView);

	}
	else{
		GetClientRect(fp->hwnd,&rc);
		SelectObject(hDC, hPenNull);
		SelectObject(hDC, hBrushBG);
		rc.right++;
		rc.bottom++;
		Rectangle(hDC,rc.left,rc.top,rc.right,rc.bottom-WINDOW_BOTTOM);
		Rectangle(hDC,rc.left,rc.bottom-READFRAME_HEIGHT,rc.right,rc.bottom);
	}

	DeleteObject(hBrushBG);
	DeleteObject(hPenNull);


	// 描画終了
	ReleaseDC(hwnd,hDC);



	//	タイトルバーの表示
	if( fp->exfunc->is_editing(editp) && totalframe ) {
		double	fps		= GetFPS(fp,editp);
		double	time		= frame/fps;

		wsprintf(b,"%s [%02d:%02d:%02d.%02d] [%d/%d]",filter.name,
			(int)(time/3600)%60,(int)(time/60)%60,(int)(time)%60,(int)((time-(int)time)*100),
			frame+1,totalframe
		);
		if(isReading){
			TCHAR buf[MAX_PATH];
			wsprintf(buf," [%d/%d]",ReadCompleteFrame,ReadFrame);
			strcat_s(b,sizeof(b),buf);
		}
	}
	else{
		wsprintf(b,"%s",filter.name);
	}
	SetWindowText(hwnd,b);
}
inline void	UpdatePreviewWindow(FILTER *fp, void *editp, int frame){
	int	totalframe	= fp->exfunc->get_frame_n(editp);
	disp(fp->hwnd, fp, editp, frame, totalframe, ReadScreenSize.cx, ReadScreenSize.cy);
}

HWND	CreateIconButton(HWND hParentWnd, int x, int y, int w, int h, int ResID, HINSTANCE hInst, LPCTSTR IcoName){
	HWND	hWnd	= CreateWindow("BUTTON","",WS_CHILD | WS_VISIBLE | BS_ICON,
		x,y,w,h,hParentWnd,(HMENU)ResID,hInst,NULL
	);
	HICON	hIcon	= (HICON)LoadImage(hInst,IcoName,IMAGE_ICON, 0,0, LR_DEFAULTCOLOR);
	SendMessage(hWnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
	return hWnd;
}

void	SetButtonPosition(HWND hWnd, HWND hWndPlay, HWND hWndPause, HWND hWndStop, HWND hWndRead){
	RECT	rc;
	GetClientRect(hWnd,&rc);

	int	x,y;
	int	paddingw	= 4;
	int	paddingh	= (WINDOW_BOTTOM-BUTTON_SIZE)/2;

	y	= rc.bottom-WINDOW_BOTTOM+paddingh;
	x	= rc.right-(BUTTON_SIZE+paddingw)*4-paddingw/2;

	MoveWindow(hWndRead,	x,y,BUTTON_SIZE,BUTTON_SIZE,TRUE);	x	+=(BUTTON_SIZE+paddingw);
	MoveWindow(hWndPlay,	x,y,BUTTON_SIZE,BUTTON_SIZE,TRUE);	x	+=(BUTTON_SIZE+paddingw);
	MoveWindow(hWndPause,	x,y,BUTTON_SIZE,BUTTON_SIZE,TRUE);	x	+=(BUTTON_SIZE+paddingw);
	MoveWindow(hWndStop,	x,y,BUTTON_SIZE,BUTTON_SIZE,TRUE);

}

SIZE	getWindowSize(int width, int height, DWORD style, DWORD exstyle, BOOL menu){
	RECT	rect;
	SIZE	size;

	SetRect(&rect,0,0,width,height);
	AdjustWindowRectEx(&rect, style, menu, exstyle);
	size.cx	= rect.right  - rect.left;
	size.cy	= rect.bottom - rect.top;

	return size;
}

void	UpdateWindowSize(HWND hwnd, int frame_w, int frame_h){
	if(ViewSize!=SIZE_WINDOW){
		SIZE	size;
		if(SIZE_PERCENT025<=ViewSize && ViewSize<=SIZE_PERCENT200){
			int PerSize	= SizePerList[ViewSize-SIZE_PERCENT025];
			size.cx	= (int)(frame_w*PerSize/100.0);
			size.cy	= (int)(frame_h*PerSize/100.0);
		}
		else if(SIZE_PX640<=ViewSize && ViewSize<=SIZE_PX160){
			int PxSize	= SizePxList[ViewSize-SIZE_PX640];
			size.cx	=  PxSize;
			size.cy	= (PxSize*3)/4;
		}
		size.cy	+=WINDOW_BOTTOM;
		size	= getWindowSize(size.cx,size.cy,WindowStyle,WindowExStyle,TRUE);

		SetWindowPos(hwnd,NULL,0,0,size.cx,size.cy,SWP_NOMOVE|SWP_NOZORDER);
	}
}

void	ShowInformation(HWND hWnd){
	TCHAR	buf[512];
	const	double	b	= 1<<10;

	_stprintf_s(buf,sizeof(buf),
		_T("映像メモリ\t%.2f KB\n音声メモリ\t%.2f KB\nプレビュー\t%d フレーム"),
		AllocateVideoMemory*sizeof(BYTE)/b,AllocateAudioMemory*sizeof(short)/b,
		ReadFrame
	);

	MessageBox(hWnd,buf,_T(filter.name),MB_OK|MB_ICONINFORMATION);
}



//---------------------------------------------------------------------
//		RAMプレビュー
//---------------------------------------------------------------------
void	ReadData(FILTER *fp, void *editp){
	int	CurrentFrame;
	int	EndFrame;
	int	MaxFrame;
	double	fps		= GetFPS(fp, editp);

	UpdateMemoryInfo(fp, editp);

	MaxFrame		= fp->exfunc->get_frame_n(editp);
	ReadCompleteFrame	= 0;

	if(!isPlaySelect){	// 現在の位置から再生
		CurrentFrame	= fp->exfunc->get_frame(editp);
	}
	else{		// 範囲選択内を再生
		fp->exfunc->get_select_frame(editp, &CurrentFrame, &MaxFrame);
	}
	EndFrame		= min(CurrentFrame+(int)ReadFrame,MaxFrame);

	PlayStartFrame		= CurrentFrame;

	int	VideoP		= 0;
	int	AudioP		= sizeof(WaveFileHeader)/sizeof(short);
	int	w		= ReadScreenSize.cx;
	int	h		= ReadScreenSize.cy;
	int	ReadVideoSize	= (w*h*sizeof(PIXEL));
	int	ReadAudioSize	= fp->exfunc->get_audio_filtered(editp,CurrentFrame,NULL);

	UINT	read		= (UINT)(EndFrame-CurrentFrame);

	FrameVideoSize		= w*h*sizeof(PIXEL);

	DebugLog("[Read] %d - %d frame (%d frame)",CurrentFrame,EndFrame,read);


	// 映像・音声読み込み
	for(UINT i=0;i<read;i++){
		if(!fp->exfunc->is_editing(editp)){	return;	}
		if(!isReading){				break;	}

		int	f	= CurrentFrame+i;
		int	ReadAudio;
		//DebugLog("    [%03d] : VideoMemory[%08d] / AudioMemory[%08d] 0x%p",f,VideoP,AudioP,&AudioMemory[AudioP]);


		if(VideoMemory!=NULL){
			fp->exfunc->get_pixel_filtered(editp,f,&VideoMemory[VideoP],NULL,NULL);
		}
		if(AudioMemory!=NULL){
			ReadAudio	= fp->exfunc->get_audio_filtered(editp,f,&AudioMemory[AudioP]);
		}
		VideoP	+= ReadVideoSize;
		AudioP	+= ReadAudio*AudioChannel;

		ReadCompleteFrame++;
		UpdatePreviewWindow(fp, editp, f);
	}

	// PlaySound用にヘッダの作成
	WAVEFORMATEX	wf	= {
		1, AudioChannel,					// wFormatTag,	nChannels
		(UINT)(ReadAudioSize*fps),				// nSamplesPerSec
		(UINT)(ReadAudioSize*fps * AudioChannel*sizeof(short)),	// nAvgBytesPerSec
		sizeof(short)*AudioChannel,				// nBlockAlign
		sizeof(short)*8						// wBitsPerSample
	};

	SETARRAYDATA(WaveFileHeader.FileSignature,	HeaderChunkID);
	SETARRAYDATA(WaveFileHeader.FileType,		HeaderFormType);
	SETARRAYDATA(WaveFileHeader.FormatChunkID,	FormatChunkID);
	SETARRAYDATA(WaveFileHeader.DataChunkID,	DataChunkID);

	WaveFileHeader.DataChunkSize	= ReadCompleteFrame*ReadAudioSize*AudioChannel*sizeof(short);
	WaveFileHeader.FileSize		= WaveFileHeader.DataChunkSize + sizeof(WAVEFILEHEADER)-sizeof(DWORD)*2;
	WaveFileHeader.FormatChunkSize	= sizeof(WaveFileHeader.Format);
	WaveFileHeader.Format		= wf;

	if(AudioMemory!=NULL){
		memcpy(AudioMemory,(BYTE*)&WaveFileHeader,sizeof(WaveFileHeader));
	}

	CanPlayMemory			= true;
}

unsigned __stdcall ReadDataSubThreadProc(LPVOID lpParam){
	DWORD	*args	= (DWORD*)lpParam;
	FILTER	*fp	= (FILTER*	)args[0];
	void	*editp	= (void*	)args[1];

	DebugLog("DEBUG : Call ReadData(0x%p, 0x%p)",fp,editp);

	ReadData(fp, editp);

	SendMessage(fp->hwnd, WM_READDATA_END, (WPARAM)hReadThread, NULL);
	_endthreadex(0);
	DebugLog("DEBUG : End Thread");
	return 0;
}

void	BeginReadDataSubThread(FILTER *fp, void *editp){
	static void	*argList[2];
	argList[0]	= (void*)fp;
	argList[1]	= (void*)editp;

	isReading	= true;

	DebugLog("DEBUG : Create Thread / args = {0x%p, 0x%p}",fp,editp);
	hReadThread	= (HANDLE)_beginthreadex(NULL, 0, ReadDataSubThreadProc, &argList, 0, NULL);
}
void	EndReadDataSubThread(void){
	isReading	= false;

	if(hReadThread==NULL){
		return;
	}

	DebugLog("DEBUG : Close Thread");
	CloseHandle(hReadThread);
	hReadThread	= NULL;
}



//---------------------------------------------------------------------
//		Other
//---------------------------------------------------------------------
void	ReadIniData(FILTER *fp){
	ViewSize	= (VIEWSIZE)	fp->exfunc->ini_load_int(fp,"ViewSize",		SIZE_WINDOW);
	PlayMode	= (PLAYMODE)	fp->exfunc->ini_load_int(fp,"PlayMode",		PLAY_ALL);
	UseMemory	=		fp->exfunc->ini_load_int(fp,"UseMemory",	DEF_USEMEMORY);
	isPlaySelect	=		fp->exfunc->ini_load_int(fp,"PlaySelect",	FALSE)!=FALSE;
	isPlayLoop	=		fp->exfunc->ini_load_int(fp,"PlayLoop",		FALSE)!=FALSE;

}
void	WriteIniData(FILTER *fp){
	fp->exfunc->ini_save_int(fp,"ViewSize",		ViewSize);
	fp->exfunc->ini_save_int(fp,"PlayMode",		PlayMode);
	fp->exfunc->ini_save_int(fp,"UseMemory",	UseMemory);
	fp->exfunc->ini_save_int(fp,"PlaySelect",	(isPlaySelect	)?TRUE:FALSE);
	fp->exfunc->ini_save_int(fp,"PlayLoop",		(isPlayLoop	)?TRUE:FALSE);
}

UINT	GetMenuItemState(HMENU hMenu, UINT ResID){
	MENUITEMINFO	MenuInfo;
	MenuInfo.cbSize	= sizeof(MenuInfo);
	MenuInfo.fMask	= MIIM_STATE;

	GetMenuItemInfo( hMenu, ResID, FALSE, &MenuInfo );
	return MenuInfo.fState;
}

void	SetMenuItemState(HMENU hMenu, UINT ResID, UINT State){
	MENUITEMINFO	MenuInfo;
	MenuInfo.cbSize	= sizeof(MenuInfo);
	MenuInfo.fMask	= MIIM_STATE;
	MenuInfo.fState	= State;

	SetMenuItemInfo(hMenu, ResID, FALSE, &MenuInfo);
}

bool	GetMenuItemEnable(HMENU hMenu, UINT ResID){
	return (GetMenuItemState(hMenu, ResID)&MFS_CHECKED)!=0;
}

bool	GetMenuItemCheck(HMENU hMenu, UINT ResID){
	return (GetMenuItemState(hMenu, ResID)&MFS_CHECKED)!=0;
}

void	SetMenuItemEnable(HMENU hMenu, UINT ResID, bool State){
	SetMenuItemState(hMenu, ResID, (State)?MFS_ENABLED:MFS_DISABLED);
}

void	SetMenuItemCheck(HMENU hMenu, UINT ResID, bool State){
	SetMenuItemState(hMenu, ResID, (State)?MFS_CHECKED:MFS_UNCHECKED);
}

void	LockUIObject(HMENU hMenu, HWND hButtonPause){
	EnableWindow(hButtonPause, FALSE);
}


//---------------------------------------------------------------------
//		WndProc - Config
//---------------------------------------------------------------------
void	SetWndInt(HWND hParentWnd, int ResID, int num){
	TCHAR	buf[512];
	int	bufsize	= sizeof(buf);

	sprintf_s(buf,bufsize,"%d",num);
	SetWindowText(GetDlgItem(hParentWnd,ResID),buf);
}
int	GetWndInt(HWND hParentWnd, int ResID){
	TCHAR	buf[512];
	int	bufsize	= sizeof(buf);

	GetWindowText(GetDlgItem(hParentWnd,ResID),buf,bufsize);
	return atoi(buf);
}

INT_PTR CALLBACK	WndProc_Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam){
	UNREFERENCED_PARAMETER(lParam);

	switch (message){
		case WM_INITDIALOG:
			SetWndInt(hDlg, IDC_USEMEMORY_EDIT, UseMemory);

			return (INT_PTR)TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL){
				if(LOWORD(wParam) == IDOK){
					UseMemory	= (UINT)GetWndInt(hDlg, IDC_USEMEMORY_EDIT);
				}

				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}


//---------------------------------------------------------------------
//		WndProc
//---------------------------------------------------------------------
#define	COPY_MODE_VIDEO	0
#define	COPY_MODE_AUDIO	1
#define	COPY_MODE_ALL	2
BOOL func_WndProc( HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam,void *editp,FILTER *fp ){
	//	TRUEを返すと全体が再描画される

	static	int		frame,frame_n,frame_w,frame_h;
	static	HWND		hWndPlay=0,hWndPause=0,hWndStop=0,hWndRead=0;
	static	HMENU		hMenu;
	static	UINT		prevUseMemory;
	static	UINT		lastFrame;
	static	double		nextTime, prevTime, addTime,waitTime;
	UINT			nowFrame	= fp->exfunc->get_frame(editp);

	switch(message) {
		//	描画要求
		case WM_PAINT:
		case WM_FILTER_CHANGE_WINDOW:
			disp(hwnd,fp,editp,frame,frame_n,frame_w,frame_h);
			break;

		case WM_TIMER:
			if(wparam==PREVIEW_TIMER){	// プレビュー
				if(CanPlayMemory){
					frame	= RANGE((UINT)frame+1,PlayStartFrame,lastFrame);

					if((UINT)frame<lastFrame){
						KillTimer(hwnd, PREVIEW_TIMER);

						prevTime		= nextTime;
						double	nowTime		= (double)timeGetTime();
						double	deltaTime	= nowTime-prevTime;
						double	p		= addTime/(addTime+deltaTime);
						int	skip		= (int)max(deltaTime/addTime,0);
						frame			+=skip;
						nextTime		+=(addTime*(skip+1));
						waitTime		= -min((nextTime-nowTime)/p,0);
						//DebugLog("[%03d] Timer : %03dms (%6.3fx) / +%dframe -> Update : %03dms",frame,(int)deltaTime,p,skip,(int)waitTime);

						SetTimer(hwnd, PREVIEW_TIMER, (UINT)waitTime, NULL);

						//disp(hwnd,fp,editp,frame,frame_n,frame_w,frame_h);
					}
					else{	// 再生終了
						KillTimer(hwnd, PREVIEW_TIMER);
						frame	= PlayStartFrame;

						if(isPlayLoop){	// ループ再生
							SendMessage(hwnd, WM_COMMAND, (WPARAM)IDM_PLAY_START, NULL);
						}
					}
					RECT rc	= GetViewPosition(fp, editp);
					//InvalidateRect(hwnd, &rc, FALSE);

					disp(hwnd,fp,editp,frame,frame_n,frame_w,frame_h);
				}
			}
			break;

		//	コマンド
		case WM_COMMAND:{
			int	CommandID	= (int)LOWORD(wparam);

			switch(CommandID){	// 編集外でも動作する	
			case IDM_PLAY_CONFIG:		// 設定
				prevUseMemory	= UseMemory;
				DialogBox(fp->dll_hinst,MAKEINTRESOURCE(IDD_CONFIG),fp->hwnd,WndProc_Config);
				if(prevUseMemory!=UseMemory && fp->exfunc->is_editing(editp)){	// 編集中のみメモリ更新
					UpdateMemory(fp,editp);
					disp(hwnd,fp,editp,frame,frame_n,frame_w,frame_h);
				}

				break;

			case IDM_PLAY_INFORMATION:	// バージョン情報
				ShowInformation(hwnd);
				break;

			case IDM_VIEW_SIZE_PERCENT025:	// 再生サイズ
			case IDM_VIEW_SIZE_PERCENT050:
			case IDM_VIEW_SIZE_PERCENT100:
			case IDM_VIEW_SIZE_PERCENT200:
			case IDM_VIEW_SIZE_PX640:
			case IDM_VIEW_SIZE_PX320:
			case IDM_VIEW_SIZE_PX160:
			case IDM_VIEW_SIZE_WINDOW:
				ViewSize	= (VIEWSIZE)(CommandID-IDM_VIEW_SIZE_PERCENT025);

				UpdateWindowSize(hwnd, frame_w, frame_h);

				CheckMenuRadioItem(hMenu,IDM_VIEW_SIZE_PERCENT025,	IDM_VIEW_SIZE_WINDOW,	CommandID,MF_BYCOMMAND);
				break;

			case IDM_PLAY_PLAY_ALL:		// 再生内容
			case IDM_PLAY_PLAY_VIDEO:
			case IDM_PLAY_PLAY_AUDIO:
				PlayMode	= (PLAYMODE)(CommandID-IDM_PLAY_PLAY_ALL);
				CheckMenuRadioItem(hMenu,IDM_PLAY_PLAY_ALL,		IDM_PLAY_PLAY_AUDIO,	CommandID,MF_BYCOMMAND);
				break;

			case IDM_PLAY_SELECT:		// 選択範囲のみ再生
				isPlaySelect	= !isPlaySelect;
				SetMenuItemCheck(hMenu, IDM_PLAY_SELECT,isPlaySelect);
				DrawMenuBar(hwnd);
				break;
			case IDM_PLAY_LOOP:		// ループ再生
				isPlayLoop	= !isPlayLoop;
				SetMenuItemCheck(hMenu, IDM_PLAY_LOOP,	isPlayLoop);
				DrawMenuBar(hwnd);
				break;
				

			}	// switch(CommandID)


			if( fp->exfunc->is_editing(editp) != TRUE ) break;
			switch(CommandID) {	// 編集中のみ有効な項目
				case IDC_BUTTON_READ:	// RAM読み込み
				case IDM_PLAY_READ:
					frame_n = fp->exfunc->get_frame_n(editp);

					if(!isPrepareMemory){
						if(!UpdateMemory(fp, editp)){
							break;
						}
					}

					if(!isReading){
						UpdateSize(fp, editp, nowFrame, &frame_w, &frame_h);
						RefreshBG(hwnd);

						BeginReadDataSubThread(fp, editp);
						PlayStartFrame	= nowFrame;
						frame		= nowFrame;
						lastFrame	= PlayStartFrame+ReadFrame;
						
						//disp(hwnd,fp,editp,frame,frame_n,frame_w,frame_h);
					}
					SetFocus(hwnd);
					break;

				case IDC_BUTTON_PLAY:
				case IDM_PLAY_START:	// 再生開始

					if(!isReading){
						if(isPlaying){
							PlaySound(NULL,NULL,0);
							KillTimer(hwnd, PREVIEW_TIMER);
							frame	= PlayStartFrame;
						}

						//DebugLog("[Start] (%dx%d)",frame_w,frame_h);

						isPlaying	= true;
						disp(hwnd,fp,editp,frame,frame_n,frame_w,frame_h);
						if(CanPlayMemory && (UINT)frame<lastFrame){
							frame	= PlayStartFrame;
							if(PlayMode==PLAY_ALL || PlayMode==PLAY_AUDIO){
								PlaySound((LPCSTR)AudioMemory,NULL,SND_MEMORY|SND_ASYNC);
							}
							// 映像はdispではじく / ウィンドウ更新もあるので
							addTime		= (1000/GetFPS(fp,editp));
							//DebugLog("FPS : %fms / %ffps",addTime,GetFPS(fp,editp));
							nextTime	= timeGetTime() + addTime;
							SetTimer(hwnd, PREVIEW_TIMER, (UINT)addTime, NULL);
						}
					}
					else{
						SendMessage(hwnd, WM_COMMAND, (WPARAM)IDM_PLAY_STOP,	NULL);
						SendMessage(hwnd, WM_COMMAND, (WPARAM)IDM_PLAY_START,	NULL);
					}
					SetFocus(hwnd);
					break;

				case IDC_BUTTON_STOP:
				case IDM_PLAY_STOP:
				{
					if(!isReading){	// 再生停止
						isPlaying	= false;
						frame		= PlayStartFrame;
						disp(hwnd,fp,editp,frame,frame_n,frame_w,frame_h);
						PlaySound(NULL,NULL,0);
						KillTimer(hwnd, PREVIEW_TIMER);
					}
					else{		// 読み込み停止
						EndReadDataSubThread();
						lastFrame	= PlayStartFrame+ReadCompleteFrame;
					}
					SetFocus(hwnd);
					break;
				}

				case IDM_PLAY_CLEARMEMORY:	// メモリ解放
					SendMessage(hwnd, WM_COMMAND, (WPARAM)IDM_PLAY_STOP,	NULL);	// 読み込み停止
					ReleaseMemory();						// メモリ解放
					UpdateSize(fp, editp, nowFrame, &frame_w, &frame_h);		// 画面の更新
					RefreshBG(hwnd);						// 
					disp(hwnd,fp,editp,nowFrame,frame_n,frame_w,frame_h);		// 
					break;

			}	// switch(CommandID)

			break;
		}	// switch(WM_COMMAND)

		case WM_READDATA_END:{	// RAM読み込みが終了すると送られてくる
			EndReadDataSubThread();
			lastFrame	= PlayStartFrame+ReadCompleteFrame;
			disp(hwnd,fp,editp,frame,frame_n,frame_w,frame_h);
			break;
		}

		//	キー入力
		case WM_KEYDOWN:
			if(CanPlayMemory){
				int	f	= 0;
				switch(LOWORD(wparam)) {
				case VK_RIGHT:
					f	= 1;
					break;
				case VK_LEFT:
					f	= -1;
					break;
				}
				frame	= RANGE(frame+f,(int)PlayStartFrame,(int)lastFrame-1);
				disp(hwnd,fp,editp,frame,frame_n,frame_w,frame_h);
			}
			break;

		case WM_SIZE:
			SetButtonPosition(hwnd, hWndPlay, hWndPause, hWndStop, hWndRead);
			break;

		case WM_FILTER_UPDATE:
			//frame_n		= fp->exfunc->get_frame_n(editp);
			break;
			

		case WM_FILTER_FILE_OPEN:
			UpdateSize(fp, editp, nowFrame, &frame_w, &frame_h);

			CanPlayMemory	= false;
			isPlaying	= false;
			isPrepareMemory	= false;
			frame		= nowFrame;
			frame_n		= fp->exfunc->get_frame_n(editp);

			disp(hwnd,fp,editp,frame,frame_n,frame_w,frame_h);

			break;

		case WM_FILTER_FILE_CLOSE:
			frame = frame_n = 0;
			disp(hwnd,fp,editp,frame,frame_n,frame_w,frame_h);
			ReleaseMemory();

			// INI 書き込み
			WriteIniData(fp);

			CanPlayMemory	= false;
			isPlaying	= false;
			isPrepareMemory	= false;

			break;

		case WM_FILTER_INIT:{
			hMenu = LoadMenu(fp->dll_hinst,"IDM_RAMPREVIEW");
			SetMenu(hwnd,hMenu);

			// ボタン類
			hWndPlay	= CreateIconButton(hwnd,0,0,BUTTON_SIZE,BUTTON_SIZE,IDC_BUTTON_PLAY	,fp->dll_hinst,"ICON_PLAY");
			hWndPause	= CreateIconButton(hwnd,0,0,BUTTON_SIZE,BUTTON_SIZE,IDC_BUTTON_PAUSE	,fp->dll_hinst,"ICON_PAUSE");
			hWndStop	= CreateIconButton(hwnd,0,0,BUTTON_SIZE,BUTTON_SIZE,IDC_BUTTON_STOP	,fp->dll_hinst,"ICON_STOP");
			hWndRead	= CreateIconButton(hwnd,0,0,BUTTON_SIZE,BUTTON_SIZE,IDC_BUTTON_READ	,fp->dll_hinst,"ICON_READ");

			SetButtonPosition(hwnd, hWndPlay, hWndPause, hWndStop, hWndRead);

			// INI読み込み
			ReadIniData(fp);

			// ラジオボタン
			CheckMenuRadioItem(hMenu, IDM_VIEW_SIZE_PERCENT025,	IDM_VIEW_SIZE_WINDOW,	NUMDATATORESID_SIZE(ViewSize),	MF_BYCOMMAND);
			CheckMenuRadioItem(hMenu, IDM_PLAY_PLAY_ALL,		IDM_PLAY_PLAY_AUDIO,	NUMDATATORESID_PLAY(PlayMode),	MF_BYCOMMAND);
			// チェック
			SetMenuItemCheck(hMenu, IDM_PLAY_SELECT,isPlaySelect);
			SetMenuItemCheck(hMenu, IDM_PLAY_LOOP,	isPlayLoop);

			LockUIObject(hMenu,hWndPause);

			DrawMenuBar(hwnd);

			break;
		}

		case WM_FILTER_EXIT:
			WriteIniData(fp);
			break;
	}

	return FALSE;
}


