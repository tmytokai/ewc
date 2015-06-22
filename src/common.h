// e-WC 共通ヘッダファイル

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h> // BROWSEINFO 構造体
#include <string.h>
#include <mmsystem.h>
#include <stdio.h>
#include "resource.h"

#define CHR_BUF 256 // 文字バッファ数
#define MAX_ARGC 10 // コマンドライン引数の最大値
#define MAX_WAVFLTOPTBUF 4096 // WAVEFLT に送るオプションデータのバッファ数
#define NOSND_MAXLEVEL	1024	// nosound の最大閾値

// ボタンの状態
#define MYID_RELEASEBUTTON              0  // リリース
#define MYID_PUSHBUTTON                 1  // 押してる

// ステータス
#define ID_STATREADY 0   // レディ
#define ID_STATPLAY 1    // 再生中
#define ID_STATCLOSE 4   // クローズ状態


// サウンドデバイスの最大数
#define MAXDEVICENUMBER 20  

// 自作メッセージ
#define WM_MYENDCOPY WM_APP+100  // ファイルのコピーが終了

// 描画周り
#define FRAMESIZE 512 // 波形描画フレームのサイズ
#define EDITUPDATERECTTOP 70 // エディタの再描画領域の y 座標
#define EDITUPDATERECTLEFT 12 // エディタの再描画領域の x 座標
#define EDITSTATUSMARGIN1 10 // エディタのステータス描画領域と再描画領域とのマージン
#define EDITSTATUSMARGIN2 10 // エディタのステータス描画領域と波形ウィンドウとのマージン
#define EDITSTATUSLINE 5 // ステータスの行数
#define EDITSTATUSSIZE (EDITSTATUSLINE*20 +2) // ステータス描画領域の高さ
#define EDITSTATUSLEFTMARGIN 235 // ステータスの[REMA〜] から右の x 座標
#define BUTTON_WIDTH	40 // ボタン幅
#define BUTTON_HEIGHT	15 // ボタン高さ
#define BUTTON_MARGIN	8 // ボタンとボタンの間の横のマージン
#define SB_HEIGHT	10 // スクロールバー高さ
#define PLAYBT_WIDTH 28	// 再生ボタンとかの幅
#define PLAYBT_HEIGHT 25	// 再生ボタンとかの高さ
#define FONT_SIZE	14 // ボタンの文字のフォントサイズ

// 無音サーチ、分割マーク関係
#define MINSEARCHBOUND -100 // 無音部サーチ時の閾値の最小値
//#define S_POINT_PER_SEC 50 // 無音部サーチ時に一秒間に何点検索するか
#define S_POINT_PER_SEC 240 // 無音部サーチ時に一秒間に何点検索するか
#define MAX_SPLITNUM 128	// スプリットマークの最大数


// その他色々
#define BUTTON_NUM	12 // ボタンの数
#define BUFSIZE (FRAMESIZE*16*8) // FRAMESIZE * 16 (nBlockAlignのMAX) * 8 (ZOOM のマックス)
#define SAVEBUFSIZE (512*1024) // SAVE ,サーチ時のバッファサイズ
#define UNDOLEVEL 64 // UNDO の階層
#define EQNUM 8 // イコライザの分割数
#define MAX_EQDB 12 // イコライザの最大デシベル


// サーチ状態
#define ID_SEARCHON		0 // サーチ中
#define ID_SEARCHOFF	1 // サーチ停止


// 無音部サーチモード
#define NSOUND_TOP 0 // 先頭
#define NSOUND_END 1 // 最後
#define NSOUND_MID 2 // 中間


// コピー状態
#define ID_COPYON		0 // コピー中
#define ID_COPYOFF		1 // 停止

// オフセット取得関数のステータス
#define ID_PREPAREWAVE	0
#define ID_OPENWAVE		1
#define ID_RECORDWAVE	2
#define ID_CLOSEWAVE	3


// UNDO データ型
typedef struct
{
LONGLONG n64DataSize[UNDOLEVEL]; //データサイズ(byte)
LONGLONG n64DataOffset[UNDOLEVEL]; //データまでのオフセット(byte)
LONG nScrPos[UNDOLEVEL]; // スクロールバーのポジション
LONG nMarkedPos[UNDOLEVEL];  // マークした位置
LONGLONG n64MarkedPosByte[UNDOLEVEL]; // マークした点(byte)
LONG nSubMarkedPos[UNDOLEVEL];  // マークした位置(副マーク)
LONGLONG n64SubMarkedPosByte[UNDOLEVEL]; // マークした点(副マーク,byte)
LONG nSplitMarkedPos[UNDOLEVEL][MAX_SPLITNUM]; // スプリットマークのポジション
LONGLONG n64SplitMarkedPosByte[UNDOLEVEL][MAX_SPLITNUM]; // スプリットマークの位置(byte)
DWORD dwSplitNum[UNDOLEVEL]; // 分割数
BOOL bDataEmpty[UNDOLEVEL]; // UNDO のデータが入っているかどうか
WORD wCurPos;  // UNDO データの現在位置
} UNDODATA;



// .dat ファイルに保存するデータ
typedef struct
{
// 昔のバージョンの名残のゴミ
double thDynX; 
double thDynY; 
double maxDyn; 
double eqLevel[EQNUM]; 
DWORD dwEqLeng; 
double normLevel;

// 追加オプション
char szOption[MAX_WAVFLTOPTBUF];

// リザーブ
DWORD dwReserve[10];
BOOL bReserve[7];

BOOL bUseAvr;  // 無音検索で平均音量を使う
// bOutfileIsNull == NULL の時、トラック設定ファイルの開いてる行のファイル名は null になる
BOOL bOutfileIsNull; 
BOOL bFoo1; 

double dbReserve[10];

}EDITSAVEDATA,*LPEDITSAVEDATA;


// 全体データ
typedef struct
{
HWND hWnd; 
int x; // X 座標
int y; // Y 座標
BOOL bShiftLock; // シフトロック機能
BOOL bShowConsole; // コンソール表示
UINT uDeviceID; // 再生デバイス ID
UINT uDevNum; // サウンドデバイスの数
WAVEOUTCAPS waveOutCaps[MAXDEVICENUMBER]; // 再生デバイス情報
PROCESS_INFORMATION* pProInfo; // WaveFLT のプロセス情報
LPWORD lpwStatus; // ステータス
WAVEFORMATEX waveFmt; // Wave フォーマットデータ
LONGLONG n64WaveDataOffset; // データまでのオフセットサイズ
LONGLONG n64WaveDataSize;   // WAVE のデータサイズ

// ファイル関係
HANDLE hdFile; // ファイルのハンドル
CHAR szIniDatFile[MAX_PATH]; // 設定保存ファイル
CHAR szIniFileName[MAX_PATH]; // 起動時のファイル名
CHAR szSettingFileName[MAX_PATH]; // 追加オプション設定保存ファイル名

CHAR szLoadFile[MAX_PATH]; //  ロードファイル名
CHAR szSaveFile[MAX_PATH]; // セーブファイル名 
LPSTR lpszFileName; // 保存するファイル名
LPSTR lpszOrgName; // 元のファイル名

LONGLONG n64NewDataOffset; //コピーブロックまでのオフセット
LONGLONG n64NewDataSize; //プロックサイズ

// トラック名関係
CHAR szSaveDir[MAX_PATH]; // 保存ディレクトリ
CHAR szBaseName[CHR_BUF]; // ベース名
CHAR szExtName[CHR_BUF]; // 拡張子
CHAR *szTrackName[MAX_SPLITNUM]; // トラック名
CHAR szTrackFile[MAX_PATH]; // トラック設定ファイル "" ならベース名使用

BOOL bCutTrack; // トラック切り出し
BOOL bSplit; // 分割
BOOL bCutCm; // CM カット

LONGLONG n64SplitMarkedPosByte[MAX_SPLITNUM]; // スプリットマークの位置(バイト)
LONG lnSplitMarkedPos[MAX_SPLITNUM]; // スプリットマークの位置
DWORD dwSplitNum; // スプリットマークの数
DWORD dwCurTrack;  // 現在のトラック番号

EDITSAVEDATA editSaveData; // 編集データ

}EWCDATA,*LPEWCDATA;


// ジャンプダイアログに渡すデータ型
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



// メインプロシージャ
LRESULT CALLBACK EditWaveProc(HWND,UINT,WPARAM,LPARAM);


// editwavefunc.cpp 内の関数
LRESULT CALLBACK EditFindDlgProc(HWND,UINT,WPARAM,LPARAM);
BOOL SaveCutData(HWND,HINSTANCE,LPEWCDATA ,LPSTR,LPSTR,BOOL);


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
				   );


// editdlg 内の関数
LRESULT CALLBACK SettingProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK EditMenuDlgProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK TrackDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK InfoProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);


// 音声関係共通(wavefunc.cpp)
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

// 再生関係(playwave.cpp)
VOID SeekPlayWave(DWORD);
VOID StopPlayWave();
BOOL PlayWave(HWND hWnd,
			  UINT uDeviceID,
			  LPSTR lpszFileName,
			  DWORD dwStartTime,
			  HANDLE hdFile,WAVEFORMATEX waveFmt,
			  LONGLONG n64WaveDataSize, // ファイルのサイズ
			  LONGLONG n64WaveOffset  // データ部分までのオフセット 
			  );
BOOL PlayWaveByte(HWND hWnd,
				  UINT uDeviceID,
				  LPSTR lpszFileName,
				  LONGLONG n64StartByte,
				  HANDLE hdFile,
				  WAVEFORMATEX waveFmt,
				  LONGLONG n64WaveDataSize, // ファイルのサイズ
				  LONGLONG n64WaveOffset  // データ部分までのオフセット 
				  );


// メッセージボックス(mymsgbox.cpp)
int MyMessageBox(HWND,LPSTR,LPSTR,UINT);


// commonfunc.cpp 内の関数
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

double SearchPeak(HANDLE hdFile, // サーチするファイルハンドル
				LONGLONG n64DataOffset, //データまでのオフセット
				WAVEFORMATEX waveFmt, // Wave フォーマット
				LONGLONG n64StartByte,  // スタート位置
				double dTime // 検索時間
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

BOOL WriteToFileMapping(LPHANDLE lphFileMap, // File Mapping Object のハンドル(戻り値)
							LPSTR lpszWriteData,   // 書き込むデータ
							DWORD dwSize, // データサイズ
							LPSTR lpszErr // エラーメッセージ
							);

// draw.cpp 内の関数
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
				HANDLE hdWaveFile, // ファイルが閉じてるなら NULL
				LONGLONG n64CurByte, // byte, データまでのバイト数
				LONG markedLineX, // マークラインの x 
				LONG markedSubLineX, // 副マークラインの x 
				LONG mardedSplitLineX[MAX_SPLITNUM], // スプリットマークラインの x
				DWORD dwSplitNum,
				DWORD dwZoomX, // ズーム倍率(横)
				DWORD dwZoomY // ズーム倍率(縦)
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
