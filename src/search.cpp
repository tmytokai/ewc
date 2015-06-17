// easy Wave Cutter
// Copyright (c) 1999-2015 Tomoya Tokairin
// ���������T�[�`�p�֐�

#include "common.h"
#include <math.h>

// �������T�[�`�_�C�A���O�ƃX���b�h�ɓn���f�[�^�^
typedef struct
{
HWND hWnd;
HWND hProgBar;  // �v���O���X�o�[�̃n���h��
HANDLE hdFile; // �T�[�`����t�@�C���n���h��
LONGLONG n64DataSize; //�t�@�C���T�C�Y
LONGLONG n64DataOffset; //�f�[�^�܂ł̃I�t�Z�b�g
WAVEFORMATEX waveFmt; // Wave �t�H�[�}�b�g
LONGLONG* lpn64StartByte; // �������J�n�ʒu(�o�C�g)
LONGLONG* lpn64EndByte;  // �I���ʒu(�o�C�g)
LPWORD lpwStatus; // �X�e�[�^�X
double dBound; // �������l
DWORD dwMaxCount;  // �����������̃J�E���^�̍ő�l(1/25 �b�P��)
BOOL bAvr; // ���ω��ʂŌ���
}SEARCHDATA,*LPSEARCHDATA;


//-------------------------------------------------------------------
// �������T�[�`�X���b�h
DWORD WINAPI SearchThread(LPVOID lpSearchData) 
{
	
	LPSEARCHDATA lpSdata = (LPSEARCHDATA)lpSearchData;
	HWND hWnd  	// �e�_�C�A���O�̃E�B���h�E�n���h��
		= lpSdata->hWnd; 
	HWND hProgBar 	// �i�s�v���O���X�o�[�̃n���h��  
		= lpSdata->hProgBar; 
	HANDLE hdFile 	// �t�@�C���n���h��
		= lpSdata->hdFile	; 
	LONGLONG n64DataSize 	//�f�[�^�T�C�Y
		= lpSdata->n64DataSize; 
	WAVEFORMATEX waveFmt // Wave �t�H�[�}�b�g
		= lpSdata->waveFmt;
	LONGLONG n64DataOffset 	//�f�[�^�܂ł̃I�t�Z�b�g
		= lpSdata->n64DataOffset; 
	LONGLONG* lpn64StartByte 	// �J�n�ʒu(�o�C�g)
		= lpSdata->lpn64StartByte; 
	LONGLONG* lpn64EndByte 	// �I���ʒu(�o�C�g)
		= lpSdata->lpn64EndByte; 
	LPWORD lpwStatus 	// �X�e�[�^�X
		= lpSdata->lpwStatus; 
	double dBound // dB, �������l
		= lpSdata->dBound; 
	DWORD dwMaxCount // �J�E���^�̍ő�l
		= lpSdata->dwMaxCount; 
	
	// �t�@�C���ǂݍ��ݕۑ��p�ϐ�
	DWORD dwByte;
	DWORD dwReadByte; // �ǂݍ��ރo�C�g��
	LONGLONG n64CurByte = *lpn64StartByte; //���݈ʒu
	LONGLONG n64StartByte = n64CurByte; // �������̊J�n�ʒu(byte)
	LONGLONG n64EndByte; // �������̏I���ʒu(byte)
	DWORD dwMovePoint; // �t�@�C���|�C���^�̈ړ���
	DWORD dwCount;  // �����������̃J�E���^
	LPBYTE lpBuffer = NULL; // �R�s�[�p�̃o�b�t�@
	double dWaveLevel[2]; // ���̓��x���v�Z��
	double dMaxLevel;
	LARGE_INTEGER LI; // SetFilePointer �p 

	BOOL bSucceed = FALSE; // ���������� TRUE
	
	DWORD i;
	DWORD dwProgPos; // �v���O���X�o�[�̃|�W�V����
	
	// �n���h���� NULL �łȂ����`�F�b�N
	if(hdFile == NULL){ 
		
		MyMessageBox(hWnd, "�t�@�C�����I�[�v������Ă��܂���B", 
			"Error", MB_OK|MB_ICONERROR);	
		
		// ���s���b�Z�[�W���M
		SendMessage(hWnd,WM_MYENDCOPY,0,1L);
		ExitThread(1L);
	}

	// �J�n�ʒu���f�[�^�T�C�Y�𒴂��Ă��鎞
	if(n64CurByte > n64DataSize)
	{
		n64CurByte = n64DataSize;
		n64StartByte = n64CurByte;
		n64EndByte = n64StartByte;
		goto L_EXIT;
	}
	
	// �i�s�v���O���X�o�[�ݒ�	 
	dwProgPos = ((DWORD)(n64CurByte/SAVEBUFSIZE));
	SendMessage(hProgBar,PBM_SETRANGE,0,MAKELPARAM(0,(DWORD)(n64DataSize/SAVEBUFSIZE))); 
	SendMessage(hProgBar,PBM_SETSTEP,(WPARAM)1,0);
	SendMessage(hProgBar,PBM_SETPOS,(WPARAM)dwProgPos,0);   

	// ���x������ (-1 - 1 �ɐ��K��)
	dMaxLevel = GetMaxWaveLevel(waveFmt);
	dBound = pow(10.,dBound/20);  // ���j�A�l�ɖ߂�
	
	// �|�C���^�̈ړ���
	// ��b�Ԃ� waveFmt.nSamplesPerSec ����g���Ė����̔�������Ă������������̂�
	// waveFmt.nSamplesPerSec/S_POINT_PER_SEC �_���Ƃɔ��肷��
	dwMovePoint = waveFmt.nSamplesPerSec/S_POINT_PER_SEC;
	
	// �o�b�t�@���������m��
	lpBuffer =(LPBYTE)GlobalAlloc(GPTR,sizeof(BYTE)*SAVEBUFSIZE);
	if(lpBuffer == NULL)
	{
		MyMessageBox(hWnd, "�������̊m�ۂɎ��s���܂����B",
			"Error", MB_OK|MB_ICONERROR);
		
		// ���s���b�Z�[�W���M
		SendMessage(hWnd,WM_MYENDCOPY,0,1L);
		return 1;
	}
	
	
	//---------------------------------------
	// �T�[�`�J�n
	
	dwCount = 0;
	while(n64CurByte < n64DataSize && *lpwStatus==ID_SEARCHON)
	{
		// �v���O���X�p�[�A�b�v
		SendMessage(hProgBar,PBM_SETPOS,(WPARAM)dwProgPos,0);
		dwProgPos++;
		
		// �f�[�^�ǂݍ��݃T�C�Y�v�Z
		dwReadByte = n64CurByte + SAVEBUFSIZE > n64DataSize
			? (DWORD)(n64DataSize - n64CurByte) : SAVEBUFSIZE;
		
		// �ǂ�
		LI.QuadPart = n64DataOffset+n64CurByte; // �T�[�`�J�n�ʒu�Ƀ|�C���^�ړ�
		SetFilePointer(hdFile,LI.LowPart, &LI.HighPart,FILE_BEGIN);
		memset(lpBuffer,0,sizeof(BYTE)*SAVEBUFSIZE);
		ReadFile(hdFile,lpBuffer,dwReadByte,&dwByte, NULL);
		
		// �o�b�t�@�����T�[�`
		for(i = 0;i < dwByte/waveFmt.nBlockAlign ;i+=dwMovePoint)
		{
			if(lpSdata->bAvr)
				//dwMovePoint�_�̕��ω��ʎ擾
				WaveLevelAverage(dWaveLevel,lpBuffer+i*waveFmt.nBlockAlign,waveFmt,dwMovePoint*waveFmt.nBlockAlign);
			else
				// �o�͒l�擾
				WaveLevel(dWaveLevel,lpBuffer+i*waveFmt.nBlockAlign,waveFmt);

			//���K��
			dWaveLevel[0] /= dMaxLevel;
			dWaveLevel[1] /= dMaxLevel;
			
			// ����
			if(fabs(dWaveLevel[0]) <= dBound && fabs(dWaveLevel[1]) <= dBound) dwCount++;
			else { // �L���������ꂽ
				
				if(dwCount >= dwMaxCount) {	// ��������
					n64EndByte = n64StartByte+waveFmt.nBlockAlign*dwMovePoint*dwCount;
					bSucceed = TRUE;
					goto L_EXIT;
				}
				
				n64StartByte = n64CurByte+i*waveFmt.nBlockAlign;
				dwCount = 0;
			}
		}
		
		n64CurByte += dwByte;
	}

L_EXIT:	
	
	// �������J��
	if(lpBuffer) GlobalFree(lpBuffer);
	
	// �ʒu�Z�b�g
	*lpn64StartByte = n64StartByte;
	*lpn64EndByte = n64EndByte;

	// ���ʑ��M
	SendMessage(hWnd,WM_MYENDCOPY,0,bSucceed);
	
	return(0);
}



//-------------------------------------
// �������T�[�`�_�C�A���O�̃v���V�[�W��
// �X���b�h�𓮂����Ă邾���B
LRESULT CALLBACK SearchDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	
	static HANDLE hThread;  // �X���b�h�̃n���h��
	static DWORD threadId; // �X���b�h ID
	static LPSEARCHDATA lpSearchData; // �X���b�h�ɓn���f�[�^�^�C�v
	static WORD wStatus; // �X�e�[�^�X
	
	switch (msg) {
		
	case WM_INITDIALOG:  // �_�C�A���O������
		
		// �_�C�A���O�𒆐S�Ɉړ�
		SetDlgCenter(hWnd);
		
		// �ۑ��X���b�h�N��
		wStatus = ID_SEARCHON;
		lpSearchData = (LPSEARCHDATA)lp;
		lpSearchData->hWnd = hWnd;
		lpSearchData->hProgBar = GetDlgItem(hWnd,IDC_PROGBAR);
		lpSearchData->lpwStatus = &wStatus;
		
		hThread = CreateThread(NULL,0,
			(LPTHREAD_START_ROUTINE)SearchThread,
			(LPVOID)lpSearchData,
			0,(LPDWORD)&threadId);
		
		if(hThread == NULL){	   
			MyMessageBox(hWnd, "�X���b�h�̋N���Ɏ��s���܂����B", 
				"Error", MB_OK|MB_ICONERROR);
			EndDialog(hWnd, IDCANCEL); 
		}
		
		break;
		
	case WM_MYENDCOPY: // �T�[�`����
		
		if((DWORD)lp == FALSE) EndDialog(hWnd, IDCANCEL); // ���s
		else EndDialog(hWnd, IDOK);  // ����
		
		break;
		
	case WM_COMMAND:
		
		switch (LOWORD(wp)) {
			
		case IDCANCEL: // ��~
			
			wStatus = ID_SEARCHOFF;
			return TRUE;
			
		}
		
		break;
		
		default: return FALSE;
	}
	
	return TRUE;
}




//-------------------------------------------
// �������T�[�`�J�n�֐�
// ���ۂ̓_�C�A���O���J�������B
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
				   ){
	
	SEARCHDATA searchData;
	
	// �_�C�A���O�ɓn���f�[�^�Z�b�g
	searchData.hdFile = hdFile;
	searchData.n64DataSize = n64DataSize;
	searchData.waveFmt = waveFmt;
	searchData.n64DataOffset = n64DataOffset;
	searchData.lpn64StartByte = lpn64StartByte;
	searchData.lpn64EndByte = lpn64EndByte;
	searchData.bAvr = bAvr;
	
	searchData.dBound = dBound;
	searchData.dwMaxCount = dwCount;
	
	// �_�C�A���O�{�b�N�X�\���ƕۑ��J�n
	if(DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_SEARCH)
		,hWnd,(DLGPROC)SearchDlgProc,
		(LPARAM)&searchData) == IDCANCEL) return FALSE;
	
	return TRUE;
}


//EOF
