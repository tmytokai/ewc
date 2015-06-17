// easy Wave Cutter
// Copyright (c) 1999-2015 Tomoya Tokairin
// ���C���̃v���V�[�W��

#include "editwave.h"
#include <math.h>


//-----------------------------------------------------
// �ĕ`��
VOID RedrawWindow(HWND hWnd){
	
	DWORD dwCurTime; // ���݂̍Đ�����(msec)
	
	LONGLONG n64CurByte;  // ���݂̃X�N���[���ʒu�܂ł̃o�C�g��
	LONG nScrPos; // ���݂̃X�N���[���o�[�̃|�W�V����
	LONG markedLineX; // �}�[�N���C���� x 
	LONG markedSubLineX; // ���}�[�N���C���� x 
	LONG markedSplitLineX[MAX_SPLITNUM]; // �X�v���b�g�}�[�N���C���� x
	CHAR szTrack[CHR_BUF];
	DWORD i;
	
	if(wCurStatus == ID_STATREADY || wCurStatus == ID_STATPLAY){ 
		
		// ���݂̈ʒu(�o�C�g)�v�Z
		nScrPos = scrInfo.nPos;
		n64CurByte = lpEwcData->waveFmt.nBlockAlign*((N64MaxBlock*FRAMESIZE*nScrPos)/nScrMax);
		
		// ���ݎ���(�~���b)�v�Z
		dwCurTime = (DWORD)(n64CurByte*1000/lpEwcData->waveFmt.nAvgBytesPerSec);

		// �}�[�N��
		markedLineX = CalcMarkedLineX(lpEwcData->waveFmt,n64CurByte,DwZoomX,RedrawRect,N64MarkedPosByte);
		markedSubLineX = CalcMarkedLineX(lpEwcData->waveFmt,n64CurByte,DwZoomX,RedrawRect,N64SubMarkedPosByte);
		for(i=0;i<lpEwcData->dwSplitNum;i++) markedSplitLineX[i] = CalcMarkedLineX(lpEwcData->waveFmt,n64CurByte,DwZoomX,RedrawRect,lpEwcData->n64SplitMarkedPosByte[i]);
		
		// �`��
		HakeiPaint(hWnd,hBufDC,RedrawRect,lpEwcData->waveFmt,
			lpEwcData->n64WaveDataOffset,
			lpEwcData->n64WaveDataSize,
			lpEwcData->hdFile,
			n64CurByte,
			markedLineX,markedSubLineX,markedSplitLineX,lpEwcData->dwSplitNum,
			DwZoomX,DwZoomY);
		
		i = 0;
		if(lpEwcData->dwSplitNum)
		{ // �������Ă���ꍇ
			while(i<lpEwcData->dwSplitNum){
				if(lpEwcData->n64SplitMarkedPosByte[i] > n64CurByte) break;
				i++;
			}
			strcpy(szTrack,lpEwcData->szTrackName[i]);
		}
		else{
			if(lpEwcData->szTrackFile[0]=='\0') 
				wsprintf(szTrack,"%s.%s",lpEwcData->szBaseName,lpEwcData->szExtName);
			else strcpy(szTrack,lpEwcData->szTrackName[0]);
		}
		SetEditStatus(hWnd,hBufDC,lpEwcData->waveFmt,
			44+lpEwcData->n64WaveDataSize,
			DwWaveTime,
			dwCurTime,
			N64MarkedPosByte,
			DbMarkedLevel,
			N64SubMarkedPosByte,
			DbSubMarkedLevel,
			lpEwcData->n64SplitMarkedPosByte,
			lpEwcData->dwSplitNum,i+1,szTrack,DwZoomX,DwZoomY);
		DrawMarkTriangle(hBufDC,nMarkedPos,nSubMarkedPos,lpEwcData->lnSplitMarkedPos,lpEwcData->dwSplitNum,nScrMax);			
		
		// �ĕ`��
		InvalidateRect(hWnd,&RedrawRect,FALSE);
	}
}



//-----------------------------------------------------
// �t�@�C���̐V�K�I�[�v���Ɖ�ʁA�ϐ�������
BOOL LoadNewData(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	
	// �`��p�G�p
	HDC hDC;
	RECT deskRt,curRt;
	LONG nWindowX,nWindowY;
	
	LONG nScrPos; // ���݂̃X�N���[���o�[�̃|�W�V����

	double dLevel[2];
	double dMaxLevel;
	
	LONG i;
	CHAR szStr[CHR_BUF];
	
	// �t�H�[�}�b�g�擾
	SetWaveFmt(&lpEwcData->waveFmt,2,22050,16,1); // �_�~�[�w�b�_
	lpEwcData->n64WaveDataSize = 100;  // �_�~�[
	lpEwcData->n64WaveDataOffset = 44; // �_�~�[
	wCurStatus = ID_STATCLOSE;
	
	if(lpEwcData->szLoadFile[0] != '\0')
	{
		// WAVE �t�@�C����?
		if(GetWaveFormat(lpEwcData->szLoadFile,&lpEwcData->waveFmt,
			&lpEwcData->n64WaveDataSize,&lpEwcData->n64WaveDataOffset,szStr))
		{
			DwWaveTime = (DWORD)((lpEwcData->n64WaveDataSize*1000)/lpEwcData->waveFmt.nAvgBytesPerSec);
			wCurStatus = ID_STATREADY;

			// �t�@�C���I�[�v��
			lpEwcData->hdFile = CreateFile(lpEwcData->szLoadFile,GENERIC_READ, 
				0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL); 
			
			if(lpEwcData->hdFile == INVALID_HANDLE_VALUE){
				MyMessageBox(hWnd, "�t�@�C�����J���܂���B", "Error", MB_OK|MB_ICONERROR);	
				return FALSE;
			}
		}
		else
		{
			MyMessageBox(hWnd,szStr, "Error", MB_OK|MB_ICONERROR);
		}

	}
	
	
	// Window ���T�C�Y(�`�����l�����ō�����ς���)
	LONG frameHeight;
	GetClientRect(hWnd,&RedrawRect);
	RedrawRect.right = RedrawRect.left + FRAMESIZE+ 2*EDITUPDATERECTLEFT;
	frameHeight = lpEwcData->waveFmt.nChannels == 2 ? FRAMESIZE/4+8+FRAMESIZE/4 + 10: FRAMESIZE/4+10;
	RedrawRect.bottom = RedrawRect.top + EDITUPDATERECTTOP+
		(wCurStatus == ID_STATREADY)*(EDITSTATUSMARGIN1+EDITSTATUSSIZE+EDITSTATUSMARGIN2+
		frameHeight);
	AdjustWindowRect(&RedrawRect,WS_CAPTION,FALSE);
	
	// �ړ����W�v�Z
	GetWindowRect(GetDesktopWindow(), &deskRt);
	GetWindowRect(hWnd, &curRt);
	
	// ���� Easy Wave Cutter ���N�����Ă�ꍇ
	if(hPreEwcWnd != NULL){
		
		RECT preRt;
		GetWindowRect(hPreEwcWnd, &preRt);
		curRt.left = preRt.left+20;
		curRt.top = preRt.top+20;
		
		if(curRt.left+(RedrawRect.right-RedrawRect.left) > deskRt.right || 
			curRt.top+(RedrawRect.bottom-RedrawRect.top) > deskRt.bottom){
			curRt.left = 0;
			curRt.top = 0;
		}
		
		hPreEwcWnd = NULL;
	}
	
	nWindowX = max(0,min(curRt.left,deskRt.right -(RedrawRect.right-RedrawRect.left)));
	nWindowY = max(0,min(curRt.top,deskRt.bottom-(RedrawRect.bottom-RedrawRect.top)));
	
	SetWindowPos(hWnd,NULL,nWindowX,nWindowY,RedrawRect.right-RedrawRect.left,RedrawRect.bottom-RedrawRect.top,SWP_NOREPOSITION);
	
	
	// �`��̈�̃T�C�Y�擾
	GetClientRect(hWnd,&RedrawRect);
	
	// �O�Ƀt�@�C�����J���Ă����炱���ŗ���ʃo�b�t�@�폜
	if(hBufDC != NULL){ 
		DeleteDC(hBufDC);
		DeleteObject(hBufBit);
	}
	
	
	// ����ʃo�b�t�@��(��)�쐬
	hDC = GetDC(hWnd);
	hBufBit = CreateCompatibleBitmap(hDC,RedrawRect.right - RedrawRect.left,RedrawRect.bottom - RedrawRect.top);
	hBufDC = CreateCompatibleDC(hDC);
	SelectObject(hBufDC,hBufBit);
	ReleaseDC(hWnd,hDC);
	
	// ������ʕ`��
	InitEditScreenDraw(hWnd,hInst,hBufDC);
	
	// ��ʍX�V�̈�Z�b�g
	RedrawRect.top += EDITUPDATERECTTOP;
	
	// �I���W�i���f�[�^�ۑ�
	LARGE_INTEGER liFoo;
	liFoo.LowPart = GetFileSize(lpEwcData->hdFile,(LPDWORD)&(liFoo.HighPart));
	N64OrgFileSize = liFoo.QuadPart;
	N64OrgDataSize = lpEwcData->n64WaveDataSize;
	N64OrgDataOffset = lpEwcData->n64WaveDataOffset;
	
	// �X�N���[���o�[������
	hScrWnd = GetDlgItem(hWnd,IDC_SBTIME);
	memset(&scrInfo,0,sizeof(SCROLLINFO));
	scrInfo.cbSize = sizeof(SCROLLINFO);
	scrInfo.fMask = SIF_POS|SIF_RANGE|SIF_TRACKPOS;
	if(wCurStatus == ID_STATREADY){
		N64MaxBlock = lpEwcData->n64WaveDataSize/(FRAMESIZE*lpEwcData->waveFmt.nBlockAlign);
		nScrMax = N64MaxBlock > MAXLONG ? MAXLONG : (LONG)N64MaxBlock+1;
		scrInfo.nMax = nScrMax;
		EnableWindow(hScrWnd,TRUE);
		SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
	}
	else{
		scrInfo.nMax = 0;
		EnableWindow(hScrWnd,FALSE);
		SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,FALSE);
	}
	
	// �e�p�����[�^������
	nScrPos = 0;
	nMarkedPos = 0;
	N64MarkedPosByte =	0;
	nSubMarkedPos = nScrMax;
	N64SubMarkedPosByte = lpEwcData->n64WaveDataSize;
	memset(lpEwcData->lnSplitMarkedPos,0,sizeof(LONG)*MAX_SPLITNUM);
	memset(lpEwcData->n64SplitMarkedPosByte,0,sizeof(LONGLONG)*MAX_SPLITNUM);
	lpEwcData->dwSplitNum = 0;
	bUpdate = FALSE;
	
	// ���x���擾
	dMaxLevel = GetMaxWaveLevel(lpEwcData->waveFmt);
	GetLevelatPoint(lpEwcData->waveFmt,lpEwcData->hdFile,dLevel,lpEwcData->n64WaveDataOffset+N64MarkedPosByte);
	DbMarkedLevel[0] = 20*log10(fabs(dLevel[0])/dMaxLevel);
	DbMarkedLevel[1] = 20*log10(fabs(dLevel[1])/dMaxLevel);
	
	GetLevelatPoint(lpEwcData->waveFmt,lpEwcData->hdFile,dLevel,lpEwcData->n64WaveDataOffset+N64SubMarkedPosByte);
	DbSubMarkedLevel[0] = 20*log10(fabs(dLevel[0])/dMaxLevel);
	DbSubMarkedLevel[1] = 20*log10(fabs(dLevel[0])/dMaxLevel);
	
	// undo �f�[�^������
	undoData.wCurPos = 0;
	for(i=0;i<UNDOLEVEL;i++) undoData.bDataEmpty[i] = FALSE;
	
	// �^�C�g���X�V
	if(wCurStatus == ID_STATREADY)wsprintf(szStr,"ewc - %s",lpEwcData->szLoadFile);
	else wsprintf(szStr,"ewc");
	SetWindowText(hWnd,szStr); 
	
	
	// �ĕ`��
	RedrawWindow(hWnd);
	
	return TRUE;
}






//-----------------------------------------------------
// ���t�@�C�������̂܂܍ăI�[�v������֐�
BOOL OpenCurrentData(HWND hWnd){		
	
	if(wCurStatus == ID_STATREADY){
		
		lpEwcData->hdFile = CreateFile(lpEwcData->szLoadFile,GENERIC_READ,0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL); 
		if(lpEwcData->hdFile == INVALID_HANDLE_VALUE){
			MyMessageBox(hWnd, "�t�@�C�����J���܂���B\n�t�@�C�������݂��邩�m�F���ĉ������B", "Error", MB_OK|MB_ICONERROR);
			lpEwcData->szLoadFile[0]='\0';
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}




//--------------------------------------------------------
// �ҏW�J�n�֐�
BOOL StartEditWave(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	
	CHAR szStr[CHR_BUF],szStr2[CHR_BUF],szSaveDir[CHR_BUF];
	LONG nReturn;
	DWORD dwFoo;
	
	// �ҏW�_�C�A���O�\��
	if((nReturn = DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_EDITWEDIT)
		,hWnd,(DLGPROC)EditMenuDlgProc,(LPARAM)lpEwcData)) == IDCANCEL) return FALSE;
	
	if(wCurStatus == ID_STATREADY){
		
		
		if(nReturn == IDC_BTSTART){
			
			// �ۑ��f�B���N�g���ݒ�
			if(lpEwcData->szSaveDir[0] == '\0'){  // �Z�[�u�t�@�C�����ݒ肳��Ė����ꍇ
				_splitpath(lpEwcData->szLoadFile,szStr,szStr2,NULL,NULL);
				wsprintf(szSaveDir,"%s%s",szStr,szStr2);
			}
			else wsprintf(szSaveDir,"%s\\",lpEwcData->szSaveDir);
			
			// �ۑ��t�@�C���ݒ�
			if(lpEwcData->bSplit){ // ����
				wsprintf(lpEwcData->szSaveFile,"%s%s.%s",szSaveDir,lpEwcData->szBaseName,lpEwcData->szExtName);
				wsprintf(szStr2,"����\n\n�o�̓f�B���N�g�� %s\n\n",szSaveDir);
			}
			else if(lpEwcData->bCutCm){ // CM �J�b�g
				if(lpEwcData->szTrackFile[0]=='\0') 
					wsprintf(lpEwcData->szSaveFile,"%s%s.%s",szSaveDir,lpEwcData->szBaseName,lpEwcData->szExtName);
				else 
					wsprintf(lpEwcData->szSaveFile,"%s%s",szSaveDir,lpEwcData->szTrackName[0]);
				wsprintf(szStr2,"CM �J�b�g\n\n�o�̓t�@�C�� : %s\n\n",lpEwcData->szSaveFile);
			}
			else
			{
				if(lpEwcData->bCutTrack && lpEwcData->dwSplitNum)
				{ // �؂��� 
					dwFoo = GetCurTrack(lpEwcData->dwSplitNum,
						lpEwcData->n64SplitMarkedPosByte,lpEwcData->waveFmt.nBlockAlign,
						N64MaxBlock,scrInfo.nPos,nScrMax);
					wsprintf(lpEwcData->szSaveFile,"%s%s",szSaveDir,lpEwcData->szTrackName[dwFoo-1]);
					wsprintf(szStr2,"�g���b�N %d �؂���\n\n�o�̓t�@�C�� : %s\n\n",dwFoo,lpEwcData->szSaveFile);
				}
				else{ // �S�̕ۑ�
					if(lpEwcData->szTrackFile[0]=='\0') 
						wsprintf(lpEwcData->szSaveFile,"%s%s.%s",szSaveDir,lpEwcData->szBaseName,lpEwcData->szExtName);
					else 
						wsprintf(lpEwcData->szSaveFile,"%s%s",szSaveDir,lpEwcData->szTrackName[0]);
					wsprintf(szStr2,"�S�̕ۑ�\n\n�o�̓t�@�C�� : %s\n\n",lpEwcData->szSaveFile);

				}
			}

			wsprintf(szStr,"%s���̓t�@�C�� : %s\n\n�ҏW���J�n���܂���?",szStr2,lpEwcData->szLoadFile);
			if(MyMessageBox(hWnd, szStr,"ewc: WAVEFLT2 �N��", MB_YESNO|MB_ICONQUESTION)==IDNO) return FALSE;
			
		}
		else strcpy(lpEwcData->szSaveFile,"waveout");
		
		
		
		//---------------
		// WAVEFTL �N��
		//---------------
		
		// �t�@�C���N���[�Y
		if(lpEwcData->hdFile) {
			CloseHandle(lpEwcData->hdFile);
			lpEwcData->hdFile = NULL;
		}

		//------------------------------
		// �_�C�A���O�ɓn���f�[�^�Z�b�g

		// �R�s�[�u���b�N�̃I�t�Z�b�g�T�C�Y�ƃT�C�Y�v�Z
		if(lpEwcData->bCutTrack && lpEwcData->dwSplitNum){  // �g���b�N�؂�o��

			dwFoo = GetCurTrack(lpEwcData->dwSplitNum,
				lpEwcData->n64SplitMarkedPosByte,lpEwcData->waveFmt.nBlockAlign,
				N64MaxBlock,scrInfo.nPos,nScrMax);
			
			if(dwFoo-1 == 0) lpEwcData->n64NewDataOffset = lpEwcData->n64WaveDataOffset;
			else
				lpEwcData->n64NewDataOffset = lpEwcData->n64WaveDataOffset+lpEwcData->n64SplitMarkedPosByte[dwFoo-2];
		
			if(dwFoo-1 == lpEwcData->dwSplitNum)
				lpEwcData->n64NewDataSize = lpEwcData->n64WaveDataSize - lpEwcData->n64SplitMarkedPosByte[dwFoo-2];
			else lpEwcData->n64NewDataSize = lpEwcData->n64SplitMarkedPosByte[dwFoo-1] - lpEwcData->n64SplitMarkedPosByte[dwFoo-2];
		}
		else{ // �����@or ���̂܂ܕۑ� or CM �J�b�g
			lpEwcData->n64NewDataOffset = lpEwcData->n64WaveDataOffset;
			lpEwcData->n64NewDataSize = lpEwcData->n64WaveDataSize;
		}

		// WAVEFLT2 �N��
		if(SaveCutData(hWnd,hInst,lpEwcData,lpEwcData->szLoadFile,lpEwcData->szSaveFile,lpEwcData->bSplit))
		{
			// �ۑ�����

			if(lpEwcData->bSplit){ // �������s�̏ꍇ
				// ���t�@�C�������̂܂܍ăI�[�v��
				if(!OpenCurrentData(hWnd)) LoadNewData(hWnd,msg,wp,lp);
				return TRUE;
			}
			
			//�ۑ������t�@�C�������݂��邩�`�F�b�N
			lpEwcData->hdFile = CreateFile(lpEwcData->szSaveFile,GENERIC_READ
				,0, 0, OPEN_EXISTING,FILE_ATTRIBUTE_READONLY, NULL); 
			
			if(lpEwcData->hdFile == INVALID_HANDLE_VALUE){ // ���݂��Ȃ�������
				
				// ���t�@�C�������̂܂܍ăI�[�v��
				if(!OpenCurrentData(hWnd)) LoadNewData(hWnd,msg,wp,lp);
				return TRUE;
			}
			
			CloseHandle(lpEwcData->hdFile);
			lpEwcData->hdFile = NULL;
			
			// �㏑���ۑ���������
			if(_strnicmp(lpEwcData->szLoadFile,lpEwcData->szSaveFile,MAX_PATH)==0){ 
				MyMessageBox(hWnd, "�ۑ����܂����B","ewc", MB_OK|MB_ICONINFORMATION);	
				// �t�@�C���ăI�[�v��
				LoadNewData(hWnd,msg,wp,lp);
				return TRUE;
			}
			else { // �Ⴄ���O�ŕۑ��������瑼�̃E�B���h�E�J���Č��f�[�^�ăI�[�v��
				
				wsprintf(szStr,"�ۑ����܂����B\n\n%s ��V�����E�B���h�E�ŊJ���܂����H",lpEwcData->szSaveFile);
				if(MyMessageBox(hWnd, szStr
					,"ewc", MB_YESNO|MB_ICONQUESTION)==IDYES){
					
					SaveIniFile(lpEwcData,lpEwcData->szIniDatFile);
					
					// �V�����E�B���h�E�N��
					GetModuleFileName(NULL,szStr,MAX_PATH); 
					wsprintf(szStr,"%s \"%s\" -dev %d",
						szStr,lpEwcData->szSaveFile,lpEwcData->uDeviceID);
					
					STARTUPINFO startInfo;
					PROCESS_INFORMATION proInfo;
					memset(&startInfo,0,sizeof(STARTUPINFO));
					startInfo.cb = sizeof(STARTUPINFO);
					if(!CreateProcess(NULL,szStr,NULL,NULL,FALSE,0,NULL,NULL,&startInfo,&proInfo))
						MyMessageBox(hWnd, "�V�����E�B���h�E�̋N���Ɏ��s���܂����B","ewc", MB_OK|MB_ICONERROR);	
				}
			}
		}
		
		// ���t�@�C�����ăI�[�v��
		OpenCurrentData(hWnd);
		return TRUE;
		
	}
	
	return FALSE;
}


//---------------------------------------------------------
// ���݈ʒu�̈ړ��֐�
VOID MovePos(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	
	DWORD dwCurTime; // ���݂̍Đ�����
	
	LONGLONG n64CurByte;  // ���݂̃X�N���[���ʒu�܂ł̃o�C�g��
	LONG nScrPos; // ���݂̃X�N���[���o�[�̃|�W�V����
	static DWORD dwJumpMark = 0;

	// �G�p
	DWORD dwMove,dwByte,dwCurTrack,i,i2;
	
	// �J���ĂȂ������炻�̂܂܃��^�[��
	if(wCurStatus == ID_STATCLOSE) return; 
	
	// ���݈ʒu
	nScrPos = scrInfo.nPos;
	n64CurByte = lpEwcData->waveFmt.nBlockAlign*((FRAMESIZE*N64MaxBlock*nScrPos)/nScrMax);
	
	// �ړ��ʌv�Z
	if(nScrMax == 0) { // �T�C�Y�� 0 �̃t�@�C���͈ړ����Ȃ�
		dwByte = 0;
		dwMove = 0;
	}
	else if(wCurStatus == ID_STATREADY || wCurStatus == ID_STATPLAY) // Wave �̏ꍇ
	{
		dwByte = (DWORD)(lpEwcData->n64WaveDataSize/nScrMax); // �X�N���[����񕪂̃o�C�g���v�Z
		
		switch(LOWORD(wp))
		{

		case IDC_BTREW:
		case IDC_BTFORWARD:
			
			if(GetKeyState(VK_SHIFT)&0x80) // �V�t�g�����Ȃ��炾�� 1 �b
				dwMove = (DWORD)(1*lpEwcData->waveFmt.nAvgBytesPerSec)/dwByte+1; //1 �b���̈ړ��ʌv�Z
			else
				dwMove = 1;

			break;

		case IDC_BTREW2:
		case IDC_BTFORWARD2:

			if(GetKeyState(VK_SHIFT)&0x80) // �V�t�g�����Ȃ��炾��
				dwMove = (DWORD)(60*lpEwcData->waveFmt.nAvgBytesPerSec)/dwByte+1; //60 �b���̈ړ��ʌv�Z
			else
				dwMove = (DWORD)(15*lpEwcData->waveFmt.nAvgBytesPerSec)/dwByte+1; //15 �b���̈ړ��ʌv�Z

			break;

		case IDC_BTREW3:
		case IDC_BTFORWARD3:

			if(GetKeyState(VK_SHIFT)&0x80) // �V�t�g�����Ȃ��炾��
				dwMove = nScrMax; // �t�@�C���̑O��
			else
				dwMove = (DWORD)(300*lpEwcData->waveFmt.nAvgBytesPerSec)/dwByte+1; //300 �b���̈ړ��ʌv�Z

		}
	
	}


	// �X�N���[���ʌv�Z
	switch(LOWORD(wp)){
		
	case IDC_BTJUMPNEXT:
	case IDC_BTJUMPNEXT2:
	case IDC_BTJUMPNEXT3:
	case IDC_BTJUMPBACK:
	case IDC_BTJUMPBACK2:
	case IDC_BTJUMPBACK3:
		
				// �g���b�N�ړ�
		if(lpEwcData->dwSplitNum){
			
			dwCurTrack = GetCurTrack(lpEwcData->dwSplitNum,
				lpEwcData->n64SplitMarkedPosByte,lpEwcData->waveFmt.nBlockAlign,
				N64MaxBlock,scrInfo.nPos,nScrMax);
			i = 0;
			switch(LOWORD(wp)){
				
			case IDC_BTJUMPNEXT3:  i+=5;
			case IDC_BTJUMPNEXT2:  i+=5;
			case IDC_BTJUMPNEXT:  

				if(dwCurTrack-1+i < lpEwcData->dwSplitNum){
					i2 = 1;
					while(
						GetCurTrack(lpEwcData->dwSplitNum,
						lpEwcData->n64SplitMarkedPosByte,
						lpEwcData->waveFmt.nBlockAlign,
						N64MaxBlock,
						lpEwcData->lnSplitMarkedPos[dwCurTrack-1+i]+i2,
						nScrMax) == dwCurTrack) i2++;
					nScrPos = lpEwcData->lnSplitMarkedPos[dwCurTrack-1+i]+i2;
				}
				else nScrPos = nScrMax;
				
				break;
				
			case IDC_BTJUMPBACK3: i+=5;
			case IDC_BTJUMPBACK2: i+=5;
			case IDC_BTJUMPBACK:  
				
				if(nScrPos == lpEwcData->lnSplitMarkedPos[dwCurTrack-2]+1) // �擪�ɂ���ꍇ
				{ 
					if(dwCurTrack-1 >1+i) nScrPos = lpEwcData->lnSplitMarkedPos[dwCurTrack-3-i]+1;
					else nScrPos = 0;
					
				}
				else
				{
					if(dwCurTrack-1 > i) nScrPos = lpEwcData->lnSplitMarkedPos[dwCurTrack-2-i]+1;
					else nScrPos = 0;
				}
				
				break;
			}
		}
		
		
		break;
		
	case IDC_BTJUMP:
		
		// �}�[�N�ɃW�����v
		if(GetKeyState(VK_SHIFT)&0x80) dwJumpMark = 1;// �V�t�g�����Ȃ��炾�ƕ��}�[�N�ɔ��
		else dwJumpMark = 0;
		if(dwJumpMark == 0)	nScrPos = nMarkedPos;
		else nScrPos = nSubMarkedPos;
		
		break;
		
		
	case IDC_BTREW:
	case IDC_BTREW2:
	case IDC_BTREW3:
		
		nScrPos = nScrPos-(LONG)dwMove;
		break;
		
	case IDC_BTFORWARD:
	case IDC_BTFORWARD2:
	case IDC_BTFORWARD3:
		
		nScrPos = nScrPos+(LONG)dwMove;
		break;
		
	default:
		
		if(dwByte == 0) nScrPos = 0;
		else nScrPos = (LONG)((LONGLONG)DwSearchTime*lpEwcData->waveFmt.nAvgBytesPerSec/dwByte)+1; 
		break;
		
	}
	
	nScrPos = max(0,min(nScrPos,nScrMax));
	
	// �X�N���[���o�[�ʒu�Z�b�g
	scrInfo.nPos = nScrPos;
	SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
	n64CurByte = lpEwcData->waveFmt.nBlockAlign*((FRAMESIZE*N64MaxBlock*nScrPos)/nScrMax);
	
	// �J�n����(�~���b)�v�Z
	dwCurTime = (DWORD)(n64CurByte*1000/lpEwcData->waveFmt.nAvgBytesPerSec);
	
	// �����Đ����Ȃ�ăV�[�N
	if(wCurStatus == ID_STATPLAY) SeekPlayWave(dwCurTime);
	
	// �ĕ`��
	RedrawWindow(hWnd);
	
}


//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------


//-------------------------------------------------------------------
// ���C���v���V�[�W��
LRESULT CALLBACK EditWaveProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	
	
	CHAR szNewFileName[MAX_PATH]; 
	
	DWORD dwCurTime; // ���݂̍Đ�����
	LONGLONG n64CurByte;  // ���݂̃X�N���[���ʒu�܂ł̃o�C�g��
	LONG nScrPos; // ���݂̃X�N���[���o�[�̃|�W�V����
	LONGLONG n64StartByte,n64EndByte;
	WORD wUndoPos; // UNDO �p�G�p

	
	// �`��p�G�p
	HDC hDC;
	RECT curRt;
	
	// �G�p
	LONG i,i2; 
	CHAR szStr[CHR_BUF];
	DWORD dwFoo,dwByte;
	LONGLONG n64Foo,n64Foo2;
	
    switch (msg) {
		
	case WM_CREATE:  // �_�C�A���O������
		
		// �R�����R���g���[��������
		InitCommonControls();
		
		// �G�f�B�g�f�[�^�Z�b�g
		lpEwcData = (LPEWCDATA)(((LPCREATESTRUCT)lp)->lpCreateParams);

		// �{�^���̍쐬
		InitEditScreenDraw(hWnd,hInst,NULL);
		
		// �����t�@�C�����Z�b�g
		strcpy(lpEwcData->szLoadFile,lpEwcData->szIniFileName);
		
		// �C���X�^���X�擾
		hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
		
		// �A�C�R�����[�h
		hIcon[0] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONREW));
		hIcon[1] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONPLAYSTOP));
		hIcon[2] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONFOR));
		hIcon[3] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONREW2));
		hIcon[4] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONFOR2));
		hIcon[5] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONMARKBACK));
		hIcon[6] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONMARK));
		hIcon[7] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONMARKNEXT));
		hIcon[8] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONREW3));
		hIcon[9] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONFOR3));
		hIcon[10] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONMARKBACK2));
		hIcon[11] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONMARKBACK3));
		hIcon[12] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONMARKNEXT2));
		hIcon[13] = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICONMARKNEXT3));
		
	
		// �p�����[�^������
		DwZoomX = 8;
		DwZoomY = 1;
		DbNoSoundBound = -96;
		dwNSoundCount = S_POINT_PER_SEC;
		wNSoundPos = NSOUND_MID;
		wCurStatus = ID_STATCLOSE;
		SetWaveFmt(&lpEwcData->waveFmt,2,22050,16,1); // �_�~�[�w�b�_
		lpEwcData->n64WaveDataSize = 100;  // �_�~�[
		lpEwcData->n64WaveDataOffset = 44; // �_�~�[

		// ���ɋN�����Ă��� ewc �� HWND �̎擾
		hPreEwcWnd = GetPreEwcHWND();
		
		// �ړ�
		SetWindowPos(hWnd,NULL,lpEwcData->x,lpEwcData->y,0,0,SWP_NOSIZE|SWP_NOREPOSITION);
	
		// �t�@�C���� Drop ����
		DragAcceptFiles(hWnd,TRUE);
		
		// �t�@�C���̐V�K�I�[�v���Ɖ�ʁA�ϐ�������
		LoadNewData(hWnd,msg,wp,lp);

		
		// �ĕ`��
		RedrawWindow(hWnd);

		return TRUE;
		
		// ----------------------------------------------------------
		
	case WM_DROPFILES: // �t�@�C�� Drop ����	 
		
		// �Đ����łȂ��Ȃ�
		if(wCurStatus != ID_STATPLAY){	
			
			// �g�b�v�Ɏ����Ă���
			SetForegroundWindow(hWnd);
			
			// �t�@�C�����擾
			HDROP hDrop; 
			UINT uLng;
			hDrop = (HDROP)wp; 
			uLng = DragQueryFile(hDrop,0,lpEwcData->szLoadFile,MAX_PATH); 
			DragFinish(hDrop);
			
			// �t�@�C���N���[�Y
			if(lpEwcData->hdFile) {
				CloseHandle(lpEwcData->hdFile);
				lpEwcData->hdFile = NULL;
			}
			
			// �t�@�C���ăI�[�v��
			LoadNewData(hWnd,msg,wp,lp);
			return TRUE;
		}
		
		break;		
		
		// ----------------------------------------------------------
		
		
	case WM_CTLCOLORSCROLLBAR: // �X�N���[���p�[�̃u���V�Z�b�g
		return ((BOOL)GetStockObject(BLACK_BRUSH)) ;    
		break;
		
		
		
		
		// ----------------------------------------------------------
		
	case WM_PAINT: // �ĕ`��̂�
		
		PAINTSTRUCT ps;
		hDC = BeginPaint(hWnd,&ps);
		
		// ����ʂ���R�s�[
		BitBlt(hDC,ps.rcPaint.left,ps.rcPaint.top,
			ps.rcPaint.right-ps.rcPaint.left,
			ps.rcPaint.bottom-ps.rcPaint.top,
			hBufDC,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
		
		ReleaseDC(hWnd,hDC);
		
		return TRUE;
		
		
		// ----------------------------------------------------------
		
	case WM_DRAWITEM: // �{�^���`��
		
		UINT uCtlID;
		UINT uButtonStat;
		LPDRAWITEMSTRUCT lpDrawItem;
		
		uCtlID = (UINT)wp;
		lpDrawItem = (LPDRAWITEMSTRUCT)lp;
		uButtonStat = MYID_RELEASEBUTTON;
		
		if(lpDrawItem->CtlType == ODT_BUTTON){
			if(SendMessage(lpDrawItem->hwndItem,BM_GETSTATE,0,0) & BST_PUSHED)
				uButtonStat = MYID_PUSHBUTTON;
			DrawMyButton(hWnd,uCtlID,lpDrawItem->hDC,lpDrawItem->rcItem,uButtonStat,hIcon,NULL);
		}
		
		break;
		
		// ----------------------------------------------------------
		
	case WM_HSCROLL:  // �X�N���[���p�[�̏���
		
		// ���݈ʒu�Z�b�g
		nScrPos = SetScrBarInfo(hScrWnd,&scrInfo,wp);
		n64CurByte = lpEwcData->waveFmt.nBlockAlign*((FRAMESIZE*N64MaxBlock*nScrPos)/nScrMax);
		
		// WAVE �����Đ����Ȃ�ăV�[�N
		dwCurTime = (DWORD)(n64CurByte*1000/lpEwcData->waveFmt.nAvgBytesPerSec);
		if(wCurStatus == ID_STATPLAY) SeekPlayWave(dwCurTime);
		
		// �ĕ`��
		RedrawWindow(hWnd);
		return TRUE;
		break;
		
		// ----------------------------------------------------------
		
	case WM_LBUTTONDOWN:  // ���{�^��������
	case WM_RBUTTONDOWN:  // �E�{�^��������
	case WM_LBUTTONDBLCLK:  // ���{�^���_�u���N���b�N
		
		if(wCurStatus == ID_STATREADY && HIWORD(lp) > RedrawRect.top+EDITSTATUSSIZE){
			
			if(!lpEwcData->bShiftLock || GetKeyState(VK_SHIFT)&0x80){
				
				// ���݂̈ʒu�Z�b�g
				nScrPos = scrInfo.nPos; 
				n64CurByte = lpEwcData->waveFmt.nBlockAlign*((FRAMESIZE*N64MaxBlock*nScrPos)/nScrMax);
				
				// �N���b�N�����_�ɑΉ�����o�C�g�����擾
				if(CalcMarkedPosByte(&n64Foo,lpEwcData->waveFmt,
					lpEwcData->n64WaveDataSize,
					n64CurByte,DwZoomX,RedrawRect,LOWORD(lp))){

					DWORD dwSplitPos;
					double dLevel[2],dMaxLevel;

					dMaxLevel = GetMaxWaveLevel(lpEwcData->waveFmt);
					GetLevelatPoint(lpEwcData->waveFmt,lpEwcData->hdFile,dLevel,
						lpEwcData->n64WaveDataOffset+n64Foo);
					
					// �}�[�N�����o�C�g���Z�b�g
					if(msg == WM_LBUTTONDOWN){
						N64MarkedPosByte =	n64Foo; // �N���b�N
						DbMarkedLevel[0] = 20*log10(fabs(dLevel[0])/dMaxLevel);
						DbMarkedLevel[1] = 20*log10(fabs(dLevel[1])/dMaxLevel);
					}
					else if(msg == WM_RBUTTONDOWN){	// �E�N���b�N
						N64SubMarkedPosByte = n64Foo;
						DbSubMarkedLevel[0] = 20*log10(fabs(dLevel[0])/dMaxLevel);
						DbSubMarkedLevel[1] = 20*log10(fabs(dLevel[0])/dMaxLevel);
						
						// �X�v���b�g�}�[�N�폜
						if(lpEwcData->dwSplitNum)
						{
							
						n64Foo2 = n64Foo > 512*lpEwcData->waveFmt.nBlockAlign ? 
								n64Foo - 512*lpEwcData->waveFmt.nBlockAlign : 0;
							
							for(dwSplitPos=0; dwSplitPos < lpEwcData->dwSplitNum; dwSplitPos++)
							{
								if(lpEwcData->n64SplitMarkedPosByte[dwSplitPos] 
									<= n64Foo + 512*lpEwcData->waveFmt.nBlockAlign &&
									lpEwcData->n64SplitMarkedPosByte[dwSplitPos] >= n64Foo2) break;
							}
 							if(dwSplitPos < lpEwcData->dwSplitNum)
							{
								// �폜
								memmove(lpEwcData->n64SplitMarkedPosByte+dwSplitPos,
									lpEwcData->n64SplitMarkedPosByte+dwSplitPos+1,sizeof(LONGLONG)*(lpEwcData->dwSplitNum-dwSplitPos-1));
								memmove(lpEwcData->lnSplitMarkedPos+dwSplitPos,
									lpEwcData->lnSplitMarkedPos+dwSplitPos+1,sizeof(LONG)*(lpEwcData->dwSplitNum-dwSplitPos-1));
								lpEwcData->dwSplitNum--;
							}
						}
						
					}
					else // �_�u���N���b�N
					{
						if(lpEwcData->dwSplitNum+1==MAX_SPLITNUM) return TRUE;

						for(i=0;i<(LONG)lpEwcData->dwSplitNum;i++)
							if(lpEwcData->n64SplitMarkedPosByte[i] == n64Foo) return TRUE;

						dwSplitPos = 0;
						while(dwSplitPos < lpEwcData->dwSplitNum 
							&& lpEwcData->n64SplitMarkedPosByte[dwSplitPos] < n64Foo) dwSplitPos++;
						
						// �}��
						memmove(lpEwcData->n64SplitMarkedPosByte+dwSplitPos+1,
							lpEwcData->n64SplitMarkedPosByte+dwSplitPos,sizeof(LONGLONG)*(lpEwcData->dwSplitNum-dwSplitPos));
						memmove(lpEwcData->lnSplitMarkedPos+dwSplitPos+1,
							lpEwcData->lnSplitMarkedPos+dwSplitPos,sizeof(LONG)*(lpEwcData->dwSplitNum-dwSplitPos));
						
						lpEwcData->n64SplitMarkedPosByte[dwSplitPos] = n64Foo;
						lpEwcData->dwSplitNum++;
					}
					
					// �}�[�N�����X�N���[���ʒu���Z�b�g
					i=0;
					while(LOWORD(lp) > EDITUPDATERECTLEFT+FRAMESIZE/DwZoomX*(i+1)) i++;
					if(msg == WM_LBUTTONDOWN) {
						nMarkedPos = min(nScrPos+i,nScrMax);
					}
					else if(msg == WM_RBUTTONDOWN){
						nSubMarkedPos = min(nScrPos+i,nScrMax);
					}
					else{
						lpEwcData->lnSplitMarkedPos[dwSplitPos] = min(nScrPos+i,nScrMax);
					}
					
					// �ĕ`��
					RedrawWindow(hWnd);
				}
			}
		}
		
		break;
		
		// ----------------------------------------------------------
		
	case MM_WOM_DONE:  // �����Đ��X���b�h����̌Ăяo��
		
		// �o�ߎ��Ԃ̌v�Z
		dwCurTime = (DWORD)lp;
		
		//���ԃo�[�̈ʒu�v�Z�ƃZ�b�g
		nScrPos = (LONG)((double)nScrMax*(double)dwCurTime/(double)DwWaveTime);
		scrInfo.nPos = nScrPos;
		SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
		
		// �ĕ`��
		RedrawWindow(hWnd);
		
		break;
		
		// ----------------------------------------------------------					
		
	case MM_WOM_CLOSE: // �Đ��� wave �f�o�C�X���N���[�Y����
		
		// �X�e�[�^�X�����f�B��
		if(wCurStatus == ID_STATPLAY) wCurStatus = ID_STATREADY;
		
		// �ĕ`��
		RedrawWindow(hWnd);
		
		break;
		
		// ----------------------------------------------------------					

	case WM_CLOSE: // �N���[�Y
		
		if(wCurStatus == ID_STATPLAY){	 // �����Ȃ��Ă������~
			StopPlayWave();
			wCurStatus = ID_STATREADY;
		}

		
		// �t�@�C���N���[�Y
		if(lpEwcData->hdFile) {
			CloseHandle(lpEwcData->hdFile);
			lpEwcData->hdFile = NULL;
		}

		// ����ʃo�b�t�@�J��
		DeleteDC(hBufDC);
		DeleteObject(hBufBit);
		
		// ���W�Q�b�g
		GetWindowRect(hWnd, &curRt);
		lpEwcData->x = curRt.left;
		lpEwcData->y = curRt.top;
		
		DestroyWindow(hWnd);  // �E�B���h�E�폜
		
		break;
		
		
	case WM_DESTROY: // �E�B���h�E�폜

		PostQuitMessage(0);
		
		break;		

		
		// ----------------------------------------------------------
		// �R�}���h
		
	case WM_COMMAND:
		
		switch (LOWORD(wp)) {
			
		case IDC_BTSETUP: // �ݒ�
			
			DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_SETTINGDIAG)
				,hWnd,(DLGPROC)SettingProc,(LPARAM)lpEwcData);
			
			break;

		case IDC_BTTRACK: // �g���b�N�ݒ�
			
			lpEwcData->dwCurTrack = 
				GetCurTrack(lpEwcData->dwSplitNum,
				lpEwcData->n64SplitMarkedPosByte,lpEwcData->waveFmt.nBlockAlign,
				N64MaxBlock,scrInfo.nPos,nScrMax);
			dwFoo = DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_TRACK)
				,hWnd,(DLGPROC)TrackDlgProc,(LPARAM)lpEwcData);
			LoadTrackFile(hWnd,lpEwcData);
			
			// �J�b�g���ǂݍ��݂���
			if(dwFoo == IDC_BTCUTLOAD)
			{
				DwWaveTime = (DWORD)((lpEwcData->n64WaveDataSize*1000)/lpEwcData->waveFmt.nAvgBytesPerSec);

				// �X�N���[���o�[�Đݒ�
				N64MaxBlock = (DWORD)(lpEwcData->n64WaveDataSize/(FRAMESIZE*lpEwcData->waveFmt.nBlockAlign));
				nScrMax = N64MaxBlock > MAXLONG ? MAXLONG : (LONG)N64MaxBlock+1;
				
				// �X�N���[���o�[�Z�b�g
				scrInfo.nPos = 0;
				scrInfo.nMax = nScrMax;
				SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
				
				// �p�����[�^������
				nScrPos = 0;
				dwCurTime = 0;
				nMarkedPos = 0;
				N64MarkedPosByte =	0;
				nSubMarkedPos = nScrMax;
				N64SubMarkedPosByte = lpEwcData->n64WaveDataSize;
				
				// undo �f�[�^�N���A
				undoData.wCurPos = 0;
				for(i=0;i<UNDOLEVEL;i++) undoData.bDataEmpty[i] = FALSE;
				
			}
			
			
			RedrawWindow(hWnd);
			
			break;

			
		case IDC_BTFILEINFO: // �t�@�C�����
			
			if(wCurStatus == ID_STATCLOSE) return TRUE; // �J���ĂȂ������炻�̂܂܃��^�[��
			
			if(wCurStatus == ID_STATREADY){ 
				
				wsprintf(SzInfo,
					"%s\r\n\r\n%d M\r\n\r\n%d hz, %d bit, %d channel, %d �b\r\n",
					lpEwcData->szLoadFile,
					(DWORD)(N64OrgFileSize/1024/1024),
					lpEwcData->waveFmt.nSamplesPerSec,
					lpEwcData->waveFmt.wBitsPerSample,lpEwcData->waveFmt.nChannels,
					(DWORD)(N64OrgFileSize/lpEwcData->waveFmt.nAvgBytesPerSec)
					);
				
				DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_INFO)
					,hWnd,(DLGPROC)InfoProc,
					(LPARAM)SzInfo);
			}
			
			
			break;
			// ----------------------------------------------------------
			
		case IDC_BTSEARCH: // ����
			
			if(wCurStatus == ID_STATCLOSE ) return TRUE; // �J���ĂȂ������炻�̂܂܃��^�[��
			
			if(wCurStatus == ID_STATPLAY){	 // �����Ȃ��Ă������~
				StopPlayWave();
				wCurStatus = ID_STATREADY;
			}
			
			if(wCurStatus == ID_STATREADY){
				
				DWORD dwRet;
				lpEwcData->dwCurTrack 
					= GetCurTrack(lpEwcData->dwSplitNum,lpEwcData->n64SplitMarkedPosByte,
					lpEwcData->waveFmt.nBlockAlign,
					N64MaxBlock,scrInfo.nPos,nScrMax);

				// �W�����v�_�C�A���O�\��
				JUMPDATA jumpData;
				jumpData.hdFile = lpEwcData->hdFile;
				jumpData.lpdwSearchTime = &DwSearchTime;
				jumpData.lpdNoSoundBound = &DbNoSoundBound;
				jumpData.lpdwNSoundCount = &dwNSoundCount;
				jumpData.lpwNSoundPos = &wNSoundPos;
				jumpData.waveFmt = lpEwcData->waveFmt;
				jumpData.n64DataOffset = lpEwcData->n64WaveDataOffset;
				jumpData.n64StartByte = N64MarkedPosByte;
				jumpData.lpEwcData = lpEwcData;
				
				dwRet = DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_EDITFIND)
					,hWnd,(DLGPROC)EditFindDlgProc,
					(LPARAM)&jumpData);
				
				
				// ���Ԏw��W�����v
				if(dwRet == IDC_BTJUMP)
				{
					MovePos(hWnd,msg,wp,lp);
					return TRUE;
				}
				
				// �g���b�N�W�����v
				if(dwRet == IDC_BTMOVETRACK){
					if(lpEwcData->dwSplitNum){
						if(lpEwcData->dwCurTrack == 1) scrInfo.nPos = 0;
						else scrInfo.nPos = lpEwcData->lnSplitMarkedPos[lpEwcData->dwCurTrack-2]+1;
						SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
						RedrawWindow(hWnd);
					}
				}
				
				// �������T�[�`�A�����}�[�N�Z�b�g
				else if(dwRet== IDC_BTSEARCH || dwRet == IDC_BTSERCHSPLIT)
				{
					// �����}�[�N�Z�b�g�̏ꍇ
					if(dwRet == IDC_BTSERCHSPLIT){
						lpEwcData->dwSplitNum = 0;
						for(i=0;i<MAX_SPLITNUM;i++){
							lpEwcData->n64SplitMarkedPosByte[i] = 0;
							lpEwcData->lnSplitMarkedPos[i] = 0;
						}
						n64StartByte = lpEwcData->n64WaveDataOffset;
					}
					else{
						// �T�[�`�J�n�����ʒu�擾
						nScrPos = SetScrBarInfo(hScrWnd,&scrInfo,wp);
						n64StartByte 
							= (lpEwcData->waveFmt.nBlockAlign*FRAMESIZE*N64MaxBlock*nScrPos)/nScrMax;
					}
					
					while(1){
						
						// �������T�[�`�J�n
						if(!SearchNoSound(
							hWnd,hInst,
							lpEwcData->hdFile,
							lpEwcData->n64WaveDataSize,
							lpEwcData->n64WaveDataOffset,
							lpEwcData->waveFmt,
							&n64StartByte,
							&n64EndByte,
							DbNoSoundBound,dwNSoundCount,
							lpEwcData->editSaveData.bUseAvr)){ 
							
							// �����������o����
							if(dwRet== IDC_BTSEARCH)
								MyMessageBox(hWnd, "�������͌�����܂���ł����B", "ewc", MB_OK|MB_ICONINFORMATION);
							else{
								wsprintf(szStr,"%d �_�ɕ����}�[�N���Z�b�g���܂����B",lpEwcData->dwSplitNum);

								MyMessageBox(hWnd, szStr, "ewc", MB_OK|MB_ICONINFORMATION);
							}
							return TRUE;
						}
						else{
							
							// �T�[�`��̈ʒu�Z�b�g
							switch(wNSoundPos){
								
							case NSOUND_TOP: // �擪
								n64CurByte = n64StartByte;
								break;
							case NSOUND_END: // ���
								n64CurByte = n64EndByte;
								break;
							case NSOUND_MID: // ����
								n64CurByte = (n64EndByte + n64StartByte)/2;
								n64CurByte =  (n64CurByte 
									/(lpEwcData->waveFmt.nChannels*(lpEwcData->waveFmt.wBitsPerSample/8))) 
									*lpEwcData->waveFmt.nBlockAlign;
							}

							// �X�N���[���o�[�ʒu�Z�b�g
							nScrPos = min(nScrMax,(LONG)(nScrMax*n64CurByte/lpEwcData->n64WaveDataSize));
							scrInfo.nPos = nScrPos;
							SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
		
							// �����}�[�N�Z�b�g�̏ꍇ
							if(dwRet == IDC_BTSERCHSPLIT)
							{
								lpEwcData->n64SplitMarkedPosByte[lpEwcData->dwSplitNum] = n64CurByte;
								lpEwcData->lnSplitMarkedPos[lpEwcData->dwSplitNum] = nScrPos;
								lpEwcData->dwSplitNum++;
								n64StartByte = n64EndByte;
							}
							
							// �ĕ`��
							RedrawWindow(hWnd);

							// �T�[�`�̏ꍇ�͂����ŏI���
							if(dwRet != IDC_BTSERCHSPLIT) break;

							if(lpEwcData->dwSplitNum >= MAX_SPLITNUM){
								MyMessageBox(hWnd, "�����}�[�L���O�Z�b�g�����ő�ɂȂ�܂����B",
									"ewc", MB_OK|MB_ICONINFORMATION);
								break;
							}
							
						}
					}
				}
			}
			
			break;
			
			// ----------------------------------------------------------
			
		case IDC_BTEDIT: // �G�f�B�b�g
			
			if(wCurStatus == ID_STATPLAY){	 // �����Ȃ��Ă������~
				StopPlayWave();
				wCurStatus = ID_STATREADY;
			}

			// �ҏW
			StartEditWave(hWnd, msg, wp, lp);
			
			break;
			
			//-------------------------------------------------------------
			
		
			
		case IDC_BTLOAD: // ���[�h
			
			if(wCurStatus == ID_STATPLAY){	 // �����Ȃ��Ă������~
				StopPlayWave();
				wCurStatus = ID_STATREADY;
			}

			
			// �t�@�C���N���[�Y
			if(lpEwcData->hdFile != NULL) {
				CloseHandle(lpEwcData->hdFile);
				lpEwcData->hdFile = NULL;
			}
			
			// ���[�h�_�C�A���O�\��
			strcpy(szNewFileName,lpEwcData->szLoadFile);
			if(SelectLoadFile(hWnd ,szNewFileName,
				"*.wav\0*.wav\0All Files(*.*)\0*.*\0\0",
				"wav",
				"WAVE �t�@�C���I��"
				)){ 
				
				// �t�@�C�����Z�b�g
				strcpy(lpEwcData->szLoadFile,szNewFileName);		
				
				// �t�@�C���ăI�[�v��
				LoadNewData(hWnd,msg,wp,lp);
				return TRUE;
				
			}
			else
			{
				// ���t�@�C�������̂܂܊J��
				OpenCurrentData(hWnd);
			}
			
			break;
			
			// ----------------------------------------------------------
			
		case IDC_BTZOOMIN: // �Y�[���C���A�A�E�g
		case IDC_BTZOOMOUT:
			
			if(wCurStatus == ID_STATCLOSE) return TRUE; // �J���ĂȂ������炻�̂܂܃��^�[��
			
			if(wCurStatus == ID_STATREADY){					
				
				// �V�t�g�����Ă���c�̃Y�[��
				if(GetKeyState(VK_SHIFT)&0x80){
					// �c����
					if(LOWORD(wp) == IDC_BTZOOMOUT && DwZoomY > 1) {
						DwZoomY /=2; 
					}
					else if(LOWORD(wp) == IDC_BTZOOMIN && DwZoomY <256){
						DwZoomY *=2; 
					}
				}
				else
				{
					// ������
					if(LOWORD(wp) == IDC_BTZOOMIN && DwZoomX > 1) {
						DwZoomX /= 2; // �C��
					}
					else if(LOWORD(wp) == IDC_BTZOOMOUT && DwZoomX <8){
						DwZoomX *=2; // �A�E�g
					}
				}
				
				// �ĕ`��
				RedrawWindow(hWnd);
				
			}
			
			break;
			
			// ----------------------------------------------------------
			
		case IDC_BTPLAYSTOP: // �����Đ��A��~
			
			if(wCurStatus == ID_STATREADY)
			{ // �Đ�
				
				//���ԃo�[�̌��݈ʒu�擾�ƍĐ��ʒu�v�Z
				nScrPos = scrInfo.nPos;
				n64CurByte = lpEwcData->waveFmt.nBlockAlign*((FRAMESIZE*N64MaxBlock*nScrPos)/nScrMax);
				
				// �J�n����(�~���b)
				dwCurTime = (DWORD)(n64CurByte*1000/lpEwcData->waveFmt.nAvgBytesPerSec);
				
				// �Đ��J�n
				if(PlayWave(hWnd,
					lpEwcData->uDeviceID,
					lpEwcData->szLoadFile,
					dwCurTime,
					lpEwcData->hdFile,
					lpEwcData->waveFmt,
					lpEwcData->n64WaveDataSize,
					lpEwcData->n64WaveDataOffset)) wCurStatus = ID_STATPLAY; 
				
			}
			else if(wCurStatus == ID_STATPLAY){	 // ��~
				
				StopPlayWave();
				
				// �ĕ`��
				InvalidateRect(hWnd,&RedrawRect,FALSE);
				
			}
				
			break;	
			
			// ----------------------------------------------------------
			
		case IDC_BTREW: // �����߂��{�^��
		case IDC_BTREW2: // �擪�{�^��
		case IDC_BTREW3: // �擪�{�^��
		case IDC_BTFORWARD: // �i�ރ{�^��
		case IDC_BTFORWARD2: // �����{�^��
		case IDC_BTFORWARD3: // �����{�^��
		case IDC_BTJUMP: // �}�[�N�ʒu��JUMP
		case IDC_BTJUMPNEXT:
		case IDC_BTJUMPNEXT2:
		case IDC_BTJUMPNEXT3:
		case IDC_BTJUMPBACK:
		case IDC_BTJUMPBACK2:
		case IDC_BTJUMPBACK3:
			
			MovePos(hWnd,msg,wp,lp);
			
			break;
			
			// ----------------------------------------------------------
			
		case IDC_BTRESET: // ���Z�b�g
			
			if(wCurStatus == ID_STATREADY){
				
				if(MyMessageBox(hWnd, "���Z�b�g���܂����H", 
					"ewc", MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDYES){
					
					if(wCurStatus == ID_STATPLAY){	 // �����Ȃ��Ă������~
						StopPlayWave();
						wCurStatus = ID_STATREADY;
					}
					
					for(i=0;i<MAX_SPLITNUM;i++){
						lpEwcData->n64SplitMarkedPosByte[i] = 0;
						lpEwcData->lnSplitMarkedPos[i] = 0;
						lpEwcData->dwSplitNum = 0;
					}
					
					// �t�@�C���N���[�Y
					if(lpEwcData->hdFile != NULL) {
						CloseHandle(lpEwcData->hdFile);
						lpEwcData->hdFile = NULL;
					}						
					// �t�@�C���ăI�[�v��
					LoadNewData(hWnd,msg,wp,lp);
					return TRUE;
				}
			}
			
			break;
			
			// ----------------------------------------------------------
			
		case IDC_BTCUTLEFT: // �����J�b�g
		case IDC_BTCUTRIGHT: // �E���J�b�g
			
			if(wCurStatus == ID_STATREADY)
			{
				DWORD dwSplitPos;
				
				// undo �̃f�[�^�Z�b�g
				nScrPos = scrInfo.nPos;
				wUndoPos = undoData.wCurPos;
				
				undoData.n64DataOffset[wUndoPos] = lpEwcData->n64WaveDataOffset;
				undoData.n64DataSize[wUndoPos] = lpEwcData->n64WaveDataSize;
				undoData.nScrPos[wUndoPos] = nScrPos;
				undoData.nMarkedPos[wUndoPos] = nMarkedPos;
				undoData.n64MarkedPosByte[wUndoPos] = N64MarkedPosByte;
				undoData.nSubMarkedPos[wUndoPos] = nSubMarkedPos;
				for(i=0;i<MAX_SPLITNUM;i++)
				{
					undoData.n64SplitMarkedPosByte[wUndoPos][i] = lpEwcData->n64SplitMarkedPosByte[i];
					undoData.nSplitMarkedPos[wUndoPos][i] = lpEwcData->lnSplitMarkedPos[i];
				}
				undoData.dwSplitNum[wUndoPos] = lpEwcData->dwSplitNum;
				undoData.n64SubMarkedPosByte[wUndoPos] = N64SubMarkedPosByte;
				undoData.bDataEmpty[wUndoPos] = TRUE;
				
				wUndoPos = (undoData.wCurPos + 1)&(UNDOLEVEL-1);
				undoData.wCurPos = wUndoPos;
				undoData.bDataEmpty[wUndoPos] = FALSE;
				
				if(LOWORD(wp) == IDC_BTCUTLEFT){ // ���J�b�g
					
					// �T�C�Y�ύX
					lpEwcData->n64WaveDataOffset+= N64MarkedPosByte;
					lpEwcData->n64WaveDataSize -= N64MarkedPosByte;
					DwWaveTime = (DWORD)((lpEwcData->n64WaveDataSize*1000)/lpEwcData->waveFmt.nAvgBytesPerSec);
					
					// �X�N���[���o�[�Đݒ�
					N64MaxBlock = lpEwcData->n64WaveDataSize/(FRAMESIZE*lpEwcData->waveFmt.nBlockAlign);
					nScrMax = N64MaxBlock > MAXLONG ? MAXLONG : (LONG)N64MaxBlock+1;
					
					// �p�����[�^������
					nScrPos = 0;
					dwCurTime = 0;
					
					// ���|�C���^�̈ʒu�v�Z
					if(N64SubMarkedPosByte < N64MarkedPosByte)
					{ // ��
						N64SubMarkedPosByte = 0;
						nSubMarkedPos = 0;
					}
					else
					{ // �E
						N64SubMarkedPosByte -= N64MarkedPosByte;
						if(lpEwcData->n64WaveDataSize > 0)
						{
							dwByte = (DWORD)(lpEwcData->n64WaveDataSize/nScrMax);	// �X�N���[����񕪂̃o�C�g��
							nSubMarkedPos = max(0,min((LONG)(N64SubMarkedPosByte/dwByte),nScrMax));
						}
						else nSubMarkedPos = 0;
					}
				
					// �X�v���b�g�|�C���^
					if(lpEwcData->dwSplitNum)
					{
						dwSplitPos = 0;
						while(dwSplitPos < lpEwcData->dwSplitNum 
							&& lpEwcData->n64SplitMarkedPosByte[dwSplitPos] < N64MarkedPosByte) dwSplitPos++;
						for(i2=dwSplitPos;i2<(LONG)lpEwcData->dwSplitNum;i2++)
						{
							lpEwcData->n64SplitMarkedPosByte[i2-dwSplitPos] = lpEwcData->n64SplitMarkedPosByte[i2] - N64MarkedPosByte;
							if(lpEwcData->n64WaveDataSize > 0){
								dwByte = (DWORD)(lpEwcData->n64WaveDataSize/nScrMax);	// �X�N���[����񕪂̃o�C�g��
								lpEwcData->lnSplitMarkedPos[i2-dwSplitPos] = max(0,min((LONG)(lpEwcData->n64SplitMarkedPosByte[i2-dwSplitPos]/dwByte),nScrMax));
							}
							else lpEwcData->lnSplitMarkedPos[i2-dwSplitPos] = 0;
						}
						lpEwcData->dwSplitNum -= dwSplitPos;

					}
					
					// ��|�C���^�̈ʒu
					nMarkedPos = 0;
					N64MarkedPosByte = 0;
					
				}
				else
				{ // �E�J�b�g
					// �T�C�Y�ύX
					lpEwcData->n64WaveDataSize = N64MarkedPosByte;
					DwWaveTime = (DWORD)((lpEwcData->n64WaveDataSize*1000)/lpEwcData->waveFmt.nAvgBytesPerSec);
					
					// �X�N���[���o�[�Đݒ�
					N64MaxBlock	= lpEwcData->n64WaveDataSize/(FRAMESIZE*lpEwcData->waveFmt.nBlockAlign);
					nScrMax = N64MaxBlock > MAXLONG ? MAXLONG : (LONG)N64MaxBlock+1;
					
					// �p�����[�^������
					nScrPos = nScrMax;
					dwCurTime = DwWaveTime;
					
					// ���|�C���^�̈ʒu�v�Z
					if(N64SubMarkedPosByte > N64MarkedPosByte){ // �E
						N64SubMarkedPosByte = N64MarkedPosByte;
						nSubMarkedPos = nScrMax;
					} 
					else{  // ��
						if(lpEwcData->n64WaveDataSize > 0){
							dwByte = (DWORD)(lpEwcData->n64WaveDataSize/nScrMax);	// �X�N���[����񕪂̃o�C�g��
							nSubMarkedPos = max(0,min((LONG)(lpEwcData->n64SplitMarkedPosByte[i]/dwByte),nScrMax));
						}
						else nSubMarkedPos = 0;
					}
					
				
					// �X�v���b�g�|�C���^�̈ʒu�v�Z
					if(lpEwcData->dwSplitNum){
						dwSplitPos = 0;
						while(dwSplitPos < lpEwcData->dwSplitNum && lpEwcData->n64SplitMarkedPosByte[dwSplitPos] < N64MarkedPosByte) dwSplitPos++;
						lpEwcData->dwSplitNum = dwSplitPos;
					}

					// ��|�C���^�̈ʒu
					nMarkedPos = nScrMax;

				}
				
				// �X�N���[���o�[�Z�b�g
				scrInfo.nPos = nScrPos;
				scrInfo.nMax = nScrMax;
				SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
				
				// �X�V�p�����[�^�Z�b�g
				bUpdate = TRUE;
				
				// �ĕ`��
				RedrawWindow(hWnd);
			}
			break;
			
			// ----------------------------------------------------------
			
			case IDC_BTUNDO: // UNDO
				
				if(wCurStatus == ID_STATREADY){					
					
					wUndoPos = (undoData.wCurPos - 1)&(UNDOLEVEL-1);
					
					if(undoData.bDataEmpty[wUndoPos])
					{ // UNDO �̃f�[�^����������
						lpEwcData->n64WaveDataOffset = undoData.n64DataOffset[wUndoPos];
						lpEwcData->n64WaveDataSize = undoData.n64DataSize[wUndoPos];
						nScrPos = undoData.nScrPos[wUndoPos];
						nMarkedPos = undoData.nMarkedPos[wUndoPos];
						N64MarkedPosByte = undoData.n64MarkedPosByte[wUndoPos];
						nSubMarkedPos = undoData.nSubMarkedPos[wUndoPos];
						N64SubMarkedPosByte = undoData.n64SubMarkedPosByte[wUndoPos];
						for(i=0;i<MAX_SPLITNUM;i++)
						{
							lpEwcData->n64SplitMarkedPosByte[i] = undoData.n64SplitMarkedPosByte[wUndoPos][i];
							lpEwcData->lnSplitMarkedPos[i] = undoData.nSplitMarkedPos[wUndoPos][i];
						}
						lpEwcData->dwSplitNum = undoData.dwSplitNum[wUndoPos];

						undoData.wCurPos = wUndoPos;
						undoData.bDataEmpty[wUndoPos] = FALSE;
						
						// ���ԕύX
						DwWaveTime = (DWORD)((lpEwcData->n64WaveDataSize*1000)/lpEwcData->waveFmt.nAvgBytesPerSec);
						
						// �X�N���[���o�[�Đݒ�
						N64MaxBlock = lpEwcData->n64WaveDataSize/(FRAMESIZE*lpEwcData->waveFmt.nBlockAlign);
						nScrMax = N64MaxBlock > MAXLONG ? MAXLONG : (LONG)N64MaxBlock+1;
						scrInfo.nPos = nScrPos;
						scrInfo.nMax = nScrMax;
						SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
						
						// �p�����[�^������
						dwCurTime = DwWaveTime;
						if(lpEwcData->n64WaveDataOffset == N64OrgDataOffset && lpEwcData->n64WaveDataSize == N64OrgDataSize) bUpdate = FALSE;
						
						// �ĕ`��
						RedrawWindow(hWnd);
					}
					
				}
				
				break;
				
			return(DefWindowProc(hWnd, msg, wp, lp));
        }

		break;
		
        default:
			return(DefWindowProc(hWnd, msg, wp, lp));
	}

	return FALSE;
}


//EOF