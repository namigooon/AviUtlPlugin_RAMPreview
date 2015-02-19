//----------------------------------------------------------------------------------------------------
// View.h
//	2015/02/19 - 2015/02/19
//			hksy
//----------------------------------------------------------------------------------------------------

#pragma	once
#ifndef	_H_APF_RAMPREVIEW_VIEW
#define	_H_APF_RAMPREVIEW_VIEW

//--------------------------------------------------

#include	<Windows.h>
#include	<MMSystem.h>
#include	"rampreview.h"

//--------------------------------------------------

class	RAMImage;
struct	RP_CONFIG;

class View{
	private:
		HWND		hWnd;		// プレビューウィンドウのハンドル
		RAMImage	*Ram;		// RAMデータ
		RP_CONFIG	*Config;	// 設定（常に反映するため参照）
		double		ProjectFPS;	// プロジェクトのFPS
		UINT		ViewingFrame;	// 表示しているフレーム
		bool		isPlaying;	// true = 再生中

		static void	Wait(HWND hWnd, DWORD WaitTime);
		RECT	GetViewPosition();
		void	Display(BYTE *pixelp);
		void	DisplayPlaybar(int frame, int totalframe);
		void	ErasePlaybar(HDC hDC, HPEN hPenNull, HBRUSH hBrushBG);
		void	UpdateWindowCaption(int frame, FILTER *fp, void *editp);

	public:
		View(HWND hWnd, RAMImage *Ram, RP_CONFIG *Config);

		bool	Play(FILTER *fp, void *editp);
		void	Stop();
		bool	PrevFrame(FILTER *fp, void *editp);
		bool	NextFrame(FILTER *fp, void *editp);
		UINT	GetViewingFrame() const;

		void	EraseImage();
		void	DisplayFrame(int frame, FILTER *fp, void *editp);
		bool	DisplayRam(int frame, FILTER *fp, void *editp);
		bool	UpdateRam(FILTER *fp, void *editp);
		bool	Playing() const;

};

//--------------------------------------------------

#endif	// _H_APF_RAMPREVIEW_VIEW


