//-----------------------
// WAVE �Đ��֐�   2002/11/24 ����
//
// (c) 1998-2015 Tomoya Tokairin
//
// �{�v���O�����̂��ׂāA�܂��͈ꕔ�� GPL �ɏ]���čĔЕz�܂��͕ύX����
// ���Ƃ��ł��܂��B�ڍׂɂ��Ă�GNU ��ʌ��L�g�p�����������ǂ݂��������B
//

// ���b�N���AEWC ����

// PlayWave() �ōĐ��J�n 
// �t�@�C���n���h���� NULL �Ȃ�֐����Ńt�@�C�����J���̂�
// waveFmt,�f�[�^�܂ł̃I�t�Z�b�g,�f�[�^�T�C�Y ���̃p�����[�^�͖�������
//
// �o�b�t�@�̐؂�ڂ��Ƃ� MM_WOM_DONE �� hWnd �ɑ���
// WPARAM �� ���x���s�[�N�l �A LPARAM �͌��ݎ���(�~���b)
// SeekPlayWave(�V�[�N�ʒu(�~���b))�ŃV�[�N
// StopPlayWave() �Œ�~���� MM_WOM_CLOSE �� hWnd �ɑ���B
// ����I�� LPARAM =0 �ُ� = 1

#include <windows.h>
#include <mmsystem.h>

#define USE_EWC  // ewc.exe �Ŏg���ꍇ
#define PLAYWAVEHDR_BUFNUM 4  // WAVE �w�b�_�̃o�b�t�@��

// �X�e�[�^�X
#define ID_THREADON   0
#define ID_THREADSTOP 1
#define ID_PREPARE 2
#define ID_CLOSEWAVE 3
#define ID_SEEKING 4

// ���ʂ̊֐�(wavefunc.cpp ��)
BOOL SetWaveHdr(HWND,LPWAVEHDR,LONG,DWORD);
BOOL DelWaveHdr(HWND,LPWAVEHDR);
BOOL GetWaveFormat(char* lpszFileName, // file name or 'stdin'
				   LPWAVEFORMATEX lpWaveFmt, 
				   LONGLONG* lpn64WaveDataSize, // size of data
				   LONGLONG* lpn64WaveOffset, // offset to data chunk
				   char* lpszErr 
				   );
VOID WaveLevelMaxPeak(double dPeak[2],
					  LPBYTE lpWaveData,  // data
					  DWORD dwWaveBlockByte, // byte, size of data
					  WAVEFORMATEX waveFmt);

DWORD ChangeBits(BYTE* lpBuf,  // input,output, buffer(BYTE*)
				DWORD dwByte, // byte, size of lpBuf
				WAVEFORMATEX waveFmtIn,
				WAVEFORMATEX waveFmtOut,
				double* lpFilterBuf[2]);

void SetWaveFmt(LPWAVEFORMATEX lpWaveFmt,WORD waveChn,
				DWORD waveRate,	WORD waveBit, WORD wTag);


// ���b�Z�[�W�{�b�N�X
int MyMessageBox(HWND,LPSTR,LPSTR,UINT);


// �X���b�h�ɓn���f�[�^�^
typedef struct{
	HWND hWnd; // �e�̃n���h��
	UINT uDeviceID; // �f�o�C�X ID
	CHAR szWaveFileName[MAX_PATH]; // �t�@�C����
	DWORD dwStartTime; // �Đ��J�n�ʒu(�~���b)
	LONGLONG n64StartByte; // �Đ��ʒu(�o�C�g)
	BOOL bStartByte; // n64StartByte �ňʒu���w�肷��
	HANDLE hdFile; // �Đ��t�@�C���̃n���h��
	WAVEFORMATEX waveFmt; // �E�F�[�u�t�H�[�}�b�g
	LONGLONG n64WaveDataSize;// �t�@�C���̃T�C�Y
	LONGLONG n64WaveOffset; // �f�[�^�����܂ł̃I�t�Z�b�g 
}PLAYDATA,*LPPLAYDATA;


// �v���V�[�W���ɓn���f�[�^�^
typedef struct
{
	HWND hWnd;
	LPWAVEFORMATEX lpWaveFmt; // wave �t�H�[�}�b�g
	LPWAVEFORMATEX lpWaveFmtOut; // wave �t�H�[�}�b�g(�o�͂�)
	DWORD dwTime; // �Đ�����
}PLAYPROCDATA,*LPPLAYPROCDATA;


// �O���[�o���ϐ�
HANDLE HdPlayThread = NULL; // �X���b�h�̃n���h��
WORD PlayStatus; // ���݂̃X�e�[�^�X
DWORD SeekPos;  // �V�[�N�ʒu(�~���b)



//-------------------------------------------------------------------
// �R�[���o�b�N�֐�
VOID CALLBACK MyWaveOutProc(HWAVEOUT hWaveOut,UINT msg,DWORD inst,DWORD dwP1,DWORD dwP2)
{
	
	HWND hWnd = ((LPPLAYPROCDATA)inst)->hWnd;
	LPWAVEFORMATEX lpWaveFmt = ((LPPLAYPROCDATA)inst)->lpWaveFmt;
	LPWAVEFORMATEX lpWaveFmtOut = ((LPPLAYPROCDATA)inst)->lpWaveFmtOut;
	
	LPWAVEHDR lpWaveHdr; 
	static double dPeak[PLAYWAVEHDR_BUFNUM][2];  // �e�� WPARAM �œn���f�[�^(�ő�s�[�N�l)
	static DWORD dwHdrNum = 0;
	DWORD i,dwTime;
	
	switch (msg)
	{
		
	case WOM_OPEN: // WAVE �f�o�C�X�I�[�v������

		for(i=0;i<PLAYWAVEHDR_BUFNUM;i++)
		{
			dPeak[i][0] = dPeak[i][1] = 0;
		}
		
		PlayStatus = ID_THREADON;
		
		break;
		
		//-------------------------------------------------------------------
		
	case WOM_DONE: // 1 �o�b�t�@���̍Đ��I��
		
		if(PlayStatus == ID_THREADON){
			
			// �w�b�_�̃A�h���X�擾
			lpWaveHdr = (LPWAVEHDR)dwP1;

			// �Đ����Ԏ擾
			dwTime = lpWaveHdr->dwUser;

#ifndef USE_EWC			
			// �e�ɓn���f�[�^�Z�b�g
			WaveLevelMaxPeak(dPeak[dwHdrNum],
				(LPBYTE)lpWaveHdr->lpData,lpWaveHdr->dwBufferLength,
				*lpWaveFmtOut);
#endif

			// �e�� MM_WOM_DONE ���M 
			if(dwTime != -1)
				SendMessage(hWnd,MM_WOM_DONE,(WPARAM)dPeak[dwHdrNum],(LPARAM)dwTime);
			else
				PlayStatus = ID_THREADSTOP;  // dwTime = -1 �Ȃ��~
				
			// ���̃o�b�t�@���g�p����
			lpWaveHdr->dwUser = 0;
			dwHdrNum = (dwHdrNum+1)&(PLAYWAVEHDR_BUFNUM-1);
		}
		
		break;
		
		//-------------------------------------------------------------------
		
	case WOM_CLOSE: // WAVE �f�o�C�X�N���[�Y
		
		PlayStatus = ID_CLOSEWAVE;
		
		break;
		
	default:
        break;
    }
}



//-------------------------------------------------------------------
// �V�[�N�֐�
VOID SeekPlayWave(DWORD dwTime)
{
	if(HdPlayThread != NULL && PlayStatus == ID_THREADON) {
		SeekPos = dwTime;  // �|�W�V�����Z�b�g
		PlayStatus = ID_SEEKING; // �V�[�N�J�n
	}
}



//-------------------------------------------------------------------
// ��~�֐�
VOID StopPlayWave()
{
	LONG i;
	
	if(HdPlayThread != NULL && PlayStatus == ID_THREADON) {
		
		PlayStatus = ID_THREADSTOP;  // �X���b�h�̃��[�v��~
		
		// �N���[�Y����܂ő҂�
		i=0;
		while(PlayStatus != ID_CLOSEWAVE && i<50){
			Sleep(50);
			i++;
		}
	}
	
}


//-------------------------------------------------------------------
// �Đ��X���b�h
DWORD WINAPI PlayWaveThread(LPVOID lpPlayData)
{
	
	// �ϐ��Z�b�g
	HWND hWnd = ((LPPLAYDATA)lpPlayData)->hWnd;
	UINT uDeviceID = ((LPPLAYDATA)lpPlayData)->uDeviceID;
	LPSTR lpszFileName = ((LPPLAYDATA)lpPlayData)->szWaveFileName;
	DWORD dwStartTime =  ((LPPLAYDATA)lpPlayData)->dwStartTime; // �Đ��J�n�ʒu(�~���b)
	HANDLE hdFile = ((LPPLAYDATA)lpPlayData)->hdFile; // �Đ��t�@�C���̃n���h��
	WAVEFORMATEX waveFmt = ((LPPLAYDATA)lpPlayData)->waveFmt; // wave �t�H�[�}�b�g
	LONGLONG n64WaveDataSize = ((LPPLAYDATA)lpPlayData)->n64WaveDataSize; // �f�[�^�����܂ł̃I�t�Z�b�g 
	LONGLONG n64WaveOffset = ((LPPLAYDATA)lpPlayData)->n64WaveOffset;  // �t�@�C���̃T�C�Y


	HWAVEOUT hWaveOut;  // �Đ��f�o�C�X�̃n���h��
	static WAVEHDR waveHdr[PLAYWAVEHDR_BUFNUM]; // wave �w�b�_�̃o�b�t�@
	PLAYPROCDATA procData; // �v���V�[�W���ɓn���f�[�^
	DWORD dwWaveBlockTime; // �o�b�t�@�̋L�^����
	DWORD dwWaveBlockByte; //�o�b�t�@�̃o�C�g��
	DWORD dwByte; // �ǂݍ��݃o�C�g��
	LONGLONG n64CurDataSize;  // ���݂̉��t�ʒu
	WORD wCurWaveHdr;  // ���ݎg�p���Ă��� wave �w�b�_�̔ԍ�
	BOOL bCloseFile = FALSE; // �t�@�C�����N���[�Y���邩
	LONG i; // �G�p
	LARGE_INTEGER LI; // SetFilePointer �p 
	char szStr[256];
	
	double* lpdBuffer[2] = {NULL,NULL};  // bit change �p
	WAVEFORMATEX waveFmtOut; // bit change �p

	// hdFile �� NULL �Ȃ炱�̃X���b�h���Ńt�@�C���̊J���߂�����B
	if(hdFile == NULL)
	{
		bCloseFile = TRUE;

		// �t�@�C�����邩�e�X�g
		hdFile = CreateFile(lpszFileName,GENERIC_READ, 
			0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL); 
		if(hdFile == INVALID_HANDLE_VALUE){
			MyMessageBox(hWnd, "�t�@�C�����J���܂���B\n���̃v���Z�X���t�@�C�����J���Ă���\��������܂��B", 
				"Error", MB_OK|MB_SETFOREGROUND|MB_ICONERROR);	
			goto ERROR_LV0;	
		}
		CloseHandle(hdFile);
		
		// �t�H�[�}�b�g�Z�b�g
		if(GetWaveFormat(lpszFileName,&waveFmt,&n64WaveDataSize,&n64WaveOffset,szStr)==FALSE) goto ERROR_LV0;
		
		// ���߂ăt�@�C���I�[�v��
		hdFile = CreateFile(lpszFileName,GENERIC_READ, 
			0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL); 
		if(hdFile == INVALID_HANDLE_VALUE){
			MyMessageBox(hWnd, "�t�@�C�����J���܂���B", 
				"Error", MB_OK|MB_SETFOREGROUND|MB_ICONERROR);	
			goto ERROR_LV0;	
		}
	}
	
	// �o�b�t�@ 1 �u���b�N�̃T�C�Y�v�Z(�� 0.2 �b��)
	dwWaveBlockByte = (DWORD)(0.2*waveFmt.nSamplesPerSec)*waveFmt.nBlockAlign;

	// 16 bit �ɕϊ�
	waveFmtOut = waveFmt;
	if(waveFmt.wBitsPerSample > 16)
	{
		SetWaveFmt(&waveFmtOut,waveFmt.nChannels,waveFmt.nSamplesPerSec,16,1);
		for(i=0;i<waveFmt.nChannels;i++) 
			lpdBuffer[i] = (double*)malloc(sizeof(double)*dwWaveBlockByte/waveFmt.nBlockAlign);
	}

	// �o�b�t�@ 1 �u���b�N�̍Đ�����(�~���b)
	dwWaveBlockTime =	(DWORD)((double)dwWaveBlockByte/(double)waveFmt.nAvgBytesPerSec*1000);
	
	// �o�b�t�@���������m��
	if(SetWaveHdr(hWnd,waveHdr,PLAYWAVEHDR_BUFNUM,dwWaveBlockByte) == FALSE) goto ERROR_LV1;
	
	// �v���V�[�W���ɓn���f�[�^���Z�b�g
	procData.hWnd = hWnd;
	procData.lpWaveFmt = &waveFmt;
	procData.lpWaveFmtOut = &waveFmtOut;

	// �f�o�C�X�I�[�v��
	PlayStatus = ID_PREPARE;
	if(waveOutOpen(&hWaveOut, uDeviceID,
		&waveFmtOut, (DWORD)MyWaveOutProc, (DWORD)&procData, CALLBACK_FUNCTION )!=MMSYSERR_NOERROR ){
		
		MyMessageBox(hWnd, "�Đ��f�o�C�X�̃I�[�v���Ɏ��s���܂����B\n���̃v���Z�X�� WAVE �f�o�C�X���g�p���Ă���\��������܂��B"
			, "Error", MB_OK|MB_SETFOREGROUND|MB_ICONERROR);
		
		goto ERROR_LV2;
	} 
	
	// �I�[�v������܂Œ�~
	i=0;
	while(PlayStatus != ID_THREADON && i<50)
	{
		Sleep(50);
		i++; 
	}
	
	// �����|�W�V�����Z�b�g
	if(((LPPLAYDATA)lpPlayData)->bStartByte)
		n64CurDataSize = ((LPPLAYDATA)lpPlayData)->n64StartByte;
	else
		n64CurDataSize = waveFmt.nBlockAlign*(((LONGLONG)dwStartTime*waveFmt.nSamplesPerSec)/1000);
	if(n64CurDataSize >= n64WaveDataSize) PlayStatus = ID_THREADSTOP;  
	
	// �t�@�C���|�C���^�ړ�
	LI.QuadPart = n64WaveOffset + n64CurDataSize;
	SetFilePointer(hdFile,LI.LowPart, &LI.HighPart,FILE_BEGIN);
	wCurWaveHdr = 0;  

	
	// �o�b�t�@�̏���
	for(i=0;i<PLAYWAVEHDR_BUFNUM;i++)
	{
		waveOutPrepareHeader(hWaveOut,&waveHdr[i],sizeof(WAVEHDR));
		waveHdr[wCurWaveHdr].dwUser = 0;
	}
	
	// --------------------
	// ���[�v�J�n
	while(PlayStatus == ID_THREADON || PlayStatus == ID_SEEKING){
		
		// �V�[�N
		if(PlayStatus == ID_SEEKING)
		{ 
			// �Đ���~
			waveOutReset(hWaveOut);
			
			// �t�@�C���|�C���^�ăZ�b�g
			n64CurDataSize = waveFmt.nBlockAlign
				*(LONGLONG)((double)SeekPos*(double)waveFmt.nSamplesPerSec/1000);
			
			LI.QuadPart = n64WaveOffset + n64CurDataSize;
			SetFilePointer(hdFile,LI.LowPart, &LI.HighPart,FILE_BEGIN);
			
			// �o�b�t�@�g�p����
			for(i=0;i<PLAYWAVEHDR_BUFNUM;i++) waveHdr[i].dwUser = 0;
			wCurWaveHdr = 0;
			
			// �X���b�h�ĊJ
			PlayStatus = ID_THREADON;
		}
		else if(waveHdr[wCurWaveHdr].dwUser == 0)
		{ // �o�b�t�@�g�p��

			// �f�[�^�ǂݍ���
			LI.QuadPart = n64WaveOffset + n64CurDataSize;
			SetFilePointer(hdFile,LI.LowPart, &LI.HighPart,FILE_BEGIN);
			ReadFile(hdFile,waveHdr[wCurWaveHdr].lpData,dwWaveBlockByte, &dwByte, NULL);
			
			if(dwByte)
			{
				//�f�o�C�X�Ƀf�[�^�𑗂�
				if(n64CurDataSize + dwByte > n64WaveDataSize) 
					dwByte = (DWORD)(n64WaveDataSize - n64CurDataSize);
				n64CurDataSize += dwByte;
				if(n64CurDataSize != n64WaveDataSize)
					waveHdr[wCurWaveHdr].dwUser  // �~���b
					= 1+(DWORD)((n64CurDataSize*1000)/waveFmt.nAvgBytesPerSec);
				else waveHdr[wCurWaveHdr].dwUser = -1; // -1 �𑗂�ƒ�~
				waveHdr[wCurWaveHdr].dwBufferLength = dwByte;
				
				if(waveFmt.wBitsPerSample > 16)
					waveHdr[wCurWaveHdr].dwBufferLength 
					= ChangeBits((LPBYTE)waveHdr[wCurWaveHdr].lpData,
					waveHdr[wCurWaveHdr].dwBufferLength,waveFmt,waveFmtOut,lpdBuffer);
				waveOutWrite(hWaveOut,&waveHdr[wCurWaveHdr],sizeof(WAVEHDR));

				// ���̃o�b�t�@��
				wCurWaveHdr = (wCurWaveHdr+1)&(PLAYWAVEHDR_BUFNUM-1);
			}
			else
			{
				// �S�Ẵw�b�_���Đ��ς݂Ȃ�I��
				for(i=0;i<PLAYWAVEHDR_BUFNUM;i++) if(waveHdr[i].dwUser) break;
				if(i>=PLAYWAVEHDR_BUFNUM) PlayStatus = ID_THREADSTOP;  
			}
		}
		else Sleep(dwWaveBlockTime/16);

	}
	// ���[�v�I���
	// --------------------

	
	// �N���[�Y����
	
	// �Đ���~
	waveOutReset(hWaveOut);
	
	// �o�b�t�@�̌�n��
	for(i=0;i<PLAYWAVEHDR_BUFNUM;i++)
		waveOutUnprepareHeader(hWaveOut,&waveHdr[i],sizeof(WAVEHDR));
	
	// �f�o�C�X�N���[�Y
	if(waveOutClose(hWaveOut)!=MMSYSERR_NOERROR) {
		MyMessageBox(hWnd, "�Đ��f�o�C�X����邱�Ƃ��ł��܂���B", 
			"Error", MB_OK|MB_SETFOREGROUND|MB_ICONERROR);	
		
		goto ERROR_LV2;
	}
	
	// �N���[�Y����܂Œ�~
	i=0;
	while(PlayStatus != ID_CLOSEWAVE && i<50){
		Sleep(50);
		i++;
	}
	
	// �t�@�C���N���[�Y
	if(bCloseFile) CloseHandle(hdFile);	 	
	
	// �o�b�t�@�������J��
	DelWaveHdr(hWnd,waveHdr);
	for(i=0;i<waveFmt.nChannels;i++) if(lpdBuffer[i]) free(lpdBuffer[i]);
	
	// �e�� MM_WOM_CLOSE �𑗐M(����I��)
	SendMessage(hWnd,MM_WOM_CLOSE,0,0);
	
	// �X���b�h��~
	HdPlayThread = NULL;
	
	return 0;
	
	//---- �G���[���� ---
	
ERROR_LV2:
	
	// �o�b�t�@�������J��
	DelWaveHdr(hWnd,waveHdr);
	for(i=0;i<waveFmt.nChannels;i++) if(lpdBuffer[i]) free(lpdBuffer[i]);
	
ERROR_LV1:
	
	// �t�@�C���N���[�Y
	if(bCloseFile) CloseHandle(hdFile);
	
ERROR_LV0:
	
	// �e�� MM_WOM_CLOSE �𑗐M(LPARAM �� 1)
	SendMessage(hWnd,MM_WOM_CLOSE,0,1);
	HdPlayThread = NULL;
	return 1;
}



//-------------------------------------------------------------------
// �Đ��J�n�֐�
BOOL PlayWave(HWND hWnd,
			  UINT uDeviceID,
			  LPSTR lpszFileName,
			  DWORD dwStartTime,
			  HANDLE hdFile,WAVEFORMATEX waveFmt,
			  LONGLONG n64WaveDataSize, // �t�@�C���̃T�C�Y
			  LONGLONG n64WaveOffset  // �f�[�^�����܂ł̃I�t�Z�b�g 
			  )
{
	static PLAYDATA playData;
	static DWORD dwThreadId;
	
	if(HdPlayThread != NULL) return FALSE; // �X���b�h�������Ă�����߂�
	else {	// �Đ��J�n
		
		// �\���̃f�[�^�Z�b�g
		playData.hWnd = hWnd;
		playData.uDeviceID = uDeviceID;
		if(lpszFileName!=NULL) wsprintf(playData.szWaveFileName,"%s",lpszFileName);
		playData.dwStartTime = dwStartTime;
		playData.bStartByte = FALSE;
		playData.hdFile = hdFile;
		playData.waveFmt = waveFmt;
		playData.n64WaveDataSize = n64WaveDataSize;
		playData.n64WaveOffset = n64WaveOffset;
		
		// �X���b�h�N��
		HdPlayThread = CreateThread(NULL,0,
			(LPTHREAD_START_ROUTINE)PlayWaveThread,
			(LPVOID)&playData,
			0,(LPDWORD)&dwThreadId);
		
	}
	
	return TRUE;
}


#ifdef USE_EWC
//-----------------------------------------------------
// �o�C�g�w��ōĐ�
BOOL PlayWaveByte(HWND hWnd,
				  UINT uDeviceID,
				  LPSTR lpszFileName,
				  LONGLONG n64StartByte,
				  HANDLE hdFile,
				  WAVEFORMATEX waveFmt,
				  LONGLONG n64WaveDataSize, // �t�@�C���̃T�C�Y
				  LONGLONG n64WaveOffset  // �f�[�^�����܂ł̃I�t�Z�b�g 
){
	static PLAYDATA playData;
	static DWORD dwThreadId;
	
	if(HdPlayThread != NULL) return FALSE; // �X���b�h�������Ă�����߂�
	else {	// �Đ��J�n
		
		// �\���̃f�[�^�Z�b�g
		playData.hWnd = hWnd;
		playData.uDeviceID = uDeviceID;
		if(lpszFileName!=NULL) wsprintf(playData.szWaveFileName,"%s",lpszFileName);
		playData.n64StartByte = n64StartByte;
		playData.bStartByte = TRUE;
		playData.hdFile = hdFile;
		playData.waveFmt = waveFmt;
		playData.n64WaveDataSize = n64WaveDataSize;
		playData.n64WaveOffset = n64WaveOffset;
		
		// �X���b�h�N��
		HdPlayThread = CreateThread(NULL,0,
			(LPTHREAD_START_ROUTINE)PlayWaveThread,
			(LPVOID)&playData,
			0,(LPDWORD)&dwThreadId);
		
	}
	
	return TRUE;
}
#endif

//EOF