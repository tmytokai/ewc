// 保存とかオフセット取得とかジャンプとかの関数

#include "common.h"
#include <math.h>


//-------------------------------------------------------------------
// Wave デバイスオープン時のコールバック関数
VOID CALLBACK MyOffsetWaveInProc(HWAVEOUT hWaveOut,UINT msg,DWORD inst,DWORD dwP1,DWORD dwP2)
{
	
	switch (msg) {
		
	case WIM_OPEN: // WAVE デバイスオープン
		*((LPWORD)inst) = ID_OPENWAVE;
		break;
		
	case WIM_DATA: // 録音終了
		
		*((LPWORD)inst) = ID_RECORDWAVE;
		break;
		
	case WIM_CLOSE: // WAVE デバイスクローズ
		
		*((LPWORD)inst) = ID_CLOSEWAVE;
		break;
		
	default:
		break;
    }
}





//-------------------------------------
// 上書き確認ダイアログ
LRESULT CALLBACK ProgDlgOverWrite(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	
    switch (msg) {
		
	case WM_INITDIALOG:  // ダイアログ初期化
		
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
// WAVEFLT 起動スレッド
//
// lpszOrgName のファイルを lpszNewFileName にコピーする

DWORD WINAPI StartWaveFlt(LPVOID lpVoid) 
{
	
	LPEWCDATA lpEwcData = (LPEWCDATA)lpVoid;
	
	HWND hWnd  	// 親ダイアログのウィンドウハンドル
		= lpEwcData->hWnd; 
	WAVEFORMATEX waveFmt // Wave フォーマットデータ
		= lpEwcData->waveFmt; 
	LPSTR lpszNewFileName 	// コピーするファイル名
		= lpEwcData->lpszFileName; 
	LPSTR lpszOrgName 	// 元ファイル名
		= lpEwcData->lpszOrgName; 
	LPWORD lpwStatus 	// ステータス
		= lpEwcData->lpwStatus; 
	PROCESS_INFORMATION* pProInfo  // WaveFLT のプロセス情報
		= lpEwcData->pProInfo; 

	unsigned long long n64DataSize; 	//コピーするデータサイズ
	
	// Wave データ用
	unsigned long long n64SrcDataSize; // コピー元ファイルのデータサイズ
	unsigned long long n64SrcDataOffset; // コピー元ファイルのヘッダオフセット
	
	CHAR szStr[CHR_BUF];
	DWORD dwExitCode;
	
	// コマンドライン起動用
	CHAR szCommandLine[CHR_BUF];
	CHAR szFoo[CHR_BUF],szFoo2[CHR_BUF];
	CHAR szData[MAX_WAVFLTOPTBUF];
	HINSTANCE hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
	HANDLE hFileMap = NULL; // File Mapping Object 用
	HANDLE hTrackFileMap = NULL; // File Mapping Object 用
	char* lpszTrackName = NULL;
	DWORD dwOVWstat = 0; // 上書き確認 (0: 問い合わせ, 1:全て上書き, 2:全て上書きしない)

	DWORD i;
	LONGLONG n64Foo,n64Foo2,n64Prev;
	LONG nRet;
	
	
	// コピーを行う場合にコピーできるかチェック
	if(lpszNewFileName != NULL)
	{
		n64DataSize = lpEwcData->n64NewDataSize;
		
		// コピー出来るかディスクの空サイズチェック
		// stdout の時はチェックしない
		ULONGLONG u64DiskFreeSpace; 
		GetFreeHddSpace64(&u64DiskFreeSpace,lpszNewFileName);
		
		if(u64DiskFreeSpace < n64DataSize/+1024)
		{
			wsprintf(szStr,"ドライブの空き容量 %lu M 必要な容量 %lu M\n\nドライブの空容量が足りません。ドライブを変更して下さい。"
				,(DWORD)(u64DiskFreeSpace/1024/1024),(DWORD)(n64DataSize/1024/1024));
			MyMessageBox(hWnd,szStr,"Error", MB_OK|MB_ICONERROR);	
			
			SendMessage(hWnd,WM_MYENDCOPY,0,1L);
			ExitThread(1L);
		}
		
		// オリジナルのフォーマット取得
		if(!GetWaveFormat(lpszOrgName,&waveFmt,&n64SrcDataSize,&n64SrcDataOffset,szStr))
		{
			MyMessageBox(hWnd,szStr,"Error", MB_OK|MB_ICONERROR);	

			// 失敗メッセージを送る
			SendMessage(hWnd,WM_MYENDCOPY,0,1L);
			ExitThread(1L);
		}
		
		// コピー出来るかデータサイズのチェック
		if(n64SrcDataSize < n64DataSize)
		{
			wsprintf(szStr,"コピー出来ません。\n\nコピー元データサイズ %lu M コピーするサイズ %lu M\n\nコピー元のファイルサイズか小さすぎます。"
				,(DWORD)(n64SrcDataSize/1024/1024),(DWORD)(n64DataSize/1024/1024));
			MyMessageBox(hWnd,szStr,"Error", MB_OK|MB_ICONERROR);	
			
			// 失敗メッセージ送信
			SendMessage(hWnd,WM_MYENDCOPY,0,1L);
			ExitThread(1L);
		}
	}
	
	SetWindowText(hWnd,"WAVEFLT 実行中");
	
	//----------------------------------
	// コマンドラインセット
	if(strcmp(lpszNewFileName,"waveout") == 0)
		wsprintf(szCommandLine,"waveflt.exe -fmap \"%s\" waveout",lpszOrgName);  // サウンド出力
	else 
		wsprintf(szCommandLine,"waveflt.exe -fmap \"%s\" \"%s\"",lpszOrgName,lpszNewFileName); 
	
	// waveflt の場合はヘッダオフセットの分引いとく
	lpEwcData->n64NewDataOffset -= n64SrcDataOffset;
	
	//---------------
	// filemapping に オプションセット

	// コピー範囲
	if(!lpEwcData->bCutCm)
	{
		sprintf(szData,"-cutb %I64d %I64d ",
			lpEwcData->n64NewDataOffset,lpEwcData->n64NewDataSize); 
	}

	// 分割
	// -outfile_fm オプション使ってトラック名を渡す
	if(lpEwcData->bSplit)
	{
		char szDrive[16],szPath[MAX_PATH],szFullPath[MAX_PATH];
		HANDLE hTmp;

		lpszTrackName = (char*)malloc(sizeof(char)*(lpEwcData->dwSplitNum+1)*MAX_PATH+1024);
		memset(lpszTrackName,0,sizeof(char)*(lpEwcData->dwSplitNum+1)*MAX_PATH);

		// 保存先ディレクトリ取得
		_splitpath(lpEwcData->lpszFileName,szDrive,szPath,NULL, NULL);	
		wsprintf(szFullPath,"%s%s",szDrive,szPath);
	
		// (注) -splitbm はサイズで指定するのでオフセットとかは考えなくて良い
		wsprintf(szData,"%s -splitbm %lu ",szData,lpEwcData->dwSplitNum+1);

		// 分割サイズセット
		n64Prev = 0;
		for(i=0;i<lpEwcData->dwSplitNum;i++)
		{
			n64Foo = lpEwcData->n64SplitMarkedPosByte[i];
			n64Foo2 = n64Foo-n64Prev;
			sprintf(szFoo,"%I64d ",n64Foo2);
			strcat(szData,szFoo);
			n64Prev = n64Foo;
		}
		sprintf(szFoo,"%I64d ",lpEwcData->n64NewDataSize); // 残り全部
		strcat(szData,szFoo);

		// ファイル名セット
		for(i=0;i<=lpEwcData->dwSplitNum;i++)
		{
			// ファイル名が "null" ならそのまま
			if(strcmp(lpEwcData->szTrackName[i],"null")==0){
				strcpy(szFoo,"null");
			}
			else
			{
				// 既にファイルが存在しているかチェック
				wsprintf(szFoo,"%s%s",szFullPath,lpEwcData->szTrackName[i]);
				hTmp = CreateFile(szFoo,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_READONLY,NULL); 
				
				// 存在したらファイル名を"null"にする
				if(hTmp != INVALID_HANDLE_VALUE){
					CloseHandle(hTmp);
					wsprintf(szFoo2,"ファイル %s は既に存在します。\n\n上書き保存しますか?",szFoo);
					
					switch(dwOVWstat){
						
					case 0: // ダイアログを出して問い合わせ
						
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
							// 中止
							(*lpwStatus) = ID_COPYOFF;
							goto L_ERROR;
						}
						
						break;
						
						case 2: // 全て上書きしない
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
			MyMessageBox(hWnd, "ファイルマッピングの作成に失敗しました","Error", MB_OK|MB_ICONERROR);	
			dwExitCode = 1;
			goto L_ERROR;
		}

		wsprintf(szFoo,"-outfile_fm %lu ",(DWORD)hTrackFileMap); 
		strcat(szData,szFoo);
	}
	else 
		if(lpEwcData->bCutCm)
		{ // CM カット

		wsprintf(szData,"-cutmb %lu ",lpEwcData->dwSplitNum/2+1);

		// (注) -cutmb は オフセット + ブロックサイズ で指定

		// 最初
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
		
		// 残り
		if(lpEwcData->dwSplitNum%2==0)
		{
			n64Foo = lpEwcData->n64NewDataOffset + lpEwcData->n64SplitMarkedPosByte[i];
			n64Foo2 = lpEwcData->n64NewDataSize - lpEwcData->n64SplitMarkedPosByte[i];
			sprintf(szFoo,"%I64d %I64d ",n64Foo,n64Foo2);
			strcat(szData,szFoo);
		}
	}

	// 残りのオプションセット
	strcat(szData,lpEwcData->editSaveData.szOption);

	// WaveFLT 起動
	// オプションは File Mapping で渡す
	if(!ExecCommandFileMapping(&hFileMap,NULL,szCommandLine,szData,MAX_WAVFLTOPTBUF,
		pProInfo,hInst,lpEwcData->bShowConsole,szStr))
	{
		SetForegroundWindow(hWnd);
		MyMessageBox(hWnd, "WAVEFLT の起動に失敗しました\n\newc.exe と同じフォルダに waveflt.exe があるか確認して下さい", 
			"Error", MB_OK|MB_ICONERROR);	
		
		dwExitCode = 1;
		goto L_ERROR;
	}
	
	// WaveFLT が終るまで停止
	WaitForSingleObject(pProInfo->hProcess,INFINITE);
	GetExitCodeProcess(pProInfo->hProcess,&dwExitCode);
	
L_ERROR:

	pProInfo->hProcess = NULL;

	// File Mapping Object 削除
	if(hFileMap) CloseHandle(hFileMap);
	if(hTrackFileMap) CloseHandle(hTrackFileMap);

	if(lpszTrackName) free(lpszTrackName);


	// 終了メッセージ送信

	// キャンセルした場合
	if((*lpwStatus) == ID_COPYOFF) SendMessage(hWnd,WM_MYENDCOPY,0,2L);
	else{
		
		if(dwExitCode == 0) SendMessage(hWnd,WM_MYENDCOPY,0,0L); // 成功
		else {
			SetForegroundWindow(hWnd);
			SendMessage(hWnd,WM_MYENDCOPY,0,1L); // 失敗
		}
	}
				
	return 0;
	
}



//-------------------------------------
// WAVEFLT 起動ダイアログのプロシージャ
// スレッドを起動するだけ
LRESULT CALLBACK ProgDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	
	static HANDLE hThread = NULL;  // スレッドのハンドル
	static DWORD threadId; // スレッド ID
	static WORD wStatus;
	static LPEWCDATA lpEwcData;  
	static PROCESS_INFORMATION proInfo; // WaveFLT のプロセス情報
	UINT uRet;
	
    switch (msg) {
		
	case WM_INITDIALOG:  // ダイアログ初期化
		
		if(hThread == NULL){
			
			// ダイアログを中心に移動
			SetDlgCenter(hWnd);
			
			// 保存スレッド起動
			proInfo.hProcess = NULL;
			wStatus = ID_COPYON;
			lpEwcData = (LPEWCDATA)lp;
			lpEwcData->hWnd = hWnd;
			lpEwcData->lpwStatus = &wStatus;
			lpEwcData->pProInfo = &proInfo;
			
			hThread = CreateThread(NULL,0,
				(LPTHREAD_START_ROUTINE)StartWaveFlt,
				(LPVOID)lpEwcData,
				0,(LPDWORD)&threadId);
			
			if(hThread == NULL){	   
				MyMessageBox(hWnd, "スレッドの起動に失敗しました。", 
					"Error", MB_OK|MB_ICONERROR);
				EndDialog(hWnd, IDCANCEL); 
			}
		}
		else {
			MyMessageBox(hWnd, "既にスレッドが動いています。", 
				"Error", MB_OK|MB_ICONERROR);
			EndDialog(hWnd, IDCANCEL); 
		}
		
		break;
		
	case WM_MYENDCOPY: // コピー完了
		
		hThread = NULL;
		
		// フォワードに持ってくる
		SetForegroundWindow(hWnd);
		Sleep(100);
		
		if((DWORD)lp == 1) {
			MyMessageBox(hWnd,"失敗しました\n\n原因を解決してもう一度コピーを行って下さい", "ewc", MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
			uRet = IDCANCEL; // 異常終了
		}
		else if((DWORD)lp == 2){ // キャンセル
			uRet = IDCANCEL; // 異常終了
		}
		else
		{
			MyMessageBox(hWnd,"正常に終了しました。", "ewc", MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND);
			uRet = IDOK; // 正常終了
		}
		
		Sleep(500);
		FreeConsole();
		
		
		
		EndDialog(hWnd, uRet); 
		
		break;
		
	case WM_COMMAND:
		
		switch (LOWORD(wp)) {
			
		case IDCANCEL: // 停止
			
			wStatus = ID_COPYOFF;
			
			if(proInfo.hProcess != NULL){
				
				// Ctrl+Break 送信
				DWORD dwMode;
				GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),&dwMode);
				SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),dwMode|ENABLE_PROCESSED_INPUT);
				SetConsoleCtrlHandler(NULL,FALSE);
				Sleep(100);
				GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT,proInfo.dwProcessId);
				
				// まだWAVEFLTが動いていたら
				if(WaitForSingleObject(proInfo.hProcess,10000) == WAIT_TIMEOUT){ 
					
					// WAVEFLT を強制停止
					TerminateProcess(proInfo.hProcess,1);
					
					// それでもまだWAVEFLTが動いていたら
					if(WaitForSingleObject(proInfo.hProcess,10000) == WAIT_TIMEOUT){ 
						Sleep(1000);
						if(proInfo.hProcess != NULL){ 
							MyMessageBox(hWnd,"スレッドの停止に失敗しました\n\nEasy Wave Cutter を再起動してください", "ewc", MB_OK|MB_ICONERROR);
							uRet = IDCANCEL; // 異常終了
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
// データセーブ
BOOL SaveCutData(HWND hWnd,
				 HINSTANCE hInst,
				 LPEWCDATA lpEwcData,
				 LPSTR lpszOrgFileName,  // コピー元ファイル名
				 LPSTR lpszNewFileName,  // コピー先ファイル名
				 // lpszNewFileName が waveout の時はデバイスに出力
				 BOOL bSplit  // 分割録音かどうか
				 ){
	
				
	BOOL bMoveFile; // 上書きで元ファイルの名前を変更したかどうか
	CHAR fPath[CHR_BUF],fDrive[CHR_BUF],fName[CHR_BUF],fExt[CHR_BUF];
	CHAR szReadFile[MAX_PATH];  // 実際にロードするファイル
	
	bMoveFile = FALSE;
	strcpy(szReadFile,lpszOrgFileName);
	
	// 上書きだったら
	if(_strnicmp(lpszOrgFileName,lpszNewFileName,MAX_PATH)==0){
		
		if(!bSplit){
			
			if(MyMessageBox(hWnd, "現在編集中のファイルに上書き保存してよろしいですか？\n\n☆注意☆  現在編集中のファイルは *.old とリネームされます。"
				,"ewc",MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2)==IDYES){
				
				// 上書きの場合は昔のファイルを *.old にリネームする
				_splitpath(lpszOrgFileName,fDrive,fPath,fName,fExt);
				wsprintf(szReadFile,"%s%s%s.old",fDrive,fPath,fName);  
				
				// ファイル名変更
				MoveFile(lpszOrgFileName,szReadFile);
				bMoveFile = TRUE;
			}
			else return FALSE;
		}
		else
		{
			// 分割の場合は出力側の名前を変える
			// -outfile_fm オプション使ってトラック名を渡してるので
			// 適当な名前でよい
			_splitpath(lpszNewFileName,fDrive,fPath,fName,fExt);
			wsprintf(lpszNewFileName,"%s%s%s2%s",fDrive,fPath,fName,fExt);  
		}
	}
	
	lpEwcData->lpszOrgName = szReadFile;
	lpEwcData->lpszFileName = lpszNewFileName;
	
	// ダイアログボックス表示と保存開始
	if(DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_COPYFILE)
		,hWnd,(DLGPROC)ProgDlgProc,
		(LPARAM)lpEwcData) == IDCANCEL){
		
		// 保存失敗
		
		// ファイル削除
		DeleteFile(lpszNewFileName);
		
		// リネームしてたら元に戻す
		if(bMoveFile) MoveFile(szReadFile,lpszOrgFileName);
		
		return FALSE;
	}

	return TRUE;
}





//-------------------------------------
// ジャンプ指定ダイアログのプロシージャ
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
		
	case WM_INITDIALOG:  // ダイアログ初期化
		
		lpJumpData = (LPJUMPDATA)lp;
		lpdNoSoundBound = lpJumpData->lpdNoSoundBound;
		lpdwNSoundCount = lpJumpData->lpdwNSoundCount;
		lpdwSearchTime = lpJumpData->lpdwSearchTime;
		lpwNSoundPos = lpJumpData->lpwNSoundPos;
		lpEwcData = lpJumpData->lpEwcData;
		
		// ダイアログを中心に移動
		SetDlgCenter(hWnd);
		
		// 時間セット
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

		// トラック設定
		SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_SETRANGE,(WPARAM)0,(LPARAM)MAKELONG((short)lpEwcData->dwSplitNum+1,1));
		SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_SETPOS,(WPARAM)0L,(LPARAM)MAKELONG((short)lpEwcData->dwCurTrack,0));

		// スライダーの設定
		
		// レベル
		SendMessage(GetDlgItem(hWnd,IDC_SLIDERBOUND),TBM_SETRANGE,(WPARAM)TRUE,(LPARAM)MAKELONG(0,(LONG)(-1*MINSEARCHBOUND)*5));
		SendMessage(GetDlgItem(hWnd,IDC_SLIDERBOUND),TBM_SETPOS,(WPARAM)TRUE,(LPARAM)(LONG)((-1*MINSEARCHBOUND + *lpdNoSoundBound)*5));
		wsprintf(szStr,"%s dB",myfcvt(*lpdNoSoundBound,1));
		Edit_SetText(GetDlgItem(hWnd,IDC_EDITBOUND),(LPCTSTR)szStr);
		
		// 秒数
		SendMessage(GetDlgItem(hWnd,IDC_SLIDERCOUNT),TBM_SETRANGE,(WPARAM)TRUE,(LPARAM)MAKELONG(0,S_POINT_PER_SEC*10));
		SendMessage(GetDlgItem(hWnd,IDC_SLIDERCOUNT),TBM_SETPOS,(WPARAM)TRUE,(LPARAM)*lpdwNSoundCount);
		wsprintf(szStr,"%s 秒",myfcvt(*lpdwNSoundCount/(double)S_POINT_PER_SEC,2));
		Edit_SetText(GetDlgItem(hWnd,IDC_EDITCOUNT),(LPCTSTR)szStr);
		
		//チェックボックス、ラジオボタンの設定
		if(*lpwNSoundPos == NSOUND_TOP) Button_SetCheck(GetDlgItem(hWnd,IDC_RADIOTOP), BST_CHECKED);
        else if(*lpwNSoundPos == NSOUND_MID) Button_SetCheck(GetDlgItem(hWnd,IDC_RADIOMID), BST_CHECKED);
		else Button_SetCheck(GetDlgItem(hWnd,IDC_RADIOEND), BST_CHECKED);

		if(lpEwcData->editSaveData.bUseAvr) Button_SetCheck(GetDlgItem(hWnd,IDC_CHKAVR), BST_CHECKED);
		
		break;
		
	case WM_HSCROLL: // スライダを動かした
		
		// 秒数指定
		if(GetDlgItem(hWnd,IDC_SLIDERCOUNT) == (HWND)lp){
			dwFoo = (DWORD)SendMessage(GetDlgItem(hWnd,IDC_SLIDERCOUNT),TBM_GETPOS,(WPARAM)0,(LPARAM)0);
			wsprintf(szStr,"%s 秒",myfcvt(dwFoo/(double)S_POINT_PER_SEC,2));
			Edit_SetText(GetDlgItem(hWnd,IDC_EDITCOUNT),(LPCTSTR)szStr);
		}
		else{ // 無音レベル
			dFoo = MINSEARCHBOUND +(LONG)SendMessage(GetDlgItem(hWnd,IDC_SLIDERBOUND),TBM_GETPOS,(WPARAM)0,(LPARAM)0)/5.;
			wsprintf(szStr,"%s dB",myfcvt(dFoo,1));
			Edit_SetText(GetDlgItem(hWnd,IDC_EDITBOUND),(LPCTSTR)szStr);
		}
		
		break;
		
		
	case WM_COMMAND:
		
		switch (LOWORD(wp)) 
		{
			
		case IDC_BTJUMP:  // 指定した時間へジャンプ
			
			// 時間セット
			dwTimeHour = min(999,SendMessage(GetDlgItem(hWnd,IDC_SPINHOUR),UDM_GETPOS,(WPARAM)0L,(LPARAM)0L));
			dwTimerMin = min(59,SendMessage(GetDlgItem(hWnd,IDC_SPINMIN),UDM_GETPOS,(WPARAM)0L,(LPARAM)0L));
			dwTimerSec = min(59,SendMessage(GetDlgItem(hWnd,IDC_SPINSEC),UDM_GETPOS,(WPARAM)0L,(LPARAM)0L));
			dwTime = dwTimeHour*60*60+dwTimerMin*60+dwTimerSec;
			*lpdwSearchTime = dwTime;
			
			EndDialog(hWnd, LOWORD(wp)); 
			break;
			
		case IDC_BTMOVETRACK:

			// トラック設定
			lpEwcData->dwCurTrack = SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_GETPOS,(WPARAM)0L,(LPARAM)0L);
			EndDialog(hWnd,IDC_BTMOVETRACK);
			break;
			
		case IDC_BTSEARCH:  // 無音部サーチ
		case IDC_BTSERCHSPLIT:  
			
			//スライダ
			*lpdNoSoundBound = MINSEARCHBOUND+(double)SendMessage(GetDlgItem(hWnd,IDC_SLIDERBOUND),TBM_GETPOS,(WPARAM)0,(LPARAM)0)/5;
			*lpdwNSoundCount = (DWORD)SendMessage(GetDlgItem(hWnd,IDC_SLIDERCOUNT),TBM_GETPOS,(WPARAM)0,(LPARAM)0);
			
			// ラジオボックス
			if(IsDlgButtonChecked(hWnd,IDC_RADIOTOP) == BST_CHECKED) *lpwNSoundPos = NSOUND_TOP;
			else if(IsDlgButtonChecked(hWnd,IDC_RADIOMID) == BST_CHECKED) *lpwNSoundPos = NSOUND_MID;
			else *lpwNSoundPos = NSOUND_END;

			if(IsDlgButtonChecked(hWnd,IDC_CHKAVR) == BST_CHECKED) lpEwcData->editSaveData.bUseAvr = TRUE;
			else lpEwcData->editSaveData.bUseAvr = FALSE;
			
			EndDialog(hWnd, LOWORD(wp)); 
			break;

		case IDC_BTOPTVAL: // 最適値検索
			
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

			
		case IDCANCEL: // 停止
			EndDialog(hWnd, IDCANCEL); 
		}
		
		break;
		
		default:
			return FALSE;
    }
    return TRUE;
}




//EOF
