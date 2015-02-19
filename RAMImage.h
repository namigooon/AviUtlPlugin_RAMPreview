//----------------------------------------------------------------------------------------------------
// RAMImage.h
//	2015/02/15 - 
//			hksy
//----------------------------------------------------------------------------------------------------

#pragma	once
#ifndef	_H_APF_RAMPREVIEW_RAMIMAGE
#define	_H_APF_RAMPREVIEW_RAMIMAGE

//--------------------------------------------------

#include	<Windows.h>
#include	"rampreview.h"

//--------------------------------------------------

#define	RP_READMAX		-1

//--------------------------------------------------

template<class T>class Memory{
	// バッファ

	private:
		T		*memory;	// 確保したメモリ
		SIZE_T		size;		// メモリサイズ[T]

		void		Init();

	public:
		Memory();
		Memory(SIZE_T size);
		~Memory();

		bool		ReleaseMemory();
		bool		UpdateMemory(SIZE_T newsize);

		T		*GetMemory() const;
		SIZE_T		GetSize() const;
};

template<class T>class RAMemory{
	// 映像、音声メモリの共通処理

	protected:
		Memory<T>	*memory;
		int		FrameSize;	// 1フレームに使用するメモリサイズ[T]
		UINT		PreviewFrame;	// プレビュー可能なフレーム数
		
	public:
		RAMemory();
		~RAMemory();

		bool		UpdateMemory(SIZE_T newsize);
		T		*GetAllMemory();
		bool		ReleaseMemory();

		virtual T	*GetMemory(UINT index)	= 0;
		SIZE_T		GetMemorySize() const;
		int		GetFrameSize() const;
		UINT		GetPreviewFrame() const;

		SIZE_T		GetUseMemory() const;
};

class VideoMemory : public RAMemory<BYTE>{
	private:
		SIZE		ScreenSize;	// 読み込み時の画像サイズ（リサイズ後）
		
	public:
		void		Update(FILTER *fp, void *editp, double percent);
		BYTE		*GetMemory(UINT index);
		SIZE		GetSize() const;
};
class AudioMemory : public RAMemory<short>{
	private:
		int		Channel;	// 音声のチャンネル数

	public:
		void		Update(FILTER *fp, void *editp);
		bool		UpdateMemoryFrame(FILTER *fp, void *editp, UINT frame);
		short		*GetMemory(UINT index);
		int		GetChannel() const;
};

class RAMImage{
	private:
		VideoMemory	*VMemory;		// 映像メモリ
		AudioMemory	*AMemory;		// 音声メモリ

		double		ProjectFPS;		// プロジェクトのFPS
		SIZE		OriginalSize;		// 読み込み時の画像サイズ（リサイズ前）
		UINT		StartFrame;		// 読み込み開始フレーム
		UINT		ReadCompleteFrame;	// 読み込み完了フレーム数
		UINT		MaxReadFrame;		// 再生可能な最長フレーム数

		bool		isReading;		// true = 読み込み中
		bool		CanPlayMemory;		// true = 再生可能

		UINT		VideoMemorySize;	// 映像メモリの目標確保サイズ
		double		PreviewQuality;		// 再生品質[0.0-1.0]

		HANDLE		hReadThread;		// 読み込みスレッドハンドル

		void		Init();
		bool		UpdateMemory(FILTER *fp, void *editp, SIZE_T VideoMemorySize);

	public:
		RAMImage();
		RAMImage(SIZE_T VideoMemorySize);
		~RAMImage();

		bool	ReadStart(FILTER *fp, void *editp, UINT StartFrame, UINT ReadFrame=RP_READMAX);
		void	ReadStop();
		bool	ReadReady();

		void	SetVideoMemorySize(SIZE_T VideoMemorySize);
		void	SetVideoQuality(double quality);
		void	ReleaseMemory();

		BITMAPINFO	GetBitmapInfo() const;
		BYTE	*GetFrame(UINT frame);
		short	*GetAudio(UINT frame);
		BYTE	*GetMemAudioFile();
		UINT	GetVideoMemorySize() const;
		UINT	GetAudioMemorySize() const;
		SIZE_T	GetVideoUseMemory() const;
		SIZE_T	GetAudioUseMemory() const;
		UINT	GetReadStartFrame() const;
		UINT	GetReadCompleteFrame() const;
		UINT	GetMaxReadFrame() const;
		SIZE	GetScreenSize() const;
		SIZE	GetOriginalScreenSize() const;
		bool	Reading() const;
		bool	Ready() const;
		bool	isRoadedFrame(UINT frame) const;
};

//--------------------------------------------------

#endif	// _H_APF_RAMPREVIEW_RAMIMAGE


