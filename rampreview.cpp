//----------------------------------------------------------------------------------------------------
// RAM Preview
// 	Ver. 0.03beta
// 
//	2014/02/28 - 2015/02/19
//			hksy
//----------------------------------------------------------------------------------------------------


#include	"rampreview.h"

TCHAR		*FilterName	= "RAM�v���r���[";
TCHAR		*FilterInfo	= "RAM�v���r���[ version 0.03 by hksy";

RAMImage	*RamImage;
View		*Viewer;
RP_CONFIG	Config;

const DWORD	WindowStyle	= 0x14CC0000;
const DWORD	WindowExStyle	= 0x00000180;
const DWORD	COLOR_READFRAME	= RGB(0x00,0xFF,0x00);
const DWORD	COLOR_VIEWFRAME	= RGB(0xFF,0x00,0x00);

const int	SizePerList[]	= {25,50,100,200};
const int	SizePxList[]	= {640,320,160};



//---------------------------------------------------------------------
//		�t�B���^�\���̒�`
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
//		�t�B���^�\���̂̃|�C���^��n���֐�
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

int	NUMDATATORESID_QUALITY(int num){
	// num��PREVIEWQUALITY�ɑ�����l�ł���΂��̃C���f�b�N�X�l�i=���j���[�̈ʒu�j��ԋp

	int	qList[]	= {PQ_ORIGINAL, PQ_HALF, PQ_QUARTER};
	int	qLSize	= ARRAYSIZE(qList);
	for(int i=0;i<qLSize;i++){
		if(num==qList[i]){
			return i;
		}
	}
	return -1;
}
PREVIEWQUALITY	RESIDTONUM_QUALITY(int ResID){
	PREVIEWQUALITY	qList[]	= {PQ_ORIGINAL, PQ_HALF, PQ_QUARTER};
	if(isRange(ResID, IDM_PLAY_QUALITY_ORIGINAL, IDM_PLAY_QUALITY_QUARTER, true, true)){
		return qList[ResID-IDM_PLAY_QUALITY_ORIGINAL];
	}
	else{
		return PQ_USER;
	}
}



//---------------------------------------------------------------------
//		�������Ǘ�
//---------------------------------------------------------------------

void	UpdateMemorySize(UINT ConfSize){
	UINT	NewSize	= ConfSize<<USEMEMORY_UNIT;
	RamImage->SetVideoMemorySize(NewSize);
}



//---------------------------------------------------------------------
//		�\��
//---------------------------------------------------------------------

SIZE	getWindowSize(int width, int height, DWORD style, DWORD exstyle, BOOL menu){
	RECT	rect;
	SIZE	size;

	SetRect(&rect,0,0,width,height);
	AdjustWindowRectEx(&rect, style, menu, exstyle);
	size.cx	= rect.right  - rect.left;
	size.cy	= rect.bottom - rect.top;

	return size;
}

HWND	CreateIconButton(HWND hParentWnd, int x, int y, int w, int h, int ResID, HINSTANCE hInst, LPCTSTR IcoName){
	HWND	hWnd	= CreateWindow("BUTTON","",WS_CHILD | WS_VISIBLE | BS_ICON,
		x,y,w,h,hParentWnd,(HMENU)ResID,hInst,NULL
	);
	HICON	hIcon	= (HICON)LoadImage(hInst,IcoName,IMAGE_ICON, 0,0, LR_DEFAULTCOLOR);
	SendMessage(hWnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
	return hWnd;
}

void	SetButtonPosition(HWND hWnd, HWND hWndPlay, HWND hWndStop, HWND hWndRead){
	RECT	rc;
	GetClientRect(hWnd,&rc);

	int	x,y;
	int	paddingw	= BUTTON_PADDING;
	int	paddingh	= (WINDOW_BOTTOM-BUTTON_SIZE)/2;

	x	= rc.right;
	y	= rc.bottom-WINDOW_BOTTOM+paddingh;

	#define	SETBUTTONPOS(hWnd, x,y)	if(hWnd!=NULL){x-=(BUTTON_SIZE+BUTTON_PADDING);MoveWindow(hWnd, x,y, BUTTON_SIZE,BUTTON_SIZE,TRUE);}

	SETBUTTONPOS(hWndStop,	x,y);
	SETBUTTONPOS(hWndPlay,	x,y);
	SETBUTTONPOS(hWndRead,	x,y);

	#undef	SETBUTTONPOS
}

void	UpdateWindowSize(HWND hwnd, int frame_w, int frame_h){
	VIEWSIZE	ViewSize	= Config.ViewSize;
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

void	ShowInformation(HINSTANCE hInst, HWND hWnd){
	TCHAR		buf[512];
	TCHAR		fmt[512];
	const double	b	= 1<<USEMEMORY_UNIT;	// to MB

	LoadString(hInst, IDS_MEMORY_INFO, fmt, sizeof(fmt));

	_stprintf_s(buf, sizeof(buf), fmt,
		RamImage->GetVideoUseMemory()/b,
		RamImage->GetAudioUseMemory()/b,
		RamImage->GetMaxReadFrame()
	);

	MessageBox(hWnd,buf,_T(filter.name),MB_OK|MB_ICONINFORMATION);
}



//---------------------------------------------------------------------
//		�ݒ�t�@�C��
//---------------------------------------------------------------------

void	ReadIniData(FILTER *fp){
	Config.ViewSize		= (VIEWSIZE)		fp->exfunc->ini_load_int(fp,"ViewSize",		SIZE_WINDOW);
	Config.PlayMode		= (PLAYMODE)		fp->exfunc->ini_load_int(fp,"PlayMode",		PLAY_ALL);
	Config.isPlaySelect	=			fp->exfunc->ini_load_int(fp,"PlaySelect",	FALSE)!=FALSE;
	Config.isPlayLoop	=			fp->exfunc->ini_load_int(fp,"PlayLoop",		FALSE)!=FALSE;
	Config.PreviewQuality	= (PREVIEWQUALITY)	fp->exfunc->ini_load_int(fp,"PlaviewQuality",	PQ_ORIGINAL);
	Config.UseMemory	=			fp->exfunc->ini_load_int(fp,"UseMemory",	DEF_USEMEMORY);
}
void	WriteIniData(FILTER *fp){
	fp->exfunc->ini_save_int(fp,"ViewSize",		Config.ViewSize);
	fp->exfunc->ini_save_int(fp,"PlayMode",		Config.PlayMode);
	fp->exfunc->ini_save_int(fp,"PlaySelect",	(Config.isPlaySelect	)?TRUE:FALSE);
	fp->exfunc->ini_save_int(fp,"PlayLoop",		(Config.isPlayLoop	)?TRUE:FALSE);
	fp->exfunc->ini_save_int(fp,"PlaviewQuality",	Config.PreviewQuality);
	fp->exfunc->ini_save_int(fp,"UseMemory",	Config.UseMemory);
}



//---------------------------------------------------------------------
//		���j���[����
//---------------------------------------------------------------------

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



//---------------------------------------------------------------------
//		�E�B���h�E����
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



//---------------------------------------------------------------------
//		WndProc - Config
//---------------------------------------------------------------------

INT_PTR CALLBACK	WndProc_Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam){
	UNREFERENCED_PARAMETER(lParam);

	switch (message){
		case WM_INITDIALOG:
			SetWndInt(hDlg, IDC_USEMEMORY_EDIT, Config.UseMemory);

			return (INT_PTR)TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL){
				if(LOWORD(wParam) == IDOK){
					Config.UseMemory	= (UINT)GetWndInt(hDlg, IDC_USEMEMORY_EDIT);
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
	//	TRUE��Ԃ��ƑS�̂��ĕ`�悳���

	static	HWND	hWndPlay=0, hWndStop=0, hWndRead=0;
	static	HMENU	hMenu;
	UINT		nowFrame	= fp->exfunc->get_frame(editp);

	switch(message) {
		//	�`��v��
		case WM_PAINT:
		case WM_FILTER_CHANGE_WINDOW:{
			int	frame_n	= fp->exfunc->get_frame_n(editp);
			if(frame_n>0 && Viewer!=NULL){
				Viewer->UpdateRam(fp, editp);
				//Viewer->DisplayFrame(nowFrame, fp, editp);
			}
			break;
		}

		//	�R�}���h
		case WM_COMMAND:{
			int	CommandID	= (int)LOWORD(wparam);

			switch(CommandID){	// �ҏW�O�ł����삷��	
				case IDM_PLAY_CONFIG:{		// �ݒ�
						RP_CONFIG	prevConfig	= Config;
						DialogBox(fp->dll_hinst,MAKEINTRESOURCE(IDD_CONFIG),fp->hwnd,WndProc_Config);
						if(prevConfig.UseMemory!=Config.UseMemory && fp->exfunc->is_editing(editp)){	// �ҏW���̂݃������X�V
							RamImage->SetVideoMemorySize(Config.UseMemory);
						}
					}
					break;

				case IDM_PLAY_INFORMATION:	// �o�[�W�������
					ShowInformation(fp->dll_hinst, hwnd);
					break;

				case IDM_VIEW_SIZE_PERCENT025:	// �Đ��T�C�Y
				case IDM_VIEW_SIZE_PERCENT050:
				case IDM_VIEW_SIZE_PERCENT100:
				case IDM_VIEW_SIZE_PERCENT200:
				case IDM_VIEW_SIZE_PX640:
				case IDM_VIEW_SIZE_PX320:
				case IDM_VIEW_SIZE_PX160:
				case IDM_VIEW_SIZE_WINDOW:{
						Config.ViewSize	= (VIEWSIZE)(CommandID-IDM_VIEW_SIZE_PERCENT025);

						SIZE	size	= RamImage->GetOriginalScreenSize();
						if(size.cx==0 || size.cy==0){
							GetScreenSize(fp, editp, nowFrame, (int*)&size.cx, (int*)&size.cy);
						}
						UpdateWindowSize(hwnd, size.cx, size.cy);

						CheckMenuRadioItem(hMenu,IDM_VIEW_SIZE_PERCENT025,	IDM_VIEW_SIZE_WINDOW,	CommandID,MF_BYCOMMAND);
					}
					break;

				case IDM_PLAY_PLAY_ALL:		// �Đ����e
				case IDM_PLAY_PLAY_VIDEO:
				case IDM_PLAY_PLAY_AUDIO:
					Config.PlayMode	= (PLAYMODE)(CommandID-IDM_PLAY_PLAY_ALL);
					CheckMenuRadioItem(hMenu,IDM_PLAY_PLAY_ALL,		IDM_PLAY_PLAY_AUDIO,	CommandID,MF_BYCOMMAND);
					break;

				case IDM_PLAY_QUALITY_ORIGINAL:	// �掿
				case IDM_PLAY_QUALITY_HALF:
				case IDM_PLAY_QUALITY_QUARTER:
					Config.PreviewQuality	= RESIDTONUM_QUALITY(CommandID);
					RamImage->SetVideoQuality(Config.PreviewQuality);
					CheckMenuRadioItem(hMenu,IDM_PLAY_QUALITY_ORIGINAL,	IDM_PLAY_QUALITY_QUARTER,CommandID,MF_BYCOMMAND);
					break;

				case IDM_PLAY_SELECT:		// �I��͈͂̂ݍĐ�
					Config.isPlaySelect	= !Config.isPlaySelect;
					SetMenuItemCheck(hMenu, IDM_PLAY_SELECT,Config.isPlaySelect);
					DrawMenuBar(hwnd);
					break;
				case IDM_PLAY_LOOP:		// ���[�v�Đ�
					Config.isPlayLoop	= !Config.isPlayLoop;
					SetMenuItemCheck(hMenu, IDM_PLAY_LOOP,	Config.isPlayLoop);
					DrawMenuBar(hwnd);
					break;
				

			}	// switch(CommandID)

			if( fp->exfunc->is_editing(editp) != TRUE ) break;
			switch(CommandID) {	// �ҏW���̂ݗL���ȍ���
				case IDC_BUTTON_READ:	// RAM�ǂݍ���
				case IDM_PLAY_READ:
					SetFocus(hwnd);

					if(Viewer->Playing()){		// �Đ����Ȃ�~�߂�
						Viewer->Stop();
					}
					if(!RamImage->Reading()){	// �ǂݍ��݂��Ă��Ȃ�
						if(RamImage->ReadReady()){
							UpdateMemorySize(Config.UseMemory);
						}

						Viewer->EraseImage();

						UINT	readFrame	= nowFrame;
						int	readLength	= RP_READMAX;
						int	select_s, select_e;
						if(Config.isPlaySelect){	// �͈͑I�����Đ�
							if(fp->exfunc->get_select_frame(editp, &select_s, &select_e)){
								readFrame	= select_s;
								readLength	= select_e-select_s;
							}
						}

						RamImage->ReadStart(fp, editp, readFrame, readLength);
					}
					break;

				case IDC_BUTTON_PLAY:
				case IDM_PLAY_START:	// �Đ��J�n
					SetFocus(hwnd);

					if(!RamImage->Reading()){	// �ǂݍ��݊���
						Viewer->Play(fp, editp);
					}
					else{				// �ǂݍ��ݒ�
						// �ǂݍ��ݒ�~���b�Z�[�W�𑗂�A�ēx�Đ�
						SendMessage(hwnd, WM_COMMAND, (WPARAM)IDM_PLAY_STOP,	NULL);
						SendMessage(hwnd, WM_COMMAND, (WPARAM)IDM_PLAY_START,	NULL);
					}
					break;

				case IDC_BUTTON_STOP:
				case IDM_PLAY_STOP:{
					SetFocus(hwnd);

					if(!RamImage->Reading()){	// �Đ���~
						Viewer->Stop();
					}
					else{				// �ǂݍ��ݒ�~
						RamImage->ReadStop();
					}
					break;
				}

				case IDM_PLAY_CLEARMEMORY:	// ���������
					RamImage->ReleaseMemory();	// ���������
					Viewer->EraseImage();		// �\������
					break;

			}	// switch(CommandID)

			break;
		}	// switch(WM_COMMAND)

		case WM_READDATA_FRAME:{	// RAM��1�t���[���ǂނ��Ƃɑ����Ă���
			UINT	frame_r	= (UINT)wparam;	// �ǂݍ��񂾃t���[���ԍ�			
			Viewer->DisplayRam(frame_r, fp, editp);

			break;
		}
		case WM_READDATA_END:{		// RAM�ǂݍ��݂��I������Ƒ����Ă���
			RamImage->ReadStop();	// �ǂݍ��݃X���b�h�̒�~
			Viewer->DisplayRam(RamImage->GetReadStartFrame(), fp, editp);	// �ŏ��̃t���[���ɖ߂�

			break;
		}

		//	�L�[����
		case WM_KEYDOWN:
			if(RamImage->Ready()){
				switch(LOWORD(wparam)){
					case VK_LEFT:
						Viewer->PrevFrame(fp, editp);
						break;
					case VK_RIGHT:
						Viewer->NextFrame(fp, editp);
						break;
				}
			}
			break;

		case WM_SIZE:
			SetButtonPosition(hwnd, hWndPlay, hWndStop, hWndRead);
			break;

		case WM_FILTER_FILE_CLOSE:
			// INI ��������
			WriteIniData(fp);

			RamImage->ReadStop();
			RamImage->ReleaseMemory();
			Viewer->EraseImage();

			break;

		case WM_FILTER_INIT:{
			hMenu	= LoadMenu(fp->dll_hinst,MAKEINTRESOURCE(IDM_RAMPREVIEW));
			SetMenu(hwnd,hMenu);

			// �{�^����
			hWndPlay	= CreateIconButton(hwnd,0,0,BUTTON_SIZE,BUTTON_SIZE,IDC_BUTTON_PLAY	,fp->dll_hinst,"ICON_PLAY");
			hWndStop	= CreateIconButton(hwnd,0,0,BUTTON_SIZE,BUTTON_SIZE,IDC_BUTTON_STOP	,fp->dll_hinst,"ICON_STOP");
			hWndRead	= CreateIconButton(hwnd,0,0,BUTTON_SIZE,BUTTON_SIZE,IDC_BUTTON_READ	,fp->dll_hinst,"ICON_READ");

			SetButtonPosition(hwnd, hWndPlay, hWndStop, hWndRead);

			// INI�ǂݍ���
			ReadIniData(fp);

			// ���W�I�{�^��
			CheckMenuRadioItem(hMenu, IDM_VIEW_SIZE_PERCENT025,	IDM_VIEW_SIZE_WINDOW,	NUMDATATORESID_SIZE(Config.ViewSize),	MF_BYCOMMAND);
			CheckMenuRadioItem(hMenu, IDM_PLAY_PLAY_ALL,		IDM_PLAY_PLAY_AUDIO,	NUMDATATORESID_PLAY(Config.PlayMode),	MF_BYCOMMAND);

			int	qualityIndex	= NUMDATATORESID_QUALITY(Config.PreviewQuality);
			if(qualityIndex>=0){
				CheckMenuRadioItem(hMenu, IDM_PLAY_QUALITY_ORIGINAL,	IDM_PLAY_QUALITY_QUARTER,
					qualityIndex+IDM_PLAY_QUALITY_ORIGINAL,	MF_BYCOMMAND);		
			}

			// �`�F�b�N
			SetMenuItemCheck(hMenu, IDM_PLAY_SELECT,Config.isPlaySelect);
			SetMenuItemCheck(hMenu, IDM_PLAY_LOOP,	Config.isPlayLoop);

			DrawMenuBar(hwnd);

			RamImage	= new RAMImage();
			UpdateMemorySize(Config.UseMemory);
			RamImage->SetVideoQuality(Config.PreviewQuality);

			Viewer		= new View(hwnd, RamImage, &Config);


			break;
		}

		case WM_FILTER_EXIT:
			WriteIniData(fp);
			delete	Viewer;		Viewer	= NULL;
			delete	RamImage;	RamImage= NULL;
			break;
	}

	return FALSE;
}


