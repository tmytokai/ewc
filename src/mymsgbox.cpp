// 自作メッセージボックス

#include <windows.h>
#include <windowsx.h>
#include "resource.h"

#define CHR_BUF 256

#define MSGBOXBTWIDTH	60  // ボタン幅
#define MSGBOXBTHEIGHT	24  // ボタン高さ
#define MSGBOXMERGIN	10  // マージン
#define MSGBOXICONWIDTH	45  // アイコン幅
#define MSGBOXICONHEIGHT 40  // アイコン高さ
#define MSGBOXTEXTSIZE	12  // テキストサイズ

// ボタンの状態
#define MYID_RELEASEBUTTON              0  // リリース
#define MYID_PUSHBUTTON                 1  // 押してる


// プロシージャに渡すデータ型
typedef struct
{
	LPSTR lpText;
	LPSTR lpCaption;
	UINT uType;
	BOOL bOwnerDraw;
}MSGBOXDATA,*LPMSGBOXDATA;


// このファイル内の関数
int MyMessageBox(HWND,LPSTR,LPSTR,UINT);
LRESULT CALLBACK MyMessageBoxProc(HWND,UINT,WPARAM,LPARAM);
VOID DrawMsgButton(HWND,UINT,HDC,RECT,UINT,UINT);


// 他のファイルの関数
VOID SetDlgCenter(HWND);
VOID DrawMyStringJP(HDC,LPSTR,LONG,LONG,LONG,LONG,LONG,COLORREF);
WORD GetTextSize(HWND,LPSTR,LPSIZE,LONG);
VOID DrawMyFrame(HDC,LONG,LONG,LONG,LONG,COLORREF);



//--------------------------------------------------------------------
// 自作メッセージボックス呼び出し関数
int MyMessageBox(HWND hWnd,LPSTR lpText,LPSTR lpCaption,UINT uType){
	
	MSGBOXDATA msgBoxData;
	HINSTANCE hInst;
	
	hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
	msgBoxData.lpText = lpText;
	msgBoxData.lpCaption = lpCaption;
	msgBoxData.uType = uType;
	msgBoxData.bOwnerDraw = FALSE;  // ボタンはオーナードローしない
	
	// ダイアログ呼び出し
	return(DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_MYMBOXDLG),hWnd,
		(DLGPROC)MyMessageBoxProc,(LPARAM)&msgBoxData));
}


//---------------------------------------------------------------------
// メッセージボックスのプロシージャ
LRESULT CALLBACK MyMessageBoxProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	
	static LPMSGBOXDATA lpMsgBoxData;
	static HICON hIcon;
	static BOOL bDrawIcon; // アイコンを貼るかどうか
	static RECT textRt;  // テキスト描画領域
	static SIZE sizeText; //  テキスト描画領域のサイズ
	static LONG width,height; // ウィンドウ幅、高さ
	static HWND hWndBt[3];
	static int nResult[3];
	static HFONT hFont; // ボタン用フォント
	static LONG nButtonNum; // ボタン数
	static BOOL bOwnerDraw; // ボタンをオーナードローするか
	static UINT uCurFocus; // 現在のフォーカスボタンID
	
	CHAR szBtStr[3][CHR_BUF];
	LONG x1,x2,x3,y;
	HINSTANCE hInst;
	HDC hDC;
	LONG nButtonSize;
	UINT uIconType,uMskType;
	DWORD dwStyle;
	COLORREF rgbStr;
	WORD i;
	
	switch (msg) {
		
	case WM_INITDIALOG:  // ダイアログ初期化
		
		lpMsgBoxData = (LPMSGBOXDATA)lp;
		hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
		
		// アイコンロード
		uIconType = lpMsgBoxData->uType & MB_ICONMASK;
		
		if(uIconType > 0) bDrawIcon = TRUE;
		else bDrawIcon = FALSE;
		
		switch(uIconType){
			
		case MB_ICONQUESTION:
			hIcon = LoadIcon(NULL,IDI_QUESTION);
			break;
			
		case MB_ICONINFORMATION:
			hIcon = LoadIcon(NULL,IDI_INFORMATION);
			break;
			
		case MB_ICONEXCLAMATION:
			hIcon = LoadIcon(NULL,IDI_EXCLAMATION);
			break;
			
		case MB_ICONERROR:
			hIcon = LoadIcon(NULL,IDI_ERROR);
		}
		
		
		// テキスト領域の幅と高さ取得
		GetTextSize(hWnd,lpMsgBoxData->lpText,&sizeText,MSGBOXTEXTSIZE);
		
		// テキスト描画領域セット
		textRt.top = MSGBOXMERGIN + (sizeText.cy > MSGBOXICONHEIGHT? 0 : (MSGBOXICONHEIGHT-sizeText.cy)/2);
		textRt.left = MSGBOXICONWIDTH*bDrawIcon+MSGBOXMERGIN;
		textRt.right = textRt.left + sizeText.cx;
		textRt.bottom = textRt.top + sizeText.cy;
		
		// タイトルセット
		SetWindowText(hWnd,lpMsgBoxData->lpCaption);
		
		// ボタン名設定
		for(i=0;i<3;i++) nResult[i] = 0;
		switch(lpMsgBoxData->uType & MB_TYPEMASK){
			
		default:
			nResult[0] = IDOK;
			wsprintf(szBtStr[0],"OK");
			
			nButtonNum = 1;
			break;
			
		case MB_OKCANCEL:
			nResult[0] = IDOK;
			wsprintf(szBtStr[0],"OK");
			
			nResult[1] = IDCANCEL;
			wsprintf(szBtStr[1],"キャンセル");
			
			nButtonNum = 2;
			break;
			
		case MB_YESNOCANCEL:
			
			nResult[0] = IDYES;
			wsprintf(szBtStr[0],"はい(&Y)");
			
			nResult[1] = IDNO;
			wsprintf(szBtStr[1],"いいえ(&N)");
			
			nResult[2] = IDCANCEL;
			wsprintf(szBtStr[2],"キャンセル");
			
			nButtonNum = 3;
			break;
			
		case MB_YESNO:
			
			nResult[0] = IDYES;
			wsprintf(szBtStr[0],"はい(&Y)");
			
			nResult[1] = IDNO;
			wsprintf(szBtStr[1],"いいえ(&N)");
			
			nButtonNum = 2;
		}
		
		// ウィンドウリサイズ
		nButtonSize =MSGBOXBTWIDTH*nButtonNum+MSGBOXMERGIN*(nButtonNum-1);
		
		width = max(nButtonSize+MSGBOXMERGIN*2,sizeText.cx+MSGBOXMERGIN*2+MSGBOXICONWIDTH*bDrawIcon);
		height = max(MSGBOXICONHEIGHT*bDrawIcon,sizeText.cy)+ MSGBOXMERGIN*3+MSGBOXBTHEIGHT;
		
		SetWindowPos(hWnd,NULL,0,0,
			width+GetSystemMetrics(SM_CXFIXEDFRAME)*2,
			height+GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYFIXEDFRAME)*2
			,SWP_NOMOVE|SWP_NOREPOSITION);
		
		
		// ボタン作成
		x1 = (width - nButtonSize)/2;
		x2 = x1 + MSGBOXBTWIDTH+MSGBOXMERGIN;
		x3 = x2 + MSGBOXBTWIDTH+MSGBOXMERGIN;
		y = height-MSGBOXBTHEIGHT - MSGBOXMERGIN;
		
		uMskType = lpMsgBoxData->uType & MB_DEFMASK;
		bOwnerDraw = lpMsgBoxData->bOwnerDraw; // オーナードローか
		
		// ボタン 1
		if((nButtonNum >=2 && uMskType == MB_DEFBUTTON2) ||
			(nButtonNum == 3 && uMskType == MB_DEFBUTTON3))
			dwStyle = WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|WS_TABSTOP;
		else dwStyle = WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP;
		if(bOwnerDraw) dwStyle |= BS_OWNERDRAW;
		CreateWindow("BUTTON","bt1",dwStyle,x1,y,MSGBOXBTWIDTH,MSGBOXBTHEIGHT,hWnd,(HMENU)IDC_BT1,hInst,NULL);
		
		// ボタン 2
		if(nButtonNum >= 2){
			if(uMskType == MB_DEFBUTTON2)
				dwStyle = WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP;
			else dwStyle = WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|WS_TABSTOP;
			if(bOwnerDraw) dwStyle |= BS_OWNERDRAW;
			CreateWindow("BUTTON","bt2",dwStyle,x2,y,MSGBOXBTWIDTH,MSGBOXBTHEIGHT,hWnd,(HMENU)IDC_BT2,hInst,NULL);
		}
		
		
		// ボタン 3
		if(nButtonNum == 3){
			if(uMskType == MB_DEFBUTTON3)
				dwStyle = WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP;
			else dwStyle = WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|WS_TABSTOP;
			if(bOwnerDraw) dwStyle |= BS_OWNERDRAW;
			CreateWindow("BUTTON","bt3",dwStyle,x3,y,MSGBOXBTWIDTH,MSGBOXBTHEIGHT,hWnd,(HMENU)IDC_BT3,hInst,NULL);
		}
		
		
		// ボタン用フォント作成
		if(!bOwnerDraw) hFont=CreateFont(10,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET
			,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"ＭＳ Ｐゴシック");
		
		// ボタンにフォントと文字セット
		hWndBt[0] = GetDlgItem(hWnd,IDC_BT1);
		hWndBt[1] = GetDlgItem(hWnd,IDC_BT2);
		hWndBt[2] = GetDlgItem(hWnd,IDC_BT3);
		
		for(i=0;i<3;i++)
		{
			// フォントセット
			if(!bOwnerDraw) SendMessage(hWndBt[i],WM_SETFONT,(WPARAM)hFont,(LPARAM)MAKELPARAM(TRUE,0));
			SetWindowText(hWndBt[i],szBtStr[i]);
		}
		
		// フォーカス移動
		switch(uMskType){
			
		case MB_DEFBUTTON2:
			uCurFocus = IDC_BT2;
			SetFocus(hWndBt[1]);
			break;
			
		case MB_DEFBUTTON3:
			uCurFocus = IDC_BT3;
			SetFocus(hWndBt[2]);
			break;
			
		default:
			uCurFocus = IDC_BT1;
			SetFocus(hWndBt[0]);
		}
		
		// 中心移動
		SetDlgCenter(hWnd);

		// forgrand
		if(lpMsgBoxData->uType & MB_SETFOREGROUND) SetForegroundWindow(hWnd);
		
		break;
		
		
		case WM_CTLCOLORDLG: // 背景色を変えたい場合
			
			if(bOwnerDraw){
				SetBkMode((HDC)wp,TRANSPARENT);			 
				SetTextColor((HDC)wp,RGB(255,255,255)) ;
				return ((BOOL)GetStockObject(GRAY_BRUSH)) ;    
			}
			
			break;
			
			
		case WM_DRAWITEM: // ボタンのオーナー描画をする場合
			
			if(bOwnerDraw){
				
				UINT uCtlID,uButtonStat;
				LPDRAWITEMSTRUCT lpDrawItem;
				
				uCtlID = (UINT)wp;
				lpDrawItem = (LPDRAWITEMSTRUCT)lp;
				uButtonStat = MYID_RELEASEBUTTON;
				
				if(lpDrawItem->CtlType == ODT_BUTTON){ // ボタンならば
					
					if(lpDrawItem->itemAction & ODA_SELECT){ // 選択
						if(lpDrawItem->itemState & ODS_FOCUS) uButtonStat = MYID_PUSHBUTTON;
					}
					else if(lpDrawItem->itemAction & ODA_FOCUS){ // フォーカス
						// 自前でフォーカスの管理をする
						if(lpDrawItem->itemState & ODS_FOCUS) uCurFocus = uCtlID;
					}
					
					// ボタン描画
					DrawMsgButton(hWnd,uCtlID,lpDrawItem->hDC,lpDrawItem->rcItem,uCurFocus,uButtonStat);
					
				}
			}
			
			break;
			
		case WM_PAINT: // 描画
			
			hDC = GetDC(hWnd);
			
			if(bOwnerDraw){
				// 枠描画
				DrawMyFrame(hDC,MSGBOXMERGIN-2,MSGBOXMERGIN-2,
					width - (MSGBOXMERGIN-2)*2,height-MSGBOXBTHEIGHT - MSGBOXMERGIN-(MSGBOXMERGIN-2)*2
					,RGB(0,0,0));
				rgbStr = RGB(255,255,255);
			}
			else rgbStr = RGB(0,0,0);
			
			// アイコン
			if(bDrawIcon)
				DrawIcon(hDC,MSGBOXMERGIN+1,MSGBOXMERGIN+1,hIcon);
			
			// テキスト
			DrawMyStringJP(hDC,lpMsgBoxData->lpText,
				textRt.left,textRt.top,textRt.right-textRt.left,
				textRt.bottom-textRt.top,MSGBOXTEXTSIZE,rgbStr);
			
			
			ReleaseDC(hWnd,hDC);
			
			break;
			
			
		case WM_SETTEXT: // ダイアログのタイトル変更
			
			// DefWindowProc に処理してもらう(なんか胡散臭いやり方 (^^; )
			return(DefWindowProc(hWnd, msg, wp, lp)); 
			
			break;
			
		case WM_COMMAND:
			
			// オーナードローの時はコントロール ID をもらえないので
			// 自前でフォーカス管理をする
			if(bOwnerDraw && HIWORD(wp)== BN_CLICKED) wp = uCurFocus;
			
			switch (LOWORD(wp)) {
				
			case IDC_BT1: 
				if(!bOwnerDraw) DeleteObject(hFont);
				EndDialog(hWnd, nResult[0]);
				return TRUE;
				
			case IDC_BT2: 
				if(!bOwnerDraw) DeleteObject(hFont);
				EndDialog(hWnd, nResult[1]);
				return TRUE;
				
			case IDC_BT3: 
				if(!bOwnerDraw) DeleteObject(hFont);
				EndDialog(hWnd, nResult[2]);
				return TRUE;
				
			case IDCANCEL: 
				if(!bOwnerDraw) DeleteObject(hFont);
				EndDialog(hWnd, IDCANCEL);
				return TRUE;
				
			}
			break;
			
			default:
				break;
	}
    return FALSE;
}



//-------------------------------------------------------------------
// テキスト領域の幅と高さを取得する関数
WORD GetTextSize(HWND hWnd,LPSTR lpszStr,LPSIZE lpTextSize,LONG size){
	
	HDC hDC;
	WORD wLeng,wRowNum;
	LONG nMaxCX,nMaxCY;
	WORD i,i2;
	SIZE sizeText;
	
	HFONT hFont,hOldFont;
	TEXTMETRIC textMet; 
	
	WORD wTextLeng = strlen(lpszStr);
	
	hDC = GetDC(hWnd);
	
	// フォント作成
	hFont=CreateFont(size,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET
		,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"ＭＳ Ｐゴシック");
	hOldFont = (HFONT)SelectObject(hDC,hFont);	
	GetTextMetrics(hDC,&textMet);
	
	
	// テキスト領域の幅と高さ取得
	wRowNum = 0; // 行数
	nMaxCX = 0; // 最大幅
	nMaxCY = 0; // 一行の最大高さ
	
	i = 0; 
	while(i < wTextLeng){
		wRowNum++; // 行数プラス
		
		// 現在の行の文字数取得
		i2 = i; wLeng = 0;
		while(lpszStr[i2+wLeng] != '\n' && i2+wLeng < wTextLeng) wLeng++;
		
		// 現在の行の幅と高さ計算
		GetTextExtentPoint32(hDC,lpszStr+i2,wLeng,&sizeText);
		
		nMaxCX = max(nMaxCX,sizeText.cx);
		nMaxCY = max(nMaxCY,sizeText.cy);
		
		// 次の行へ
		i = i+wLeng+1;
	}
	
	lpTextSize->cx = nMaxCX;
	lpTextSize->cy = nMaxCY * wRowNum;
	
	// フォント削除
	SelectObject(hDC,hOldFont);			  
	DeleteObject(hFont);
	
	ReleaseDC(hWnd,hDC); 
	
	return wRowNum; // 行数を返す
}



//-------------------------------------------------------------------
// ボタンの描画
// ボタン名をボタンの中に描画するバージョン
VOID DrawMsgButton(HWND hWnd,UINT uCtlID,HDC hCtlDC,
				   RECT rt,UINT uFocus,UINT uStatus){
	
	HFONT hFont,hOldFont;
	LONG width,height,pos;
	HPEN hOldPen;
	HBRUSH hBrush,hOldBrush;
	CHAR szStr[CHR_BUF];
	
	width = rt.right - rt.left;
	height = rt.bottom - rt.top;
	pos = (uStatus == MYID_PUSHBUTTON);
	GetWindowText(GetDlgItem(hWnd,uCtlID),szStr,CHR_BUF);

	// 枠描画
	hOldPen = (HPEN)SelectObject(hCtlDC,GetStockObject(BLACK_PEN));
	hOldBrush = (HBRUSH)SelectObject(hCtlDC,GetStockObject(NULL_BRUSH));
	Rectangle(hCtlDC, rt.left,rt.top,rt.right,rt.bottom);     
	SelectObject(hCtlDC, hOldPen); 
	SelectObject(hCtlDC, hOldBrush); 
	
	// 本体描画
	hOldPen = (HPEN)SelectObject(hCtlDC,GetStockObject(BLACK_PEN));
	hOldBrush = (HBRUSH)SelectObject(hCtlDC,GetStockObject(GRAY_BRUSH));
	Rectangle(hCtlDC, rt.left+pos,rt.top+pos,rt.right-1+pos,rt.bottom-1+pos);     
	SelectObject(hCtlDC, hOldPen); 
	SelectObject(hCtlDC, hOldBrush); 

	// 明るい部分
	if(uCtlID == uFocus) hBrush = CreateSolidBrush(RGB(0,255,255));
	else hBrush = CreateSolidBrush(RGB(255,255,255));
	hOldPen = (HPEN)SelectObject(hCtlDC,GetStockObject(NULL_PEN));
	hOldBrush = (HBRUSH)SelectObject(hCtlDC,hBrush);
	Rectangle(hCtlDC, rt.left+2+pos,rt.top+2+pos,rt.right-2+pos,rt.top+5+pos);
	SelectObject(hCtlDC, hOldPen); 
	SelectObject(hCtlDC, hOldBrush); 
	DeleteObject(hBrush);


	// 文字描画
	TEXTMETRIC textMet; 
	LONG nStrHeight;

	hFont=CreateFont(10,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET
			,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"ＭＳ Ｐゴシック");
	hOldFont = (HFONT)SelectObject(hCtlDC,hFont);	
	GetTextMetrics(hCtlDC,&textMet);
	nStrHeight = textMet.tmAscent;
	rt.top += ((height-nStrHeight)/2)+1;
	rt.left += (pos+1);
	rt.right += (pos+1);
	
	// 文字描画
	SetBkMode(hCtlDC,TRANSPARENT);			 
    SetTextColor(hCtlDC,RGB(0,0,0)) ;
	DrawText(hCtlDC,szStr,strlen(szStr),&rt,DT_CENTER);
	rt.top -= 1;rt.left -= 1;rt.right -= 1;
    SetTextColor(hCtlDC,RGB(255,255,255)) ;
	DrawText(hCtlDC,szStr,strlen(szStr),&rt,DT_CENTER);
	// フォント削除
	SelectObject(hCtlDC,hOldFont);			  
	DeleteObject(hFont);
}


//EOF
