// easy Wave Cutter
// Copyright (c) 1999-2015 Tomoya Tokairin
//
// 一般的な関数


#include "common.h"


//------------------------------------------------------------------
// 現在のトラック番号取得
DWORD GetCurTrack(DWORD dwSplitNum,
				  LONGLONG n64SplitMarkedPosByte[MAX_SPLITNUM],
				  LONG nBlockAlign,
				  LONGLONG n64MaxBlock,
				  LONG nPos,
				  LONG nScrMax){
	
	DWORD dwFoo;
	LONGLONG n64Foo2;
	
	n64Foo2 = (nBlockAlign*FRAMESIZE*n64MaxBlock*nPos)/nScrMax;
	dwFoo=0;
	while(dwFoo < dwSplitNum)
	{
		if(n64SplitMarkedPosByte[dwFoo] > n64Foo2) break;
		dwFoo++;
	}
	
	return dwFoo+1;
}



//-------------------------------------------------------------------
// トラック名設定ファイル読み込み関数
BOOL LoadTrackFile(HWND hWnd,
				   LPEWCDATA lpEwcData
			   ){
	
	DWORD i,i2,dwNum = 0;;
	FILE* f;
	char szStr[MAX_PATH];

	// ディフォルト名セット
	for(i=0;i<MAX_SPLITNUM;i++) 
		wsprintf(lpEwcData->szTrackName[i],"%s-%03d.%s",lpEwcData->szBaseName,i+1,lpEwcData->szExtName);

	// トラック設定ファイルオープン
	if(lpEwcData->szTrackFile[0] == '\0') return FALSE;
	f = fopen(lpEwcData->szTrackFile,"r");
	if(f == NULL){
		
		// カレントディレクトリで試してみる
		char szFile[MAX_PATH];
		char szDriveName[MAX_PATH],szPathName[MAX_PATH],szFileName[MAX_PATH],szExt[MAX_PATH];
		GetModuleFileName(NULL,szStr,MAX_PATH); 
		_splitpath(szStr,szDriveName,szPathName,NULL,NULL);	
		_splitpath(lpEwcData->szTrackFile,NULL,NULL,szFileName,szExt);
		wsprintf(szFile,"%s%s%s%s",szDriveName,szPathName,szFileName,szExt);
		f = fopen(szFile,"r");
		
		if(f == NULL)
		{
			MyMessageBox(NULL, "トラック名設定ファイルを開けません"
				, "Error", MB_OK|MB_ICONERROR|MB_SETFOREGROUND);	
			return FALSE;
		}
	}
	
	while(1){
		fgets(szStr,MAX_PATH,f);
		if(feof(f) != 0) break;
		
		// 改行文字と空白を消しとく
		i=i2=0;
		while(szStr[i2] == ' ') i2++;
		while(szStr[i2] != '\0'
				&& szStr[i2] != '\n'
				&& szStr[i2] != '\r'			
			) szStr[i++] = szStr[i2++];
		szStr[i] = '\0'; 
		if(szStr[0] != '\0') { 
			wsprintf(lpEwcData->szTrackName[dwNum],"%s.%s",szStr,lpEwcData->szExtName);
		}
		else if(lpEwcData->editSaveData.bOutfileIsNull)
			wsprintf(lpEwcData->szTrackName[dwNum],"null");

		dwNum++;
		if(dwNum >= MAX_SPLITNUM) break;
	}
	fclose(f);
	
	if(!dwNum) return FALSE;

	return TRUE;
}



//-------------------------------------------
// 指定した地点の音レベル取得
void GetLevelatPoint(WAVEFORMATEX waveFmt,HANDLE hdFile,double dLevel[2],LONGLONG n64Pos){
	
	BYTE lpBuf[64];
	LARGE_INTEGER LI;
	DWORD dwByte;

	if(hdFile == NULL) return;

	LI.QuadPart = n64Pos;
	SetFilePointer(hdFile,LI.LowPart, &LI.HighPart,FILE_BEGIN);
	ReadFile(hdFile,lpBuf,waveFmt.nBlockAlign,&dwByte, NULL);
	
	dLevel[0] = dLevel[1] = 0;
	if(dwByte){
		WaveLevel(dLevel,lpBuf,waveFmt);
	}
}




//---------------------------------
// 指定した位置から指定した時間の間のピークをサーチする関数
// 戻り値: ピーク
double SearchPeak(HANDLE hdFile, // サーチするファイルハンドル
				LONGLONG n64DataOffset, //データまでのオフセット
				WAVEFORMATEX waveFmt, // Wave フォーマット
				LONGLONG n64StartByte,  // スタート位置
				double dTime // 検索時間
				){
	
	DWORD dwSize = (DWORD)(dTime * waveFmt.nSamplesPerSec);
	double dRet = 0;
	double dMax = 0;
	LARGE_INTEGER LI; // SetFilePointer 用
	double dLevel[2];
	DWORD i,dwByte;
	BYTE buffer[64];
	
	if(hdFile)
	{
		LI.QuadPart = n64DataOffset+n64StartByte;
		SetFilePointer(hdFile,LI.LowPart, &LI.HighPart,FILE_BEGIN);
		
		for(i=0;i<dwSize;i++){
				ReadFile(hdFile,buffer,waveFmt.nBlockAlign,&dwByte, NULL);
				if(dwByte == 0) break;
				WaveLevel(dLevel,buffer,waveFmt);
				dMax = max(dLevel[0],dLevel[1]);
				dRet = max(dRet,dMax);
		}
	}
	
	return dRet;
}


//-------------------------------------------------------------------
// ファイル名をいれてパスが存在しているか調べる関数
// ディレクトリじゃないか存在しなければ FALSE
BOOL ExistDirectory(LPSTR lpszFileName){
	
	CHAR fPath[MAX_PATH],fDrive[MAX_PATH],fName[MAX_PATH],fExt[MAX_PATH]; 
	CHAR szFind[MAX_PATH];
	DWORD dwResult;
	
	_splitpath(lpszFileName,fDrive,fPath,fName,fExt);	
	wsprintf(szFind,"%s%s",fDrive,fPath); 
	dwResult = GetFileAttributes(szFind);
	
	if(dwResult & FILE_ATTRIBUTE_DIRECTORY == 0
		||dwResult == 0xFFFFFFFF ) return FALSE;
	
	return TRUE;
}




//-----------------------------------
// コマンドラインの文字列取得関数
// 戻り値 = argc
int GetArgv(char* lpszCmdLine,char argv[MAX_ARGC][CHR_BUF],int start_pos){
	
	int cmdln;
	int i,i2,argc;
	char c;
	
	cmdln = strlen(lpszCmdLine);
	for(i=0;i<MAX_ARGC;i++) argv[i][0] = '\0';
	
	argc = start_pos;
	if(cmdln > 0){
		
		i = 0;
		while(i < cmdln && argc < MAX_ARGC){
			
			// 空白を飛ばす
			while(lpszCmdLine[i] == ' ') i++;
			
			// "" で囲まれてる場合
			if(lpszCmdLine[i] == '"') {
				c = '"'; 
				i++;
			}
			else c = ' ';
			
			// コピー
			i2 = 0;
			while(lpszCmdLine[i] != '\0' && lpszCmdLine[i] != c){
				argv[argc][i2] = lpszCmdLine[i];
				i++;i2++;
			}
			
			if(c ==  '"') i++;
			
			argv[argc][i2] = '\0';
			argc++;i++;
		}
	}
	
	return argc;
}

//------------------------------------------
// コンソールの HWND をゲットする関数
HWND GetConsoleWindowHandle()
{
	char szOldTitle[CHR_BUF],szNewTitle[CHR_BUF];
	HWND hWnd;
	
	GetConsoleTitle(szOldTitle,CHR_BUF);
    wsprintf(szNewTitle,"console(%d-%d)",GetCurrentProcessId(),GetTickCount());
	SetConsoleTitle(szNewTitle);
	Sleep(40);
	hWnd = FindWindow(NULL, szNewTitle);
	SetConsoleTitle(szOldTitle);
	
	return(hWnd);
}



//-------------------------------------
// ショートファイル名をロングファイル名へ変換する関数
BOOL ShortToLongName(LPSTR lpszFileName){
	
	char fPath[MAX_PATH],fDrive[MAX_PATH],fName[MAX_PATH],fExt[MAX_PATH]; 
	WIN32_FIND_DATA findData;
	HANDLE hFind;
	
	hFind = FindFirstFile(lpszFileName,&findData);
	
	if(hFind==INVALID_HANDLE_VALUE) return FALSE;
	
	FindClose(hFind); 
	
	_splitpath(lpszFileName,fDrive,fPath,fName, fExt);	
	wsprintf(lpszFileName,"%s%s%s",fDrive,fPath,findData.cFileName);  
	
	return TRUE;
}




//-------------------------------------------------------------------
// ハードディスクの空き取得関数(FAT32,NTFS 対応 64 bit 版)

typedef BOOL (WINAPI *PGETDISKEX)(LPCTSTR,PULARGE_INTEGER,PULARGE_INTEGER,PULARGE_INTEGER);

VOID GetFreeHddSpace64(ULONGLONG* lpullSize,LPSTR lpszFileName){
	
	CHAR fDrive[CHR_BUF],str[CHR_BUF];
	DWORD sectorClus,byteSect,freeClus,totalClus;
	DWORD dwType;
	HMODULE hModKer32;
	PGETDISKEX pGetDiskEx = NULL;
	ULARGE_INTEGER uliFreeByteAvailCaller;
	ULARGE_INTEGER uliTotalByte;
	
	// kernel32.dll のハンドル取得
	hModKer32 = GetModuleHandle("kernel32.dll");
	
	// GetDiskFreeSpaceEx の場所を探す
	if(hModKer32 != NULL){
		pGetDiskEx = (PGETDISKEX)GetProcAddress(hModKer32,"GetDiskFreeSpaceExA");
	}
	
	_splitpath(lpszFileName,str,NULL,NULL,NULL);
	wsprintf(fDrive,"%s\\",str);
	
	dwType = GetDriveType(fDrive);
	
	//ハードディスク存在するかチェック。無かったら空サイズを 0 にセット
	if(dwType == DRIVE_NO_ROOT_DIR || dwType == DRIVE_UNKNOWN) uliFreeByteAvailCaller.QuadPart = 0;
	else {
		// 旧バージョンの Windows
		if(pGetDiskEx == NULL){
			GetDiskFreeSpace(fDrive,&sectorClus,&byteSect,&freeClus,&totalClus);
			uliFreeByteAvailCaller.QuadPart = sectorClus*byteSect*freeClus;
		}
		else // 新バージョン(FAT32,NTFS 対応) 
			(*pGetDiskEx)(fDrive,&uliFreeByteAvailCaller,&uliTotalByte,NULL);
	}
	
	*lpullSize = uliFreeByteAvailCaller.QuadPart;
}


//-------------------------------------------------------------------
// ディレクトリ選択ダイアログ
BOOL SelectDir(HWND hWnd,
			   LPSTR lpszDir,
			   LPSTR lpszTitle
			   ){

	BROWSEINFO  brInfo;
	LPITEMIDLIST lpIl = NULL;  

	memset(&brInfo,0,sizeof(BROWSEINFO));
	brInfo.hwndOwner = hWnd; 
	brInfo.pidlRoot  = lpIl;
	brInfo.pszDisplayName = lpszDir;
	brInfo.lpszTitle = lpszTitle; 
	brInfo.ulFlags = 0;  
	
	if((lpIl=SHBrowseForFolder(&brInfo)) == NULL) return FALSE;
    SHGetPathFromIDList(lpIl, lpszDir);   

	return TRUE;
}



//-------------------------------------------------------------------
// ロードファイル選択ダイアログ表示関数
BOOL SelectLoadFile(HWND hWnd ,LPSTR lpszFileName,
					 LPSTR lpstrFilter,
					 LPSTR lpstrDefExt,
					 LPSTR lpstrTitle
					 )
{
    OPENFILENAME ofn; 
	CHAR szCurDir[MAX_PATH]; 
    CHAR fPath[CHR_BUF],fDrive[CHR_BUF],fName[CHR_BUF],fExt[CHR_BUF];
	CHAR szFileName[MAX_PATH];
	
	
	// フォルダセット
	// パスが通ってないか 
	// lpszFileName が空白なら カレントディレクトリになる
	if(ExistDirectory(lpszFileName)){
		_splitpath(lpszFileName,fDrive,fPath,fName,fExt);
		wsprintf(szFileName,"%s%s",fName,fExt);  
		wsprintf(szCurDir,"%s%s",fDrive,fPath); 
	}
	else {
		wsprintf(szFileName,""); 
		wsprintf(szCurDir,""); 
	}
	
	// 設定
	memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.nMaxFile = MAX_PATH;
    ofn.nMaxFileTitle = CHR_BUF;
    ofn.lpstrFileTitle = NULL;
	ofn.lpstrInitialDir = szCurDir;
	ofn.lpstrFilter = lpstrFilter;
    ofn.lpstrFile = szFileName;
    ofn.Flags = OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = lpstrDefExt;
    ofn.lpstrTitle = lpstrTitle;
	
	if (GetOpenFileName(&ofn) == 0) return FALSE;
	
	wsprintf(lpszFileName,"%s",szFileName);
	
    return TRUE; 
}


//-------------------------------------------------------------------
// 保存ファイル選択ダイアログ表示関数
BOOL SelectSaveFile(HWND hWnd ,LPSTR lpszFileName,
					 LPSTR lpstrFilter,
					 LPSTR lpstrDefExt,
					 LPSTR lpstrTitle)
{
    OPENFILENAME ofn; 
	CHAR szCurDir[MAX_PATH]; 
    CHAR fPath[CHR_BUF],fDrive[CHR_BUF],fName[CHR_BUF],fExt[CHR_BUF];
	CHAR szFileName[MAX_PATH];
	
	// フォルダセット
	// パスが通ってないか 
	// lpszFileName が空白なら カレントディレクトリになる
	if(ExistDirectory(lpszFileName)){
		_splitpath(lpszFileName,fDrive,fPath,fName,fExt);
		wsprintf(szFileName,"%s%s",fName,fExt);  
		wsprintf(szCurDir,"%s%s",fDrive,fPath); 
	}
	else {
		wsprintf(szFileName,""); 
		wsprintf(szCurDir,""); 
	}
	
	// 設定
	memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.nMaxFile = MAX_PATH;
    ofn.nMaxFileTitle = CHR_BUF;
    ofn.lpstrFileTitle = NULL;
	ofn.lpstrInitialDir = szCurDir;
	ofn.lpstrFilter = lpstrFilter;
    ofn.lpstrFile = szFileName;
    ofn.Flags = OFN_SHAREAWARE|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOREADONLYRETURN|OFN_HIDEREADONLY;
    ofn.lpstrDefExt = lpstrDefExt;
    ofn.lpstrTitle = lpstrTitle;
	
	if (GetSaveFileName(&ofn) == 0) return FALSE;
	
	wsprintf(lpszFileName,"%s",szFileName);
	
    return TRUE; 
}


//-------------------------------------------------------------------
// スクロールバーセット
LONG SetScrBarInfo(HWND hScrWnd,LPSCROLLINFO lpScrInfo,WPARAM wp){
	
	// 現在位置セット
	SCROLLINFO curScrInfo;
	LONG nMove = (lpScrInfo->nMax - lpScrInfo->nMin)/50;
	LONG nPos = lpScrInfo->nPos;
	
	switch (LOWORD(wp)) {
		
	case SB_LINEUP:   
		nPos -= 1;	
		break;
		
	case SB_LINEDOWN:	
		nPos += 1;	   
		break;
		
	case SB_PAGEUP:   
		nPos -= nMove;
		break;
		
	case SB_PAGEDOWN: 
		nPos += nMove;
		break;
		
	case SB_THUMBPOSITION:	 
	case SB_THUMBTRACK: 
		curScrInfo.cbSize = sizeof(SCROLLINFO);
		curScrInfo.fMask = SIF_TRACKPOS;
		GetScrollInfo(hScrWnd,SB_CTL,&curScrInfo);
		nPos = curScrInfo.nTrackPos;	
		break;
		
	}
	
	lpScrInfo->nPos = max(lpScrInfo->nMin,min(nPos,lpScrInfo->nMax));
	SetScrollInfo(hScrWnd,SB_CTL,lpScrInfo,TRUE);
	
	return(lpScrInfo->nPos);
}



//-------------------------------------------------------------------
// double -> CHAR 変換関数  2000/3/11
LPCSTR myfcvt(double val,
			  int count // 少数点以下の桁数
			  ){
	
	static char szOutStr[CHR_BUF];
	char szStr[CHR_BUF];
	char* lpszStr;
	int dec,sign,ii;
	
	lpszStr = _fcvt(val,count,&dec,&sign);
	
	if(dec>CHR_BUF){
		wsprintf(szOutStr,"0");
		return (LPCSTR)szOutStr;
	}
	
	// 整数部コピー
	ii=0;
	if(dec>0){
		for(ii=0;ii < dec;ii++) szStr[ii] = lpszStr[ii];
	}
	else {
		wsprintf(szStr,"0");
		ii++;
	}
	
	szStr[ii++] = '.';
	
	// 0 挿入
	while(dec < 0 && count > 0) {szStr[ii++] = '0'; dec++; count--;};
	
	// 小数部コピー
	wsprintf(szStr+ii,"%s",lpszStr+dec);
	
	//符号
	if(sign) wsprintf(szOutStr,"-%s",szStr);
	else wsprintf(szOutStr,"%s",szStr);
	
	return (LPCSTR)szOutStr;
}



//--------------------------------------------------------
// コマンドライン起動関数
BOOL ExecCommand(LPSTR lpApplicationName,
				 LPSTR lpszCommandLine,
				 PROCESS_INFORMATION* pProInfo,
				 BOOL bCreateNewProcess, // 新しいプロセスグループを作るか
				 BOOL bInheritHandles, // ハンドル継承するか
				 BOOL bShowConsole	// コンソール表示するか
				 ){
	
	BOOL bReturn;
	STARTUPINFO startInfo;
	PROCESS_INFORMATION proInfo;
	PROCESS_INFORMATION* lpProInfo;
	DWORD  dwCreation =0;
	
	if(pProInfo == NULL) lpProInfo = &proInfo;
	else lpProInfo = pProInfo;
	
	if(bCreateNewProcess) dwCreation = CREATE_NEW_PROCESS_GROUP;
	else dwCreation = 0;

	memset(&startInfo,0,sizeof(STARTUPINFO));
	startInfo.cb = sizeof(STARTUPINFO);

	if(bShowConsole){
		// コンソール割り当て
		FreeConsole();
		Sleep(100); 
		AllocConsole();
		Sleep(600); // NT の場合ここに Sleep 入れないと重くなる
		
		// コンソールの X ボタンを無効にする
		HWND hConsoleWnd;
		hConsoleWnd = GetConsoleWindowHandle();
		DeleteMenu(GetSystemMenu(hConsoleWnd,FALSE),SC_CLOSE, MF_BYCOMMAND);
		
		//Ctrl+C を無効にする
		DWORD dwMode;
		GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),&dwMode);
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),dwMode&~ENABLE_PROCESSED_INPUT);
		
	}
	else{ // コンソール非表示
		startInfo.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
		startInfo.wShowWindow = SW_HIDE;
		dwCreation = CREATE_NEW_PROCESS_GROUP;
	}

	bReturn = CreateProcess(lpApplicationName,lpszCommandLine,NULL,NULL,bInheritHandles,
		dwCreation,NULL,NULL,&startInfo,lpProInfo);
	
	return bReturn;
}





//-------------------------------------------------
// ファイルマッピング作ってデータ書き込み
BOOL WriteToFileMapping(LPHANDLE lphFileMap, // File Mapping Object のハンドル(戻り値)
							LPSTR lpszWriteData,   // 書き込むデータ
							DWORD dwSize, // データサイズ
							LPSTR lpszErr // エラーメッセージ
							){
	
	HANDLE hMapAddress = NULL; // Mapped View のアドレス
	SECURITY_ATTRIBUTES secAtt;
	
	// 名前無し file-mapping object の作成
	secAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
	secAtt.lpSecurityDescriptor = NULL;
	secAtt.bInheritHandle = TRUE; // ハンドル継承
	if((*lphFileMap = CreateFileMapping((HANDLE)0xFFFFFFFF,&secAtt,PAGE_READWRITE,0,dwSize,NULL))==NULL){
		if(lpszErr) wsprintf(lpszErr,"CreateFileMapping に失敗しました\n");
		goto L_ERR;
	}
	
	// file-mapping object のアドレス空間へのマッピング
	if((hMapAddress = MapViewOfFileEx(*lphFileMap,FILE_MAP_ALL_ACCESS,0,0,0,NULL))==NULL){
		if(lpszErr) wsprintf(lpszErr,"MapViewOfFileEx に失敗しました\n");
		goto L_ERR;
	}
	
	// データ書き込み
	strcpy((LPSTR)hMapAddress,lpszWriteData);
	UnmapViewOfFile(hMapAddress);
	
	return TRUE;
	
L_ERR:
	
	if(*lphFileMap) CloseHandle(*lphFileMap);
	*lphFileMap = NULL;

	return FALSE;
}



//---------------------------------------------------------------------
// 他のプロセスを起動して 名前無し File Mapping Object でデータを渡す関数
// 引数の最後に File Mapping Object のハンドルを付ける
BOOL ExecCommandFileMapping(LPHANDLE lphFileMap, // File Mapping Object のハンドル
							LPSTR lpApplicationName , // アプリ名
							LPSTR lpszCommandLine,  // コマンドライン
							LPSTR lpszWriteData,   // 書き込むデータ
							DWORD dwSize, // File Mapping Object のサイズ
							PROCESS_INFORMATION* pProInfo, // プロセスデータ
							HINSTANCE hInst,
							BOOL bShowConsole,
							LPSTR lpszErr // エラーメッセージ
							){
	
	CHAR szCommand[CHR_BUF];

	// データ書き込み
	if(!WriteToFileMapping(lphFileMap,lpszWriteData,dwSize,lpszErr)) return FALSE;
	
	// 子プロセス起動(引数の最後に hFileMap のハンドルを付ける)
	wsprintf(szCommand,"%s %d",lpszCommandLine,(DWORD)*lphFileMap);
	if(!ExecCommand(lpApplicationName,szCommand,pProInfo,TRUE,TRUE,bShowConsole)){
		if(lpszErr != NULL) wsprintf(lpszErr,"プロセスの起動に失敗しました\n");
		return FALSE;
	}
	
	return TRUE;
}






//-------------------------------------------------------------------
// ダイアログを中心に移動する関数
VOID SetDlgCenter(HWND hWnd){
	
	RECT parentRt, deskRt,rt;
	HWND hParentWnd,hDeskWnd; 
	LONG x, y;
	
	// 親窓のハンドル取得
	hParentWnd = GetParent(hWnd);
	
	// トップウィンドウのハンドル取得
	hDeskWnd = GetDesktopWindow();
	
	// 親がトップウィンドウの場合
	if(hParentWnd == NULL) hParentWnd = hDeskWnd;
	
	// ダイアログを中心に移動
	GetWindowRect(hParentWnd, &parentRt);
	GetWindowRect(hWnd, &rt);
	x = parentRt.left+((parentRt.right-parentRt.left) - (rt.right-rt.left)) / 2;
	y = parentRt.top+((parentRt.bottom-parentRt.top) - (rt.bottom-rt.top)) / 2;
	
	// 画面からはみ出さないかチェック
	GetWindowRect(hDeskWnd, &deskRt);
	x = max(0,min(x,deskRt.right-(rt.right-rt.left)));
	y = max(0,min(y,deskRt.bottom-(rt.bottom-rt.top)));
	
	SetWindowPos(hWnd,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOREPOSITION|SWP_NOZORDER);
}





//----------------------------------------------------
// 既に起動している e-WC の HWND を得る関数
BOOL CALLBACK SearchEwcCallBack(HWND,LPARAM);

HWND GetPreEwcHWND(){
	
	HWND hResultWnd = NULL;
	
	EnumWindows((WNDENUMPROC)SearchEwcCallBack,(LPARAM)&hResultWnd);
	
	return hResultWnd;
}



//----------------------------------------------------
// EnumWindows のコールバック関数
BOOL CALLBACK SearchEwcCallBack(HWND hWnd,LPARAM lp){
	
	CHAR szTitle[CHR_BUF];
	
	GetWindowText(hWnd,szTitle,CHR_BUF);
	
	if(!strncmp(szTitle,"e-wc",4)){
		*((HWND*)lp) = hWnd;
		return FALSE;
	}
	
	return TRUE;
}





//----------------------------------
// 編集データの初期化関数
VOID SetEditData(LPEWCDATA ewcData,LPEDITSAVEDATA lpEditSaveData){
	
	ewcData->x = 0;
	ewcData->y = 0;
	ewcData->bShiftLock = FALSE;
	ewcData->bShowConsole = TRUE;
	ewcData->uDeviceID = WAVE_MAPPER ;
	
	ZeroMemory(lpEditSaveData,sizeof(EDITSAVEDATA));
	lpEditSaveData->thDynX = 0.5;  
	lpEditSaveData->thDynY = 0.8;  
	lpEditSaveData->maxDyn = 0.9;  
	lpEditSaveData->dwEqLeng = 255;
	lpEditSaveData->normLevel = 0.9;
}



//--------------------------------------------------------
// 設定ファイルから設定を読む関数
VOID ReadIniFile(LPEWCDATA ewcData,LPCSTR szIniFile){
	
	LPEDITSAVEDATA lpEditSaveData = &ewcData->editSaveData;
	DWORD dwByte;
	HANDLE hdFile;
	int i2;
	BYTE checksum,foo;
	DWORD checksum2,foo2;
	char szHead[CHR_BUF];
	
	
	// ファイルオープン
	hdFile = CreateFile(szIniFile,GENERIC_READ
		,0, 0, OPEN_EXISTING,FILE_ATTRIBUTE_READONLY, NULL); 
	
	if(hdFile == INVALID_HANDLE_VALUE){
		SetEditData(ewcData,lpEditSaveData);
		SaveIniFile(ewcData,szIniFile);
		return;
	}
	
	SetFilePointer(hdFile, 0, 0, FILE_BEGIN);
	
	// ヘッダチェック
	ReadFile(hdFile,szHead,7,&dwByte,NULL);
	szHead[7] = '\0';
	if(strcmp(szHead,"EWCDATA")!=0)
	{
		CloseHandle(hdFile);
		MyMessageBox(NULL, "設定ファイルが破損しています。新しい設定ファイルを作成します。"
			, "Error", MB_OK|MB_ICONERROR|MB_SETFOREGROUND);	
		SetEditData(ewcData,lpEditSaveData);
		SaveIniFile(ewcData,szIniFile);
		return;
	}
	
	// 読み込み
	ReadFile(hdFile,&ewcData->x,sizeof(int),&dwByte,NULL);
	ReadFile(hdFile,&ewcData->y,sizeof(int),&dwByte,NULL);
	ReadFile(hdFile,&ewcData->bShiftLock,sizeof(BOOL),&dwByte,NULL);
	ReadFile(hdFile,&ewcData->bShowConsole,sizeof(BOOL),&dwByte,NULL);
	ReadFile(hdFile,&ewcData->uDeviceID,sizeof(UINT),&dwByte,NULL);
		
	ReadFile(hdFile,lpEditSaveData,sizeof(EDITSAVEDATA),&dwByte,NULL);
	
	// チェックサム計算
	checksum = 0;
	checksum2 = 0;
	for(i2=0;i2<sizeof(EDITSAVEDATA);i2++){
		foo = *((LPBYTE)(lpEditSaveData)+i2);
		checksum ^= foo;
		checksum2 += (DWORD)foo;
	}
	
	ReadFile(hdFile,&foo,sizeof(BYTE),&dwByte,NULL);
	ReadFile(hdFile,&foo2,sizeof(DWORD),&dwByte,NULL);
	
	if(foo != checksum || foo2 != checksum2){
		CloseHandle(hdFile);
		MyMessageBox(NULL, "設定ファイルが破損しています。新しい設定ファイルを作成します。"
			, "Error", MB_OK|MB_ICONERROR|MB_SETFOREGROUND);	
		SetEditData(ewcData,lpEditSaveData);
		SaveIniFile(ewcData,szIniFile);
		return;
	}
	
	
	CloseHandle(hdFile);
}



//--------------------------------------------------------
// 設定ファイルに保存する関数
VOID SaveIniFile(LPEWCDATA ewcData,LPCSTR szIniFile){
	
	LPEDITSAVEDATA lpEditSaveData = &ewcData->editSaveData;
	DWORD dwByte;
	HANDLE hdFile;
	int i2;
	BYTE checksum,foo;
	DWORD checksum2;
	
	// オープン
	if((hdFile = CreateFile(szIniFile,GENERIC_WRITE, 
		0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL))==INVALID_HANDLE_VALUE){
		MyMessageBox(NULL, "設定ファイルを開く事ができません。","Error", MB_OK|MB_ICONERROR|MB_SETFOREGROUND);	
		return;
	}
	SetFilePointer(hdFile, 0, 0, FILE_BEGIN);
	
	// ヘッダ
	WriteFile(hdFile,"EWCDATA",7,&dwByte,NULL);
	
	// 設定書き込み
	WriteFile(hdFile,&ewcData->x,sizeof(int),&dwByte,NULL);
	WriteFile(hdFile,&ewcData->y,sizeof(int),&dwByte,NULL);
	WriteFile(hdFile,&ewcData->bShiftLock,sizeof(BOOL),&dwByte,NULL);
	WriteFile(hdFile,&ewcData->bShowConsole,sizeof(BOOL),&dwByte,NULL);
	WriteFile(hdFile,&ewcData->uDeviceID,sizeof(UINT),&dwByte,NULL);
	
	WriteFile(hdFile,lpEditSaveData,sizeof(EDITSAVEDATA),&dwByte,NULL);
	
	// チェックサム計算
	checksum = 0;
	checksum2 = 0;
	for(i2=0;i2<sizeof(EDITSAVEDATA);i2++){
		foo = *((LPBYTE)(lpEditSaveData)+i2);
		checksum ^= foo;
		checksum2 += (DWORD)foo;
	}
	WriteFile(hdFile,&checksum,sizeof(BYTE),&dwByte,NULL);
	WriteFile(hdFile,&checksum2,sizeof(DWORD),&dwByte,NULL);
	
	CloseHandle(hdFile);
	
}



//EOF
