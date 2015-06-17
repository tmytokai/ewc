// e-WC ���ʃw�b�_�t�@�C��

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h> // BROWSEINFO �\����
#include <string.h>
#include <mmsystem.h>
#include <stdio.h>
#include "resource.h"

#define CHR_BUF 256 // �����o�b�t�@��
#define MAX_ARGC 10 // �R�}���h���C�������̍ő�l
#define MAX_WAVFLTOPTBUF 4096 // WAVEFLT �ɑ���I�v�V�����f�[�^�̃o�b�t�@��
#define NOSND_MAXLEVEL	1024	// nosound �̍ő�臒l

// �{�^���̏��
#define MYID_RELEASEBUTTON              0  // �����[�X
#define MYID_PUSHBUTTON                 1  // �����Ă�

// �X�e�[�^�X
#define ID_STATREADY 0   // ���f�B
#define ID_STATPLAY 1    // �Đ���
#define ID_STATCLOSE 4   // �N���[�Y���


// �T�E���h�f�o�C�X�̍ő吔
#define MAXDEVICENUMBER 20  

// ���상�b�Z�[�W
#define WM_MYENDCOPY WM_APP+100  // �t�@�C���̃R�s�[���I��

// �`�����
#define FRAMESIZE 512 // �g�`�`��t���[���̃T�C�Y
#define EDITUPDATERECTTOP 70 // �G�f�B�^�̍ĕ`��̈�� y ���W
#define EDITUPDATERECTLEFT 12 // �G�f�B�^�̍ĕ`��̈�� x ���W
#define EDITSTATUSMARGIN1 10 // �G�f�B�^�̃X�e�[�^�X�`��̈�ƍĕ`��̈�Ƃ̃}�[�W��
#define EDITSTATUSMARGIN2 10 // �G�f�B�^�̃X�e�[�^�X�`��̈�Ɣg�`�E�B���h�E�Ƃ̃}�[�W��
#define EDITSTATUSLINE 5 // �X�e�[�^�X�̍s��
#define EDITSTATUSSIZE (EDITSTATUSLINE*20 +2) // �X�e�[�^�X�`��̈�̍���
#define EDITSTATUSLEFTMARGIN 235 // �X�e�[�^�X��[REMA�`] ����E�� x ���W
#define BUTTON_WIDTH	40 // �{�^����
#define BUTTON_HEIGHT	15 // �{�^������
#define BUTTON_MARGIN	8 // �{�^���ƃ{�^���̊Ԃ̉��̃}�[�W��
#define SB_HEIGHT	10 // �X�N���[���o�[����
#define PLAYBT_WIDTH 28	// �Đ��{�^���Ƃ��̕�
#define PLAYBT_HEIGHT 25	// �Đ��{�^���Ƃ��̍���
#define FONT_SIZE	14 // �{�^���̕����̃t�H���g�T�C�Y

// �����T�[�`�A�����}�[�N�֌W
#define MINSEARCHBOUND -100 // �������T�[�`����臒l�̍ŏ��l
//#define S_POINT_PER_SEC 50 // �������T�[�`���Ɉ�b�Ԃɉ��_�������邩
#define S_POINT_PER_SEC 240 // �������T�[�`���Ɉ�b�Ԃɉ��_�������邩
#define MAX_SPLITNUM 128	// �X�v���b�g�}�[�N�̍ő吔


// ���̑��F�X
#define BUTTON_NUM	12 // �{�^���̐�
#define BUFSIZE (FRAMESIZE*16*8) // FRAMESIZE * 16 (nBlockAlign��MAX) * 8 (ZOOM �̃}�b�N�X)
#define SAVEBUFSIZE (512*1024) // SAVE ,�T�[�`���̃o�b�t�@�T�C�Y
#define UNDOLEVEL 64 // UNDO �̊K�w
#define EQNUM 8 // �C�R���C�U�̕�����
#define MAX_EQDB 12 // �C�R���C�U�̍ő�f�V�x��


// �T�[�`���
#define ID_SEARCHON		0 // �T�[�`��
#define ID_SEARCHOFF	1 // �T�[�`��~


// �������T�[�`���[�h
#define NSOUND_TOP 0 // �擪
#define NSOUND_END 1 // �Ō�
#define NSOUND_MID 2 // ����


// �R�s�[���
#define ID_COPYON		0 // �R�s�[��
#define ID_COPYOFF		1 // ��~

// �I�t�Z�b�g�擾�֐��̃X�e�[�^�X
#define ID_PREPAREWAVE	0
#define ID_OPENWAVE		1
#define ID_RECORDWAVE	2
#define ID_CLOSEWAVE	3


// UNDO �f�[�^�^
typedef struct
{
LONGLONG n64DataSize[UNDOLEVEL]; //�f�[�^�T�C�Y(byte)
LONGLONG n64DataOffset[UNDOLEVEL]; //�f�[�^�܂ł̃I�t�Z�b�g(byte)
LONG nScrPos[UNDOLEVEL]; // �X�N���[���o�[�̃|�W�V����
LONG nMarkedPos[UNDOLEVEL];  // �}�[�N�����ʒu
LONGLONG n64MarkedPosByte[UNDOLEVEL]; // �}�[�N�����_(byte)
LONG nSubMarkedPos[UNDOLEVEL];  // �}�[�N�����ʒu(���}�[�N)
LONGLONG n64SubMarkedPosByte[UNDOLEVEL]; // �}�[�N�����_(���}�[�N,byte)
LONG nSplitMarkedPos[UNDOLEVEL][MAX_SPLITNUM]; // �X�v���b�g�}�[�N�̃|�W�V����
LONGLONG n64SplitMarkedPosByte[UNDOLEVEL][MAX_SPLITNUM]; // �X�v���b�g�}�[�N�̈ʒu(byte)
DWORD dwSplitNum[UNDOLEVEL]; // ������
BOOL bDataEmpty[UNDOLEVEL]; // UNDO �̃f�[�^�������Ă��邩�ǂ���
WORD wCurPos;  // UNDO �f�[�^�̌��݈ʒu
} UNDODATA;



// .dat �t�@�C���ɕۑ�����f�[�^
typedef struct
{
// �̂̃o�[�W�����̖��c�̃S�~
double thDynX; 
double thDynY; 
double maxDyn; 
double eqLevel[EQNUM]; 
DWORD dwEqLeng; 
double normLevel;

// �ǉ��I�v�V����
char szOption[MAX_WAVFLTOPTBUF];

// ���U�[�u
DWORD dwReserve[10];
BOOL bReserve[7];

BOOL bUseAvr;  // ���������ŕ��ω��ʂ��g��
// bOutfileIsNull == NULL �̎��A�g���b�N�ݒ�t�@�C���̊J���Ă�s�̃t�@�C������ null �ɂȂ�
BOOL bOutfileIsNull; 
BOOL bFoo1; 

double dbReserve[10];

}EDITSAVEDATA,*LPEDITSAVEDATA;


// �S�̃f�[�^
typedef struct
{
HWND hWnd; 
int x; // X ���W
int y; // Y ���W
BOOL bShiftLock; // �V�t�g���b�N�@�\
BOOL bShowConsole; // �R���\�[���\��
UINT uDeviceID; // �Đ��f�o�C�X ID
UINT uDevNum; // �T�E���h�f�o�C�X�̐�
WAVEOUTCAPS waveOutCaps[MAXDEVICENUMBER]; // �Đ��f�o�C�X���
PROCESS_INFORMATION* pProInfo; // WaveFLT �̃v���Z�X���
LPWORD lpwStatus; // �X�e�[�^�X
WAVEFORMATEX waveFmt; // Wave �t�H�[�}�b�g�f�[�^
LONGLONG n64WaveDataOffset; // �f�[�^�܂ł̃I�t�Z�b�g�T�C�Y
LONGLONG n64WaveDataSize;   // WAVE �̃f�[�^�T�C�Y

// �t�@�C���֌W
HANDLE hdFile; // �t�@�C���̃n���h��
CHAR szIniDatFile[MAX_PATH]; // �ݒ�ۑ��t�@�C��
CHAR szIniFileName[MAX_PATH]; // �N�����̃t�@�C����
CHAR szSettingFileName[MAX_PATH]; // �ǉ��I�v�V�����ݒ�ۑ��t�@�C����

CHAR szLoadFile[MAX_PATH]; //  ���[�h�t�@�C����
CHAR szSaveFile[MAX_PATH]; // �Z�[�u�t�@�C���� 
LPSTR lpszFileName; // �ۑ�����t�@�C����
LPSTR lpszOrgName; // ���̃t�@�C����

LONGLONG n64NewDataOffset; //�R�s�[�u���b�N�܂ł̃I�t�Z�b�g
LONGLONG n64NewDataSize; //�v���b�N�T�C�Y

// �g���b�N���֌W
CHAR szSaveDir[MAX_PATH]; // �ۑ��f�B���N�g��
CHAR szBaseName[CHR_BUF]; // �x�[�X��
CHAR szExtName[CHR_BUF]; // �g���q
CHAR *szTrackName[MAX_SPLITNUM]; // �g���b�N��
CHAR szTrackFile[MAX_PATH]; // �g���b�N�ݒ�t�@�C�� "" �Ȃ�x�[�X���g�p

BOOL bCutTrack; // �g���b�N�؂�o��
BOOL bSplit; // ����
BOOL bCutCm; // CM �J�b�g

LONGLONG n64SplitMarkedPosByte[MAX_SPLITNUM]; // �X�v���b�g�}�[�N�̈ʒu(�o�C�g)
LONG lnSplitMarkedPos[MAX_SPLITNUM]; // �X�v���b�g�}�[�N�̈ʒu
DWORD dwSplitNum; // �X�v���b�g�}�[�N�̐�
DWORD dwCurTrack;  // ���݂̃g���b�N�ԍ�

EDITSAVEDATA editSaveData; // �ҏW�f�[�^

}EWCDATA,*LPEWCDATA;


// �W�����v�_�C�A���O�ɓn���f�[�^�^
typedef struct
{
HANDLE  hdFile;
double*  lpdNoSoundBound;
LPDWORD lpdwNSoundCount;
LPDWORD lpdwSearchTime;
LPWORD lpwNSoundPos;
WAVEFORMATEX waveFmt;
LONGLONG n64DataOffset;
LONGLONG n64StartByte;
LPEWCDATA lpEwcData;
}JUMPDATA,*LPJUMPDATA;



// ���C���v���V�[�W��
LRESULT CALLBACK EditWaveProc(HWND,UINT,WPARAM,LPARAM);


// editwavefunc.cpp ���̊֐�
LRESULT CALLBACK EditFindDlgProc(HWND,UINT,WPARAM,LPARAM);
BOOL SaveCutData(HWND,HINSTANCE,LPEWCDATA ,LPSTR,LPSTR,BOOL);


BOOL SearchNoSound(HWND hWnd,
				   HINSTANCE hInst,
				   HANDLE hdFile, // �T�[�`����t�@�C���n���h��
				   LONGLONG n64DataSize, //�t�@�C���T�C�Y
				   LONGLONG n64DataOffset, //�f�[�^�܂ł̃I�t�Z�b�g
				   WAVEFORMATEX waveFmt, // Wave �t�H�[�}�b�g
				   LONGLONG* lpn64StartByte, // �J�n�ʒu(�o�C�g)
				   LONGLONG* lpn64EndByte, // �I���ʒu(�o�C�g)
				   double dBound, // �������l
				   DWORD dwCount,  // �����������̃J�E���^�̍ő�l
				   BOOL bAvr	// ���ω��ʒl���g�p
				   );


// editdlg ���̊֐�
LRESULT CALLBACK SettingProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK EditMenuDlgProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK TrackDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK InfoProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);


// �����֌W����(wavefunc.cpp)
double GetMaxWaveLevel(WAVEFORMATEX waveFmt);
UINT GetWaveInDevCap(LPWAVEINCAPS,UINT);
UINT GetWaveOutDevCap(LPWAVEOUTCAPS,UINT);

void SetWaveFmt(LPWAVEFORMATEX lpWaveFmt,WORD waveChn,
				DWORD waveRate,	WORD waveBit, WORD wTag);

void WaveLevel(double dLevel[2],  // output of left and right
			   BYTE* lpWaveData,  // input
			   WAVEFORMATEX waveFmt);
void WaveLevelAverage(double dLevel[2],  // output of left and right
			   BYTE* lpWaveData,  // input
			   WAVEFORMATEX waveFmt,
			   DWORD dwSize
			   );
BOOL GetWaveFormat(char* lpszFileName, // file name or 'stdin'
				   LPWAVEFORMATEX lpWaveFmt, 
				   LONGLONG* lpn64WaveDataSize, // size of data
				   LONGLONG* lpn64WaveOffset, // offset to data chunk
				   char* lpszErr 
				   );

// �Đ��֌W(playwave.cpp)
VOID SeekPlayWave(DWORD);
VOID StopPlayWave();
BOOL PlayWave(HWND hWnd,
			  UINT uDeviceID,
			  LPSTR lpszFileName,
			  DWORD dwStartTime,
			  HANDLE hdFile,WAVEFORMATEX waveFmt,
			  LONGLONG n64WaveDataSize, // �t�@�C���̃T�C�Y
			  LONGLONG n64WaveOffset  // �f�[�^�����܂ł̃I�t�Z�b�g 
			  );
BOOL PlayWaveByte(HWND hWnd,
				  UINT uDeviceID,
				  LPSTR lpszFileName,
				  LONGLONG n64StartByte,
				  HANDLE hdFile,
				  WAVEFORMATEX waveFmt,
				  LONGLONG n64WaveDataSize, // �t�@�C���̃T�C�Y
				  LONGLONG n64WaveOffset  // �f�[�^�����܂ł̃I�t�Z�b�g 
				  );


// ���b�Z�[�W�{�b�N�X(mymsgbox.cpp)
int MyMessageBox(HWND,LPSTR,LPSTR,UINT);


// commonfunc.cpp ���̊֐�
VOID ReadIniFile(LPEWCDATA,LPCSTR);
VOID SaveIniFile(LPEWCDATA,LPCSTR);
int GetArgv(LPSTR,CHAR[MAX_ARGC][CHR_BUF],int);
BOOL ShortToLongName(LPSTR);
LONG SetScrBarInfo(HWND,LPSCROLLINFO,WPARAM);
VOID SetDlgCenter(HWND);
VOID GetFreeHddSpace64(ULONGLONG*,LPSTR);
LPCSTR myfcvt(double,int);
BOOL ExecCommand(LPSTR,LPSTR,PROCESS_INFORMATION*,BOOL,BOOL,BOOL);
HWND GetConsoleWindowHandle();
HWND GetPreEwcHWND();
BOOL ExecCommandFileMapping(LPHANDLE,LPSTR,LPSTR,LPSTR,DWORD,PROCESS_INFORMATION*,HINSTANCE,BOOL,LPSTR);
VOID GetButtonPos(HWND,UINT,LPRECT);
VOID SetCommandLine(LPSTR,LPEDITSAVEDATA,LPEWCDATA,BOOL);

double SearchPeak(HANDLE hdFile, // �T�[�`����t�@�C���n���h��
				LONGLONG n64DataOffset, //�f�[�^�܂ł̃I�t�Z�b�g
				WAVEFORMATEX waveFmt, // Wave �t�H�[�}�b�g
				LONGLONG n64StartByte,  // �X�^�[�g�ʒu
				double dTime // ��������
				);

BOOL LoadTrackFile(HWND hWnd,
				   LPEWCDATA lpEwcData
				   );

BOOL SelectLoadFile(HWND hWnd ,LPSTR lpszFileName,
					 LPSTR lpstrFilter,
					 LPSTR lpstrDefExt,
					 LPSTR lpstrTitle
					 );
BOOL SelectSaveFile(HWND hWnd ,LPSTR lpszFileName,
					 LPSTR lpstrFilter,
					 LPSTR lpstrDefExt,
					 LPSTR lpstrTitle);

DWORD GetCurTrack(DWORD dwSplitNum,
				  LONGLONG n64SplitMarkedPosByte[MAX_SPLITNUM],
				  LONG nBlockAlign,
				  LONGLONG n64MaxBlock,
				  LONG nPos,
				  LONG nScrMax);
BOOL SelectDir(HWND hWnd,
			   LPSTR lpszDir,
			   LPSTR lpszTitle
			   );

BOOL WriteToFileMapping(LPHANDLE lphFileMap, // File Mapping Object �̃n���h��(�߂�l)
							LPSTR lpszWriteData,   // �������ރf�[�^
							DWORD dwSize, // �f�[�^�T�C�Y
							LPSTR lpszErr // �G���[���b�Z�[�W
							);

// draw.cpp ���̊֐�
VOID DrawMyButton(HWND,UINT,HDC,RECT,UINT,HICON*,HDC);
VOID InitEditScreenDraw(HWND hWnd,HINSTANCE hInst,HDC hBufDC);
BOOL CalcMarkedPosByte(LONGLONG* lpn64MarkedPosByte,
					   WAVEFORMATEX waveFmt,
					   LONGLONG n64WaveDataSize,
					   LONGLONG n64CurByte,
					   DWORD dwZoomX,
					   RECT rt,
					   LONG clickX);
LONG CalcMarkedLineX(WAVEFORMATEX waveFmt,
					 LONGLONG n64StartByte,
					 DWORD dwZoomX,
					 RECT rt,
					 LONGLONG n64MarkedPosByte);

VOID HakeiPaint(HWND hWnd,
				HDC hBufDC,
				RECT rt,
				WAVEFORMATEX waveFmt,
				LONGLONG n64WaveDataOffset,
				LONGLONG n64WaveDataSize,
				HANDLE hdWaveFile, // �t�@�C�������Ă�Ȃ� NULL
				LONGLONG n64CurByte, // byte, �f�[�^�܂ł̃o�C�g��
				LONG markedLineX, // �}�[�N���C���� x 
				LONG markedSubLineX, // ���}�[�N���C���� x 
				LONG mardedSplitLineX[MAX_SPLITNUM], // �X�v���b�g�}�[�N���C���� x
				DWORD dwSplitNum,
				DWORD dwZoomX, // �Y�[���{��(��)
				DWORD dwZoomY // �Y�[���{��(�c)
				);

VOID DrawMarkTriangle(HDC hDC,LONG nPos,LONG nPos2,LONG nPos3[MAX_SPLITNUM],DWORD dwSplitNum,LONG nMax);

VOID SetEditStatus(HWND hWnd,HDC hBufDC,
				   WAVEFORMATEX waveFmt,
				   LONGLONG n64DataSize,
				   DWORD dwFileTime,
				   DWORD dwCurTime,
				   LONGLONG n64MarkPosByte,
				   double dMarkLevel[2],  // db
				   LONGLONG n64SubMarkPosByte,
				   double dSubMarkLevel[2], // db
				   LONGLONG n64SplitMarkedPosByte[MAX_SPLITNUM], 
				   DWORD dwSplitNum,
				   DWORD dwTrack,
				   LPSTR lpszTrackName,
				   DWORD dwZoomX,
				   DWORD dwZoomY
				   );

void GetLevelatPoint(WAVEFORMATEX waveFmt,HANDLE hdFile,double dLevel[2],LONGLONG n64Pos);


//EOF
