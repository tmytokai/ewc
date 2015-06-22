// 無音部分サーチ用関数

#include "common.h"
#include <math.h>

// 無音部サーチダイアログとスレッドに渡すデータ型
typedef struct
{
HWND hWnd;
HWND hProgBar;  // プログレスバーのハンドル
HANDLE hdFile; // サーチするファイルハンドル
LONGLONG n64DataSize; //ファイルサイズ
LONGLONG n64DataOffset; //データまでのオフセット
WAVEFORMATEX waveFmt; // Wave フォーマット
LONGLONG* lpn64StartByte; // 無音部開始位置(バイト)
LONGLONG* lpn64EndByte;  // 終了位置(バイト)
LPWORD lpwStatus; // ステータス
double dBound; // しきい値
DWORD dwMaxCount;  // 無音部検索のカウンタの最大値(1/25 秒単位)
BOOL bAvr; // 平均音量で検索
}SEARCHDATA,*LPSEARCHDATA;


//-------------------------------------------------------------------
// 無音部サーチスレッド
DWORD WINAPI SearchThread(LPVOID lpSearchData) 
{
	
	LPSEARCHDATA lpSdata = (LPSEARCHDATA)lpSearchData;
	HWND hWnd  	// 親ダイアログのウィンドウハンドル
		= lpSdata->hWnd; 
	HWND hProgBar 	// 進行プログレスバーのハンドル  
		= lpSdata->hProgBar; 
	HANDLE hdFile 	// ファイルハンドル
		= lpSdata->hdFile	; 
	LONGLONG n64DataSize 	//データサイズ
		= lpSdata->n64DataSize; 
	WAVEFORMATEX waveFmt // Wave フォーマット
		= lpSdata->waveFmt;
	LONGLONG n64DataOffset 	//データまでのオフセット
		= lpSdata->n64DataOffset; 
	LONGLONG* lpn64StartByte 	// 開始位置(バイト)
		= lpSdata->lpn64StartByte; 
	LONGLONG* lpn64EndByte 	// 終了位置(バイト)
		= lpSdata->lpn64EndByte; 
	LPWORD lpwStatus 	// ステータス
		= lpSdata->lpwStatus; 
	double dBound // dB, しきい値
		= lpSdata->dBound; 
	DWORD dwMaxCount // カウンタの最大値
		= lpSdata->dwMaxCount; 
	
	// ファイル読み込み保存用変数
	DWORD dwByte;
	DWORD dwReadByte; // 読み込むバイト数
	LONGLONG n64CurByte = *lpn64StartByte; //現在位置
	LONGLONG n64StartByte = n64CurByte; // 無音部の開始位置(byte)
	LONGLONG n64EndByte; // 無音部の終了位置(byte)
	DWORD dwMovePoint; // ファイルポインタの移動量
	DWORD dwCount;  // 無音部検索のカウンタ
	LPBYTE lpBuffer = NULL; // コピー用のバッファ
	double dWaveLevel[2]; // 入力レベル計算量
	double dMaxLevel;
	LARGE_INTEGER LI; // SetFilePointer 用 

	BOOL bSucceed = FALSE; // 発見したら TRUE
	
	DWORD i;
	DWORD dwProgPos; // プログレスバーのポジション
	
	// ハンドルが NULL でないかチェック
	if(hdFile == NULL){ 
		
		MyMessageBox(hWnd, "ファイルがオープンされていません。", 
			"Error", MB_OK|MB_ICONERROR);	
		
		// 失敗メッセージ送信
		SendMessage(hWnd,WM_MYENDCOPY,0,1L);
		ExitThread(1L);
	}

	// 開始位置がデータサイズを超えている時
	if(n64CurByte > n64DataSize)
	{
		n64CurByte = n64DataSize;
		n64StartByte = n64CurByte;
		n64EndByte = n64StartByte;
		goto L_EXIT;
	}
	
	// 進行プログレスバー設定	 
	dwProgPos = ((DWORD)(n64CurByte/SAVEBUFSIZE));
	SendMessage(hProgBar,PBM_SETRANGE,0,MAKELPARAM(0,(DWORD)(n64DataSize/SAVEBUFSIZE))); 
	SendMessage(hProgBar,PBM_SETSTEP,(WPARAM)1,0);
	SendMessage(hProgBar,PBM_SETPOS,(WPARAM)dwProgPos,0);   

	// レベル調整 (-1 - 1 に正規化)
	dMaxLevel = GetMaxWaveLevel(waveFmt);
	dBound = pow(10.,dBound/20);  // リニア値に戻す
	
	// ポインタの移動量
	// 一秒間に waveFmt.nSamplesPerSec 回も使って無音の判定をしていたら日が暮れるので
	// waveFmt.nSamplesPerSec/S_POINT_PER_SEC 点ごとに判定する
	dwMovePoint = waveFmt.nSamplesPerSec/S_POINT_PER_SEC;
	
	// バッファメモリを確保
	lpBuffer =(LPBYTE)GlobalAlloc(GPTR,sizeof(BYTE)*SAVEBUFSIZE);
	if(lpBuffer == NULL)
	{
		MyMessageBox(hWnd, "メモリの確保に失敗しました。",
			"Error", MB_OK|MB_ICONERROR);
		
		// 失敗メッセージ送信
		SendMessage(hWnd,WM_MYENDCOPY,0,1L);
		return 1;
	}
	
	
	//---------------------------------------
	// サーチ開始
	
	dwCount = 0;
	while(n64CurByte < n64DataSize && *lpwStatus==ID_SEARCHON)
	{
		// プログレスパーアップ
		SendMessage(hProgBar,PBM_SETPOS,(WPARAM)dwProgPos,0);
		dwProgPos++;
		
		// データ読み込みサイズ計算
		dwReadByte = n64CurByte + SAVEBUFSIZE > n64DataSize
			? (DWORD)(n64DataSize - n64CurByte) : SAVEBUFSIZE;
		
		// 読み
		LI.QuadPart = n64DataOffset+n64CurByte; // サーチ開始位置にポインタ移動
		SetFilePointer(hdFile,LI.LowPart, &LI.HighPart,FILE_BEGIN);
		memset(lpBuffer,0,sizeof(BYTE)*SAVEBUFSIZE);
		ReadFile(hdFile,lpBuffer,dwReadByte,&dwByte, NULL);
		
		// バッファ内をサーチ
		for(i = 0;i < dwByte/waveFmt.nBlockAlign ;i+=dwMovePoint)
		{
			if(lpSdata->bAvr)
				//dwMovePoint点の平均音量取得
				WaveLevelAverage(dWaveLevel,lpBuffer+i*waveFmt.nBlockAlign,waveFmt,dwMovePoint*waveFmt.nBlockAlign);
			else
				// 出力値取得
				WaveLevel(dWaveLevel,lpBuffer+i*waveFmt.nBlockAlign,waveFmt);

			//正規化
			dWaveLevel[0] /= dMaxLevel;
			dWaveLevel[1] /= dMaxLevel;
			
			// 無音
			if(fabs(dWaveLevel[0]) <= dBound && fabs(dWaveLevel[1]) <= dBound) dwCount++;
			else { // 有音部が現れた
				
				if(dwCount >= dwMaxCount) {	// 検索完了
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
	
	// メモリ開放
	if(lpBuffer) GlobalFree(lpBuffer);
	
	// 位置セット
	*lpn64StartByte = n64StartByte;
	*lpn64EndByte = n64EndByte;

	// 結果送信
	SendMessage(hWnd,WM_MYENDCOPY,0,bSucceed);
	
	return(0);
}



//-------------------------------------
// 無音部サーチダイアログのプロシージャ
// スレッドを動かしてるだけ。
LRESULT CALLBACK SearchDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	
	static HANDLE hThread;  // スレッドのハンドル
	static DWORD threadId; // スレッド ID
	static LPSEARCHDATA lpSearchData; // スレッドに渡すデータタイプ
	static WORD wStatus; // ステータス
	
	switch (msg) {
		
	case WM_INITDIALOG:  // ダイアログ初期化
		
		// ダイアログを中心に移動
		SetDlgCenter(hWnd);
		
		// 保存スレッド起動
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
			MyMessageBox(hWnd, "スレッドの起動に失敗しました。", 
				"Error", MB_OK|MB_ICONERROR);
			EndDialog(hWnd, IDCANCEL); 
		}
		
		break;
		
	case WM_MYENDCOPY: // サーチ完了
		
		if((DWORD)lp == FALSE) EndDialog(hWnd, IDCANCEL); // 失敗
		else EndDialog(hWnd, IDOK);  // 成功
		
		break;
		
	case WM_COMMAND:
		
		switch (LOWORD(wp)) {
			
		case IDCANCEL: // 停止
			
			wStatus = ID_SEARCHOFF;
			return TRUE;
			
		}
		
		break;
		
		default: return FALSE;
	}
	
	return TRUE;
}




//-------------------------------------------
// 無音部サーチ開始関数
// 実際はダイアログを開くだけ。
BOOL SearchNoSound(HWND hWnd,
				   HINSTANCE hInst,
				   HANDLE hdFile, // サーチするファイルハンドル
				   LONGLONG n64DataSize, //ファイルサイズ
				   LONGLONG n64DataOffset, //データまでのオフセット
				   WAVEFORMATEX waveFmt, // Wave フォーマット
				   LONGLONG* lpn64StartByte, // 開始位置(バイト)
				   LONGLONG* lpn64EndByte, // 終了位置(バイト)
				   double dBound, // しきい値
				   DWORD dwCount,  // 無音部検索のカウンタの最大値
				   BOOL bAvr	// 平均音量値を使用
				   ){
	
	SEARCHDATA searchData;
	
	// ダイアログに渡すデータセット
	searchData.hdFile = hdFile;
	searchData.n64DataSize = n64DataSize;
	searchData.waveFmt = waveFmt;
	searchData.n64DataOffset = n64DataOffset;
	searchData.lpn64StartByte = lpn64StartByte;
	searchData.lpn64EndByte = lpn64EndByte;
	searchData.bAvr = bAvr;
	
	searchData.dBound = dBound;
	searchData.dwMaxCount = dwCount;
	
	// ダイアログボックス表示と保存開始
	if(DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_SEARCH)
		,hWnd,(DLGPROC)SearchDlgProc,
		(LPARAM)&searchData) == IDCANCEL) return FALSE;
	
	return TRUE;
}


//EOF
