// メインのプロシージャ

#include "editwave.h"
#include <math.h>


//-----------------------------------------------------
// 再描画
VOID RedrawWindow(HWND hWnd){
	
	DWORD dwCurTime; // 現在の再生時間(msec)
	
	LONGLONG n64CurByte;  // 現在のスクロール位置までのバイト数
	LONG nScrPos; // 現在のスクロールバーのポジション
	LONG markedLineX; // マークラインの x 
	LONG markedSubLineX; // 副マークラインの x 
	LONG markedSplitLineX[MAX_SPLITNUM]; // スプリットマークラインの x
	CHAR szTrack[CHR_BUF];
	DWORD i;
	
	if(wCurStatus == ID_STATREADY || wCurStatus == ID_STATPLAY){ 
		
		// 現在の位置(バイト)計算
		nScrPos = scrInfo.nPos;
		n64CurByte = lpEwcData->waveFmt.nBlockAlign*((N64MaxBlock*FRAMESIZE*nScrPos)/nScrMax);
		
		// 現在時刻(ミリ秒)計算
		dwCurTime = (DWORD)(n64CurByte*1000/lpEwcData->waveFmt.nAvgBytesPerSec);

		// マーク線
		markedLineX = CalcMarkedLineX(lpEwcData->waveFmt,n64CurByte,DwZoomX,RedrawRect,N64MarkedPosByte);
		markedSubLineX = CalcMarkedLineX(lpEwcData->waveFmt,n64CurByte,DwZoomX,RedrawRect,N64SubMarkedPosByte);
		for(i=0;i<lpEwcData->dwSplitNum;i++) markedSplitLineX[i] = CalcMarkedLineX(lpEwcData->waveFmt,n64CurByte,DwZoomX,RedrawRect,lpEwcData->n64SplitMarkedPosByte[i]);
		
		// 描画
		HakeiPaint(hWnd,hBufDC,RedrawRect,lpEwcData->waveFmt,
			lpEwcData->n64WaveDataOffset,
			lpEwcData->n64WaveDataSize,
			lpEwcData->hdFile,
			n64CurByte,
			markedLineX,markedSubLineX,markedSplitLineX,lpEwcData->dwSplitNum,
			DwZoomX,DwZoomY);
		
		i = 0;
		if(lpEwcData->dwSplitNum)
		{ // 分割している場合
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
		
		// 再描画
		InvalidateRect(hWnd,&RedrawRect,FALSE);
	}
}



//-----------------------------------------------------
// ファイルの新規オープンと画面、変数初期化
BOOL LoadNewData(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	
	// 描画用雑用
	HDC hDC;
	RECT deskRt,curRt;
	LONG nWindowX,nWindowY;
	
	LONG nScrPos; // 現在のスクロールバーのポジション

	double dLevel[2];
	double dMaxLevel;
	
	LONG i;
	CHAR szStr[CHR_BUF];
	
	// フォーマット取得
	SetWaveFmt(&lpEwcData->waveFmt,2,22050,16,1); // ダミーヘッダ
	lpEwcData->n64WaveDataSize = 100;  // ダミー
	lpEwcData->n64WaveDataOffset = 44; // ダミー
	wCurStatus = ID_STATCLOSE;
	
	if(lpEwcData->szLoadFile[0] != '\0')
	{
		// WAVE ファイルか?
		if(GetWaveFormat(lpEwcData->szLoadFile,&lpEwcData->waveFmt,
			&lpEwcData->n64WaveDataSize,&lpEwcData->n64WaveDataOffset,szStr))
		{
			DwWaveTime = (DWORD)((lpEwcData->n64WaveDataSize*1000)/lpEwcData->waveFmt.nAvgBytesPerSec);
			wCurStatus = ID_STATREADY;

			// ファイルオープン
			lpEwcData->hdFile = CreateFile(lpEwcData->szLoadFile,GENERIC_READ, 
				0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL); 
			
			if(lpEwcData->hdFile == INVALID_HANDLE_VALUE){
				MyMessageBox(hWnd, "ファイルを開けません。", "Error", MB_OK|MB_ICONERROR);	
				return FALSE;
			}
		}
		else
		{
			MyMessageBox(hWnd,szStr, "Error", MB_OK|MB_ICONERROR);
		}

	}
	
	
	// Window リサイズ(チャンネル数で高さを変える)
	LONG frameHeight;
	GetClientRect(hWnd,&RedrawRect);
	RedrawRect.right = RedrawRect.left + FRAMESIZE+ 2*EDITUPDATERECTLEFT;
	frameHeight = lpEwcData->waveFmt.nChannels == 2 ? FRAMESIZE/4+8+FRAMESIZE/4 + 10: FRAMESIZE/4+10;
	RedrawRect.bottom = RedrawRect.top + EDITUPDATERECTTOP+
		(wCurStatus == ID_STATREADY)*(EDITSTATUSMARGIN1+EDITSTATUSSIZE+EDITSTATUSMARGIN2+
		frameHeight);
	AdjustWindowRect(&RedrawRect,WS_CAPTION,FALSE);
	
	// 移動座標計算
	GetWindowRect(GetDesktopWindow(), &deskRt);
	GetWindowRect(hWnd, &curRt);
	
	// 既に Easy Wave Cutter が起動してる場合
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
	
	
	// 描画領域のサイズ取得
	GetClientRect(hWnd,&RedrawRect);
	
	// 前にファイルを開いていたらここで裏画面バッファ削除
	if(hBufDC != NULL){ 
		DeleteDC(hBufDC);
		DeleteObject(hBufBit);
	}
	
	
	// 裏画面バッファの(再)作成
	hDC = GetDC(hWnd);
	hBufBit = CreateCompatibleBitmap(hDC,RedrawRect.right - RedrawRect.left,RedrawRect.bottom - RedrawRect.top);
	hBufDC = CreateCompatibleDC(hDC);
	SelectObject(hBufDC,hBufBit);
	ReleaseDC(hWnd,hDC);
	
	// 初期画面描画
	InitEditScreenDraw(hWnd,hInst,hBufDC);
	
	// 画面更新領域セット
	RedrawRect.top += EDITUPDATERECTTOP;
	
	// オリジナルデータ保存
	LARGE_INTEGER liFoo;
	liFoo.LowPart = GetFileSize(lpEwcData->hdFile,(LPDWORD)&(liFoo.HighPart));
	N64OrgFileSize = liFoo.QuadPart;
	N64OrgDataSize = lpEwcData->n64WaveDataSize;
	N64OrgDataOffset = lpEwcData->n64WaveDataOffset;
	
	// スクロールバー初期化
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
	
	// 各パラメータ初期化
	nScrPos = 0;
	nMarkedPos = 0;
	N64MarkedPosByte =	0;
	nSubMarkedPos = nScrMax;
	N64SubMarkedPosByte = lpEwcData->n64WaveDataSize;
	memset(lpEwcData->lnSplitMarkedPos,0,sizeof(LONG)*MAX_SPLITNUM);
	memset(lpEwcData->n64SplitMarkedPosByte,0,sizeof(LONGLONG)*MAX_SPLITNUM);
	lpEwcData->dwSplitNum = 0;
	bUpdate = FALSE;
	
	// レベル取得
	dMaxLevel = GetMaxWaveLevel(lpEwcData->waveFmt);
	GetLevelatPoint(lpEwcData->waveFmt,lpEwcData->hdFile,dLevel,lpEwcData->n64WaveDataOffset+N64MarkedPosByte);
	DbMarkedLevel[0] = 20*log10(fabs(dLevel[0])/dMaxLevel);
	DbMarkedLevel[1] = 20*log10(fabs(dLevel[1])/dMaxLevel);
	
	GetLevelatPoint(lpEwcData->waveFmt,lpEwcData->hdFile,dLevel,lpEwcData->n64WaveDataOffset+N64SubMarkedPosByte);
	DbSubMarkedLevel[0] = 20*log10(fabs(dLevel[0])/dMaxLevel);
	DbSubMarkedLevel[1] = 20*log10(fabs(dLevel[0])/dMaxLevel);
	
	// undo データ初期化
	undoData.wCurPos = 0;
	for(i=0;i<UNDOLEVEL;i++) undoData.bDataEmpty[i] = FALSE;
	
	// タイトル更新
	if(wCurStatus == ID_STATREADY)wsprintf(szStr,"ewc - %s",lpEwcData->szLoadFile);
	else wsprintf(szStr,"ewc");
	SetWindowText(hWnd,szStr); 
	
	
	// 再描画
	RedrawWindow(hWnd);
	
	return TRUE;
}






//-----------------------------------------------------
// 元ファイルをそのまま再オープンする関数
BOOL OpenCurrentData(HWND hWnd){		
	
	if(wCurStatus == ID_STATREADY){
		
		lpEwcData->hdFile = CreateFile(lpEwcData->szLoadFile,GENERIC_READ,0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL); 
		if(lpEwcData->hdFile == INVALID_HANDLE_VALUE){
			MyMessageBox(hWnd, "ファイルを開けません。\nファイルが存在するか確認して下さい。", "Error", MB_OK|MB_ICONERROR);
			lpEwcData->szLoadFile[0]='\0';
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}




//--------------------------------------------------------
// 編集開始関数
BOOL StartEditWave(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	
	CHAR szStr[CHR_BUF],szStr2[CHR_BUF],szSaveDir[CHR_BUF];
	LONG nReturn;
	DWORD dwFoo;
	
	// 編集ダイアログ表示
	if((nReturn = DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_EDITWEDIT)
		,hWnd,(DLGPROC)EditMenuDlgProc,(LPARAM)lpEwcData)) == IDCANCEL) return FALSE;
	
	if(wCurStatus == ID_STATREADY){
		
		
		if(nReturn == IDC_BTSTART){
			
			// 保存ディレクトリ設定
			if(lpEwcData->szSaveDir[0] == '\0'){  // セーブファイルが設定されて無い場合
				_splitpath(lpEwcData->szLoadFile,szStr,szStr2,NULL,NULL);
				wsprintf(szSaveDir,"%s%s",szStr,szStr2);
			}
			else wsprintf(szSaveDir,"%s\\",lpEwcData->szSaveDir);
			
			// 保存ファイル設定
			if(lpEwcData->bSplit){ // 分割
				wsprintf(lpEwcData->szSaveFile,"%s%s.%s",szSaveDir,lpEwcData->szBaseName,lpEwcData->szExtName);
				wsprintf(szStr2,"分割\n\n出力ディレクトリ %s\n\n",szSaveDir);
			}
			else if(lpEwcData->bCutCm){ // CM カット
				if(lpEwcData->szTrackFile[0]=='\0') 
					wsprintf(lpEwcData->szSaveFile,"%s%s.%s",szSaveDir,lpEwcData->szBaseName,lpEwcData->szExtName);
				else 
					wsprintf(lpEwcData->szSaveFile,"%s%s",szSaveDir,lpEwcData->szTrackName[0]);
				wsprintf(szStr2,"CM カット\n\n出力ファイル : %s\n\n",lpEwcData->szSaveFile);
			}
			else
			{
				if(lpEwcData->bCutTrack && lpEwcData->dwSplitNum)
				{ // 切り取り 
					dwFoo = GetCurTrack(lpEwcData->dwSplitNum,
						lpEwcData->n64SplitMarkedPosByte,lpEwcData->waveFmt.nBlockAlign,
						N64MaxBlock,scrInfo.nPos,nScrMax);
					wsprintf(lpEwcData->szSaveFile,"%s%s",szSaveDir,lpEwcData->szTrackName[dwFoo-1]);
					wsprintf(szStr2,"トラック %d 切り取り\n\n出力ファイル : %s\n\n",dwFoo,lpEwcData->szSaveFile);
				}
				else{ // 全体保存
					if(lpEwcData->szTrackFile[0]=='\0') 
						wsprintf(lpEwcData->szSaveFile,"%s%s.%s",szSaveDir,lpEwcData->szBaseName,lpEwcData->szExtName);
					else 
						wsprintf(lpEwcData->szSaveFile,"%s%s",szSaveDir,lpEwcData->szTrackName[0]);
					wsprintf(szStr2,"全体保存\n\n出力ファイル : %s\n\n",lpEwcData->szSaveFile);

				}
			}

			wsprintf(szStr,"%s入力ファイル : %s\n\n編集を開始しますか?",szStr2,lpEwcData->szLoadFile);
			if(MyMessageBox(hWnd, szStr,"ewc: WAVEFLT 起動", MB_YESNO|MB_ICONQUESTION)==IDNO) return FALSE;
			
		}
		else strcpy(lpEwcData->szSaveFile,"waveout");
		
		
		
		//---------------
		// WAVEFTL 起動
		//---------------
		
		// ファイルクローズ
		if(lpEwcData->hdFile) {
			CloseHandle(lpEwcData->hdFile);
			lpEwcData->hdFile = NULL;
		}

		//------------------------------
		// ダイアログに渡すデータセット

		// コピーブロックのオフセットサイズとサイズ計算
		if(lpEwcData->bCutTrack && lpEwcData->dwSplitNum){  // トラック切り出し

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
		else{ // 分割　or そのまま保存 or CM カット
			lpEwcData->n64NewDataOffset = lpEwcData->n64WaveDataOffset;
			lpEwcData->n64NewDataSize = lpEwcData->n64WaveDataSize;
		}

		// WAVEFLT 起動
		if(SaveCutData(hWnd,hInst,lpEwcData,lpEwcData->szLoadFile,lpEwcData->szSaveFile,lpEwcData->bSplit))
		{
			// 保存成功

			if(lpEwcData->bSplit){ // 分割実行の場合
				// 元ファイルをそのまま再オープン
				if(!OpenCurrentData(hWnd)) LoadNewData(hWnd,msg,wp,lp);
				return TRUE;
			}
			
			//保存したファイルが存在するかチェック
			lpEwcData->hdFile = CreateFile(lpEwcData->szSaveFile,GENERIC_READ
				,0, 0, OPEN_EXISTING,FILE_ATTRIBUTE_READONLY, NULL); 
			
			if(lpEwcData->hdFile == INVALID_HANDLE_VALUE){ // 存在しなかったら
				
				// 元ファイルをそのまま再オープン
				if(!OpenCurrentData(hWnd)) LoadNewData(hWnd,msg,wp,lp);
				return TRUE;
			}
			
			CloseHandle(lpEwcData->hdFile);
			lpEwcData->hdFile = NULL;
			
			// 上書き保存だったら
			if(_strnicmp(lpEwcData->szLoadFile,lpEwcData->szSaveFile,MAX_PATH)==0){ 
				MyMessageBox(hWnd, "保存しました。","ewc", MB_OK|MB_ICONINFORMATION);	
				// ファイル再オープン
				LoadNewData(hWnd,msg,wp,lp);
				return TRUE;
			}
			else { // 違う名前で保存だったら他のウィンドウ開いて元データ再オープン
				
				wsprintf(szStr,"保存しました。\n\n%s を新しいウィンドウで開きますか？",lpEwcData->szSaveFile);
				if(MyMessageBox(hWnd, szStr
					,"ewc", MB_YESNO|MB_ICONQUESTION)==IDYES){
					
					SaveIniFile(lpEwcData,lpEwcData->szIniDatFile);
					
					// 新しいウィンドウ起動
					GetModuleFileName(NULL,szStr,MAX_PATH); 
					wsprintf(szStr,"%s \"%s\" -dev %d",
						szStr,lpEwcData->szSaveFile,lpEwcData->uDeviceID);
					
					STARTUPINFO startInfo;
					PROCESS_INFORMATION proInfo;
					memset(&startInfo,0,sizeof(STARTUPINFO));
					startInfo.cb = sizeof(STARTUPINFO);
					if(!CreateProcess(NULL,szStr,NULL,NULL,FALSE,0,NULL,NULL,&startInfo,&proInfo))
						MyMessageBox(hWnd, "新しいウィンドウの起動に失敗しました。","ewc", MB_OK|MB_ICONERROR);	
				}
			}
		}
		
		// 元ファイルを再オープン
		OpenCurrentData(hWnd);
		return TRUE;
		
	}
	
	return FALSE;
}


//---------------------------------------------------------
// 現在位置の移動関数
VOID MovePos(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	
	DWORD dwCurTime; // 現在の再生時間
	
	LONGLONG n64CurByte;  // 現在のスクロール位置までのバイト数
	LONG nScrPos; // 現在のスクロールバーのポジション
	static DWORD dwJumpMark = 0;

	// 雑用
	DWORD dwMove,dwByte,dwCurTrack,i,i2;
	
	// 開いてなかったらそのままリターン
	if(wCurStatus == ID_STATCLOSE) return; 
	
	// 現在位置
	nScrPos = scrInfo.nPos;
	n64CurByte = lpEwcData->waveFmt.nBlockAlign*((FRAMESIZE*N64MaxBlock*nScrPos)/nScrMax);
	
	// 移動量計算
	if(nScrMax == 0) { // サイズが 0 のファイルは移動しない
		dwByte = 0;
		dwMove = 0;
	}
	else if(wCurStatus == ID_STATREADY || wCurStatus == ID_STATPLAY) // Wave の場合
	{
		dwByte = (DWORD)(lpEwcData->n64WaveDataSize/nScrMax); // スクロール一回分のバイト数計算
		
		switch(LOWORD(wp))
		{

		case IDC_BTREW:
		case IDC_BTFORWARD:
			
			if(GetKeyState(VK_SHIFT)&0x80) // シフト押しながらだと 1 秒
				dwMove = (DWORD)(1*lpEwcData->waveFmt.nAvgBytesPerSec)/dwByte+1; //1 秒分の移動量計算
			else
				dwMove = 1;

			break;

		case IDC_BTREW2:
		case IDC_BTFORWARD2:

			if(GetKeyState(VK_SHIFT)&0x80) // シフト押しながらだと
				dwMove = (DWORD)(60*lpEwcData->waveFmt.nAvgBytesPerSec)/dwByte+1; //60 秒分の移動量計算
			else
				dwMove = (DWORD)(15*lpEwcData->waveFmt.nAvgBytesPerSec)/dwByte+1; //15 秒分の移動量計算

			break;

		case IDC_BTREW3:
		case IDC_BTFORWARD3:

			if(GetKeyState(VK_SHIFT)&0x80) // シフト押しながらだと
				dwMove = nScrMax; // ファイルの前後
			else
				dwMove = (DWORD)(300*lpEwcData->waveFmt.nAvgBytesPerSec)/dwByte+1; //300 秒分の移動量計算

		}
	
	}


	// スクロール量計算
	switch(LOWORD(wp)){
		
	case IDC_BTJUMPNEXT:
	case IDC_BTJUMPNEXT2:
	case IDC_BTJUMPNEXT3:
	case IDC_BTJUMPBACK:
	case IDC_BTJUMPBACK2:
	case IDC_BTJUMPBACK3:
		
				// トラック移動
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
				
				if(nScrPos == lpEwcData->lnSplitMarkedPos[dwCurTrack-2]+1) // 先頭にある場合
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
		
		// マークにジャンプ
		if(GetKeyState(VK_SHIFT)&0x80) dwJumpMark = 1;// シフト押しながらだと副マークに飛ぶ
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
	
	// スクロールバー位置セット
	scrInfo.nPos = nScrPos;
	SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
	n64CurByte = lpEwcData->waveFmt.nBlockAlign*((FRAMESIZE*N64MaxBlock*nScrPos)/nScrMax);
	
	// 開始時刻(ミリ秒)計算
	dwCurTime = (DWORD)(n64CurByte*1000/lpEwcData->waveFmt.nAvgBytesPerSec);
	
	// 音声再生中なら再シーク
	if(wCurStatus == ID_STATPLAY) SeekPlayWave(dwCurTime);
	
	// 再描画
	RedrawWindow(hWnd);
	
}


//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------


//-------------------------------------------------------------------
// メインプロシージャ
LRESULT CALLBACK EditWaveProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	
	
	CHAR szNewFileName[MAX_PATH]; 
	
	DWORD dwCurTime; // 現在の再生時間
	LONGLONG n64CurByte;  // 現在のスクロール位置までのバイト数
	LONG nScrPos; // 現在のスクロールバーのポジション
	LONGLONG n64StartByte,n64EndByte;
	WORD wUndoPos; // UNDO 用雑用

	
	// 描画用雑用
	HDC hDC;
	RECT curRt;
	
	// 雑用
	LONG i,i2; 
	CHAR szStr[CHR_BUF];
	DWORD dwFoo,dwByte;
	LONGLONG n64Foo,n64Foo2;
	
    switch (msg) {
		
	case WM_CREATE:  // ダイアログ初期化
		
		// コモンコントロール初期化
		InitCommonControls();
		
		// エディトデータセット
		lpEwcData = (LPEWCDATA)(((LPCREATESTRUCT)lp)->lpCreateParams);

		// ボタンの作成
		InitEditScreenDraw(hWnd,hInst,NULL);
		
		// 初期ファイル名セット
		strcpy(lpEwcData->szLoadFile,lpEwcData->szIniFileName);
		
		// インスタンス取得
		hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
		
		// アイコンロード
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
		
	
		// パラメータ初期化
		DwZoomX = 8;
		DwZoomY = 1;
		DbNoSoundBound = -96;
		dwNSoundCount = S_POINT_PER_SEC;
		wNSoundPos = NSOUND_MID;
		wCurStatus = ID_STATCLOSE;
		SetWaveFmt(&lpEwcData->waveFmt,2,22050,16,1); // ダミーヘッダ
		lpEwcData->n64WaveDataSize = 100;  // ダミー
		lpEwcData->n64WaveDataOffset = 44; // ダミー

		// 既に起動している ewc の HWND の取得
		hPreEwcWnd = GetPreEwcHWND();
		
		// 移動
		SetWindowPos(hWnd,NULL,lpEwcData->x,lpEwcData->y,0,0,SWP_NOSIZE|SWP_NOREPOSITION);
	
		// ファイルの Drop 許可
		DragAcceptFiles(hWnd,TRUE);
		
		// ファイルの新規オープンと画面、変数初期化
		LoadNewData(hWnd,msg,wp,lp);

		
		// 再描画
		RedrawWindow(hWnd);

		return TRUE;
		
		// ----------------------------------------------------------
		
	case WM_DROPFILES: // ファイル Drop 処理	 
		
		// 再生中でないなら
		if(wCurStatus != ID_STATPLAY){	
			
			// トップに持ってくる
			SetForegroundWindow(hWnd);
			
			// ファイル名取得
			HDROP hDrop; 
			UINT uLng;
			hDrop = (HDROP)wp; 
			uLng = DragQueryFile(hDrop,0,lpEwcData->szLoadFile,MAX_PATH); 
			DragFinish(hDrop);
			
			// ファイルクローズ
			if(lpEwcData->hdFile) {
				CloseHandle(lpEwcData->hdFile);
				lpEwcData->hdFile = NULL;
			}
			
			// ファイル再オープン
			LoadNewData(hWnd,msg,wp,lp);
			return TRUE;
		}
		
		break;		
		
		// ----------------------------------------------------------
		
		
	case WM_CTLCOLORSCROLLBAR: // スクロールパーのブラシセット
		return ((BOOL)GetStockObject(BLACK_BRUSH)) ;    
		break;
		
		
		
		
		// ----------------------------------------------------------
		
	case WM_PAINT: // 再描画のみ
		
		PAINTSTRUCT ps;
		hDC = BeginPaint(hWnd,&ps);
		
		// 裏画面からコピー
		BitBlt(hDC,ps.rcPaint.left,ps.rcPaint.top,
			ps.rcPaint.right-ps.rcPaint.left,
			ps.rcPaint.bottom-ps.rcPaint.top,
			hBufDC,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
		
		ReleaseDC(hWnd,hDC);
		
		return TRUE;
		
		
		// ----------------------------------------------------------
		
	case WM_DRAWITEM: // ボタン描画
		
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
		
	case WM_HSCROLL:  // スクロールパーの処理
		
		// 現在位置セット
		nScrPos = SetScrBarInfo(hScrWnd,&scrInfo,wp);
		n64CurByte = lpEwcData->waveFmt.nBlockAlign*((FRAMESIZE*N64MaxBlock*nScrPos)/nScrMax);
		
		// WAVE 音声再生中なら再シーク
		dwCurTime = (DWORD)(n64CurByte*1000/lpEwcData->waveFmt.nAvgBytesPerSec);
		if(wCurStatus == ID_STATPLAY) SeekPlayWave(dwCurTime);
		
		// 再描画
		RedrawWindow(hWnd);
		return TRUE;
		break;
		
		// ----------------------------------------------------------
		
	case WM_LBUTTONDOWN:  // 左ボタン押した
	case WM_RBUTTONDOWN:  // 右ボタン押した
	case WM_LBUTTONDBLCLK:  // 左ボタンダブルクリック
		
		if(wCurStatus == ID_STATREADY && HIWORD(lp) > RedrawRect.top+EDITSTATUSSIZE){
			
			if(!lpEwcData->bShiftLock || GetKeyState(VK_SHIFT)&0x80){
				
				// 現在の位置セット
				nScrPos = scrInfo.nPos; 
				n64CurByte = lpEwcData->waveFmt.nBlockAlign*((FRAMESIZE*N64MaxBlock*nScrPos)/nScrMax);
				
				// クリックした点に対応するバイト数を取得
				if(CalcMarkedPosByte(&n64Foo,lpEwcData->waveFmt,
					lpEwcData->n64WaveDataSize,
					n64CurByte,DwZoomX,RedrawRect,LOWORD(lp))){

					DWORD dwSplitPos;
					double dLevel[2],dMaxLevel;

					dMaxLevel = GetMaxWaveLevel(lpEwcData->waveFmt);
					GetLevelatPoint(lpEwcData->waveFmt,lpEwcData->hdFile,dLevel,
						lpEwcData->n64WaveDataOffset+n64Foo);
					
					// マークしたバイトをセット
					if(msg == WM_LBUTTONDOWN){
						N64MarkedPosByte =	n64Foo; // クリック
						DbMarkedLevel[0] = 20*log10(fabs(dLevel[0])/dMaxLevel);
						DbMarkedLevel[1] = 20*log10(fabs(dLevel[1])/dMaxLevel);
					}
					else if(msg == WM_RBUTTONDOWN){	// 右クリック
						N64SubMarkedPosByte = n64Foo;
						DbSubMarkedLevel[0] = 20*log10(fabs(dLevel[0])/dMaxLevel);
						DbSubMarkedLevel[1] = 20*log10(fabs(dLevel[0])/dMaxLevel);
						
						// スプリットマーク削除
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
								// 削除
								memmove(lpEwcData->n64SplitMarkedPosByte+dwSplitPos,
									lpEwcData->n64SplitMarkedPosByte+dwSplitPos+1,sizeof(LONGLONG)*(lpEwcData->dwSplitNum-dwSplitPos-1));
								memmove(lpEwcData->lnSplitMarkedPos+dwSplitPos,
									lpEwcData->lnSplitMarkedPos+dwSplitPos+1,sizeof(LONG)*(lpEwcData->dwSplitNum-dwSplitPos-1));
								lpEwcData->dwSplitNum--;
							}
						}
						
					}
					else // ダブルクリック
					{
						if(lpEwcData->dwSplitNum+1==MAX_SPLITNUM) return TRUE;

						for(i=0;i<(LONG)lpEwcData->dwSplitNum;i++)
							if(lpEwcData->n64SplitMarkedPosByte[i] == n64Foo) return TRUE;

						dwSplitPos = 0;
						while(dwSplitPos < lpEwcData->dwSplitNum 
							&& lpEwcData->n64SplitMarkedPosByte[dwSplitPos] < n64Foo) dwSplitPos++;
						
						// 挿入
						memmove(lpEwcData->n64SplitMarkedPosByte+dwSplitPos+1,
							lpEwcData->n64SplitMarkedPosByte+dwSplitPos,sizeof(LONGLONG)*(lpEwcData->dwSplitNum-dwSplitPos));
						memmove(lpEwcData->lnSplitMarkedPos+dwSplitPos+1,
							lpEwcData->lnSplitMarkedPos+dwSplitPos,sizeof(LONG)*(lpEwcData->dwSplitNum-dwSplitPos));
						
						lpEwcData->n64SplitMarkedPosByte[dwSplitPos] = n64Foo;
						lpEwcData->dwSplitNum++;
					}
					
					// マークしたスクロール位置をセット
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
					
					// 再描画
					RedrawWindow(hWnd);
				}
			}
		}
		
		break;
		
		// ----------------------------------------------------------
		
	case MM_WOM_DONE:  // 音声再生スレッドからの呼び出し
		
		// 経過時間の計算
		dwCurTime = (DWORD)lp;
		
		//時間バーの位置計算とセット
		nScrPos = (LONG)((double)nScrMax*(double)dwCurTime/(double)DwWaveTime);
		scrInfo.nPos = nScrPos;
		SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
		
		// 再描画
		RedrawWindow(hWnd);
		
		break;
		
		// ----------------------------------------------------------					
		
	case MM_WOM_CLOSE: // 再生時 wave デバイスがクローズした
		
		// ステータスをレディに
		if(wCurStatus == ID_STATPLAY) wCurStatus = ID_STATREADY;
		
		// 再描画
		RedrawWindow(hWnd);
		
		break;
		
		// ----------------------------------------------------------					

	case WM_CLOSE: // クローズ
		
		if(wCurStatus == ID_STATPLAY){	 // 音がなっていたら停止
			StopPlayWave();
			wCurStatus = ID_STATREADY;
		}

		
		// ファイルクローズ
		if(lpEwcData->hdFile) {
			CloseHandle(lpEwcData->hdFile);
			lpEwcData->hdFile = NULL;
		}

		// 裏画面バッファ開放
		DeleteDC(hBufDC);
		DeleteObject(hBufBit);
		
		// 座標ゲット
		GetWindowRect(hWnd, &curRt);
		lpEwcData->x = curRt.left;
		lpEwcData->y = curRt.top;
		
		DestroyWindow(hWnd);  // ウィンドウ削除
		
		break;
		
		
	case WM_DESTROY: // ウィンドウ削除

		PostQuitMessage(0);
		
		break;		

		
		// ----------------------------------------------------------
		// コマンド
		
	case WM_COMMAND:
		
		switch (LOWORD(wp)) {
			
		case IDC_BTSETUP: // 設定
			
			DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_SETTINGDIAG)
				,hWnd,(DLGPROC)SettingProc,(LPARAM)lpEwcData);
			
			break;

		case IDC_BTTRACK: // トラック設定
			
			lpEwcData->dwCurTrack = 
				GetCurTrack(lpEwcData->dwSplitNum,
				lpEwcData->n64SplitMarkedPosByte,lpEwcData->waveFmt.nBlockAlign,
				N64MaxBlock,scrInfo.nPos,nScrMax);
			dwFoo = DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_TRACK)
				,hWnd,(DLGPROC)TrackDlgProc,(LPARAM)lpEwcData);
			LoadTrackFile(hWnd,lpEwcData);
			
			// カット情報読み込みした
			if(dwFoo == IDC_BTCUTLOAD)
			{
				DwWaveTime = (DWORD)((lpEwcData->n64WaveDataSize*1000)/lpEwcData->waveFmt.nAvgBytesPerSec);

				// スクロールバー再設定
				N64MaxBlock = (DWORD)(lpEwcData->n64WaveDataSize/(FRAMESIZE*lpEwcData->waveFmt.nBlockAlign));
				nScrMax = N64MaxBlock > MAXLONG ? MAXLONG : (LONG)N64MaxBlock+1;
				
				// スクロールバーセット
				scrInfo.nPos = 0;
				scrInfo.nMax = nScrMax;
				SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
				
				// パラメータ初期化
				nScrPos = 0;
				dwCurTime = 0;
				nMarkedPos = 0;
				N64MarkedPosByte =	0;
				nSubMarkedPos = nScrMax;
				N64SubMarkedPosByte = lpEwcData->n64WaveDataSize;
				
				// undo データクリア
				undoData.wCurPos = 0;
				for(i=0;i<UNDOLEVEL;i++) undoData.bDataEmpty[i] = FALSE;
				
			}
			
			
			RedrawWindow(hWnd);
			
			break;

			
		case IDC_BTFILEINFO: // ファイル情報
			
			if(wCurStatus == ID_STATCLOSE) return TRUE; // 開いてなかったらそのままリターン
			
			if(wCurStatus == ID_STATREADY){ 
				
				wsprintf(SzInfo,
					"%s\r\n\r\n%d M\r\n\r\n%d hz, %d bit, %d channel, %d 秒\r\n",
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
			
		case IDC_BTSEARCH: // 検索
			
			if(wCurStatus == ID_STATCLOSE ) return TRUE; // 開いてなかったらそのままリターン
			
			if(wCurStatus == ID_STATPLAY){	 // 音がなっていたら停止
				StopPlayWave();
				wCurStatus = ID_STATREADY;
			}
			
			if(wCurStatus == ID_STATREADY){
				
				DWORD dwRet;
				lpEwcData->dwCurTrack 
					= GetCurTrack(lpEwcData->dwSplitNum,lpEwcData->n64SplitMarkedPosByte,
					lpEwcData->waveFmt.nBlockAlign,
					N64MaxBlock,scrInfo.nPos,nScrMax);

				// ジャンプダイアログ表示
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
				
				
				// 時間指定ジャンプ
				if(dwRet == IDC_BTJUMP)
				{
					MovePos(hWnd,msg,wp,lp);
					return TRUE;
				}
				
				// トラックジャンプ
				if(dwRet == IDC_BTMOVETRACK){
					if(lpEwcData->dwSplitNum){
						if(lpEwcData->dwCurTrack == 1) scrInfo.nPos = 0;
						else scrInfo.nPos = lpEwcData->lnSplitMarkedPos[lpEwcData->dwCurTrack-2]+1;
						SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
						RedrawWindow(hWnd);
					}
				}
				
				// 無音部サーチ、分割マークセット
				else if(dwRet== IDC_BTSEARCH || dwRet == IDC_BTSERCHSPLIT)
				{
					// 分割マークセットの場合
					if(dwRet == IDC_BTSERCHSPLIT){
						lpEwcData->dwSplitNum = 0;
						for(i=0;i<MAX_SPLITNUM;i++){
							lpEwcData->n64SplitMarkedPosByte[i] = 0;
							lpEwcData->lnSplitMarkedPos[i] = 0;
						}
						n64StartByte = lpEwcData->n64WaveDataOffset;
					}
					else{
						// サーチ開始初期位置取得
						nScrPos = SetScrBarInfo(hScrWnd,&scrInfo,wp);
						n64StartByte 
							= (lpEwcData->waveFmt.nBlockAlign*FRAMESIZE*N64MaxBlock*nScrPos)/nScrMax;
					}
					
					while(1){
						
						// 無音部サーチ開始
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
							
							// 無音部発見出来ず
							if(dwRet== IDC_BTSEARCH)
								MyMessageBox(hWnd, "無音部は見つかりませんでした。", "ewc", MB_OK|MB_ICONINFORMATION);
							else{
								wsprintf(szStr,"%d 点に分割マークをセットしました。",lpEwcData->dwSplitNum);

								MyMessageBox(hWnd, szStr, "ewc", MB_OK|MB_ICONINFORMATION);
							}
							return TRUE;
						}
						else{
							
							// サーチ後の位置セット
							switch(wNSoundPos){
								
							case NSOUND_TOP: // 先頭
								n64CurByte = n64StartByte;
								break;
							case NSOUND_END: // 後ろ
								n64CurByte = n64EndByte;
								break;
							case NSOUND_MID: // 中間
								n64CurByte = (n64EndByte + n64StartByte)/2;
								n64CurByte =  (n64CurByte 
									/(lpEwcData->waveFmt.nChannels*(lpEwcData->waveFmt.wBitsPerSample/8))) 
									*lpEwcData->waveFmt.nBlockAlign;
							}

							// スクロールバー位置セット
							nScrPos = min(nScrMax,(LONG)(nScrMax*n64CurByte/lpEwcData->n64WaveDataSize));
							scrInfo.nPos = nScrPos;
							SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
		
							// 分割マークセットの場合
							if(dwRet == IDC_BTSERCHSPLIT)
							{
								lpEwcData->n64SplitMarkedPosByte[lpEwcData->dwSplitNum] = n64CurByte;
								lpEwcData->lnSplitMarkedPos[lpEwcData->dwSplitNum] = nScrPos;
								lpEwcData->dwSplitNum++;
								n64StartByte = n64EndByte;
							}
							
							// 再描画
							RedrawWindow(hWnd);

							// サーチの場合はここで終わり
							if(dwRet != IDC_BTSERCHSPLIT) break;

							if(lpEwcData->dwSplitNum >= MAX_SPLITNUM){
								MyMessageBox(hWnd, "分割マーキングセット数が最大になりました。",
									"ewc", MB_OK|MB_ICONINFORMATION);
								break;
							}
							
						}
					}
				}
			}
			
			break;
			
			// ----------------------------------------------------------
			
		case IDC_BTEDIT: // エディット
			
			if(wCurStatus == ID_STATPLAY){	 // 音がなっていたら停止
				StopPlayWave();
				wCurStatus = ID_STATREADY;
			}

			// 編集
			StartEditWave(hWnd, msg, wp, lp);
			
			break;
			
			//-------------------------------------------------------------
			
		
			
		case IDC_BTLOAD: // ロード
			
			if(wCurStatus == ID_STATPLAY){	 // 音がなっていたら停止
				StopPlayWave();
				wCurStatus = ID_STATREADY;
			}

			
			// ファイルクローズ
			if(lpEwcData->hdFile != NULL) {
				CloseHandle(lpEwcData->hdFile);
				lpEwcData->hdFile = NULL;
			}
			
			// ロードダイアログ表示
			strcpy(szNewFileName,lpEwcData->szLoadFile);
			if(SelectLoadFile(hWnd ,szNewFileName,
				"*.wav\0*.wav\0All Files(*.*)\0*.*\0\0",
				"wav",
				"WAVE ファイル選択"
				)){ 
				
				// ファイル名セット
				strcpy(lpEwcData->szLoadFile,szNewFileName);		
				
				// ファイル再オープン
				LoadNewData(hWnd,msg,wp,lp);
				return TRUE;
				
			}
			else
			{
				// 元ファイルをそのまま開く
				OpenCurrentData(hWnd);
			}
			
			break;
			
			// ----------------------------------------------------------
			
		case IDC_BTZOOMIN: // ズームイン、アウト
		case IDC_BTZOOMOUT:
			
			if(wCurStatus == ID_STATCLOSE) return TRUE; // 開いてなかったらそのままリターン
			
			if(wCurStatus == ID_STATREADY){					
				
				// シフト押してたら縦のズーム
				if(GetKeyState(VK_SHIFT)&0x80){
					// 縦方向
					if(LOWORD(wp) == IDC_BTZOOMOUT && DwZoomY > 1) {
						DwZoomY /=2; 
					}
					else if(LOWORD(wp) == IDC_BTZOOMIN && DwZoomY <256){
						DwZoomY *=2; 
					}
				}
				else
				{
					// 横方向
					if(LOWORD(wp) == IDC_BTZOOMIN && DwZoomX > 1) {
						DwZoomX /= 2; // イン
					}
					else if(LOWORD(wp) == IDC_BTZOOMOUT && DwZoomX <8){
						DwZoomX *=2; // アウト
					}
				}
				
				// 再描画
				RedrawWindow(hWnd);
				
			}
			
			break;
			
			// ----------------------------------------------------------
			
		case IDC_BTPLAYSTOP: // 音声再生、停止
			
			if(wCurStatus == ID_STATREADY)
			{ // 再生
				
				//時間バーの現在位置取得と再生位置計算
				nScrPos = scrInfo.nPos;
				n64CurByte = lpEwcData->waveFmt.nBlockAlign*((FRAMESIZE*N64MaxBlock*nScrPos)/nScrMax);
				
				// 開始時刻(ミリ秒)
				dwCurTime = (DWORD)(n64CurByte*1000/lpEwcData->waveFmt.nAvgBytesPerSec);
				
				// 再生開始
				if(PlayWave(hWnd,
					lpEwcData->uDeviceID,
					lpEwcData->szLoadFile,
					dwCurTime,
					lpEwcData->hdFile,
					lpEwcData->waveFmt,
					lpEwcData->n64WaveDataSize,
					lpEwcData->n64WaveDataOffset)) wCurStatus = ID_STATPLAY; 
				
			}
			else if(wCurStatus == ID_STATPLAY){	 // 停止
				
				StopPlayWave();
				
				// 再描画
				InvalidateRect(hWnd,&RedrawRect,FALSE);
				
			}
				
			break;	
			
			// ----------------------------------------------------------
			
		case IDC_BTREW: // 巻き戻しボタン
		case IDC_BTREW2: // 先頭ボタン
		case IDC_BTREW3: // 先頭ボタン
		case IDC_BTFORWARD: // 進むボタン
		case IDC_BTFORWARD2: // 末尾ボタン
		case IDC_BTFORWARD3: // 末尾ボタン
		case IDC_BTJUMP: // マーク位置にJUMP
		case IDC_BTJUMPNEXT:
		case IDC_BTJUMPNEXT2:
		case IDC_BTJUMPNEXT3:
		case IDC_BTJUMPBACK:
		case IDC_BTJUMPBACK2:
		case IDC_BTJUMPBACK3:
			
			MovePos(hWnd,msg,wp,lp);
			
			break;
			
			// ----------------------------------------------------------
			
		case IDC_BTRESET: // リセット
			
			if(wCurStatus == ID_STATREADY){
				
				if(MyMessageBox(hWnd, "リセットしますか？", 
					"ewc", MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDYES){
					
					if(wCurStatus == ID_STATPLAY){	 // 音がなっていたら停止
						StopPlayWave();
						wCurStatus = ID_STATREADY;
					}
					
					for(i=0;i<MAX_SPLITNUM;i++){
						lpEwcData->n64SplitMarkedPosByte[i] = 0;
						lpEwcData->lnSplitMarkedPos[i] = 0;
						lpEwcData->dwSplitNum = 0;
					}
					
					// ファイルクローズ
					if(lpEwcData->hdFile != NULL) {
						CloseHandle(lpEwcData->hdFile);
						lpEwcData->hdFile = NULL;
					}						
					// ファイル再オープン
					LoadNewData(hWnd,msg,wp,lp);
					return TRUE;
				}
			}
			
			break;
			
			// ----------------------------------------------------------
			
		case IDC_BTCUTLEFT: // 左側カット
		case IDC_BTCUTRIGHT: // 右側カット
			
			if(wCurStatus == ID_STATREADY)
			{
				DWORD dwSplitPos;
				
				// undo のデータセット
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
				
				if(LOWORD(wp) == IDC_BTCUTLEFT){ // 左カット
					
					// サイズ変更
					lpEwcData->n64WaveDataOffset+= N64MarkedPosByte;
					lpEwcData->n64WaveDataSize -= N64MarkedPosByte;
					DwWaveTime = (DWORD)((lpEwcData->n64WaveDataSize*1000)/lpEwcData->waveFmt.nAvgBytesPerSec);
					
					// スクロールバー再設定
					N64MaxBlock = lpEwcData->n64WaveDataSize/(FRAMESIZE*lpEwcData->waveFmt.nBlockAlign);
					nScrMax = N64MaxBlock > MAXLONG ? MAXLONG : (LONG)N64MaxBlock+1;
					
					// パラメータ初期化
					nScrPos = 0;
					dwCurTime = 0;
					
					// 副ポインタの位置計算
					if(N64SubMarkedPosByte < N64MarkedPosByte)
					{ // 左
						N64SubMarkedPosByte = 0;
						nSubMarkedPos = 0;
					}
					else
					{ // 右
						N64SubMarkedPosByte -= N64MarkedPosByte;
						if(lpEwcData->n64WaveDataSize > 0)
						{
							dwByte = (DWORD)(lpEwcData->n64WaveDataSize/nScrMax);	// スクロール一回分のバイト数
							nSubMarkedPos = max(0,min((LONG)(N64SubMarkedPosByte/dwByte),nScrMax));
						}
						else nSubMarkedPos = 0;
					}
				
					// スプリットポインタ
					if(lpEwcData->dwSplitNum)
					{
						dwSplitPos = 0;
						while(dwSplitPos < lpEwcData->dwSplitNum 
							&& lpEwcData->n64SplitMarkedPosByte[dwSplitPos] < N64MarkedPosByte) dwSplitPos++;
						for(i2=dwSplitPos;i2<(LONG)lpEwcData->dwSplitNum;i2++)
						{
							lpEwcData->n64SplitMarkedPosByte[i2-dwSplitPos] = lpEwcData->n64SplitMarkedPosByte[i2] - N64MarkedPosByte;
							if(lpEwcData->n64WaveDataSize > 0){
								dwByte = (DWORD)(lpEwcData->n64WaveDataSize/nScrMax);	// スクロール一回分のバイト数
								lpEwcData->lnSplitMarkedPos[i2-dwSplitPos] = max(0,min((LONG)(lpEwcData->n64SplitMarkedPosByte[i2-dwSplitPos]/dwByte),nScrMax));
							}
							else lpEwcData->lnSplitMarkedPos[i2-dwSplitPos] = 0;
						}
						lpEwcData->dwSplitNum -= dwSplitPos;

					}
					
					// 主ポインタの位置
					nMarkedPos = 0;
					N64MarkedPosByte = 0;
					
				}
				else
				{ // 右カット
					// サイズ変更
					lpEwcData->n64WaveDataSize = N64MarkedPosByte;
					DwWaveTime = (DWORD)((lpEwcData->n64WaveDataSize*1000)/lpEwcData->waveFmt.nAvgBytesPerSec);
					
					// スクロールバー再設定
					N64MaxBlock	= lpEwcData->n64WaveDataSize/(FRAMESIZE*lpEwcData->waveFmt.nBlockAlign);
					nScrMax = N64MaxBlock > MAXLONG ? MAXLONG : (LONG)N64MaxBlock+1;
					
					// パラメータ初期化
					nScrPos = nScrMax;
					dwCurTime = DwWaveTime;
					
					// 副ポインタの位置計算
					if(N64SubMarkedPosByte > N64MarkedPosByte){ // 右
						N64SubMarkedPosByte = N64MarkedPosByte;
						nSubMarkedPos = nScrMax;
					} 
					else{  // 左
						if(lpEwcData->n64WaveDataSize > 0){
							dwByte = (DWORD)(lpEwcData->n64WaveDataSize/nScrMax);	// スクロール一回分のバイト数
							nSubMarkedPos = max(0,min((LONG)(lpEwcData->n64SplitMarkedPosByte[i]/dwByte),nScrMax));
						}
						else nSubMarkedPos = 0;
					}
					
				
					// スプリットポインタの位置計算
					if(lpEwcData->dwSplitNum){
						dwSplitPos = 0;
						while(dwSplitPos < lpEwcData->dwSplitNum && lpEwcData->n64SplitMarkedPosByte[dwSplitPos] < N64MarkedPosByte) dwSplitPos++;
						lpEwcData->dwSplitNum = dwSplitPos;
					}

					// 主ポインタの位置
					nMarkedPos = nScrMax;

				}
				
				// スクロールバーセット
				scrInfo.nPos = nScrPos;
				scrInfo.nMax = nScrMax;
				SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
				
				// 更新パラメータセット
				bUpdate = TRUE;
				
				// 再描画
				RedrawWindow(hWnd);
			}
			break;
			
			// ----------------------------------------------------------
			
			case IDC_BTUNDO: // UNDO
				
				if(wCurStatus == ID_STATREADY){					
					
					wUndoPos = (undoData.wCurPos - 1)&(UNDOLEVEL-1);
					
					if(undoData.bDataEmpty[wUndoPos])
					{ // UNDO のデータがあったら
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
						
						// 時間変更
						DwWaveTime = (DWORD)((lpEwcData->n64WaveDataSize*1000)/lpEwcData->waveFmt.nAvgBytesPerSec);
						
						// スクロールバー再設定
						N64MaxBlock = lpEwcData->n64WaveDataSize/(FRAMESIZE*lpEwcData->waveFmt.nBlockAlign);
						nScrMax = N64MaxBlock > MAXLONG ? MAXLONG : (LONG)N64MaxBlock+1;
						scrInfo.nPos = nScrPos;
						scrInfo.nMax = nScrMax;
						SetScrollInfo(hScrWnd,SB_CTL,&scrInfo,TRUE);
						
						// パラメータ初期化
						dwCurTime = DwWaveTime;
						if(lpEwcData->n64WaveDataOffset == N64OrgDataOffset && lpEwcData->n64WaveDataSize == N64OrgDataSize) bUpdate = FALSE;
						
						// 再描画
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
