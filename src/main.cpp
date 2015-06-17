// WAVEFLT FRONT END  easy Wave Cutter メイン
// Copyright (c) 1999-2015 Tomoya Tokairin
//
// 本プログラムのすべて、または一部を GPL に従って再頒布または変更する
// ことができます。詳細についてはGNU 一般公有使用許諾書をお読みください。


#include "common.h"


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPreInst,
                   LPSTR lpszCmdLine, int nCmdShow)
{
	HWND hWnd;
    MSG msg;
    WNDCLASS wndClass;
	HACCEL hAccel = NULL;
	
	DWORD i;
	
	// 編集データ用変数
	EWCDATA ewcData;
	
	// コマンドライン用
	CHAR argv[MAX_ARGC][CHR_BUF];
	int argc;

	// ウィンドウ登録
    if (!hPreInst) {
        wndClass.style = CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
        wndClass.lpfnWndProc = EditWaveProc;
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra = 0;
        wndClass.hInstance = hInst;
		wndClass.hIcon  = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON));
        wndClass.hCursor = LoadCursor(NULL,IDC_ARROW);
        wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wndClass.lpszMenuName = NULL;
        wndClass.lpszClassName = "E-WC";
        if (!RegisterClass(&wndClass)) return 0;
    }	


	// EWCDATA 構造体初期設定
	ZeroMemory(&ewcData,sizeof(EWCDATA));
	
	// サウンドデバイスの情報取得
	ewcData.uDevNum = GetWaveOutDevCap(ewcData.waveOutCaps,MAXDEVICENUMBER);
	

	// 設定ファイル読み込み

	// 設定ファイルセット
	CHAR szStr[MAX_PATH];
	CHAR szDriveName[MAX_PATH],szPathName[MAX_PATH];
	GetModuleFileName(NULL,szStr,MAX_PATH); 
	_splitpath(szStr,szDriveName,szPathName,NULL,NULL);	
	wsprintf(ewcData.szIniDatFile,"%s%sewc.dat",szDriveName,szPathName);
	ReadIniFile(&ewcData,ewcData.szIniDatFile);
	
	// コマンドライン取得
	argc = GetArgv(lpszCmdLine,argv,1);
	
	// ファイル名セット
	if(argc > 1) if(ShortToLongName(argv[1])) wsprintf(ewcData.szIniFileName,argv[1]);
	
	// デバイスセット
	if(strcmp(argv[2],"-dev") == 0){
		ewcData.uDeviceID = atoi(argv[3]);
	}
	
	// トラック名設定
	ewcData.szTrackFile[0] = '\0';
	strcpy(ewcData.szBaseName,"track");
	strcpy(ewcData.szExtName,"wav");
	ewcData.szTrackName[0] = (CHAR*)malloc(sizeof(CHAR)*MAX_SPLITNUM*CHR_BUF+1024);
	for(i=1;i<MAX_SPLITNUM;i++) ewcData.szTrackName[i] = ewcData.szTrackName[0] + i*CHR_BUF;
	for(i=0;i<MAX_SPLITNUM;i++) wsprintf(ewcData.szTrackName[i],"%s-%03d.%s",ewcData.szBaseName,i+1,ewcData.szExtName);

	// ウィンドウ作成
    hWnd = CreateWindow(
		"E-WC",
        "ewc",
        WS_VISIBLE|WS_CAPTION|WS_POPUPWINDOW|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,NULL,hInst,(LPVOID)&ewcData);
	
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);	

	// アクセレーターテーブルのロード
	hAccel = LoadAccelerators(hInst,MAKEINTRESOURCE(IDR_ACCELERATOR));

	if(hAccel == NULL) 
		MyMessageBox(NULL,"アクセレーターテーブルのハンドル\n取得に失敗しました。"
		,"error",MB_OK|MB_ICONERROR);
	else 
		while (GetMessage(&msg, NULL, 0, 0)) {	// メッセージループ
			if(!TranslateAccelerator(hWnd,hAccel,&msg))  // アクセレーター
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}


	// 設定ファイル保存
	SaveIniFile(&ewcData,ewcData.szIniDatFile);

	// メモリ開放
	free(ewcData.szTrackName[0]);

	
	return 0;
}


//EOF