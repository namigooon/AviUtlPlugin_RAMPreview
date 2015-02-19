//----------------------------------------------------------------------------------------------------
// View.cpp
//	2015/02/19 - 2015/02/19
//			hksy
//----------------------------------------------------------------------------------------------------

#include	"View.h"

//--------------------------------------------------
// class View
//--------------------------------------------------

View::View(HWND hWnd, RAMImage *Ram, RP_CONFIG *Config){
	this->hWnd	= hWnd;
	this->Ram	= Ram;
	this->Config	= Config;

	this->ProjectFPS	= 0;
	this->ViewingFrame	= 0;
	this->isPlaying		= false;
}

void	View::Wait(HWND hWnd, DWORD waitTime){
	// 指定ミリ秒ウィンドウ処理を行いながら待機します[ms]
	// オーバーフロー耐性あるつもりだけど未確認

	DWORD	startTime	= timeGetTime();
	DWORD	nowTime		= startTime;
	MSG	msg;

	while((nowTime-waitTime)<startTime){
		//DebugLog("  wait	: (%d-%d[%d])<%d", nowTime,waitTime,(nowTime-waitTime),startTime);
		if(PeekMessage(&msg, NULL, 0,0, PM_REMOVE)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else{
			Sleep(1);
		}
		nowTime	= timeGetTime();
	}
}

RECT	View::GetViewPosition(){
	// 画像表示座標の取得

	SIZE		readsize	= Ram->GetScreenSize();
	int		screen_w	= readsize.cx;
	int		screen_h	= readsize.cy;
	RECT		r;
	const VIEWSIZE	ViewSize	= Config->ViewSize;
	const PLAYMODE	PlayMode	= Config->PlayMode;

	GetClientRect(this->hWnd,&r);
	r.bottom	-=WINDOW_BOTTOM;

	// 画面サイズの計算
	double	wx	= (double)r.right /screen_w;
	double	hx	= (double)r.bottom/screen_h;
	double	p	= 1.0;	// 表示倍率

	if(Config->ViewSize==SIZE_WINDOW){					// window
		p	= min(wx,hx);
	}
	else if(SIZE_PERCENT025<=ViewSize && ViewSize<=SIZE_PERCENT200){	// %
		p	= Range(min(wx,hx), 0.0, SizePerList[ViewSize-SIZE_PERCENT025]/100.0);
	}
	else if(SIZE_PX640<=ViewSize && ViewSize<=SIZE_PX160){			// px
		p	= Range(min(wx,hx), 0.0, (double)SizePxList[ViewSize-SIZE_PX640]/screen_w);
	}
	r.left		= (int)((r.right -screen_w*p)/2);
	r.top		= (int)((r.bottom-screen_h*p)/2);
	r.right		= (int)(screen_w*p);
	r.bottom	= (int)(screen_h*p);

	return r;
}

void	View::Display(BYTE *pixelp){
	RECT		r		= this->GetViewPosition();
	SIZE		imgsize		= this->Ram->GetScreenSize();
	BITMAPINFO	bmi		= this->Ram->GetBitmapInfo();

	HDC		hDC		= GetDC(this->hWnd);
	HPEN		hPenNull	= (HPEN)GetStockObject(NULL_PEN);
	SetStretchBltMode(hDC,COLORONCOLOR);

	StretchDIBits(hDC,
		r.left, r.top, r.right, r.bottom,
		0, 0, imgsize.cx, imgsize.cy,
		pixelp, &bmi, DIB_RGB_COLORS, SRCCOPY
	);

	DeleteObject(hPenNull);

	ReleaseDC(this->hWnd,hDC);
}
void	View::DisplayPlaybar(int frame, int totalframe){
	// フレーム情報をウィンドウ下部に表示

	HDC		hDC		= GetDC(this->hWnd);
	HPEN		hPenNull	= (HPEN)GetStockObject(NULL_PEN);
	HBRUSH		hBrushBG	= (HBRUSH)CreateSolidBrush( GetSysColor( COLOR_3DFACE ) );
	RECT		RectRead;
	RECT		r;

	UINT		ReadStartFrame		= this->Ram->GetReadStartFrame();
	UINT		ReadCompleteFrame	= this->Ram->GetReadCompleteFrame();

	GetClientRect(this->hWnd, &r);
	r.right++;
	r.bottom++;
	RectRead.left	= (LONG)(((ReadStartFrame                  )/(double)totalframe)*r.right);
	RectRead.right	= (LONG)(((ReadStartFrame+ReadCompleteFrame)/(double)totalframe)*r.right);
	RectRead.top	= r.bottom-READFRAME_HEIGHT;
	RectRead.bottom	= r.bottom;

	LONG	CurrentPos	= (LONG)(((frame                   )/(double)totalframe)*r.right);

	HPEN	hPenView	= CreatePen(PS_SOLID,1,COLOR_VIEWFRAME);
	HBRUSH	hBrushRead	= CreateSolidBrush(COLOR_READFRAME);

	this->ErasePlaybar(hDC, hPenNull, hBrushBG);	// 消去
	SelectObject(hDC, hPenNull);
	SelectObject(hDC, hBrushRead);	Rectangle(hDC, RectRead.left,	RectRead.top,	RectRead.right,	RectRead.bottom);
	SelectObject(hDC, hPenView);	MoveToEx(hDC, CurrentPos, RectRead.top, NULL);	LineTo(hDC, CurrentPos, RectRead.bottom);
	DeleteObject(hBrushRead);
	DeleteObject(hPenView);
	DeleteObject(hBrushBG);
	DeleteObject(hPenNull);

	ReleaseDC(this->hWnd,hDC);
}

void	View::EraseImage(){
	// 画像表示領域を背景色で塗りつぶし

	RECT	r;
	GetClientRect(this->hWnd, &r);

	HDC	hDC		= GetDC(this->hWnd);
	HPEN	hPenNull	= (HPEN)GetStockObject(NULL_PEN);
	HBRUSH	hBrushBG	= (HBRUSH)CreateSolidBrush( GetSysColor( COLOR_3DFACE ) );

	SelectObject(hDC, hPenNull);
	SelectObject(hDC, hBrushBG);
	Rectangle(hDC, r.left, r.top, r.right+1, r.bottom-WINDOW_BOTTOM+1);
	this->ErasePlaybar(hDC, hPenNull, hBrushBG);

	// 描画終了
	DeleteObject(hPenNull);
	DeleteObject(hBrushBG);
	ReleaseDC(this->hWnd, hDC);
}
void	View::ErasePlaybar(HDC hDC, HPEN hPenNull, HBRUSH hBrushBG){
	RECT	r;
	GetClientRect(this->hWnd, &r);
	r.right++;
	r.bottom++;
	r.top	= r.bottom-READFRAME_HEIGHT;

	SelectObject(hDC, hPenNull);
	SelectObject(hDC, hBrushBG);
	Rectangle(hDC, r.left, r.top, r.right, r.bottom);
}
void	View::DisplayFrame(int frame, FILTER *fp, void *editp){
	// 直接フレーム画像を取得し表示

	if(!fp->exfunc->is_editing(editp)){
		return;
	}

	int	w,h;
	fp->exfunc->get_pixel_filtered(editp, frame, NULL, &w, &h);

	BYTE	*pixelp	= new BYTE[w*h];
	fp->exfunc->get_pixel_filtered(editp, frame, pixelp, NULL, NULL);

	int	totalframe	= fp->exfunc->get_frame_n(editp);

	this->Display(pixelp);
	this->DisplayPlaybar(frame, totalframe);

	this->UpdateWindowCaption(frame, fp, editp);
	this->ViewingFrame	= 0;

	delete[] pixelp;
}
bool	View::DisplayRam(int frame, FILTER *fp, void *editp){
	// RAMから画像を取得し表示

	if(this->Ram->isRoadedFrame(frame)){	// 読み込み済み
		BYTE	*pixelp		= this->Ram->GetFrame(frame);
		if(pixelp==NULL){
			return false;
		}

		int	totalframe	= fp->exfunc->get_frame_n(editp);

		this->Display(pixelp);
		this->DisplayPlaybar(frame, totalframe);

		this->ViewingFrame	= frame;

		this->UpdateWindowCaption(frame, fp, editp);

		return true;
	}
	else{					// 読み込み範囲外
		return false;
	}

}
bool	View::UpdateRam(FILTER *fp, void *editp){
	return this->DisplayRam(this->GetViewingFrame(), fp, editp);
}
void	View::UpdateWindowCaption(int frame, FILTER *fp, void *editp){
	// ウィンドウ名更新

	TCHAR	caption[512];
	int	totalframe	= fp->exfunc->get_frame_n(editp);
	int	use		= 0;

	if( fp->exfunc->is_editing(editp) && totalframe!=0 ) {
		double	fps		= GetFPS(fp, editp);
		double	time		= frame/fps;

		use	= _stprintf_s(caption, sizeof(caption), "%s [%02d:%02d:%02d.%02d] [%d/%d]",filter.name,
			(int)(time/3600)%60,(int)(time/60)%60,(int)(time)%60,(int)((time-(int)time)*100),
			frame+1,totalframe
		);
		if(this->Ram->Reading()){	// 読み込み中は進捗状況も表示
			TCHAR buf[MAX_PATH];
			_stprintf_s(buf," [%d/%d]",this->Ram->GetReadCompleteFrame(),this->Ram->GetMaxReadFrame());
			_tcscat_s(caption, sizeof(caption)-use, buf);
		}
	}
	else{	// 編集中でなければフィルタ名
		_stprintf_s(caption, sizeof(caption), "%s", filter.name);
	}

	SetWindowText(this->hWnd, caption);
}

bool	View::Play(FILTER *fp, void *editp){
	DWORD	startTime	= timeGetTime();
	int	playFrame	= this->Ram->GetReadCompleteFrame();
	int	startFrame	= this->Ram->GetReadStartFrame();
	int	nowFrame	= startFrame;
	int	endFrame	= nowFrame + playFrame;	// now < end

	if(!this->Ram->Ready()){
		return false;
	}

	this->isPlaying		= true;
	this->ProjectFPS	= GetFPS(fp, editp);

	while(this->isPlaying){
		// 再生終了
		if(nowFrame>=endFrame){
			if(this->Config->isPlayLoop){	// ループ再生
				nowFrame	= startFrame;
				startTime	= timeGetTime();
				nowFrame	= startFrame;
				endFrame	= nowFrame + playFrame;
			}
			else{				// 再生終了
				break;
			}
		}

		// 開始フレーム
		if(startFrame==nowFrame){
			// 背景消去
			this->EraseImage();

			// 音声再生
			if(this->Config->PlayMode==PLAY_ALL || this->Config->PlayMode==PLAY_AUDIO){
				PlaySound((LPCSTR)this->Ram->GetMemAudioFile(), NULL, SND_MEMORY|SND_ASYNC);
			}
		}

		// FPS制御
		//   オーバーフロー耐性はないけど1フレーム落ちるだけ？未確認
		nowFrame++;
		int	frame		= nowFrame - startFrame;	// index
		DWORD	nextTime	= startTime + (DWORD)(frame*1000.0/this->ProjectFPS);
		DWORD	nowTime		= timeGetTime();
		DWORD	waitTime	= nextTime-nowTime;

		//DebugLog("Play	: [%03d] wait %d ms ( %d -> %d )", frame-1, waitTime, nowTime,nextTime);

		if(nextTime>nowTime){	// 表示する余裕がある
			// 画面表示
			PLAYMODE	play	= this->Config->PlayMode;
			if(play==PLAY_ALL || play==PLAY_VIDEO){
				this->DisplayRam(nowFrame, fp, editp);
			}

			this->Wait(this->hWnd, waitTime);
		}
	}

	// 再生終了
	this->isPlaying	= false;
	PlaySound(NULL,NULL,0);

	this->DisplayRam(startFrame, fp, editp);

	return true;
}
void	View::Stop(){
	this->isPlaying	= false;
	PlaySound(NULL,NULL,0);
}
bool	View::PrevFrame(FILTER *fp, void *editp){
	// 次のフレームを表示

	UINT	frame	= this->ViewingFrame-1;
	bool	range	= this->Ram->isRoadedFrame(frame);

	if(range){	// RAMに読み込んでいるか
		this->ViewingFrame	= frame;
		DisplayRam(frame, fp, editp);
		return true;
	}
	else{
		return false;
	}
}
bool	View::NextFrame(FILTER *fp, void *editp){
	// 次のフレームを表示

	UINT	frame	= this->ViewingFrame+1;
	bool	range	= this->Ram->isRoadedFrame(frame);

	if(range){	// RAMに読み込んでいるか
		this->ViewingFrame	= frame;
		DisplayRam(frame, fp, editp);
		return true;
	}
	else{
		return false;
	}
}
UINT	View::GetViewingFrame() const{
	// 現在RAMで表示しているフレームを取得
	return this->ViewingFrame;
}
bool	View::Playing() const{
	// 現在再生中か
	return this->isPlaying;
}


