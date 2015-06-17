// easy Wave Cutter
// Copyright (c) 1999-2015 Tomoya Tokairin
// �ۑ��Ƃ��I�t�Z�b�g�擾�Ƃ��W�����v�Ƃ��̊֐�

#include "common.h"
#include <math.h>


//-------------------------------------------------------------------
// Wave �f�o�C�X�I�[�v�����̃R�[���o�b�N�֐�
VOID CALLBACK MyOffsetWaveInProc(HWAVEOUT hWaveOut,UINT msg,DWORD inst,DWORD dwP1,DWORD dwP2)
{
	
	switch (msg) {
		
	case WIM_OPEN: // WAVE �f�o�C�X�I�[�v��
		*((LPWORD)inst) = ID_OPENWAVE;
		break;
		
	case WIM_DATA: // �^���I��
		
		*((LPWORD)inst) = ID_RECORDWAVE;
		break;
		
	case WIM_CLOSE: // WAVE �f�o�C�X�N���[�Y
		
		*((LPWORD)inst) = ID_CLOSEWAVE;
		break;
		
	default:
		break;
    }
}





//-------------------------------------
// �㏑���m�F�_�C�A���O
LRESULT CALLBACK ProgDlgOverWrite(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	
    switch (msg) {
		
	case WM_INITDIALOG:  // �_�C�A���O������
		
		SetDlgCenter(hWnd);
		Edit_SetText(GetDlgItem(hWnd,IDC_EDITMSG),(LPCTSTR)lp);
		break;
		
	case WM_COMMAND:
		switch (LOWORD(wp)) {
			
		case IDOK:
		case IDCANCEL: 
		case IDC_ALLYES:
		case IDC_ALLNO:
		case IDC_STOP:			
			EndDialog(hWnd, LOWORD(wp)); 
		}
		
		return TRUE;
		
		default: return FALSE;
			
	}
	
	return TRUE;
}



//-------------------------------------------------------------------
// WAVEFLT2 �N���X���b�h
//
// lpszOrgName �̃t�@�C���� lpszNewFileName �ɃR�s�[����

DWORD WINAPI StartWaveFlt2(LPVOID lpVoid) 
{
	
	LPEWCDATA lpEwcData = (LPEWCDATA)lpVoid;
	
	HWND hWnd  	// �e�_�C�A���O�̃E�B���h�E�n���h��
		= lpEwcData->hWnd; 
	WAVEFORMATEX waveFmt // Wave �t�H�[�}�b�g�f�[�^
		= lpEwcData->waveFmt; 
	LPSTR lpszNewFileName 	// �R�s�[����t�@�C����
		= lpEwcData->lpszFileName; 
	LPSTR lpszOrgName 	// ���t�@�C����
		= lpEwcData->lpszOrgName; 
	LPWORD lpwStatus 	// �X�e�[�^�X
		= lpEwcData->lpwStatus; 
	PROCESS_INFORMATION* pProInfo  // WaveFLT �̃v���Z�X���
		= lpEwcData->pProInfo; 

	LONGLONG n64DataSize; 	//�R�s�[����f�[�^�T�C�Y
	
	// Wave �f�[�^�p
	LONGLONG n64SrcDataSize; // �R�s�[���t�@�C���̃f�[�^�T�C�Y
	LONGLONG n64SrcDataOffset; // �R�s�[���t�@�C���̃w�b�_�I�t�Z�b�g
	
	CHAR szStr[CHR_BUF];
	DWORD dwExitCode;
	
	// �R�}���h���C���N���p
	CHAR szCommandLine[CHR_BUF];
	CHAR szFoo[CHR_BUF],szFoo2[CHR_BUF];
	CHAR szData[MAX_WAVFLTOPTBUF];
	HINSTANCE hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
	HANDLE hFileMap = NULL; // File Mapping Object �p
	HANDLE hTrackFileMap = NULL; // File Mapping Object �p
	char* lpszTrackName = NULL;
	DWORD dwOVWstat = 0; // �㏑���m�F (0: �₢���킹, 1:�S�ď㏑��, 2:�S�ď㏑�����Ȃ�)

	DWORD i;
	LONGLONG n64Foo,n64Foo2,n64Prev;
	LONG nRet;
	
	
	// �R�s�[���s���ꍇ�ɃR�s�[�ł��邩�`�F�b�N
	if(lpszNewFileName != NULL)
	{
		n64DataSize = lpEwcData->n64NewDataSize;
		
		// �R�s�[�o���邩�f�B�X�N�̋�T�C�Y�`�F�b�N
		// stdout �̎��̓`�F�b�N���Ȃ�
		ULONGLONG u64DiskFreeSpace; 
		GetFreeHddSpace64(&u64DiskFreeSpace,lpszNewFileName);
		
		if((LONGLONG)u64DiskFreeSpace < n64DataSize/+1024)
		{
			wsprintf(szStr,"�h���C�u�̋󂫗e�� %lu M �K�v�ȗe�� %lu M\n\n�h���C�u�̋�e�ʂ�����܂���B�h���C�u��ύX���ĉ������B"
				,(DWORD)(u64DiskFreeSpace/1024/1024),(DWORD)(n64DataSize/1024/1024));
			MyMessageBox(hWnd,szStr,"Error", MB_OK|MB_ICONERROR);	
			
			SendMessage(hWnd,WM_MYENDCOPY,0,1L);
			ExitThread(1L);
		}
		
		// �I���W�i���̃t�H�[�}�b�g�擾
		if(!GetWaveFormat(lpszOrgName,&waveFmt,&n64SrcDataSize,&n64SrcDataOffset,szStr))
		{
			MyMessageBox(hWnd,szStr,"Error", MB_OK|MB_ICONERROR);	

			// ���s���b�Z�[�W�𑗂�
			SendMessage(hWnd,WM_MYENDCOPY,0,1L);
			ExitThread(1L);
		}
		
		// �R�s�[�o���邩�f�[�^�T�C�Y�̃`�F�b�N
		if(n64SrcDataSize < n64DataSize)
		{
			wsprintf(szStr,"�R�s�[�o���܂���B\n\n�R�s�[���f�[�^�T�C�Y %lu M �R�s�[����T�C�Y %lu M\n\n�R�s�[���̃t�@�C���T�C�Y�����������܂��B"
				,(DWORD)(n64SrcDataSize/1024/1024),(DWORD)(n64DataSize/1024/1024));
			MyMessageBox(hWnd,szStr,"Error", MB_OK|MB_ICONERROR);	
			
			// ���s���b�Z�[�W���M
			SendMessage(hWnd,WM_MYENDCOPY,0,1L);
			ExitThread(1L);
		}
	}
	
	SetWindowText(hWnd,"WAVEFLT2 ���s��");
	
	//----------------------------------
	// �R�}���h���C���Z�b�g
	if(strcmp(lpszNewFileName,"waveout") == 0)
		wsprintf(szCommandLine,"waveflt2.exe -fmap \"%s\" waveout",lpszOrgName);  // �T�E���h�o��
	else 
		wsprintf(szCommandLine,"waveflt2.exe -fmap \"%s\" \"%s\"",lpszOrgName,lpszNewFileName); 
	
	// waveflt2 �̏ꍇ�̓w�b�_�I�t�Z�b�g�̕������Ƃ�
	lpEwcData->n64NewDataOffset -= n64SrcDataOffset;
	
	//---------------
	// filemapping �� �I�v�V�����Z�b�g

	// �R�s�[�͈�
	if(!lpEwcData->bCutCm)
	{
		sprintf(szData,"-cutb %I64d %I64d ",
			lpEwcData->n64NewDataOffset,lpEwcData->n64NewDataSize); 
	}

	// ����
	// -outfile_fm �I�v�V�����g���ăg���b�N����n��
	if(lpEwcData->bSplit)
	{
		char szDrive[16],szPath[MAX_PATH],szFullPath[MAX_PATH];
		HANDLE hTmp;

		lpszTrackName = (char*)malloc(sizeof(char)*(lpEwcData->dwSplitNum+1)*MAX_PATH+1024);
		memset(lpszTrackName,0,sizeof(char)*(lpEwcData->dwSplitNum+1)*MAX_PATH);

		// �ۑ���f�B���N�g���擾
		_splitpath(lpEwcData->lpszFileName,szDrive,szPath,NULL, NULL);	
		wsprintf(szFullPath,"%s%s",szDrive,szPath);
	
		// (��) -splitbm �̓T�C�Y�Ŏw�肷��̂ŃI�t�Z�b�g�Ƃ��͍l���Ȃ��ėǂ�
		wsprintf(szData,"%s -splitbm %lu ",szData,lpEwcData->dwSplitNum+1);

		// �����T�C�Y�Z�b�g
		n64Prev = 0;
		for(i=0;i<lpEwcData->dwSplitNum;i++)
		{
			n64Foo = lpEwcData->n64SplitMarkedPosByte[i];
			n64Foo2 = n64Foo-n64Prev;
			sprintf(szFoo,"%I64d ",n64Foo2);
			strcat(szData,szFoo);
			n64Prev = n64Foo;
		}
		sprintf(szFoo,"%I64d ",lpEwcData->n64NewDataSize); // �c��S��
		strcat(szData,szFoo);

		// �t�@�C�����Z�b�g
		for(i=0;i<=lpEwcData->dwSplitNum;i++)
		{
			// �t�@�C������ "null" �Ȃ炻�̂܂�
			if(strcmp(lpEwcData->szTrackName[i],"null")==0){
				strcpy(szFoo,"null");
			}
			else
			{
				// ���Ƀt�@�C�������݂��Ă��邩�`�F�b�N
				wsprintf(szFoo,"%s%s",szFullPath,lpEwcData->szTrackName[i]);
				hTmp = CreateFile(szFoo,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_READONLY,NULL); 
				
				// ���݂�����t�@�C������"null"�ɂ���
				if(hTmp != INVALID_HANDLE_VALUE){
					CloseHandle(hTmp);
					wsprintf(szFoo2,"�t�@�C�� %s �͊��ɑ��݂��܂��B\n\n�㏑���ۑ����܂���?",szFoo);
					
					switch(dwOVWstat){
						
					case 0: // �_�C�A���O���o���Ė₢���킹
						
						nRet = DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_OVERWRITE)
							,hWnd,(DLGPROC)ProgDlgOverWrite,(LPARAM)szFoo2);
						
						switch(nRet){
						case IDCANCEL:
							strcpy(szFoo,"null");
							break;
						case IDC_ALLYES:
							dwOVWstat = 1;
							break;
						case IDC_ALLNO:
							strcpy(szFoo,"null");
							dwOVWstat = 2;
							break;
						case IDC_STOP:
							// ���~
							(*lpwStatus) = ID_COPYOFF;
							goto L_ERROR;
						}
						
						break;
						
						case 2: // �S�ď㏑�����Ȃ�
							strcpy(szFoo,"null");
							break;
					}
				}
			}

			wsprintf(szFoo2,"%s\n",szFoo);
			strcat(lpszTrackName,szFoo2);
		}

		if(!WriteToFileMapping(&hTrackFileMap,lpszTrackName,sizeof(char)*(lpEwcData->dwSplitNum+1)*MAX_PATH,NULL))
		{
			MyMessageBox(hWnd, "�t�@�C���}�b�s���O�̍쐬�Ɏ��s���܂���","Error", MB_OK|MB_ICONERROR);	
			dwExitCode = 1;
			goto L_ERROR;
		}

		wsprintf(szFoo,"-outfile_fm %lu ",(DWORD)hTrackFileMap); 
		strcat(szData,szFoo);
	}
	else 
		if(lpEwcData->bCutCm)
		{ // CM �J�b�g

		wsprintf(szData,"-cutmb %lu ",lpEwcData->dwSplitNum/2+1);

		// (��) -cutmb �� �I�t�Z�b�g + �u���b�N�T�C�Y �Ŏw��

		// �ŏ�
		n64Foo = lpEwcData->n64NewDataOffset;
		n64Foo2 = lpEwcData->n64SplitMarkedPosByte[0];
		sprintf(szFoo,"%I64d %I64d ",n64Foo,n64Foo2);
		strcat(szData,szFoo);

		for(i=1;i<lpEwcData->dwSplitNum-1;i+=2){
			n64Foo = lpEwcData->n64NewDataOffset + lpEwcData->n64SplitMarkedPosByte[i];
			n64Foo2 = lpEwcData->n64SplitMarkedPosByte[i+1] - lpEwcData->n64SplitMarkedPosByte[i];
			sprintf(szFoo,"%I64d %I64d ",n64Foo,n64Foo2);
			strcat(szData,szFoo);
		}
		
		// �c��
		if(lpEwcData->dwSplitNum%2==0)
		{
			n64Foo = lpEwcData->n64NewDataOffset + lpEwcData->n64SplitMarkedPosByte[i];
			n64Foo2 = lpEwcData->n64NewDataSize - lpEwcData->n64SplitMarkedPosByte[i];
			sprintf(szFoo,"%I64d %I64d ",n64Foo,n64Foo2);
			strcat(szData,szFoo);
		}
	}

	// �c��̃I�v�V�����Z�b�g
	strcat(szData,lpEwcData->editSaveData.szOption);

	// WaveFLT2 �N��
	// �I�v�V������ File Mapping �œn��
	if(!ExecCommandFileMapping(&hFileMap,NULL,szCommandLine,szData,MAX_WAVFLTOPTBUF,
		pProInfo,hInst,lpEwcData->bShowConsole,szStr))
	{
		SetForegroundWindow(hWnd);
		MyMessageBox(hWnd, "WAVEFLT �̋N���Ɏ��s���܂���\n\newc.exe �Ɠ����t�H���_�� waveflt2.exe �����邩�m�F���ĉ�����", 
			"Error", MB_OK|MB_ICONERROR);	
		
		dwExitCode = 1;
		goto L_ERROR;
	}
	
	// WaveFLT ���I��܂Œ�~
	WaitForSingleObject(pProInfo->hProcess,INFINITE);
	GetExitCodeProcess(pProInfo->hProcess,&dwExitCode);
	
L_ERROR:

	pProInfo->hProcess = NULL;

	// File Mapping Object �폜
	if(hFileMap) CloseHandle(hFileMap);
	if(hTrackFileMap) CloseHandle(hTrackFileMap);

	if(lpszTrackName) free(lpszTrackName);


	// �I�����b�Z�[�W���M

	// �L�����Z�������ꍇ
	if((*lpwStatus) == ID_COPYOFF) SendMessage(hWnd,WM_MYENDCOPY,0,2L);
	else{
		
		if(dwExitCode == 0) SendMessage(hWnd,WM_MYENDCOPY,0,0L); // ����
		else {
			SetForegroundWindow(hWnd);
			SendMessage(hWnd,WM_MYENDCOPY,0,1L); // ���s
		}
	}
				
	return 0;
	
}



//-------------------------------------
// WAVEFLT �N���_�C�A���O�̃v���V�[�W��
// �X���b�h���N�����邾��
LRESULT CALLBACK ProgDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	
	static HANDLE hThread = NULL;  // �X���b�h�̃n���h��
	static DWORD threadId; // �X���b�h ID
	static WORD wStatus;
	static LPEWCDATA lpEwcData;  
	static PROCESS_INFORMATION proInfo; // WaveFLT �̃v���Z�X���
	UINT uRet;
	
    switch (msg) {
		
	case WM_INITDIALOG:  // �_�C�A���O������
		
		if(hThread == NULL){
			
			// �_�C�A���O�𒆐S�Ɉړ�
			SetDlgCenter(hWnd);
			
			// �ۑ��X���b�h�N��
			proInfo.hProcess = NULL;
			wStatus = ID_COPYON;
			lpEwcData = (LPEWCDATA)lp;
			lpEwcData->hWnd = hWnd;
			lpEwcData->lpwStatus = &wStatus;
			lpEwcData->pProInfo = &proInfo;
			
			hThread = CreateThread(NULL,0,
				(LPTHREAD_START_ROUTINE)StartWaveFlt2,
				(LPVOID)lpEwcData,
				0,(LPDWORD)&threadId);
			
			if(hThread == NULL){	   
				MyMessageBox(hWnd, "�X���b�h�̋N���Ɏ��s���܂����B", 
					"Error", MB_OK|MB_ICONERROR);
				EndDialog(hWnd, IDCANCEL); 
			}
		}
		else {
			MyMessageBox(hWnd, "���ɃX���b�h�������Ă��܂��B", 
				"Error", MB_OK|MB_ICONERROR);
			EndDialog(hWnd, IDCANCEL); 
		}
		
		break;
		
	case WM_MYENDCOPY: // �R�s�[����
		
		hThread = NULL;
		
		// �t�H���[�h�Ɏ����Ă���
		SetForegroundWindow(hWnd);
		Sleep(100);
		
		if((DWORD)lp == 1) {
			MyMessageBox(hWnd,"���s���܂���\n\n�������������Ă�����x�R�s�[���s���ĉ�����", "ewc", MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
			uRet = IDCANCEL; // �ُ�I��
		}
		else if((DWORD)lp == 2){ // �L�����Z��
			uRet = IDCANCEL; // �ُ�I��
		}
		else
		{
			MyMessageBox(hWnd,"����ɏI�����܂����B", "ewc", MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND);
			uRet = IDOK; // ����I��
		}
		
		Sleep(500);
		FreeConsole();
		
		
		
		EndDialog(hWnd, uRet); 
		
		break;
		
	case WM_COMMAND:
		
		switch (LOWORD(wp)) {
			
		case IDCANCEL: // ��~
			
			wStatus = ID_COPYOFF;
			
			if(proInfo.hProcess != NULL){
				
				// Ctrl+Break ���M
				DWORD dwMode;
				GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),&dwMode);
				SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),dwMode|ENABLE_PROCESSED_INPUT);
				SetConsoleCtrlHandler(NULL,FALSE);
				Sleep(100);
				GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT,proInfo.dwProcessId);
				
				// �܂�WAVEFLT�������Ă�����
				if(WaitForSingleObject(proInfo.hProcess,10000) == WAIT_TIMEOUT){ 
					
					// WAVEFLT ��������~
					TerminateProcess(proInfo.hProcess,1);
					
					// ����ł��܂�WAVEFLT�������Ă�����
					if(WaitForSingleObject(proInfo.hProcess,10000) == WAIT_TIMEOUT){ 
						Sleep(1000);
						if(proInfo.hProcess != NULL){ 
							MyMessageBox(hWnd,"�X���b�h�̒�~�Ɏ��s���܂���\n\nEasy Wave Cutter ���ċN�����Ă�������", "ewc", MB_OK|MB_ICONERROR);
							uRet = IDCANCEL; // �ُ�I��
							EndDialog(hWnd, uRet); 
						}
					}
				}
			}
			
			return TRUE;
			
		}
		
		break;		
		
		default: return FALSE;
    }
	
    return TRUE;
}



//-------------------------------------------
// �f�[�^�Z�[�u
BOOL SaveCutData(HWND hWnd,
				 HINSTANCE hInst,
				 LPEWCDATA lpEwcData,
				 LPSTR lpszOrgFileName,  // �R�s�[���t�@�C����
				 LPSTR lpszNewFileName,  // �R�s�[��t�@�C����
				 // lpszNewFileName �� waveout �̎��̓f�o�C�X�ɏo��
				 BOOL bSplit  // �����^�����ǂ���
				 ){
	
				
	BOOL bMoveFile; // �㏑���Ō��t�@�C���̖��O��ύX�������ǂ���
	CHAR fPath[CHR_BUF],fDrive[CHR_BUF],fName[CHR_BUF],fExt[CHR_BUF];
	CHAR szReadFile[MAX_PATH];  // ���ۂɃ��[�h����t�@�C��
	
	bMoveFile = FALSE;
	strcpy(szReadFile,lpszOrgFileName);
	
	// �㏑����������
	if(_strnicmp(lpszOrgFileName,lpszNewFileName,MAX_PATH)==0){
		
		if(!bSplit){
			
			if(MyMessageBox(hWnd, "���ݕҏW���̃t�@�C���ɏ㏑���ۑ����Ă�낵���ł����H\n\n�����Ӂ�  ���ݕҏW���̃t�@�C���� *.old �ƃ��l�[������܂��B"
				,"ewc",MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2)==IDYES){
				
				// �㏑���̏ꍇ�͐̂̃t�@�C���� *.old �Ƀ��l�[������
				_splitpath(lpszOrgFileName,fDrive,fPath,fName,fExt);
				wsprintf(szReadFile,"%s%s%s.old",fDrive,fPath,fName);  
				
				// �t�@�C�����ύX
				MoveFile(lpszOrgFileName,szReadFile);
				bMoveFile = TRUE;
			}
			else return FALSE;
		}
		else
		{
			// �����̏ꍇ�͏o�͑��̖��O��ς���
			// -outfile_fm �I�v�V�����g���ăg���b�N����n���Ă�̂�
			// �K���Ȗ��O�ł悢
			_splitpath(lpszNewFileName,fDrive,fPath,fName,fExt);
			wsprintf(lpszNewFileName,"%s%s%s2%s",fDrive,fPath,fName,fExt);  
		}
	}
	
	lpEwcData->lpszOrgName = szReadFile;
	lpEwcData->lpszFileName = lpszNewFileName;
	
	// �_�C�A���O�{�b�N�X�\���ƕۑ��J�n
	if(DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_COPYFILE)
		,hWnd,(DLGPROC)ProgDlgProc,
		(LPARAM)lpEwcData) == IDCANCEL){
		
		// �ۑ����s
		
		// �t�@�C���폜
		DeleteFile(lpszNewFileName);
		
		// ���l�[�����Ă��猳�ɖ߂�
		if(bMoveFile) MoveFile(szReadFile,lpszOrgFileName);
		
		return FALSE;
	}

	return TRUE;
}





//-------------------------------------
// �W�����v�w��_�C�A���O�̃v���V�[�W��
LRESULT CALLBACK EditFindDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static LPJUMPDATA lpJumpData;
	static double*	lpdNoSoundBound;
	static LPDWORD lpdwNSoundCount;
	static LPDWORD lpdwSearchTime;
	static LPWORD lpwNSoundPos;
	static LPEWCDATA lpEwcData;
	DWORD dwTime,dwTimeHour,dwTimerMin,dwTimerSec,dwFoo;
	CHAR szStr[CHR_BUF];
	double dFoo,dMaxLevel;
	
    switch (msg) {
		
	case WM_INITDIALOG:  // �_�C�A���O������
		
		lpJumpData = (LPJUMPDATA)lp;
		lpdNoSoundBound = lpJumpData->lpdNoSoundBound;
		lpdwNSoundCount = lpJumpData->lpdwNSoundCount;
		lpdwSearchTime = lpJumpData->lpdwSearchTime;
		lpwNSoundPos = lpJumpData->lpwNSoundPos;
		lpEwcData = lpJumpData->lpEwcData;
		
		// �_�C�A���O�𒆐S�Ɉړ�
		SetDlgCenter(hWnd);
		
		// ���ԃZ�b�g
		dwTime = *lpdwSearchTime;
		dwTimeHour = dwTime/60/60;
		dwTimerMin = (dwTime/60)%60;
		dwTimerSec = dwTime%60;
		SendMessage(GetDlgItem(hWnd,IDC_SPINHOUR),UDM_SETRANGE,(WPARAM)0,(LPARAM)MAKELONG((short)999,0));
		SendMessage(GetDlgItem(hWnd,IDC_SPINHOUR),UDM_SETPOS,(WPARAM)0L,(LPARAM)MAKELONG((short)dwTimeHour,0));
		SendMessage(GetDlgItem(hWnd,IDC_SPINMIN),UDM_SETRANGE,(WPARAM)0,(LPARAM)MAKELONG((short)59,0));
		SendMessage(GetDlgItem(hWnd,IDC_SPINMIN),UDM_SETPOS,(WPARAM)0L,(LPARAM)MAKELONG((short)dwTimerMin,0));
		SendMessage(GetDlgItem(hWnd,IDC_SPINSEC),UDM_SETRANGE,(WPARAM)0,(LPARAM)MAKELONG((short)59,0));
		SendMessage(GetDlgItem(hWnd,IDC_SPINSEC),UDM_SETPOS,(WPARAM)0L,(LPARAM)MAKELONG((short)dwTimerSec,0));

		// �g���b�N�ݒ�
		SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_SETRANGE,(WPARAM)0,(LPARAM)MAKELONG((short)lpEwcData->dwSplitNum+1,1));
		SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_SETPOS,(WPARAM)0L,(LPARAM)MAKELONG((short)lpEwcData->dwCurTrack,0));

		// �X���C�_�[�̐ݒ�
		
		// ���x��
		SendMessage(GetDlgItem(hWnd,IDC_SLIDERBOUND),TBM_SETRANGE,(WPARAM)TRUE,(LPARAM)MAKELONG(0,(LONG)(-1*MINSEARCHBOUND)*5));
		SendMessage(GetDlgItem(hWnd,IDC_SLIDERBOUND),TBM_SETPOS,(WPARAM)TRUE,(LPARAM)(LONG)((-1*MINSEARCHBOUND + *lpdNoSoundBound)*5));
		wsprintf(szStr,"%s dB",myfcvt(*lpdNoSoundBound,1));
		Edit_SetText(GetDlgItem(hWnd,IDC_EDITBOUND),(LPCTSTR)szStr);
		
		// �b��
		SendMessage(GetDlgItem(hWnd,IDC_SLIDERCOUNT),TBM_SETRANGE,(WPARAM)TRUE,(LPARAM)MAKELONG(0,S_POINT_PER_SEC*10));
		SendMessage(GetDlgItem(hWnd,IDC_SLIDERCOUNT),TBM_SETPOS,(WPARAM)TRUE,(LPARAM)*lpdwNSoundCount);
		wsprintf(szStr,"%s �b",myfcvt(*lpdwNSoundCount/(double)S_POINT_PER_SEC,2));
		Edit_SetText(GetDlgItem(hWnd,IDC_EDITCOUNT),(LPCTSTR)szStr);
		
		//�`�F�b�N�{�b�N�X�A���W�I�{�^���̐ݒ�
		if(*lpwNSoundPos == NSOUND_TOP) Button_SetCheck(GetDlgItem(hWnd,IDC_RADIOTOP), BST_CHECKED);
        else if(*lpwNSoundPos == NSOUND_MID) Button_SetCheck(GetDlgItem(hWnd,IDC_RADIOMID), BST_CHECKED);
		else Button_SetCheck(GetDlgItem(hWnd,IDC_RADIOEND), BST_CHECKED);

		if(lpEwcData->editSaveData.bUseAvr) Button_SetCheck(GetDlgItem(hWnd,IDC_CHKAVR), BST_CHECKED);
		
		break;
		
	case WM_HSCROLL: // �X���C�_�𓮂�����
		
		// �b���w��
		if(GetDlgItem(hWnd,IDC_SLIDERCOUNT) == (HWND)lp){
			dwFoo = (DWORD)SendMessage(GetDlgItem(hWnd,IDC_SLIDERCOUNT),TBM_GETPOS,(WPARAM)0,(LPARAM)0);
			wsprintf(szStr,"%s �b",myfcvt(dwFoo/(double)S_POINT_PER_SEC,2));
			Edit_SetText(GetDlgItem(hWnd,IDC_EDITCOUNT),(LPCTSTR)szStr);
		}
		else{ // �������x��
			dFoo = MINSEARCHBOUND +(LONG)SendMessage(GetDlgItem(hWnd,IDC_SLIDERBOUND),TBM_GETPOS,(WPARAM)0,(LPARAM)0)/5.;
			wsprintf(szStr,"%s dB",myfcvt(dFoo,1));
			Edit_SetText(GetDlgItem(hWnd,IDC_EDITBOUND),(LPCTSTR)szStr);
		}
		
		break;
		
		
	case WM_COMMAND:
		
		switch (LOWORD(wp)) 
		{
			
		case IDC_BTJUMP:  // �w�肵�����ԂփW�����v
			
			// ���ԃZ�b�g
			dwTimeHour = min(999,SendMessage(GetDlgItem(hWnd,IDC_SPINHOUR),UDM_GETPOS,(WPARAM)0L,(LPARAM)0L));
			dwTimerMin = min(59,SendMessage(GetDlgItem(hWnd,IDC_SPINMIN),UDM_GETPOS,(WPARAM)0L,(LPARAM)0L));
			dwTimerSec = min(59,SendMessage(GetDlgItem(hWnd,IDC_SPINSEC),UDM_GETPOS,(WPARAM)0L,(LPARAM)0L));
			dwTime = dwTimeHour*60*60+dwTimerMin*60+dwTimerSec;
			*lpdwSearchTime = dwTime;
			
			EndDialog(hWnd, LOWORD(wp)); 
			break;
			
		case IDC_BTMOVETRACK:

			// �g���b�N�ݒ�
			lpEwcData->dwCurTrack = SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_GETPOS,(WPARAM)0L,(LPARAM)0L);
			EndDialog(hWnd,IDC_BTMOVETRACK);
			break;
			
		case IDC_BTSEARCH:  // �������T�[�`
		case IDC_BTSERCHSPLIT:  
			
			//�X���C�_
			*lpdNoSoundBound = MINSEARCHBOUND+(double)SendMessage(GetDlgItem(hWnd,IDC_SLIDERBOUND),TBM_GETPOS,(WPARAM)0,(LPARAM)0)/5;
			*lpdwNSoundCount = (DWORD)SendMessage(GetDlgItem(hWnd,IDC_SLIDERCOUNT),TBM_GETPOS,(WPARAM)0,(LPARAM)0);
			
			// ���W�I�{�b�N�X
			if(IsDlgButtonChecked(hWnd,IDC_RADIOTOP) == BST_CHECKED) *lpwNSoundPos = NSOUND_TOP;
			else if(IsDlgButtonChecked(hWnd,IDC_RADIOMID) == BST_CHECKED) *lpwNSoundPos = NSOUND_MID;
			else *lpwNSoundPos = NSOUND_END;

			if(IsDlgButtonChecked(hWnd,IDC_CHKAVR) == BST_CHECKED) lpEwcData->editSaveData.bUseAvr = TRUE;
			else lpEwcData->editSaveData.bUseAvr = FALSE;
			
			EndDialog(hWnd, LOWORD(wp)); 
			break;

		case IDC_BTOPTVAL: // �œK�l����
			
			dwFoo = (DWORD)SendMessage(GetDlgItem(hWnd,IDC_SLIDERCOUNT),TBM_GETPOS,(WPARAM)0,(LPARAM)0);
			dFoo = (double)dwFoo/S_POINT_PER_SEC;
			dMaxLevel = GetMaxWaveLevel(lpJumpData->waveFmt);
			
			dFoo = SearchPeak(lpJumpData->hdFile,
				lpJumpData->n64DataOffset,lpJumpData->waveFmt,
				lpJumpData->n64StartByte,dFoo);
			dFoo = max(20*log10(fabs(dFoo)/dMaxLevel),MINSEARCHBOUND);
			dFoo += .5;
			
			SendMessage(GetDlgItem(hWnd,IDC_SLIDERBOUND),TBM_SETPOS,(WPARAM)TRUE,(LPARAM)(LONG)((-1*MINSEARCHBOUND + dFoo)*5));
			wsprintf(szStr,"%s dB",myfcvt(dFoo,1));
			Edit_SetText(GetDlgItem(hWnd,IDC_EDITBOUND),(LPCTSTR)szStr);

		break;

			
		case IDCANCEL: // ��~
			EndDialog(hWnd, IDCANCEL); 
		}
		
		break;
		
		default:
			return FALSE;
    }
    return TRUE;
}




//EOF