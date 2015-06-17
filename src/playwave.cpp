//-----------------------
// WAVE 再生関数   2002/11/24 日版
//
// (c) 1998-2015 Tomoya Tokairin
//
// 本プログラムのすべて、または一部を GPL に従って再頒布または変更する
// ことができます。詳細についてはGNU 一般公有使用許諾書をお読みください。
//

// ロック音、EWC 共通

// PlayWave() で再生開始 
// ファイルハンドルが NULL なら関数内でファイルを開くので
// waveFmt,データまでのオフセット,データサイズ 等のパラメータは無視する
//
// バッファの切れ目ごとに MM_WOM_DONE を hWnd に送る
// WPARAM は レベルピーク値 、 LPARAM は現在時刻(ミリ秒)
// SeekPlayWave(シーク位置(ミリ秒))でシーク
// StopPlayWave() で停止して MM_WOM_CLOSE を hWnd に送る。
// 正常終了 LPARAM =0 異常 = 1

#include <windows.h>
#include <mmsystem.h>

#define USE_EWC  // ewc.exe で使う場合
#define PLAYWAVEHDR_BUFNUM 4  // WAVE ヘッダのバッファ数

// ステータス
#define ID_THREADON   0
#define ID_THREADSTOP 1
#define ID_PREPARE 2
#define ID_CLOSEWAVE 3
#define ID_SEEKING 4

// 共通の関数(wavefunc.cpp 内)
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


// メッセージボックス
int MyMessageBox(HWND,LPSTR,LPSTR,UINT);


// スレッドに渡すデータ型
typedef struct{
	HWND hWnd; // 親のハンドル
	UINT uDeviceID; // デバイス ID
	CHAR szWaveFileName[MAX_PATH]; // ファイル名
	DWORD dwStartTime; // 再生開始位置(ミリ秒)
	LONGLONG n64StartByte; // 再生位置(バイト)
	BOOL bStartByte; // n64StartByte で位置を指定する
	HANDLE hdFile; // 再生ファイルのハンドル
	WAVEFORMATEX waveFmt; // ウェーブフォーマット
	LONGLONG n64WaveDataSize;// ファイルのサイズ
	LONGLONG n64WaveOffset; // データ部分までのオフセット 
}PLAYDATA,*LPPLAYDATA;


// プロシージャに渡すデータ型
typedef struct
{
	HWND hWnd;
	LPWAVEFORMATEX lpWaveFmt; // wave フォーマット
	LPWAVEFORMATEX lpWaveFmtOut; // wave フォーマット(出力の)
	DWORD dwTime; // 再生時間
}PLAYPROCDATA,*LPPLAYPROCDATA;


// グローバル変数
HANDLE HdPlayThread = NULL; // スレッドのハンドル
WORD PlayStatus; // 現在のステータス
DWORD SeekPos;  // シーク位置(ミリ秒)



//-------------------------------------------------------------------
// コールバック関数
VOID CALLBACK MyWaveOutProc(HWAVEOUT hWaveOut,UINT msg,DWORD inst,DWORD dwP1,DWORD dwP2)
{
	
	HWND hWnd = ((LPPLAYPROCDATA)inst)->hWnd;
	LPWAVEFORMATEX lpWaveFmt = ((LPPLAYPROCDATA)inst)->lpWaveFmt;
	LPWAVEFORMATEX lpWaveFmtOut = ((LPPLAYPROCDATA)inst)->lpWaveFmtOut;
	
	LPWAVEHDR lpWaveHdr; 
	static double dPeak[PLAYWAVEHDR_BUFNUM][2];  // 親に WPARAM で渡すデータ(最大ピーク値)
	static DWORD dwHdrNum = 0;
	DWORD i,dwTime;
	
	switch (msg)
	{
		
	case WOM_OPEN: // WAVE デバイスオープン完了

		for(i=0;i<PLAYWAVEHDR_BUFNUM;i++)
		{
			dPeak[i][0] = dPeak[i][1] = 0;
		}
		
		PlayStatus = ID_THREADON;
		
		break;
		
		//-------------------------------------------------------------------
		
	case WOM_DONE: // 1 バッファ分の再生終了
		
		if(PlayStatus == ID_THREADON){
			
			// ヘッダのアドレス取得
			lpWaveHdr = (LPWAVEHDR)dwP1;

			// 再生時間取得
			dwTime = lpWaveHdr->dwUser;

#ifndef USE_EWC			
			// 親に渡すデータセット
			WaveLevelMaxPeak(dPeak[dwHdrNum],
				(LPBYTE)lpWaveHdr->lpData,lpWaveHdr->dwBufferLength,
				*lpWaveFmtOut);
#endif

			// 親に MM_WOM_DONE 送信 
			if(dwTime != -1)
				SendMessage(hWnd,MM_WOM_DONE,(WPARAM)dPeak[dwHdrNum],(LPARAM)dwTime);
			else
				PlayStatus = ID_THREADSTOP;  // dwTime = -1 なら停止
				
			// このバッファを使用化に
			lpWaveHdr->dwUser = 0;
			dwHdrNum = (dwHdrNum+1)&(PLAYWAVEHDR_BUFNUM-1);
		}
		
		break;
		
		//-------------------------------------------------------------------
		
	case WOM_CLOSE: // WAVE デバイスクローズ
		
		PlayStatus = ID_CLOSEWAVE;
		
		break;
		
	default:
        break;
    }
}



//-------------------------------------------------------------------
// シーク関数
VOID SeekPlayWave(DWORD dwTime)
{
	if(HdPlayThread != NULL && PlayStatus == ID_THREADON) {
		SeekPos = dwTime;  // ポジションセット
		PlayStatus = ID_SEEKING; // シーク開始
	}
}



//-------------------------------------------------------------------
// 停止関数
VOID StopPlayWave()
{
	LONG i;
	
	if(HdPlayThread != NULL && PlayStatus == ID_THREADON) {
		
		PlayStatus = ID_THREADSTOP;  // スレッドのループ停止
		
		// クローズするまで待つ
		i=0;
		while(PlayStatus != ID_CLOSEWAVE && i<50){
			Sleep(50);
			i++;
		}
	}
	
}


//-------------------------------------------------------------------
// 再生スレッド
DWORD WINAPI PlayWaveThread(LPVOID lpPlayData)
{
	
	// 変数セット
	HWND hWnd = ((LPPLAYDATA)lpPlayData)->hWnd;
	UINT uDeviceID = ((LPPLAYDATA)lpPlayData)->uDeviceID;
	LPSTR lpszFileName = ((LPPLAYDATA)lpPlayData)->szWaveFileName;
	DWORD dwStartTime =  ((LPPLAYDATA)lpPlayData)->dwStartTime; // 再生開始位置(ミリ秒)
	HANDLE hdFile = ((LPPLAYDATA)lpPlayData)->hdFile; // 再生ファイルのハンドル
	WAVEFORMATEX waveFmt = ((LPPLAYDATA)lpPlayData)->waveFmt; // wave フォーマット
	LONGLONG n64WaveDataSize = ((LPPLAYDATA)lpPlayData)->n64WaveDataSize; // データ部分までのオフセット 
	LONGLONG n64WaveOffset = ((LPPLAYDATA)lpPlayData)->n64WaveOffset;  // ファイルのサイズ


	HWAVEOUT hWaveOut;  // 再生デバイスのハンドル
	static WAVEHDR waveHdr[PLAYWAVEHDR_BUFNUM]; // wave ヘッダのバッファ
	PLAYPROCDATA procData; // プロシージャに渡すデータ
	DWORD dwWaveBlockTime; // バッファの記録時間
	DWORD dwWaveBlockByte; //バッファのバイト数
	DWORD dwByte; // 読み込みバイト数
	LONGLONG n64CurDataSize;  // 現在の演奏位置
	WORD wCurWaveHdr;  // 現在使用している wave ヘッダの番号
	BOOL bCloseFile = FALSE; // ファイルをクローズするか
	LONG i; // 雑用
	LARGE_INTEGER LI; // SetFilePointer 用 
	char szStr[256];
	
	double* lpdBuffer[2] = {NULL,NULL};  // bit change 用
	WAVEFORMATEX waveFmtOut; // bit change 用

	// hdFile が NULL ならこのスレッド内でファイルの開け閉めをする。
	if(hdFile == NULL)
	{
		bCloseFile = TRUE;

		// ファイルあるかテスト
		hdFile = CreateFile(lpszFileName,GENERIC_READ, 
			0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL); 
		if(hdFile == INVALID_HANDLE_VALUE){
			MyMessageBox(hWnd, "ファイルを開けません。\n他のプロセスがファイルを開いている可能性があります。", 
				"Error", MB_OK|MB_SETFOREGROUND|MB_ICONERROR);	
			goto ERROR_LV0;	
		}
		CloseHandle(hdFile);
		
		// フォーマットセット
		if(GetWaveFormat(lpszFileName,&waveFmt,&n64WaveDataSize,&n64WaveOffset,szStr)==FALSE) goto ERROR_LV0;
		
		// 改めてファイルオープン
		hdFile = CreateFile(lpszFileName,GENERIC_READ, 
			0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL); 
		if(hdFile == INVALID_HANDLE_VALUE){
			MyMessageBox(hWnd, "ファイルを開けません。", 
				"Error", MB_OK|MB_SETFOREGROUND|MB_ICONERROR);	
			goto ERROR_LV0;	
		}
	}
	
	// バッファ 1 ブロックのサイズ計算(約 0.2 秒分)
	dwWaveBlockByte = (DWORD)(0.2*waveFmt.nSamplesPerSec)*waveFmt.nBlockAlign;

	// 16 bit に変換
	waveFmtOut = waveFmt;
	if(waveFmt.wBitsPerSample > 16)
	{
		SetWaveFmt(&waveFmtOut,waveFmt.nChannels,waveFmt.nSamplesPerSec,16,1);
		for(i=0;i<waveFmt.nChannels;i++) 
			lpdBuffer[i] = (double*)malloc(sizeof(double)*dwWaveBlockByte/waveFmt.nBlockAlign);
	}

	// バッファ 1 ブロックの再生時間(ミリ秒)
	dwWaveBlockTime =	(DWORD)((double)dwWaveBlockByte/(double)waveFmt.nAvgBytesPerSec*1000);
	
	// バッファメモリを確保
	if(SetWaveHdr(hWnd,waveHdr,PLAYWAVEHDR_BUFNUM,dwWaveBlockByte) == FALSE) goto ERROR_LV1;
	
	// プロシージャに渡すデータをセット
	procData.hWnd = hWnd;
	procData.lpWaveFmt = &waveFmt;
	procData.lpWaveFmtOut = &waveFmtOut;

	// デバイスオープン
	PlayStatus = ID_PREPARE;
	if(waveOutOpen(&hWaveOut, uDeviceID,
		&waveFmtOut, (DWORD)MyWaveOutProc, (DWORD)&procData, CALLBACK_FUNCTION )!=MMSYSERR_NOERROR ){
		
		MyMessageBox(hWnd, "再生デバイスのオープンに失敗しました。\n他のプロセスが WAVE デバイスを使用している可能性があります。"
			, "Error", MB_OK|MB_SETFOREGROUND|MB_ICONERROR);
		
		goto ERROR_LV2;
	} 
	
	// オープンするまで停止
	i=0;
	while(PlayStatus != ID_THREADON && i<50)
	{
		Sleep(50);
		i++; 
	}
	
	// 初期ポジションセット
	if(((LPPLAYDATA)lpPlayData)->bStartByte)
		n64CurDataSize = ((LPPLAYDATA)lpPlayData)->n64StartByte;
	else
		n64CurDataSize = waveFmt.nBlockAlign*(((LONGLONG)dwStartTime*waveFmt.nSamplesPerSec)/1000);
	if(n64CurDataSize >= n64WaveDataSize) PlayStatus = ID_THREADSTOP;  
	
	// ファイルポインタ移動
	LI.QuadPart = n64WaveOffset + n64CurDataSize;
	SetFilePointer(hdFile,LI.LowPart, &LI.HighPart,FILE_BEGIN);
	wCurWaveHdr = 0;  

	
	// バッファの準備
	for(i=0;i<PLAYWAVEHDR_BUFNUM;i++)
	{
		waveOutPrepareHeader(hWaveOut,&waveHdr[i],sizeof(WAVEHDR));
		waveHdr[wCurWaveHdr].dwUser = 0;
	}
	
	// --------------------
	// ループ開始
	while(PlayStatus == ID_THREADON || PlayStatus == ID_SEEKING){
		
		// シーク
		if(PlayStatus == ID_SEEKING)
		{ 
			// 再生停止
			waveOutReset(hWaveOut);
			
			// ファイルポインタ再セット
			n64CurDataSize = waveFmt.nBlockAlign
				*(LONGLONG)((double)SeekPos*(double)waveFmt.nSamplesPerSec/1000);
			
			LI.QuadPart = n64WaveOffset + n64CurDataSize;
			SetFilePointer(hdFile,LI.LowPart, &LI.HighPart,FILE_BEGIN);
			
			// バッファ使用化に
			for(i=0;i<PLAYWAVEHDR_BUFNUM;i++) waveHdr[i].dwUser = 0;
			wCurWaveHdr = 0;
			
			// スレッド再開
			PlayStatus = ID_THREADON;
		}
		else if(waveHdr[wCurWaveHdr].dwUser == 0)
		{ // バッファ使用化

			// データ読み込み
			LI.QuadPart = n64WaveOffset + n64CurDataSize;
			SetFilePointer(hdFile,LI.LowPart, &LI.HighPart,FILE_BEGIN);
			ReadFile(hdFile,waveHdr[wCurWaveHdr].lpData,dwWaveBlockByte, &dwByte, NULL);
			
			if(dwByte)
			{
				//デバイスにデータを送る
				if(n64CurDataSize + dwByte > n64WaveDataSize) 
					dwByte = (DWORD)(n64WaveDataSize - n64CurDataSize);
				n64CurDataSize += dwByte;
				if(n64CurDataSize != n64WaveDataSize)
					waveHdr[wCurWaveHdr].dwUser  // ミリ秒
					= 1+(DWORD)((n64CurDataSize*1000)/waveFmt.nAvgBytesPerSec);
				else waveHdr[wCurWaveHdr].dwUser = -1; // -1 を送ると停止
				waveHdr[wCurWaveHdr].dwBufferLength = dwByte;
				
				if(waveFmt.wBitsPerSample > 16)
					waveHdr[wCurWaveHdr].dwBufferLength 
					= ChangeBits((LPBYTE)waveHdr[wCurWaveHdr].lpData,
					waveHdr[wCurWaveHdr].dwBufferLength,waveFmt,waveFmtOut,lpdBuffer);
				waveOutWrite(hWaveOut,&waveHdr[wCurWaveHdr],sizeof(WAVEHDR));

				// 次のバッファへ
				wCurWaveHdr = (wCurWaveHdr+1)&(PLAYWAVEHDR_BUFNUM-1);
			}
			else
			{
				// 全てのヘッダが再生済みなら終了
				for(i=0;i<PLAYWAVEHDR_BUFNUM;i++) if(waveHdr[i].dwUser) break;
				if(i>=PLAYWAVEHDR_BUFNUM) PlayStatus = ID_THREADSTOP;  
			}
		}
		else Sleep(dwWaveBlockTime/16);

	}
	// ループ終わり
	// --------------------

	
	// クローズ処理
	
	// 再生停止
	waveOutReset(hWaveOut);
	
	// バッファの後始末
	for(i=0;i<PLAYWAVEHDR_BUFNUM;i++)
		waveOutUnprepareHeader(hWaveOut,&waveHdr[i],sizeof(WAVEHDR));
	
	// デバイスクローズ
	if(waveOutClose(hWaveOut)!=MMSYSERR_NOERROR) {
		MyMessageBox(hWnd, "再生デバイスを閉じることができません。", 
			"Error", MB_OK|MB_SETFOREGROUND|MB_ICONERROR);	
		
		goto ERROR_LV2;
	}
	
	// クローズするまで停止
	i=0;
	while(PlayStatus != ID_CLOSEWAVE && i<50){
		Sleep(50);
		i++;
	}
	
	// ファイルクローズ
	if(bCloseFile) CloseHandle(hdFile);	 	
	
	// バッファメモリ開放
	DelWaveHdr(hWnd,waveHdr);
	for(i=0;i<waveFmt.nChannels;i++) if(lpdBuffer[i]) free(lpdBuffer[i]);
	
	// 親に MM_WOM_CLOSE を送信(正常終了)
	SendMessage(hWnd,MM_WOM_CLOSE,0,0);
	
	// スレッド停止
	HdPlayThread = NULL;
	
	return 0;
	
	//---- エラー処理 ---
	
ERROR_LV2:
	
	// バッファメモリ開放
	DelWaveHdr(hWnd,waveHdr);
	for(i=0;i<waveFmt.nChannels;i++) if(lpdBuffer[i]) free(lpdBuffer[i]);
	
ERROR_LV1:
	
	// ファイルクローズ
	if(bCloseFile) CloseHandle(hdFile);
	
ERROR_LV0:
	
	// 親に MM_WOM_CLOSE を送信(LPARAM は 1)
	SendMessage(hWnd,MM_WOM_CLOSE,0,1);
	HdPlayThread = NULL;
	return 1;
}



//-------------------------------------------------------------------
// 再生開始関数
BOOL PlayWave(HWND hWnd,
			  UINT uDeviceID,
			  LPSTR lpszFileName,
			  DWORD dwStartTime,
			  HANDLE hdFile,WAVEFORMATEX waveFmt,
			  LONGLONG n64WaveDataSize, // ファイルのサイズ
			  LONGLONG n64WaveOffset  // データ部分までのオフセット 
			  )
{
	static PLAYDATA playData;
	static DWORD dwThreadId;
	
	if(HdPlayThread != NULL) return FALSE; // スレッドが動いていたら戻る
	else {	// 再生開始
		
		// 構造体データセット
		playData.hWnd = hWnd;
		playData.uDeviceID = uDeviceID;
		if(lpszFileName!=NULL) wsprintf(playData.szWaveFileName,"%s",lpszFileName);
		playData.dwStartTime = dwStartTime;
		playData.bStartByte = FALSE;
		playData.hdFile = hdFile;
		playData.waveFmt = waveFmt;
		playData.n64WaveDataSize = n64WaveDataSize;
		playData.n64WaveOffset = n64WaveOffset;
		
		// スレッド起動
		HdPlayThread = CreateThread(NULL,0,
			(LPTHREAD_START_ROUTINE)PlayWaveThread,
			(LPVOID)&playData,
			0,(LPDWORD)&dwThreadId);
		
	}
	
	return TRUE;
}


#ifdef USE_EWC
//-----------------------------------------------------
// バイト指定で再生
BOOL PlayWaveByte(HWND hWnd,
				  UINT uDeviceID,
				  LPSTR lpszFileName,
				  LONGLONG n64StartByte,
				  HANDLE hdFile,
				  WAVEFORMATEX waveFmt,
				  LONGLONG n64WaveDataSize, // ファイルのサイズ
				  LONGLONG n64WaveOffset  // データ部分までのオフセット 
){
	static PLAYDATA playData;
	static DWORD dwThreadId;
	
	if(HdPlayThread != NULL) return FALSE; // スレッドが動いていたら戻る
	else {	// 再生開始
		
		// 構造体データセット
		playData.hWnd = hWnd;
		playData.uDeviceID = uDeviceID;
		if(lpszFileName!=NULL) wsprintf(playData.szWaveFileName,"%s",lpszFileName);
		playData.n64StartByte = n64StartByte;
		playData.bStartByte = TRUE;
		playData.hdFile = hdFile;
		playData.waveFmt = waveFmt;
		playData.n64WaveDataSize = n64WaveDataSize;
		playData.n64WaveOffset = n64WaveOffset;
		
		// スレッド起動
		HdPlayThread = CreateThread(NULL,0,
			(LPTHREAD_START_ROUTINE)PlayWaveThread,
			(LPVOID)&playData,
			0,(LPDWORD)&dwThreadId);
		
	}
	
	return TRUE;
}
#endif

//EOF