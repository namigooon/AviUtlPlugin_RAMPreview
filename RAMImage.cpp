//----------------------------------------------------------------------------------------------------
// RAMImage.cpp
//	2015/02/15 - 2015/11/03
//			hksy
//----------------------------------------------------------------------------------------------------

#include	"RAMImage.h"

//--------------------------------------------------
// Wave file
//--------------------------------------------------

const BYTE	HeaderChunkID[]		= {'R','I','F','F'};
const BYTE	HeaderFormType[]	= {'W','A','V','E'};
const BYTE	FormatChunkID[]		= {'f','m','t',' '};
const BYTE	DataChunkID[]		= {'d','a','t','a'};

#pragma pack(push,1)	// �A���C�����g���l�߂�

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

#pragma pack(pop)



//--------------------------------------------------
// Utility
//--------------------------------------------------

#define	Temp_T	template<class T>

// �w��t���[���̔z��擪�ʒu���擾
inline UINT	GetVideoIndex(int index, UINT size){
	return size*index;
}
inline UINT	GetAudioIndex(int index, UINT size){
	return sizeof(WAVEFILEHEADER) + size*index;
}



//--------------------------------------------------
// class Memory
//--------------------------------------------------

#define	CMemory	Memory<T>

Temp_T CMemory::Memory(){
	Init();
}
Temp_T CMemory::Memory(SIZE_T size){
	Init();
	UpdateMemory(size);
}

Temp_T CMemory::~Memory(){
	ReleaseMemory();
}

Temp_T void	CMemory::Init(){
	this->memory	= NULL;
	this->size	= 0;
}

Temp_T bool	CMemory::UpdateMemory(SIZE_T newsize){
	// �������̍X�V�i�T�C�Y�g���j

	if(newsize<=this->size){
		return true;	// �ړI�T�C�Y�𖞂����Ă�̂ňꉞ�����Ƃ������Ƃ�
	}

	// �m�ێ��s���̂��߂Ɋ�����������ێ����V�K�̃��������m��
	T	*buf	= (T*)VirtualAlloc(NULL, newsize*sizeof(T), MEM_COMMIT, PAGE_READWRITE);
	if(buf!=NULL){
		if(memory!=NULL){	// ���łɊm�ۂ��ꂽ������������
			DebugLog("delete	: Memory = 0x%p, 0x%08x byte", memory,size);
			VirtualFree(memory, size, MEM_DECOMMIT);
		}

		// �V�����������ɍX�V
		memory	= buf;
		size	= newsize;

		DebugLog("new		: Memory = 0x%p, 0x%08x byte", memory,size);
		return true;
	}
	else{		// �m�ێ��s���̓������X�V���s��Ȃ�
		return false;
	}
}
Temp_T bool	CMemory::ReleaseMemory(){
	// ���������

	if(memory!=NULL){
		bool	result	= VirtualFree(memory, size, MEM_DECOMMIT)==TRUE;
		Init();
		return result;
	}
	else{
		return false;
	}
}

Temp_T T	*CMemory::GetMemory() const{
	// �������̃o�b�t�@���擾
	return this->memory;
}
Temp_T SIZE_T	CMemory::GetSize() const{
	// �m�ۂ��Ă��郁�����̃T�C�Y���擾[T]
	return this->size;
}

#undef	CMemory



//--------------------------------------------------
// class RAMemory
//--------------------------------------------------

#define	CRAMemory	RAMemory<T>

Temp_T CRAMemory::RAMemory(){
	memory		= new Memory<T>();
	FrameSize	= 0;
	PreviewFrame	= 0;
}
Temp_T CRAMemory::~RAMemory(){
	delete	memory;	memory	= NULL;
}

Temp_T bool	CRAMemory::UpdateMemory(SIZE_T newsize){
	// �������T�C�Y�̍X�V
	return this->memory->UpdateMemory(newsize);
}
Temp_T T	*CRAMemory::GetAllMemory(){
	// �������̐擪�A�h���X���擾
	return this->memory->GetMemory();
}
Temp_T bool	CRAMemory::ReleaseMemory(){
	// �������̉��

	FrameSize	= 0;
	PreviewFrame	= 0;
	return this->memory->ReleaseMemory();
}

Temp_T SIZE_T	CRAMemory::GetMemorySize() const{
	// �m�ۂ��Ă��郁�����̃T�C�Y���擾[T]
	return this->memory->GetSize();
}
Temp_T int	CRAMemory::GetFrameSize() const{
	// 1�t���[���Ɏg�p���郁�����̃T�C�Y���擾[T]
	return this->FrameSize;
}
Temp_T UINT	CRAMemory::GetPreviewFrame() const{
	// �v���r���[�\�ȃt���[�������擾
	return this->PreviewFrame;
}

Temp_T SIZE_T	CRAMemory::GetUseMemory() const{
	// �m�ۂ��Ă��郁�����̃T�C�Y��byte�P�ʂŎ擾[byte]
	return this->GetMemorySize()*sizeof(T);
}

#undef	CRAMemory



//--------------------------------------------------
// class VideoMemory
//--------------------------------------------------

void	VideoMemory::Update(FILTER *fp, void *editp, double quality){
	// quality [0.0-1.0]

	int	w,h;
	fp->exfunc->get_pixel_filtered(editp, 0, NULL, &w, &h);

	this->ScreenSize.cx	= (LONG)(w*quality);
	this->ScreenSize.cy	= (LONG)(h*quality);

	// 4�o�C�g���E�ɍ��킹��
	this->ScreenSize.cx	+=(4-this->ScreenSize.cx&0x03)&0x03;
	this->ScreenSize.cy	+=(4-this->ScreenSize.cy&0x03)&0x03;

	this->FrameSize		= ScreenSize.cx*ScreenSize.cy * sizeof(PIXEL);
	this->PreviewFrame	= (UINT)floor((double)this->GetMemorySize()/FrameSize);

	DebugLog("Video : %dx%d->%dx%d %dbpp %dbyte %dframe", w, h, this->ScreenSize.cx, this->ScreenSize.cy, sizeof(PIXEL)*8, FrameSize, PreviewFrame);
}

BYTE	*VideoMemory::GetMemory(UINT index){
	// �w��t���[���̉摜�̎擾[read frame]
	// �͈͊O���̎��s����NULL

	BYTE	*mem	= this->memory->GetMemory();

	if(this->PreviewFrame<=index){
		return NULL;
	}
	return &mem[GetVideoIndex(index, this->FrameSize)];
}
SIZE	VideoMemory::GetSize() const{
	// �ǂݍ��ݎ��̉摜�T�C�Y���擾
	// �掿�ɂ�郊�T�C�Y���܂܂�Ă��܂�
	return this->ScreenSize;
}



//--------------------------------------------------
// class AudioMemory
//--------------------------------------------------

void	AudioMemory::Update(FILTER *fp, void *editp){
	double		ProjectFPS	= GetFPS(fp, editp);
	FILE_INFO	fi;
	fp->exfunc->get_file_info(editp,&fi);

	this->Channel		= fi.audio_ch;
	int	sample		= fp->exfunc->get_audio_filtered(editp,0,NULL);

	this->FrameSize		= sample*this->Channel;
	this->PreviewFrame	= (UINT)floor((double)this->GetMemorySize()/this->FrameSize);

	DebugLog("Audio : %dsample %dch %.1ffps %dbyte %dframe", sample, Channel, ProjectFPS, FrameSize, PreviewFrame);
}

bool	AudioMemory::UpdateMemoryFrame(FILTER *fp, void *editp, UINT frame){
	// �v���r���[����t���[��������K�v�����������肵�X�V���܂�

	Update(fp, editp);

	UINT	newsize	= sizeof(WAVEFILEHEADER) + FrameSize*frame;

	DebugLog("Audio update	: 0x%08X + 0x%08X * %d -> 0x%08X", sizeof(WAVEFILEHEADER), FrameSize, frame, newsize);

	if(UpdateMemory(newsize)){
		Update(fp, editp);
		return true;
	}
	else{
		return false;
	}
}
short	*AudioMemory::GetMemory(UINT index){
	// �w��t���[���̉摜�̎擾[read frame]
	// �͈͊O���̎��s����NULL

	short	*mem	= this->memory->GetMemory();

	if(PreviewFrame<=index){
		return NULL;
	}
	return &mem[GetAudioIndex(index, FrameSize)];
}
int	AudioMemory::GetChannel() const{
	// �`�����l�������擾
	return this->Channel;
}



//--------------------------------------------------
// class RAMImage
//--------------------------------------------------

void	ResizeImage(PIXEL *dst, PIXEL *src, int dw, int dh, int sw, int sh){
	// ���T�C�Y �߂�ǂ������̂Ńj�A���X�g�l�C�o�[��

	double	addx	= (double)sw/dw;
	double	addy	= (double)sh/dh;

	for(int iy=0;iy<dh;iy++){
		int	dyi	= iy*dw;
		int	syi	= (int)(iy*addy)*sw;

		for(int ix=0;ix<dw;ix++){
			int	dxi	= ix;
			int	sxi	= (int)(ix*addx);
			dst[dyi+dxi]	= src[syi+sxi];
		}
	}
}

void	ReadFrame(FILTER *fp, void* editp, int frame, int index,
	VideoMemory *VMemory, AudioMemory *AMemory, PIXEL *resize)
{
	// 1�t���[���ǂݍ���

	BYTE	*vmem	= VMemory->GetMemory(index);
	short	*amem	= AMemory->GetMemory(index);

	if(vmem!=NULL){
		SIZE	dst	= VMemory->GetSize();
		int	dw=dst.cx, dh=dst.cy;
		int	sw,sh;

		fp->exfunc->get_pixel_filtered(editp, frame, NULL, &sw, &sh);

		if(dw==sw && dh==sh){	// original
			fp->exfunc->get_pixel_filtered(editp, frame, vmem, NULL, NULL);
		}
		else{			// resize
			fp->exfunc->get_pixel_filtered(editp, frame, resize, NULL, NULL);
			ResizeImage((PIXEL*)vmem, resize, dw,dh, sw, sh);
		}
	}
	if(amem!=NULL){
		fp->exfunc->get_audio_filtered(editp, frame, amem);
	}
	
}

unsigned __stdcall ReadDataSubThreadProc(LPVOID lpParam){
	// �ǂݍ��݃X���b�h

	DWORD	*args			= (DWORD*)lpParam;
	FILTER	*fp			= (FILTER*	)args[0];
	void	*editp			= (void*	)args[1];
	UINT	StartFrame		= (UINT		)args[2];
	VideoMemory	*VMemory	= (VideoMemory*	)args[3];
	AudioMemory	*AMemory	= (AudioMemory*	)args[4];
	UINT	MaxReadFrame		= (UINT		)args[5];
	UINT	*ReadCompleteFrame	= (UINT*	)args[6];
	bool	*isReading		= (bool*	)args[7];

	DebugLog("Read Thread / %dframe +%dframe", StartFrame, MaxReadFrame);

	// create work buffer
	int	sw,sh;
	fp->exfunc->get_pixel_filtered(editp, StartFrame, NULL, &sw, &sh);
	PIXEL	*resize			= new PIXEL[sw*sh];

	// read init
	int	frame			= StartFrame;
	*ReadCompleteFrame		= 0;

	// read
	while(*ReadCompleteFrame<MaxReadFrame){
		if(!fp->exfunc->is_editing(editp) || !(*isReading)){
			break;
		}

		int	index	= *ReadCompleteFrame;

		ReadFrame(fp, editp, frame, index, VMemory, AMemory, resize);

		PostMessage(fp->hwnd, WM_READDATA_FRAME, (WPARAM)frame, (LPARAM)index);

		frame++;
		(*ReadCompleteFrame)++;
	}

	// read end
	*isReading	= false;
	delete[]	resize;

	DebugLog("ReadEnd : Read = %d", *ReadCompleteFrame);

	PostMessage(fp->hwnd, WM_READDATA_END, (WPARAM)*ReadCompleteFrame, NULL);

	DebugLog("End Thread");
	_endthreadex(0);
	return 0;
}

//--------------------------------------------------

RAMImage::RAMImage(){
	Init();
}
RAMImage::RAMImage(SIZE_T VideoMemorySize){
	Init();
	SetVideoMemorySize(VideoMemorySize);
}
RAMImage::~RAMImage(){
	delete	this->VMemory;	this->VMemory	= NULL;
	delete	this->AMemory;	this->AMemory	= NULL;
}

void	RAMImage::Init(){
	this->VMemory		= new VideoMemory();
	this->AMemory		= new AudioMemory();

	StartFrame		= 0;
	MaxReadFrame		= 0;
	ReadCompleteFrame	= 0;
	OriginalSize.cx		= 0;
	OriginalSize.cy		= 0;

	CanPlayMemory		= false;
	isReading		= false;

	VideoMemorySize		= 0;
	PreviewQuality		= 1.0;

	hReadThread		= NULL;
}

void	RAMImage::ReleaseMemory(){
	// ���������
	this->VMemory->ReleaseMemory();
	this->AMemory->ReleaseMemory();
	this->CanPlayMemory	= false;
}
bool	RAMImage::UpdateMemory(FILTER *fp, void *editp, SIZE_T VideoMemorySize){
	bool	video	= this->VMemory->UpdateMemory(VideoMemorySize);
	UINT	vframe	= this->VMemory->GetPreviewFrame();
	bool	audio	= this->AMemory->UpdateMemoryFrame(fp, editp, vframe);

	return video && audio;
}

bool	RAMImage::ReadStart(FILTER *fp, void *editp, UINT StartFrame, UINT ReadFrame){
	// �ǂݍ��݊J�n

	// setting
	//--------------------------------------------------
	UINT	VideoReadFrame;
	UINT	AudioReadFrame;
	int	w,h;
	UINT	frame_n		= fp->exfunc->get_frame_n(editp);

	fp->exfunc->get_pixel_filtered(editp, StartFrame, NULL, &w, &h);
	this->OriginalSize.cx	= w;
	this->OriginalSize.cy	= h;
	this->ProjectFPS	= GetFPS(fp, editp);

	this->UpdateMemory(fp, editp, this->VideoMemorySize);

	//   video
	this->VMemory->Update(fp, editp, this->PreviewQuality);
	VideoReadFrame		= this->VMemory->GetPreviewFrame();

	//   audio
	//this->AMemory->Update(fp, editp);
	this->AMemory->UpdateMemoryFrame(fp, editp, VideoReadFrame);
	AudioReadFrame		= this->AMemory->GetPreviewFrame();

	this->StartFrame	= StartFrame;
	this->MaxReadFrame	= min(min(VideoReadFrame, AudioReadFrame), ReadFrame);	// min(vide, audio, read)
	this->ReadCompleteFrame	= 0;
	this->isReading		= true;

	if(StartFrame+this->MaxReadFrame > frame_n){	// �v���W�F�N�g�̃t���[�����I�[�o�[
		this->MaxReadFrame	= frame_n - StartFrame;
	}


	// thread arg
	//--------------------------------------------------
	static void	*argList[12];
	argList[0]	= (void*)fp;
	argList[1]	= (void*)editp;
	argList[2]	= (void*)StartFrame;
	argList[3]	= (void*)VMemory;
	argList[4]	= (void*)AMemory;
	argList[5]	= (void*)MaxReadFrame;
	argList[6]	= (void*)&ReadCompleteFrame;
	argList[7]	= (void*)&isReading;


	DebugLog("Create Thread / args = {0x%p, 0x%p, %d, 0x%p, 0x%p, %d, 0x%p, 0x%p}",
		fp, editp, StartFrame, VMemory, AMemory, MaxReadFrame, ReadCompleteFrame, isReading	
	);
	hReadThread		= (HANDLE)_beginthreadex(NULL, 0, ReadDataSubThreadProc, &argList, 0, NULL);

	return hReadThread!=NULL;
}
void	RAMImage::ReadStop(){
	// �ǂݍ��ݏI��
	// WM_READDATA_END�����C���v���V�[�W���ɂ��Ă��A�X���b�h���~�����邽�ߕK���Ăяo���Ă�������

	// isReading�̓X���b�h�쐬���̈����ɂ���Afalse��������ƒ�~�c����͂�
	isReading	= false;

	// �ǂݍ��݃X���b�h�̏I���ҋ@
	WaitForSingleObject(this->hReadThread, INFINITE);
	CloseHandle(this->hReadThread);
	this->hReadThread	= NULL;

	// �ǂݍ��݊����t���[��������
	if(ReadCompleteFrame>0){
		CanPlayMemory	= true;
	}
}
bool	RAMImage::ReadReady(){
	// �ǂݍ��ݏ������ł��Ă��邩
	bool	v	= VMemory->GetMemorySize()>0;
	bool	a	= AMemory->GetMemorySize()>0;
	return v&&a;
}

void	RAMImage::SetVideoMemorySize(SIZE_T VideoMemorySize){
	// �f���p�������̃T�C�Y��ݒ�(byte)
	this->VideoMemorySize	= VideoMemorySize;
}
void	RAMImage::SetVideoQuality(double quality){
	// �f���̕i����ݒ�(%)
	this->PreviewQuality	= quality/100.0;
}

BITMAPINFO	RAMImage::GetBitmapInfo() const{
	// BITMAPINFO�̍쐬
	BITMAPINFO	bmi;
	SIZE		size	= this->VMemory->GetSize();

	ZeroMemory(&bmi,sizeof(bmi));
	bmi.bmiHeader.biSize		= sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth		= size.cx;
	bmi.bmiHeader.biHeight		= size.cy;
	bmi.bmiHeader.biPlanes		= 1;
	bmi.bmiHeader.biBitCount	= sizeof(PIXEL)*8;
	bmi.bmiHeader.biCompression	= BI_RGB;
	bmi.bmiHeader.biPlanes		= 1;

	return bmi;
}
BYTE	*RAMImage::GetFrame(UINT frame){
	// �w��t���[���̉摜�̎擾[project frame]
	// ���s����NULL�iVideoMemory�Ŏ����j
	UINT	index	= frame-StartFrame;
	return this->VMemory->GetMemory(index);
}
short	*RAMImage::GetAudio(UINT frame){
	// �w��t���[���̉����̎擾[project frame]
	// ���s����NULL�iAudioMemory�Ŏ����j
	UINT	index	= frame-StartFrame;
	return this->AMemory->GetMemory(index);
}
BYTE	*RAMImage::GetMemAudioFile(){
	// �w�b�_��t�����Awav�t�@�C���Ƃ��Ĉ�����悤�ɂ����o�b�t�@�̎擾

	int	Channel	= AMemory->GetChannel();
	UINT	Sample	= AMemory->GetFrameSize() / Channel;

	WAVEFILEHEADER	WaveFileHeader;
	WAVEFORMATEX	wf	= {
		1, Channel,						// wFormatTag, nChannels
		(DWORD)(Sample*ProjectFPS),				// nSamplesPerSec
		(DWORD)(Sample*ProjectFPS * Channel*sizeof(short)),	// nAvgBytesPerSec
		sizeof(short)*Channel, sizeof(short)*8, 0		// nBlockAlign, wBitsPerSample, cbSize
	};

	SETARRAYDATA(WaveFileHeader.FileSignature,	HeaderChunkID);
	SETARRAYDATA(WaveFileHeader.FileType,		HeaderFormType);
	SETARRAYDATA(WaveFileHeader.FormatChunkID,	FormatChunkID);
	SETARRAYDATA(WaveFileHeader.DataChunkID,	DataChunkID);

	WaveFileHeader.DataChunkSize	= ReadCompleteFrame*Sample*Channel*sizeof(short);
	WaveFileHeader.FileSize		= WaveFileHeader.DataChunkSize + sizeof(WAVEFILEHEADER)-sizeof(DWORD)*2;
	WaveFileHeader.FormatChunkSize	= sizeof(WaveFileHeader.Format);
	WaveFileHeader.Format		= wf;

	// set header
	BYTE	*AudioBuffer	= (BYTE*)this->AMemory->GetAllMemory();
	if(AudioBuffer!=NULL){
		memcpy_s(AudioBuffer, sizeof(WaveFileHeader), (BYTE*)&WaveFileHeader, sizeof(WaveFileHeader));
	}

	return AudioBuffer;
}
UINT	RAMImage::GetVideoMemorySize() const{
	// �m�ۂ��Ă���f���������̃T�C�Y���擾[BYTE]
	// this->VideoMemorySize�͖ڕW�l�Ȃ̂Ŏ��ۂɊm�ۂ��Ă���T�C�Y�Ƃ͈قȂ�܂�
	return this->VMemory->GetMemorySize();
}
UINT	RAMImage::GetAudioMemorySize() const{
	// �m�ۂ��Ă��鉹���������̃T�C�Y���擾[short]
	return this->AMemory->GetMemorySize();
}
SIZE_T	RAMImage::GetVideoUseMemory() const{
	// �m�ۂ��Ă���f���������̃T�C�Y��byte�P�ʂŎ擾[byte]
	return this->VMemory->GetUseMemory();
}
SIZE_T	RAMImage::GetAudioUseMemory() const{
	// �m�ۂ��Ă��鉹���������̃T�C�Y��byte�P�ʂŎ擾[byte]
	return this->AMemory->GetUseMemory();
}
UINT	RAMImage::GetReadStartFrame() const{
	return this->StartFrame;
}
UINT	RAMImage::GetReadCompleteFrame() const{
	// �ǂݍ��݊��������t���[�������擾
	return this->ReadCompleteFrame;
}
UINT	RAMImage::GetMaxReadFrame() const{
	// �Đ��\�ȍŒ��t���[�������擾
	return this->MaxReadFrame;
}
SIZE	RAMImage::GetScreenSize() const{
	// �ǂݍ��ݎ��̉摜�T�C�Y���擾
	// �掿�ɂ�郊�T�C�Y���܂܂�Ă��܂�
	return this->VMemory->GetSize();
}
SIZE	RAMImage::GetOriginalScreenSize() const{
	// �ǂݍ��ݎ��̉摜�T�C�Y���擾
	// �掿�ɂ�郊�T�C�Y�͊܂܂�܂���
	return this->OriginalSize;
}
bool	RAMImage::Reading() const{
	// ���ݓǂݍ��ݒ����擾
	return this->isReading;
}
bool	RAMImage::Ready() const{
	// �Đ��\���擾
	return (!this->isReading) && this->CanPlayMemory;
}
bool	RAMImage::isRoadedFrame(UINT frame) const{
	// �w��t���[�����ǂݍ��񂾃t���[���Ɋ܂܂�Ă��邩�擾
	return isRange(frame, this->StartFrame, this->StartFrame+this->ReadCompleteFrame);
}


