// easy Wave Cutter
// Copyright (c) 1999-2015 Tomoya Tokairin
// 設定ダイアログ関係

#include "common.h"




//------------------------------------------
// トラック情報をセット
BOOL SetTrackInfo(HWND hWnd,
				  LPEWCDATA lpEwcData,
				  LPSTR lpszInfo){
	
	DWORD i,dwFoo,dwFoo2;
	CHAR szStr[CHR_BUF];
	
	LoadTrackFile(hWnd,lpEwcData);

	for(i=1;i<=lpEwcData->dwSplitNum+1;i++){
		
		// 秒数取得
		dwFoo = (DWORD)(
			((i-1 == 0) ? 0 : lpEwcData->n64SplitMarkedPosByte[i-2])
			/lpEwcData->waveFmt.nAvgBytesPerSec);
		dwFoo2 = (DWORD)(
			((i-1 == lpEwcData->dwSplitNum) ? lpEwcData->n64WaveDataSize : lpEwcData->n64SplitMarkedPosByte[i-1])
			/lpEwcData->waveFmt.nAvgBytesPerSec);
		
		wsprintf(szStr,"[Track %3d ]  %d:%02d:%02d - %d:%02d:%02d (%d:%02d:%02d) %s\r\n",i,
			dwFoo/60/60,
			(dwFoo/60)%60,
			(dwFoo)%60,
			
			dwFoo2/60/60,
			(dwFoo2/60)%60,
			(dwFoo2)%60,
			
			(dwFoo2-dwFoo)/60/60,
			((dwFoo2-dwFoo)/60)%60,
			(dwFoo2-dwFoo)%60,
			
			lpEwcData->szTrackName[i-1]
			);
		
		strcat(lpszInfo,szStr);
	}

	return TRUE;
}



//-------------------------------------
// トラック設定ダイアログ
LRESULT CALLBACK TrackDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static LPEWCDATA lpEwcData;
	static char szSplitFile[MAX_PATH];
	FILE* f;
	DWORD i;
	char szStr[CHR_BUF];
	static char* lpszInfo = NULL;
	static DWORD dwCurTrack;
	LONGLONG n64StartByte;
	DWORD dwFoo,dwToTrack;
	LONGLONG n64Foo;

    switch (msg) {
		
	case WM_INITDIALOG:  // ダイアログ初期化

		lpEwcData = (LPEWCDATA)lp;
		
		// ダイアログを中心に移動
		SetDlgCenter(hWnd);

		lpszInfo = (char*)malloc(sizeof(char)*8192 + 1024);

		Edit_SetText(GetDlgItem(hWnd,IDC_EDITBASE),(LPCTSTR)lpEwcData->szBaseName);
		Edit_SetText(GetDlgItem(hWnd,IDC_EDITEXT),(LPCTSTR)lpEwcData->szExtName);

		Edit_SetText(GetDlgItem(hWnd,IDC_EDITDIR),(LPCTSTR)lpEwcData->szSaveDir);
	
		// トラック設定
		dwCurTrack = lpEwcData->dwCurTrack;
		SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_SETRANGE,(WPARAM)0,(LPARAM)MAKELONG((short)1,(short)lpEwcData->dwSplitNum+1));
		SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_SETPOS,(WPARAM)0L,(LPARAM)MAKELONG((short)dwCurTrack,0));

		// トラック結合設定
		SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK2),UDM_SETRANGE,(WPARAM)0,(LPARAM)MAKELONG((short)1,(short)lpEwcData->dwSplitNum+1));
		SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK2),UDM_SETPOS,(WPARAM)0L,(LPARAM)MAKELONG((short)dwCurTrack,0));

		// トラック一覧
		lpszInfo[0] = '\0';
		SetTrackInfo(hWnd,lpEwcData,lpszInfo);
		Edit_SetText(GetDlgItem(hWnd,IDC_EDITSETTING),(LPCTSTR)lpszInfo);
		SendMessage(GetDlgItem(hWnd,IDC_EDITSETTING),EM_LINESCROLL,(WPARAM)0,(LPARAM)(dwCurTrack > 3 ? dwCurTrack-4 : 0));

		if(lpEwcData->editSaveData.bOutfileIsNull) 
			Button_SetCheck(GetDlgItem(hWnd,IDC_CHKNULL), BST_CHECKED);

		break;

	case WM_VSCROLL:

		if((HWND)lp == GetDlgItem(hWnd,IDC_SPINTRACK))
			goto L_BTRELOAD; // 手を抜いて禁断の括弧越え goto 

		break;

		
	case WM_COMMAND:
		
		StopPlayWave();

		switch (LOWORD(wp)) {

		case IDC_BTLOAD: // 設定名ロード
			
			strcpy(lpszInfo,lpEwcData->szTrackFile);
			if(SelectLoadFile(hWnd,lpszInfo,
				"Text Files(*.txt)\0*.txt\0All Files(*.*)\0*.*\0\0",
				"txt",
				"トラック名設定ファイル読み込み")){
				strcpy(lpEwcData->szTrackFile,lpszInfo);
			}	
			else return TRUE;

			goto L_BTRELOAD;
			
		case IDC_BTCONNECT:  // トラック結合
			
			dwCurTrack = SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_GETPOS,(WPARAM)0,(LPARAM)0);
			dwToTrack = SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK2),UDM_GETPOS,(WPARAM)0,(LPARAM)0);

			if(dwCurTrack-1 < lpEwcData->dwSplitNum){
				dwToTrack = max(dwToTrack,dwCurTrack+1);
				wsprintf(lpszInfo,"トラック %d から %d まで結合しますか?",dwCurTrack,dwToTrack);
				if(MyMessageBox(hWnd, lpszInfo,"ewc", MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDNO) return TRUE;

				for(i=0;i<dwToTrack-dwCurTrack;i++){
					memmove(lpEwcData->n64SplitMarkedPosByte+(dwCurTrack-1),
						lpEwcData->n64SplitMarkedPosByte+(dwCurTrack-1)+1,
						sizeof(LONGLONG)*(lpEwcData->dwSplitNum-(dwCurTrack-1)-1));
					memmove(lpEwcData->lnSplitMarkedPos+(dwCurTrack-1),
						lpEwcData->lnSplitMarkedPos+(dwCurTrack-1)+1,
						sizeof(LONG)*(lpEwcData->dwSplitNum-(dwCurTrack-1)-1));
					
					lpEwcData->dwSplitNum--;
				}
			}

			goto L_BTRELOAD;


		case IDC_BTRESET: // リセット

			if(MyMessageBox(hWnd, "リセットしますか？", 
					"ewc", MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDNO) return TRUE;
			lpEwcData->szTrackFile[0] = '\0';

			goto L_BTRELOAD;
			
		
		case IDC_BTSPLITLOAD: // 分割マーク設定読み込み
			
			if(SelectLoadFile(hWnd,szSplitFile,
				"*.spt\0*.spt\0All Files(*.*)\0*.*\0\0",
				"spt",
				"分割マーク設定読み込み")){	

				f = fopen(szSplitFile,"rb");
				if(f)
				{
					fread(&dwFoo,1,sizeof(DWORD),f);
					if(dwFoo >  MAX_SPLITNUM) dwFoo = MAX_SPLITNUM;
					lpEwcData->dwSplitNum = 0;
					for(i=0;i<dwFoo;i++)
					{
						fread(&(lpEwcData->n64SplitMarkedPosByte[i]),1,sizeof(LONGLONG),f);
						fread(&(lpEwcData->lnSplitMarkedPos[i]),1,sizeof(LONG),f);
						if(lpEwcData->n64SplitMarkedPosByte[i] > lpEwcData->n64WaveDataSize) break;
						lpEwcData->dwSplitNum++;
					}
					
					fclose(f);
					goto L_BTRELOAD;

				}
				else
					MyMessageBox(hWnd,"設定ファイルを開けません。","Error",MB_OK|MB_ICONERROR);
				
			}
			
			break;
			
		case IDC_BTSPLITSAVE:  // 分割設定保存
			 
			if(lpEwcData->dwSplitNum){
				if(SelectSaveFile(hWnd,szSplitFile,
					"*.spt\0*.spt\0All Files(*.*)\0*.*\0\0",
					"spt",
					"分割マーク設定保存")){
					
					f = fopen(szSplitFile,"wb");
					if(f)
					{
						fwrite(&(lpEwcData->dwSplitNum),1,sizeof(DWORD),f);
						for(i=0;i<lpEwcData->dwSplitNum;i++){
							fwrite(&(lpEwcData->n64SplitMarkedPosByte[i]),1,sizeof(LONGLONG),f);
							fwrite(&(lpEwcData->lnSplitMarkedPos[i]),1,sizeof(LONG),f);

						}

						fclose(f);

						goto L_BTRELOAD;

					}
					else
						MyMessageBox(hWnd,"設定ファイルを開けません。","Error",MB_OK|MB_ICONERROR);
					
				}
			}

			break;
	

		case IDC_BTCUTLOAD: // カット設定読み込み
			
			if(SelectLoadFile(hWnd,szSplitFile,
				"*.ct\0*.ct\0All Files(*.*)\0*.*\0\0",
				"ct",
				"カット設定読み込み")){	

				f = fopen(szSplitFile,"rb");
				if(f)
				{
					fread(&(lpEwcData->n64WaveDataOffset),1,sizeof(LONGLONG),f);
					fread(&(lpEwcData->n64WaveDataSize),1,sizeof(LONGLONG),f);
					
					fclose(f);

					EndDialog(hWnd, IDC_BTCUTLOAD); 

				}
				else
					MyMessageBox(hWnd,"設定ファイルを開けません。","Error",MB_OK|MB_ICONERROR);
				
			}
			
			break;
			
		case IDC_BTCUTSAVE:  // カット設定保存
			
			if(SelectSaveFile(hWnd,szSplitFile,
				"*.ct\0*.ct\0All Files(*.*)\0*.*\0\0",
				"ct",
				"カット設定保存")){
				
				f = fopen(szSplitFile,"wb");
				if(f)
				{
					fwrite(&(lpEwcData->n64WaveDataOffset),1,sizeof(LONGLONG),f);
					fwrite(&(lpEwcData->n64WaveDataSize),1,sizeof(LONGLONG),f);
					
					fclose(f);
					
					goto L_BTRELOAD;
					
				}
				else
					MyMessageBox(hWnd,"設定ファイルを開けません。","Error",MB_OK|MB_ICONERROR);
				
			}

			break;


L_BTRELOAD:
			
		case IDC_BTRELOAD: // トラック設定名再読み込み

			Edit_GetText(GetDlgItem(hWnd,IDC_EDITBASE),(LPSTR)lpEwcData->szBaseName,CHR_BUF);
			Edit_GetText(GetDlgItem(hWnd,IDC_EDITEXT),(LPSTR)lpEwcData->szExtName,CHR_BUF);
			Edit_GetText(GetDlgItem(hWnd,IDC_EDITDIR),(LPSTR)lpEwcData->szSaveDir,MAX_PATH);
			if(IsDlgButtonChecked(hWnd,IDC_CHKNULL) == BST_CHECKED) lpEwcData->editSaveData.bOutfileIsNull = TRUE;
			else lpEwcData->editSaveData.bOutfileIsNull = FALSE;
			lpszInfo[0] = '\0';
			SetTrackInfo(hWnd,lpEwcData,lpszInfo);
			Edit_SetText(GetDlgItem(hWnd,IDC_EDITSETTING),(LPCTSTR)lpszInfo);
			dwCurTrack = SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_GETPOS,(WPARAM)0,(LPARAM)0);
			SendMessage(GetDlgItem(hWnd,IDC_EDITSETTING),EM_LINESCROLL,(WPARAM)0,(LPARAM)(dwCurTrack > 3 ? dwCurTrack-4 : 0));
			
			dwFoo = lpEwcData->dwSplitNum+1;
			SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_SETRANGE,(WPARAM)0,(LPARAM)MAKELONG(1,(short)dwFoo));
			SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_SETPOS,(WPARAM)0L,(LPARAM)MAKELONG((short)dwCurTrack,0));

			SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK2),UDM_SETRANGE,(WPARAM)0,(LPARAM)MAKELONG(1,(short)dwFoo));
			SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK2),UDM_SETPOS,(WPARAM)0L,(LPARAM)MAKELONG((short)dwCurTrack,0));
			
			return FALSE;
			
		case IDC_BTPLAYSTOP:  // 再生
			
			Edit_GetText(GetDlgItem(hWnd,IDC_EDITSETTING),(LPSTR)lpszInfo,4096);
			Edit_SetText(GetDlgItem(hWnd,IDC_EDITSETTING),(LPCTSTR)lpszInfo);
			dwCurTrack = SendMessage(GetDlgItem(hWnd,IDC_SPINTRACK),UDM_GETPOS,(WPARAM)0,(LPARAM)0);
			SendMessage(GetDlgItem(hWnd,IDC_EDITSETTING),EM_LINESCROLL,(WPARAM)0,(LPARAM)(dwCurTrack > 3 ? dwCurTrack-4 : 0));
			if(dwCurTrack>1)
				n64StartByte = lpEwcData->n64SplitMarkedPosByte[dwCurTrack-2];
			else n64StartByte = 0;

			 // 再生サイズ設定 max 10 秒
			if(dwCurTrack == 1) 
				n64Foo 
				= min(
				10 * lpEwcData->waveFmt.nAvgBytesPerSec,
				lpEwcData->n64SplitMarkedPosByte[0]
				);
			else if(dwCurTrack != lpEwcData->dwSplitNum+1) 
				n64Foo = 
				min(
				lpEwcData->n64SplitMarkedPosByte[dwCurTrack-2] +10 * lpEwcData->waveFmt.nAvgBytesPerSec,
				lpEwcData->n64SplitMarkedPosByte[dwCurTrack-1]
				);
			else n64Foo = lpEwcData->n64WaveDataSize;

			// 再生開始
			PlayWaveByte(hWnd,
				lpEwcData->uDeviceID,
				lpEwcData->szLoadFile,
				n64StartByte,
				lpEwcData->hdFile,
				lpEwcData->waveFmt,
				n64Foo,
				lpEwcData->n64WaveDataOffset
				);
			
			break;



		case IDC_BTREFF: //　参照

			Edit_GetText(GetDlgItem(hWnd,IDC_EDITDIR),(LPSTR)szStr,MAX_PATH);
			SelectDir(hWnd,szStr,"保存先");
			Edit_SetText(GetDlgItem(hWnd,IDC_EDITDIR),(LPCTSTR)szStr);

			break;

			
		case IDOK:

			Edit_GetText(GetDlgItem(hWnd,IDC_EDITBASE),(LPSTR)lpEwcData->szBaseName,CHR_BUF);
			Edit_GetText(GetDlgItem(hWnd,IDC_EDITEXT),(LPSTR)lpEwcData->szExtName,CHR_BUF);
			Edit_GetText(GetDlgItem(hWnd,IDC_EDITDIR),(LPSTR)lpEwcData->szSaveDir,MAX_PATH);
			if(IsDlgButtonChecked(hWnd,IDC_CHKNULL) == BST_CHECKED) lpEwcData->editSaveData.bOutfileIsNull = TRUE;
			else lpEwcData->editSaveData.bOutfileIsNull = FALSE;

			if(lpszInfo) free(lpszInfo);
			EndDialog(hWnd, IDOK); 
			break;
			
		case IDCANCEL: 
			if(lpszInfo) free(lpszInfo);
			EndDialog(hWnd, IDCANCEL); 
		}
		
		break;
		
		default:
			return FALSE;
    }
    return TRUE;
}






//-------------------------------------
// 編集メニュー選択プロシージャ
LRESULT CALLBACK EditMenuDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	
	
	// 編集データ
	static LPEWCDATA lpEwcData; 
	static LPEDITSAVEDATA lpEditSaveData; 
	
	// 保存用
	static EDITSAVEDATA editSaveData; 
	CHAR szData[MAX_WAVFLTOPTBUF];
	HANDLE hdFile;
	DWORD dwByte;
	
    switch (msg) {
		
	case WM_INITDIALOG:  // ダイアログ初期化
		
		lpEwcData = (LPEWCDATA)lp;
		lpEditSaveData = &(lpEwcData->editSaveData);
		CopyMemory(&editSaveData,lpEditSaveData,sizeof(EDITSAVEDATA));
		Edit_SetText(GetDlgItem(hWnd,IDC_EDITSETTING),(LPCTSTR)editSaveData.szOption);

		lpEwcData->bSplit = FALSE;
		lpEwcData->bCutCm = FALSE;
		lpEwcData->bCutTrack = FALSE; 

		// ダイアログを中心に移動
		SetDlgCenter(hWnd);
		
		break;
		
		case WM_COMMAND:

			switch (LOWORD(wp)) {
				
			case IDC_BTSTART: // 保存
				goto L_START;
			case IDC_BTSTARTCUTCM: // CM カット
				if(lpEwcData->dwSplitNum) lpEwcData->bCutCm = TRUE;
				goto L_START;
			case IDC_BTSPLIT: // 分割
				if(lpEwcData->dwSplitNum) lpEwcData->bSplit = TRUE;
				goto L_START;
			case IDC_BTWAVEOUT: // 試聴
			case IDC_BTSTARTCUT: // 切り抜き
				lpEwcData->bCutTrack = TRUE; 
L_START:
			case IDOK:

				Edit_GetText(GetDlgItem(hWnd,IDC_EDITSETTING),(LPSTR)editSaveData.szOption,MAX_WAVFLTOPTBUF);
				CopyMemory(lpEditSaveData,&editSaveData,sizeof(EDITSAVEDATA));

				if(LOWORD(wp) == IDC_BTWAVEOUT)
					EndDialog(hWnd, IDC_BTWAVEOUT); // 再生開始
				else if(LOWORD(wp) == IDOK)
					EndDialog(hWnd, IDCANCEL);
				else 
					EndDialog(hWnd, IDC_BTSTART); // コピー開始

				return TRUE;
				
			case IDCANCEL: 

				 EndDialog(hWnd, IDCANCEL);
				
				return TRUE;
				
			case IDC_BTSAVESETTING: // 設定保存
				
				if(SelectSaveFile(hWnd ,lpEwcData->szSettingFileName,
					"Text Files(*.txt)\0*.txt\0All Files(*.*)\0*.*\0\0",
					"txt",
					"設定保存"
					)){

					hdFile = CreateFile(lpEwcData->szSettingFileName,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL); 
					SetFilePointer(hdFile, 0, 0, FILE_BEGIN);
					Edit_GetText(GetDlgItem(hWnd,IDC_EDITSETTING),(LPSTR)szData,MAX_WAVFLTOPTBUF);
					WriteFile(hdFile,szData,strlen(szData),&dwByte,NULL);
					CloseHandle(hdFile);
					MyMessageBox(hWnd, "現在の設定をファイルに保存しました。", "ewc", MB_OK|MB_ICONINFORMATION);
				}

				return TRUE;

			case IDC_BTLOADSETTING: // 設定読み込み
				
				if(SelectLoadFile(hWnd ,lpEwcData->szSettingFileName,
					"Text Files(*.txt)\0*.txt\0All Files(*.*)\0*.*\0\0",
					"txt",
					"設定読み込み")){

					hdFile = CreateFile(lpEwcData->szSettingFileName,GENERIC_READ,0,0,
						OPEN_EXISTING, FILE_ATTRIBUTE_READONLY,NULL); 
					SetFilePointer(hdFile, 0, 0, FILE_BEGIN);
					memset(szData,0,MAX_WAVFLTOPTBUF);
					ReadFile(hdFile,szData,MAX_WAVFLTOPTBUF,&dwByte,NULL);
					Edit_SetText(GetDlgItem(hWnd,IDC_EDITSETTING),(LPCTSTR)szData);
					CloseHandle(hdFile);
				}				
				
				return TRUE;


			case IDC_BTCLEAR: // クリア
				szData[0] = '\0';
				Edit_SetText(GetDlgItem(hWnd,IDC_EDITSETTING),(LPCTSTR)szData);
				return TRUE;


		case IDC_BTHELP: // WAVFLT2 ヘルプ
			
			CHAR path[MAX_PATH],driveName[MAX_PATH],pathName[MAX_PATH],str[MAX_PATH];
			
			// ヘルプファイルのパス取得
			GetModuleFileName(NULL,str,CHR_BUF);
			_splitpath(str,driveName,pathName,NULL, NULL);	
			wsprintf(path,"%s%s",driveName,pathName);
			wsprintf(str,"%s%s",path,"waveflt2.htm");
			
			// 呼び出し
			if((WORD)ShellExecute(hWnd,NULL,str,NULL,path,SW_SHOWNORMAL) == ERROR_FILE_NOT_FOUND ){
				MyMessageBox(hWnd,"ヘルプファイル(waveflt2.htm)が見つかりませんでした。","Error",MB_OK|MB_ICONERROR);
			}
			
			return TRUE;

			}
			
			break;
			
			default:
				return FALSE;
    }
    return TRUE;
}





//-------------------------------------------------------------------
//デバイス選択ダイアログのプロシージャ
LRESULT CALLBACK SettingProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static HWND hPlayWnd;
	static LPEWCDATA lpEwcData;
	UINT i;
	RECT rt;
	
    switch (msg) {
		
	case WM_INITDIALOG:  // ダイアログ初期化
		
		// ダイアログを中心に移動
		SetDlgCenter(hWnd);
		
		lpEwcData = (LPEWCDATA)lp;
		hPlayWnd = GetDlgItem(hWnd,IDC_CMBPLAY);
		
		if(lpEwcData->bShiftLock) Button_SetCheck(GetDlgItem(hWnd,IDC_CHKSHIFT), BST_CHECKED);
		if(lpEwcData->bShowConsole) Button_SetCheck(GetDlgItem(hWnd,IDC_CHKSHOWCON), BST_CHECKED);
		
		// 再生デバイス名セット
		GetWindowRect(hPlayWnd,&rt);
		SetWindowPos(hPlayWnd,NULL,0,0,
			rt.right-rt.left,(rt.bottom-rt.top)*3,SWP_NOMOVE|SWP_NOREPOSITION);
		ComboBox_InsertString(hPlayWnd,0,"自動選択");
		for(i=0;i<lpEwcData->uDevNum;i++){
			if(lpEwcData->waveOutCaps[i].wChannels > 0)
				ComboBox_InsertString(hPlayWnd,i+1,lpEwcData->waveOutCaps[i].szPname);
		}
		ComboBox_SetCurSel(hPlayWnd,(lpEwcData->uDeviceID!=WAVE_MAPPER)*(lpEwcData->uDeviceID+1));
	
		return TRUE;
		
		//-------------------------------------------------------------------
		
	case WM_COMMAND:
		
		switch (LOWORD(wp)) {
			
		case IDOK: // OK 押した
			
			if(IsDlgButtonChecked(hWnd,IDC_CHKSHIFT) == BST_CHECKED) lpEwcData->bShiftLock = TRUE;
			else lpEwcData->bShiftLock = FALSE;
			if(IsDlgButtonChecked(hWnd,IDC_CHKSHOWCON) == BST_CHECKED) lpEwcData->bShowConsole = TRUE;
			else lpEwcData->bShowConsole = FALSE;

			// 再生デバイス
			i = ComboBox_GetCurSel(hPlayWnd); 
			lpEwcData->uDeviceID = WAVE_MAPPER*(i==0)+(i!=0)*(i-1);
			
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
			
		case IDCANCEL: 
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
			
		case IDC_BTHELP: // ヘルプ
			
			CHAR path[MAX_PATH],driveName[MAX_PATH],pathName[MAX_PATH],str[MAX_PATH];
			
			// ヘルプファイルのパス取得
			GetModuleFileName(NULL,str,CHR_BUF);
			_splitpath(str,driveName,pathName,NULL, NULL);	
			wsprintf(path,"%s%s",driveName,pathName);
			wsprintf(str,"%s%s",path,"ewc.htm");
			
			// 呼び出し
			if((WORD)ShellExecute(hWnd,NULL,str,NULL,path,SW_SHOWNORMAL) == ERROR_FILE_NOT_FOUND ){
				MyMessageBox(hWnd,"ヘルプファイル(ewc.htm)が見つかりませんでした。","Error",MB_OK|MB_ICONERROR);
			}
			
			
			return TRUE;

		case IDC_BTMALTI: // マルチメディア
			
			// プロパティ呼び出し
			STARTUPINFO startInfo;
			PROCESS_INFORMATION proInfo;
			memset(&startInfo,0,sizeof(STARTUPINFO));
			startInfo.cb = sizeof(STARTUPINFO);
			if(!CreateProcess(NULL,"RUNDLL32.EXE SHELL32.DLL,Control_RunDLL Mmsys.cpl"
				,NULL,NULL,FALSE,0,
				NULL,NULL,&startInfo,&proInfo)){
				MyMessageBox(hWnd,"マルチメディアプロパティの起動に失敗しました。",
					"Error",MB_OK|MB_ICONERROR);
			}
			
			return TRUE;
			
		}	
		
		default:
			break;
	}
    return FALSE;
}



//-------------------------------------------------------------------
//ファイル情報ダイアログのプロシージャ
LRESULT CALLBACK InfoProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{

	
    switch (msg) {
		
	case WM_INITDIALOG:  // ダイアログ初期化
		
		// ダイアログを中心に移動
		SetDlgCenter(hWnd);

		Edit_SetText(GetDlgItem(hWnd,IDC_EDITSETTING),(LPCTSTR)lp);
		
	
		return TRUE;
		
		
	case WM_COMMAND:
		
		switch (LOWORD(wp)) {
			
		case IDOK: // OK 押した
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
			
		case IDCANCEL: 
			EndDialog(hWnd, IDCANCEL);
			
		}	
		
		default:
			break;
	}
    return FALSE;
}

//EOF
