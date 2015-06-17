// e-WC  editwave.cpp のグローバル関数

#include "common.h"

// グローバル変数

HINSTANCE hInst;

CHAR SzInfo[4192];  // インフォメーション


DWORD DwWaveTime; // WAVE の長さ(ミリ秒)

LONGLONG N64OrgFileSize; // ファイルの元サイズ
LONGLONG N64OrgDataSize; // ファイルの元データサイズ
LONGLONG N64OrgDataOffset; // ファイルの元オフセット

DWORD DwZoomX; // ズームサイズ、横
DWORD DwZoomY; // 縦


// 無音部サーチ用
double DbNoSoundBound; // 無音部サーチのしきい値
DWORD dwNSoundCount; // 無音部サーチのカウント数
WORD wNSoundPos; // サーチ後の位置

// スクロールバー用
HWND hScrWnd; // スクロールバーのハンドル
LONG nScrMax; // スクロールバーのマックス値
SCROLLINFO scrInfo; // スクロール情報構造体
LONGLONG N64MaxBlock; // n64WaveDataSizeを FRAMESIZE*waveFmt.nBlockAlign 等分した数

// マーク用
LONG nMarkedPos; // マークしたポジション
LONGLONG N64MarkedPosByte; // オフセットからマークした点までのバイト数(>=0 < n64WaveDataSize)
double DbMarkedLevel[2]; // db, 主マークのレベル

LONG nSubMarkedPos; // 副マークのポジション
LONGLONG N64SubMarkedPosByte; // 副マークのバイト数
double DbSubMarkedLevel[2]; // db, 副マークのレベル
	

// ステータス、UNDO 用
WORD wCurStatus;	 // 現在の状態
BOOL bUpdate; // 更新したか
UNDODATA undoData; // UNDO のデータ

// 編集、サーチ用
DWORD DwSearchTime = 0; // 時間サーチ用(sec)
LPEWCDATA lpEwcData;


// 描画用
HDC hBufDC = NULL; 
HBITMAP hBufBit;
RECT RedrawRect;
HICON hIcon[14]; // ボタンのアイコン
HWND hPreEwcWnd;

//EOF