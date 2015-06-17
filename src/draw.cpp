// easy Wave Cutter
// Copyright (c) 1999-2015 Tomoya Tokairin
// 描画関係

#include "common.h"



//-------------------------------------------------------------------
// 枠の描画
VOID DrawMyFrame(HDC hBufDC,
				 LONG frameX,
				 LONG frameY,
				 LONG frameWidth,
				 LONG frameHeight,
				 COLORREF windowColor){
	
	HPEN hOldPen;
	HBRUSH hBrush,hOldBrush;
	
	// 枠描画	
	hOldPen = (HPEN)SelectObject(hBufDC,GetStockObject(BLACK_PEN));
	hBrush = CreateSolidBrush(windowColor);
	hOldBrush = (HBRUSH)SelectObject(hBufDC,hBrush);
	Rectangle(hBufDC,frameX,frameY,frameX+frameWidth,frameY+frameHeight);
	SelectObject(hBufDC,hOldPen);
	SelectObject(hBufDC,hOldBrush);
	DeleteObject(hBrush);
	
	// 陰影
	hOldPen = (HPEN)SelectObject(hBufDC,GetStockObject(WHITE_PEN));
	MoveToEx(hBufDC,frameX+frameWidth,frameY,NULL);
	LineTo(hBufDC,frameX+frameWidth,frameY+frameHeight);
	MoveToEx(hBufDC,frameX,frameY+frameHeight,NULL);
	LineTo(hBufDC,frameX+frameWidth,frameY+frameHeight);
	SelectObject(hBufDC,hOldPen);
}



//-------------------------------------------------------------------
// 文字の描画
VOID DrawMyString(HDC hBufDC,LPSTR lpszStr, LONG x,LONG y,LONG width,LONG height,LONG size,COLORREF crf){
	
	HFONT hFont,hOldFont;
	TEXTMETRIC textMet; 
	RECT rt;
	
	// フォント作成
	hFont=CreateFont(size,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET
		,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"Courier New");
	hOldFont = (HFONT)SelectObject(hBufDC,hFont);	
	GetTextMetrics(hBufDC,&textMet);
	
	// 文字描画
	SetBkMode(hBufDC,TRANSPARENT);			 
    SetTextColor(hBufDC,crf) ;
	rt.top = y;rt.left = x; rt.right = x+width;rt.bottom = y+height;
	DrawText(hBufDC,lpszStr,strlen(lpszStr),&rt,DT_LEFT);
	
	// フォント削除
	SelectObject(hBufDC,hOldFont);			  
	DeleteObject(hFont);
}


//-------------------------------------------------------------------
// 文字の描画(日本語版)
VOID DrawMyStringJP(HDC hBufDC,LPSTR lpszStr, LONG x,LONG y,LONG width,LONG height,LONG size,COLORREF crf){
	
	HFONT hFont,hOldFont;
	TEXTMETRIC textMet; 
	RECT rt;
	
	// フォント作成
	hFont=CreateFont(size,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET
		,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"ＭＳ Ｐゴシック");
	hOldFont = (HFONT)SelectObject(hBufDC,hFont);	
	GetTextMetrics(hBufDC,&textMet);
	
	// 文字描画
	SetBkMode(hBufDC,TRANSPARENT);			 
    SetTextColor(hBufDC,crf) ;
	rt.top = y;rt.left = x; rt.right = x+width;rt.bottom = y+height;
	DrawText(hBufDC,lpszStr,strlen(lpszStr),&rt,DT_LEFT);
	
	// フォント削除
	SelectObject(hBufDC,hOldFont);			  
	DeleteObject(hFont);
}



//-------------------------------------------------------------------
// ボタンの描画
VOID DrawMyButton(HWND hWnd,UINT uCtlID,HDC hCtlDC,RECT rt,UINT uStatus,HICON* lphIcon,
				  HDC hBufDC2){
	
	LONG width,height,pos;
	HPEN hOldPen;
	HBRUSH hOldBrush;
	
	width = rt.right - rt.left;
	height = rt.bottom - rt.top;
	pos = (uStatus == MYID_PUSHBUTTON);
	
	
	// 枠描画
	hOldPen = (HPEN)SelectObject(hCtlDC,GetStockObject(BLACK_PEN));
	hOldBrush = (HBRUSH)SelectObject(hCtlDC,GetStockObject(NULL_BRUSH));
	Rectangle(hCtlDC, rt.left,rt.top,rt.right,rt.bottom);     
	SelectObject(hCtlDC, hOldPen); 
	SelectObject(hCtlDC, hOldBrush); 
	
	// 本体描画
	hOldPen = (HPEN)SelectObject(hCtlDC,GetStockObject(BLACK_PEN));
	hOldBrush = (HBRUSH)SelectObject(hCtlDC,GetStockObject(GRAY_BRUSH));
	Rectangle(hCtlDC, rt.left+pos,rt.top+pos,
		rt.right-1+pos,rt.bottom-1+pos);     
	SelectObject(hCtlDC, hOldPen); 
	SelectObject(hCtlDC, hOldBrush); 
	
	// 明るい部分
	hOldPen = (HPEN)SelectObject(hCtlDC,GetStockObject(NULL_PEN));
	hOldBrush = (HBRUSH)SelectObject(hCtlDC,GetStockObject(LTGRAY_BRUSH));
	Rectangle(hCtlDC, rt.left+2+pos,rt.top+2+pos,rt.right-2+pos,rt.top+5+pos);
	SelectObject(hCtlDC, hOldPen); 
	SelectObject(hCtlDC, hOldBrush); 
	
	// アイコン貼り付け
	LONG id;
	id = (uCtlID == IDC_BTREW)
		+(uCtlID == IDC_BTPLAYSTOP)*2
		+(uCtlID == IDC_BTFORWARD)*3
		+(uCtlID == IDC_BTREW2)*4
		+(uCtlID == IDC_BTFORWARD2)*5
		+(uCtlID == IDC_BTJUMPBACK)*6
		+(uCtlID == IDC_BTJUMP)*7
		+(uCtlID == IDC_BTJUMPNEXT)*8
		+(uCtlID == IDC_BTREW3)*9
		+(uCtlID == IDC_BTFORWARD3)*10
		+(uCtlID == IDC_BTJUMPBACK2)*11
		+(uCtlID == IDC_BTJUMPBACK3)*12
		+(uCtlID == IDC_BTJUMPNEXT2)*13
		+(uCtlID == IDC_BTJUMPNEXT3)*14;

	if(id>0) DrawIconEx(hCtlDC,rt.left+pos,rt.top+pos,lphIcon[id-1],width,height,0,NULL,DI_NORMAL);
}



//-------------------------------------------------------------------
// 波形描画関数
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
				)
{
	// 描画用各種変数
	HPEN hPen,hOldPen;	  
	
	//　WAVE の情報
	SHORT waveChn,waveBit; 
	LONG waveRate;
	double dWaveLevel[2];
	double dMaxLevel;
	BYTE waveBuf[BUFSIZE]; // データ読み込み用バッファ
	DWORD readByte,dwByte; // 読み込み用
	
	// 雑用変数
	DWORD i,i2;
	LONG ix,iy[2],prevY[2];
	LARGE_INTEGER LI; // SetFilePointer 用 
	
	// 変数設定	  
	waveChn=2; // チャンネル数(ダミー)
	
	if(hdWaveFile != NULL){
		waveChn = (SHORT)waveFmt.nChannels;	 // チャンネル
		waveRate = (LONG)waveFmt.nSamplesPerSec; // レート
		waveBit = (SHORT)waveFmt.wBitsPerSample; // ビット数
	}
	
	// 枠の描画位置計算
	DWORD clientHeight = rt.bottom-rt.top;
	DWORD leftTopX = rt.left + EDITUPDATERECTLEFT; // 枠の左上　の　x ,y座標
	DWORD leftTopY = rt.top+EDITSTATUSMARGIN1+EDITSTATUSSIZE+EDITSTATUSMARGIN2;
	
	// 左側
	DrawMyFrame(hBufDC,leftTopX,leftTopY,FRAMESIZE+2,FRAMESIZE/4+2,RGB(0,0,0));
	// 右側
	if(waveChn==2) DrawMyFrame(hBufDC,leftTopX,leftTopY+FRAMESIZE/4+2+8,FRAMESIZE+2,FRAMESIZE/4+2,RGB(0,0,0));
	
	if(hdWaveFile == NULL){	  // ファイルを閉じている状態
		return;
	}
	else{ // ファイルが開いている状態
		
		// 読み込み位置計算
		LI.QuadPart = n64WaveDataOffset+n64CurByte;
		SetFilePointer(hdWaveFile,LI.LowPart, &LI.HighPart,FILE_BEGIN);
		
		// データ読み込みサイズ計算
		readByte = (n64CurByte + FRAMESIZE*waveFmt.nBlockAlign*dwZoomX ) > n64WaveDataSize
			? (DWORD)(n64WaveDataSize - n64CurByte) : FRAMESIZE*waveFmt.nBlockAlign*dwZoomX;
		
		// データ読み込み
		ReadFile(hdWaveFile,waveBuf,readByte,&dwByte, NULL);
		
		for(i=0;i<(DWORD)waveChn;i++){ 
			
			// 中央線描画
			hOldPen = (HPEN)SelectObject(hBufDC,GetStockObject(WHITE_PEN));
			MoveToEx(hBufDC,leftTopX+1,leftTopY+FRAMESIZE/4/2+(2+8+FRAMESIZE/4)*i,NULL); 
			LineTo(hBufDC,leftTopX+1+FRAMESIZE,leftTopY+FRAMESIZE/4/2+(2+8+FRAMESIZE/4)*i); 
			SelectObject(hBufDC,hOldPen); // ストックペンなのでデリート不要
			
			// マークラインを引く
			if(markedLineX >= 0 && markedLineX < FRAMESIZE){
				hPen = CreatePen(PS_SOLID,1,RGB(255,255,0));
				hOldPen = (HPEN)SelectObject(hBufDC,hPen);
				
				MoveToEx(hBufDC,leftTopX+1+markedLineX,leftTopY+1+(2+8+FRAMESIZE/4)*i,NULL); 
				LineTo(hBufDC,leftTopX+1+markedLineX,leftTopY+1+FRAMESIZE/4+(2+8+FRAMESIZE/4)*i); 
				
				SelectObject(hBufDC,hOldPen);
				DeleteObject(hPen);	
				
			}
			
			// 副マークライン
			if(markedSubLineX >= 0 && markedSubLineX < FRAMESIZE){
				hPen = CreatePen(PS_SOLID,1,RGB(0,255,0));
				hOldPen = (HPEN)SelectObject(hBufDC,hPen);
				
				MoveToEx(hBufDC,leftTopX+1+markedSubLineX,leftTopY+1+(2+8+FRAMESIZE/4)*i,NULL); 
				LineTo(hBufDC,leftTopX+1+markedSubLineX,leftTopY+1+FRAMESIZE/4+(2+8+FRAMESIZE/4)*i); 
				
				SelectObject(hBufDC,hOldPen);
				DeleteObject(hPen);	
			}

			// スプリットマークライン
			for(i2=0;i2<dwSplitNum;i2++){
				if(mardedSplitLineX[i2] > 0 && mardedSplitLineX[i2] < FRAMESIZE){
					hPen = CreatePen(PS_SOLID,1,RGB(255,0,255));
					hOldPen = (HPEN)SelectObject(hBufDC,hPen);
					
					MoveToEx(hBufDC,leftTopX+1+mardedSplitLineX[i2],leftTopY+1+(2+8+FRAMESIZE/4)*i,NULL); 
					LineTo(hBufDC,leftTopX+1+mardedSplitLineX[i2],leftTopY+1+FRAMESIZE/4+(2+8+FRAMESIZE/4)*i); 
					
					SelectObject(hBufDC,hOldPen);
					DeleteObject(hPen);	
				}		
			}
			
		}
		
		// グラフ描画	
		hPen = CreatePen(PS_SOLID,1,RGB(255,255,0)); 
		hOldPen = (HPEN)SelectObject(hBufDC,hPen);
		
		dMaxLevel = GetMaxWaveLevel(waveFmt);
		WaveLevel(dWaveLevel,waveBuf,waveFmt);
		for(i=0;i<(DWORD)waveChn;i++){
			iy[i] =	(DWORD)(dWaveLevel[i]/dMaxLevel*FRAMESIZE/4/2*dwZoomY);
			prevY[i] = leftTopY+FRAMESIZE/4/2+(2+8+FRAMESIZE/4)*i-iy[i];
		}
		
		for(ix=1;ix<(LONG)readByte/waveFmt.nBlockAlign;ix+=dwZoomX){

			// 出力値計算
			WaveLevel(dWaveLevel,waveBuf+ix*waveFmt.nBlockAlign,waveFmt);
			
			for(i=0;i<(DWORD)waveChn;i++){ 
				
				// 波の高さ計算
				iy[i] =	(LONG)(dWaveLevel[i]/dMaxLevel*FRAMESIZE/4/2*dwZoomY);
				iy[i] = max(-FRAMESIZE/4/2+1, min( FRAMESIZE/4/2-1, iy[i]));
				
				MoveToEx(hBufDC,leftTopX+1+(DWORD)((double)ix/(double)dwZoomX),prevY[i],NULL); 
				prevY[i] = leftTopY+FRAMESIZE/4/2+(2+8+FRAMESIZE/4)*i-iy[i];
				LineTo(hBufDC,leftTopX+1+(DWORD)((double)ix/(double)dwZoomX+1),prevY[i]);   
			}			
		}  
		
		SelectObject(hBufDC,hOldPen); 	  
		DeleteObject(hPen);	
	}
}




//-------------------------------------------------------------------
// マウスでクリックした点までの「データオフセットからの」バイト数を計算
// オフセットは含まれないのに注意 !!
BOOL CalcMarkedPosByte(LONGLONG* lpn64MarkedPosByte,
					   WAVEFORMATEX waveFmt,
					   LONGLONG n64WaveDataSize,
					   LONGLONG n64CurByte,
					   DWORD dwZoomX,
					   RECT rt,
					   LONG clickX){
	
	LONG leftTopX = rt.left + (rt.right-rt.left-(FRAMESIZE+2))/2; //枠の右の x 座標
	LONG clickFrameX = clickX - (leftTopX+1); // クリックした点の枠の中の x 座標
	
	if(clickFrameX < 0 || clickFrameX >= FRAMESIZE) return FALSE; // 枠の外をクリックした。
	
	LONGLONG n64EndByte = n64CurByte+clickFrameX*waveFmt.nBlockAlign*dwZoomX;
	
	*lpn64MarkedPosByte = n64EndByte < n64WaveDataSize ? n64EndByte : n64WaveDataSize;
	
	return TRUE;
}



//-------------------------------------------------------------------
// マークラインの x 座標計算
// 範囲外の時は -1 を返す
LONG CalcMarkedLineX(WAVEFORMATEX waveFmt,
					 LONGLONG n64StartByte,
					 DWORD dwZoomX,
					 RECT rt,
					 LONGLONG n64MarkedPosByte){
	
	LONGLONG n64EndByte = n64StartByte+(FRAMESIZE-1)*waveFmt.nBlockAlign*dwZoomX;
	
	if(n64MarkedPosByte < n64StartByte || n64MarkedPosByte > n64EndByte) return -1; // 範囲外
	
	return((LONG)((n64MarkedPosByte-n64StartByte) / (waveFmt.nBlockAlign*dwZoomX)));
}



//-------------------------------------------------------------------
// エディタの初期画面描画 & ボタン作成
VOID InitEditScreenDraw(HWND hWnd,HINSTANCE hInst,
						HDC hBufDC  // == NULL ならボタンとスクロールバーを作る
						){
	
	RECT rt;
	LONG width,height;
	LONG id,i;
	HPEN hOldPen;
	HBRUSH hOldBrush;
	HFONT hFont,hOldFont;
	TEXTMETRIC textMet; 
	
	RECT btRt;
	LONG parentX,parentY,strHeight;
	LPSTR btStr[BUTTON_NUM]={
		"Info","Setup",
			"Track","Load",
			"Left","In",
			"Right","Out",
			"Undo","Reset",
			"Jump","Edit"
	};
	
	LONG btId[BUTTON_NUM]={
		IDC_BTFILEINFO,IDC_BTSETUP,
			IDC_BTTRACK,IDC_BTLOAD,
			IDC_BTCUTLEFT,IDC_BTZOOMIN,
			IDC_BTCUTRIGHT,IDC_BTZOOMOUT,
			IDC_BTUNDO,IDC_BTRESET,
			IDC_BTSEARCH,IDC_BTEDIT
	};
	
	LONG btPlayId[14]={
		IDC_BTREW3,
			IDC_BTREW2,
			IDC_BTREW,
			IDC_BTPLAYSTOP,
			IDC_BTFORWARD,
			IDC_BTFORWARD2,
			IDC_BTFORWARD3,
			IDC_BTJUMPBACK3,
			IDC_BTJUMPBACK2,
			IDC_BTJUMPBACK,
			IDC_BTJUMP,
			IDC_BTJUMPNEXT,
			IDC_BTJUMPNEXT2,
			IDC_BTJUMPNEXT3
	};		
	
	LONG nBtPos[BUTTON_NUM][2];
	
	GetClientRect(hWnd,&rt);
	width = rt.right - rt.left;
	height = rt.bottom - rt.top;
	
	for(i=0;i<BUTTON_NUM;i++){
		nBtPos[i][0] = 
			EDITUPDATERECTLEFT + (BUTTON_WIDTH+BUTTON_MARGIN)*(i/2);
		nBtPos[i][1] = i%2 ? (FONT_SIZE-2)*2+BUTTON_HEIGHT : (FONT_SIZE-2);
	}
	
	// コントロール作成
	if(!hBufDC){
		
		// ボタン作成
		DWORD dwStyle = WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_OWNERDRAW;

		for(id=0;id<BUTTON_NUM;id++){
			CreateWindow("BUTTON",btStr[id],dwStyle,
				nBtPos[id][0],nBtPos[id][1],
				BUTTON_WIDTH,BUTTON_HEIGHT,hWnd,(HMENU)btId[id],hInst,NULL);
		}
		
		// 再生とかのボタンの作成
		LONG margin = (EDITUPDATERECTTOP - PLAYBT_HEIGHT*2 - SB_HEIGHT)/2;
		LONG left = nBtPos[BUTTON_NUM-1][0] + BUTTON_WIDTH + BUTTON_MARGIN;
		for(id=0;id<14;id++){
			CreateWindow("BUTTON","",dwStyle,
				left
				+ (PLAYBT_WIDTH + margin)*((id%7)),
				1 + (PLAYBT_HEIGHT + margin)*(id>6),
				PLAYBT_WIDTH,PLAYBT_HEIGHT,hWnd,
				(HMENU)btPlayId[id],hInst,NULL);
			
		}
			
		// スクロールバー作成
		CreateWindow("SCROLLBAR","",WS_CHILD|WS_VISIBLE,
			EDITUPDATERECTLEFT,
			EDITUPDATERECTTOP - SB_HEIGHT -2,
			FRAMESIZE,SB_HEIGHT,
			hWnd,(HMENU)IDC_SBTIME,hInst,NULL);
		
		return;
	}
	
	
	// 背景色セット
	hOldPen = (HPEN)SelectObject(hBufDC,GetStockObject(NULL_PEN));
	hOldBrush = (HBRUSH)SelectObject(hBufDC,GetStockObject(GRAY_BRUSH));
	Rectangle(hBufDC,-1,-1,width+2,height+2);
	SelectObject(hBufDC,hOldPen);
	SelectObject(hBufDC,hOldBrush);
	
	// ボタン名描画
	GetWindowRect(hWnd,&rt);
	parentX = rt.left+GetSystemMetrics(SM_CXBORDER);
	parentY = rt.bottom - height-GetSystemMetrics(SM_CXBORDER);
	
	// フォントサイズ取得
	hFont=CreateFont(FONT_SIZE,0,0,0,FW_LIGHT,FALSE,FALSE,FALSE,DEFAULT_CHARSET
		,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
		DEFAULT_PITCH,"Courier New");
	hOldFont = (HFONT)SelectObject(hBufDC,hFont);	
	GetTextMetrics(hBufDC,&textMet);
	strHeight = textMet.tmAscent;
	SetBkMode(hBufDC,TRANSPARENT);		
	
	// ボタン配置 & 名前描画
	for(id=0;id<BUTTON_NUM;id++){
		for(i=0;i<2;i++){
			GetWindowRect(GetDlgItem(hWnd,btId[id]),&btRt);
			SetTextColor(hBufDC,RGB(255*i,255*i,255*i)) ;
			btRt.left -= (parentX+1+i); 
			btRt.right -= (parentX+1+i); 
			btRt.top -= (parentY+strHeight+i);
			btRt.bottom -=(parentY+strHeight+i);
			DrawText(hBufDC,btStr[id],strlen(btStr[id]),&btRt,DT_CENTER);
		}
	}
	
	SelectObject(hBufDC,hOldFont);			  
	DeleteObject(hFont);
	
}




//-------------------------------------------------------------------
// ステータス表示
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
				   ){
	
	CHAR szStr[CHR_BUF],szStr2[CHR_BUF],szSize[CHR_BUF];
	DWORD dwMarkTime,dwRemTime,dwFoo,dwFoo2;
	RECT rt;
	char szDBL[64],szDBR[64];
	LONGLONG n64Foo,n64Foo2;
	
	// データサイズ
	dwFoo = (DWORD)(n64DataSize/1024/1024);
	wsprintf(szSize,"%d M",dwFoo);
	
	// 残り時間
	LONG foo = (LONG)dwFileTime - (LONG)dwCurTime;
	dwRemTime = foo >=0 ? (DWORD)foo : 0;
	dwCurTime /= 1000;
	dwFileTime /= 1000;
	dwRemTime /= 1000;
	
	wsprintf(szStr,"[TIME]%3d:%02d:%02d (R %3d:%02d:%02d)",
		dwCurTime/60/60,
		(dwCurTime/60)%60,
		(dwCurTime)%60,
		dwRemTime/60/60,
		(dwRemTime/60)%60,
		(dwRemTime)%60
		);
	
	wsprintf(szStr2,"[x%d,x%d][%s][%2dk %2dbit %3d:%02d:%02d]",
		dwZoomX,
		dwZoomY,
		szSize,
		waveFmt.nSamplesPerSec/1000,
		waveFmt.wBitsPerSample,

		dwFileTime/60/60,
		(dwFileTime/60)%60,
		(dwFileTime)%60
		);	
	
	// 時間情報表示
	GetClientRect(hWnd,&rt);
	DrawMyFrame(hBufDC,
		rt.left+EDITUPDATERECTLEFT,
		rt.top + EDITUPDATERECTTOP+EDITSTATUSMARGIN1,FRAMESIZE,EDITSTATUSSIZE,RGB(0,0,0));

	DrawMyString(hBufDC,szStr,
		rt.left+EDITUPDATERECTLEFT+3,
		rt.top + EDITUPDATERECTTOP+EDITSTATUSMARGIN1+3,
		FRAMESIZE-6,(EDITSTATUSSIZE-6)/EDITSTATUSLINE,14,RGB(255,255,0));
	DrawMyString(hBufDC,szStr2,
		rt.left+EDITUPDATERECTLEFT+3+EDITSTATUSLEFTMARGIN,
		rt.top + EDITUPDATERECTTOP+EDITSTATUSMARGIN1+3,
		(FRAMESIZE-6)-EDITSTATUSLEFTMARGIN,(EDITSTATUSSIZE-6)/EDITSTATUSLINE,14,RGB(255,255,0));
	
	
	// マーク情報表示
	dwMarkTime = (DWORD)(n64MarkPosByte/waveFmt.nAvgBytesPerSec);
	foo = (LONG)dwFileTime - (LONG)dwMarkTime;
	dwRemTime = foo >=0 ? (DWORD)foo : 0;
	
	wsprintf(szStr,"Mark1>%3d:%02d:%02d (R %3d:%02d:%02d)",
		dwMarkTime/60/60,
		(dwMarkTime/60)%60,
		(dwMarkTime)%60,

		dwRemTime/60/60,
		(dwRemTime/60)%60,
		(dwRemTime)%60
		);

	if(waveFmt.nChannels == 2){
		wsprintf(szDBL,"%s",myfcvt(dMarkLevel[0],2));
		wsprintf(szDBR,"%s",myfcvt(dMarkLevel[1],2));
		wsprintf(szStr2,"[LEVEL] L: %s dB  R: %s dB",szDBL,szDBR);
	}
	else{
		wsprintf(szDBL,"%s",myfcvt(dMarkLevel[0],2));
		wsprintf(szStr2,"[LEVEL] %s dB",szDBL);
	}
	
	DrawMyString(hBufDC,szStr,
		rt.left+EDITUPDATERECTLEFT+3,
		rt.top + EDITUPDATERECTTOP+EDITSTATUSMARGIN1+(EDITSTATUSSIZE-6)/EDITSTATUSLINE+3,
		FRAMESIZE-6,(EDITSTATUSSIZE-6)/EDITSTATUSLINE,14,RGB(255,255,0));
	DrawMyString(hBufDC,szStr2,
		rt.left+EDITUPDATERECTLEFT+3+EDITSTATUSLEFTMARGIN,
		rt.top + EDITUPDATERECTTOP+EDITSTATUSMARGIN1+(EDITSTATUSSIZE-6)/EDITSTATUSLINE+3,
		(FRAMESIZE-6)-EDITSTATUSLEFTMARGIN,(EDITSTATUSSIZE-6)/EDITSTATUSLINE,14,RGB(255,255,0));
		
	
	// マーク 2
	dwMarkTime = (DWORD)(n64SubMarkPosByte/waveFmt.nAvgBytesPerSec);
	foo = (LONG)dwFileTime - (LONG)dwMarkTime;
	dwRemTime = foo >=0 ? (DWORD)foo : 0;
	
	wsprintf(szStr,"Mark2>%3d:%02d:%02d (R %3d:%02d:%02d)",
		dwMarkTime/60/60,
		(dwMarkTime/60)%60,
		(dwMarkTime)%60,

		dwRemTime/60/60,
		(dwRemTime/60)%60,
		(dwRemTime)%60
		);

	if(waveFmt.nChannels == 2){
		wsprintf(szDBL,"%s",myfcvt(dSubMarkLevel[0],2));
		wsprintf(szDBR,"%s",myfcvt(dSubMarkLevel[1],2));
		wsprintf(szStr2,"[LEVEL] L: %s dB  R: %s dB",szDBL,szDBR);
	}
	else{
		wsprintf(szDBL,"%s",myfcvt(dSubMarkLevel[0],2));
		wsprintf(szStr2,"[LEVEL] %s dB",szDBL);
	}
	
	DrawMyString(hBufDC,szStr,
		rt.left+EDITUPDATERECTLEFT+3,
		rt.top + EDITUPDATERECTTOP+EDITSTATUSMARGIN1+((EDITSTATUSSIZE-6)/EDITSTATUSLINE)*2+3,
		FRAMESIZE-6,(EDITSTATUSSIZE-6)/EDITSTATUSLINE,14,RGB(255,255,0));
	DrawMyString(hBufDC,szStr2,
		rt.left+EDITUPDATERECTLEFT+3+EDITSTATUSLEFTMARGIN,
		rt.top + EDITUPDATERECTTOP+EDITSTATUSMARGIN1+((EDITSTATUSSIZE-6)/EDITSTATUSLINE)*2+3,
		(FRAMESIZE-6)-EDITSTATUSLEFTMARGIN,(EDITSTATUSSIZE-6)/EDITSTATUSLINE,14,RGB(255,255,0));
	

	// トラック
	n64Foo = (dwTrack-1 == 0) ? 0 : n64SplitMarkedPosByte[dwTrack-2];
	dwMarkTime = (DWORD)(n64Foo/waveFmt.nAvgBytesPerSec);
	n64Foo2 = (dwTrack-1 == dwSplitNum) ? n64DataSize : n64SplitMarkedPosByte[dwTrack-1];
	dwRemTime = (DWORD)(n64Foo2/waveFmt.nAvgBytesPerSec);

	dwCurTime -= dwMarkTime;

	dwFoo2 = dwRemTime-dwMarkTime;
	wsprintf(szStr,"[Track %3d/%d] %d:%02d:%02d-%d:%02d:%02d (%d:%02d:%02d) %d:%02d:%02d",
		dwTrack,dwSplitNum+1,

		dwMarkTime/60/60,
		(dwMarkTime/60)%60,
		(dwMarkTime)%60,

		dwRemTime/60/60,
		(dwRemTime/60)%60,
		(dwRemTime)%60,

		dwFoo2/60/60,
		(dwFoo2/60)%60,
		(dwFoo2)%60,

		dwCurTime/60/60,
		(dwCurTime/60)%60,
		(dwCurTime)%60

		);
	
	DrawMyString(hBufDC,szStr,
		rt.left+EDITUPDATERECTLEFT+3,
		rt.top + EDITUPDATERECTTOP+EDITSTATUSMARGIN1+((EDITSTATUSSIZE-6)/EDITSTATUSLINE)*3+3,
		FRAMESIZE-6,(EDITSTATUSSIZE-6)/EDITSTATUSLINE,14,RGB(255,255,0));

	DrawMyStringJP(hBufDC,lpszTrackName,
		rt.left+EDITUPDATERECTLEFT+3,
		rt.top + EDITUPDATERECTTOP+EDITSTATUSMARGIN1+((EDITSTATUSSIZE-6)/EDITSTATUSLINE)*4+3,
		FRAMESIZE-6,(EDITSTATUSSIZE-6)/EDITSTATUSLINE,14,RGB(255,255,0));

}



//-------------------------------------------------------------------
// マークの三角描画
VOID DrawMarkTriangle(HDC hDC,LONG nPos,LONG nPos2,LONG nPos3[MAX_SPLITNUM],
					  DWORD dwSplitNum,LONG nMax){
	
	POINT point[4];
	LONG x[2+MAX_SPLITNUM];
	int nPoints;
	HPEN hOldPen;
	HBRUSH hOldBrush,hBrush;
	WORD i;
	
	x[1] = (INT)(482.0/nMax*nPos)+EDITUPDATERECTLEFT+12;
	x[0] = (INT)(482.0/nMax*nPos2)+EDITUPDATERECTLEFT+12;
	for(i=0;i<dwSplitNum;i++){
		if(nPos3[i] > 0) x[2+i] = (INT)(482.0/nMax*nPos3[i])+EDITUPDATERECTLEFT+12;
		else x[2+i] = 0;
	}
	
	// 背景塗りつぶし
	hOldPen = (HPEN)SelectObject(hDC,GetStockObject(NULL_PEN));
	hOldBrush = (HBRUSH)SelectObject(hDC,GetStockObject(GRAY_BRUSH));
	Rectangle(hDC,0,EDITUPDATERECTTOP,FRAMESIZE+10,EDITUPDATERECTTOP+7);
	
	// 三角描画
	SelectObject(hDC,GetStockObject(BLACK_PEN));
	
	for(i=0;i<2+dwSplitNum;i++){
		
		if(x[i] > 0){
			
			if(i==1){
				hBrush = CreateSolidBrush(RGB(255,255,0));
				SelectObject(hDC,hBrush);
			}
			else if(i==0){
				hBrush = CreateSolidBrush(RGB(0,255,0));
				SelectObject(hDC,hBrush);
			}
			else{
				hBrush = CreateSolidBrush(RGB(255,0,255));
				SelectObject(hDC,hBrush);
			}
	
			point[0].x = x[i];
			point[0].y = EDITUPDATERECTTOP;
			
			point[1].x = x[i]+5;
			point[1].y = EDITUPDATERECTTOP+5;
			
			point[2].x = x[i]-5;
			point[2].y = EDITUPDATERECTTOP+5;
			
			point[3].x = x[i];
			point[3].y = EDITUPDATERECTTOP;
			
			nPoints = 4;
			
			PolyPolygon(hDC,point,&nPoints,1); 
			
			DeleteObject(hBrush);
		}
	}
	
	// ペンとかを戻す
	SelectObject(hDC,hOldPen);
	SelectObject(hDC,hOldBrush);
}



//EOF
