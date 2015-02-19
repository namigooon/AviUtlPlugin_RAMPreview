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
	// �o�b�t�@

	private:
		T		*memory;	// �m�ۂ���������
		SIZE_T		size;		// �������T�C�Y[T]

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
	// �f���A�����������̋��ʏ���

	protected:
		Memory<T>	*memory;
		int		FrameSize;	// 1�t���[���Ɏg�p���郁�����T�C�Y[T]
		UINT		PreviewFrame;	// �v���r���[�\�ȃt���[����
		
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
		SIZE		ScreenSize;	// �ǂݍ��ݎ��̉摜�T�C�Y�i���T�C�Y��j
		
	public:
		void		Update(FILTER *fp, void *editp, double percent);
		BYTE		*GetMemory(UINT index);
		SIZE		GetSize() const;
};
class AudioMemory : public RAMemory<short>{
	private:
		int		Channel;	// �����̃`�����l����

	public:
		void		Update(FILTER *fp, void *editp);
		bool		UpdateMemoryFrame(FILTER *fp, void *editp, UINT frame);
		short		*GetMemory(UINT index);
		int		GetChannel() const;
};

class RAMImage{
	private:
		VideoMemory	*VMemory;		// �f��������
		AudioMemory	*AMemory;		// ����������

		double		ProjectFPS;		// �v���W�F�N�g��FPS
		SIZE		OriginalSize;		// �ǂݍ��ݎ��̉摜�T�C�Y�i���T�C�Y�O�j
		UINT		StartFrame;		// �ǂݍ��݊J�n�t���[��
		UINT		ReadCompleteFrame;	// �ǂݍ��݊����t���[����
		UINT		MaxReadFrame;		// �Đ��\�ȍŒ��t���[����

		bool		isReading;		// true = �ǂݍ��ݒ�
		bool		CanPlayMemory;		// true = �Đ��\

		UINT		VideoMemorySize;	// �f���������̖ڕW�m�ۃT�C�Y
		double		PreviewQuality;		// �Đ��i��[0.0-1.0]

		HANDLE		hReadThread;		// �ǂݍ��݃X���b�h�n���h��

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


