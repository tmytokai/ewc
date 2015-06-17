// WAVEFLT FRONT END  easy Wave Cutter ���C��
// Copyright (c) 1999-2015 Tomoya Tokairin
//
// �{�v���O�����̂��ׂāA�܂��͈ꕔ�� GPL �ɏ]���čĔЕz�܂��͕ύX����
// ���Ƃ��ł��܂��B�ڍׂɂ��Ă�GNU ��ʌ��L�g�p�����������ǂ݂��������B


#include "common.h"


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPreInst,
                   LPSTR lpszCmdLine, int nCmdShow)
{
	HWND hWnd;
    MSG msg;
    WNDCLASS wndClass;
	HACCEL hAccel = NULL;
	
	DWORD i;
	
	// �ҏW�f�[�^�p�ϐ�
	EWCDATA ewcData;
	
	// �R�}���h���C���p
	CHAR argv[MAX_ARGC][CHR_BUF];
	int argc;

	// �E�B���h�E�o�^
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


	// EWCDATA �\���̏����ݒ�
	ZeroMemory(&ewcData,sizeof(EWCDATA));
	
	// �T�E���h�f�o�C�X�̏��擾
	ewcData.uDevNum = GetWaveOutDevCap(ewcData.waveOutCaps,MAXDEVICENUMBER);
	

	// �ݒ�t�@�C���ǂݍ���

	// �ݒ�t�@�C���Z�b�g
	CHAR szStr[MAX_PATH];
	CHAR szDriveName[MAX_PATH],szPathName[MAX_PATH];
	GetModuleFileName(NULL,szStr,MAX_PATH); 
	_splitpath(szStr,szDriveName,szPathName,NULL,NULL);	
	wsprintf(ewcData.szIniDatFile,"%s%sewc.dat",szDriveName,szPathName);
	ReadIniFile(&ewcData,ewcData.szIniDatFile);
	
	// �R�}���h���C���擾
	argc = GetArgv(lpszCmdLine,argv,1);
	
	// �t�@�C�����Z�b�g
	if(argc > 1) if(ShortToLongName(argv[1])) wsprintf(ewcData.szIniFileName,argv[1]);
	
	// �f�o�C�X�Z�b�g
	if(strcmp(argv[2],"-dev") == 0){
		ewcData.uDeviceID = atoi(argv[3]);
	}
	
	// �g���b�N���ݒ�
	ewcData.szTrackFile[0] = '\0';
	strcpy(ewcData.szBaseName,"track");
	strcpy(ewcData.szExtName,"wav");
	ewcData.szTrackName[0] = (CHAR*)malloc(sizeof(CHAR)*MAX_SPLITNUM*CHR_BUF+1024);
	for(i=1;i<MAX_SPLITNUM;i++) ewcData.szTrackName[i] = ewcData.szTrackName[0] + i*CHR_BUF;
	for(i=0;i<MAX_SPLITNUM;i++) wsprintf(ewcData.szTrackName[i],"%s-%03d.%s",ewcData.szBaseName,i+1,ewcData.szExtName);

	// �E�B���h�E�쐬
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

	// �A�N�Z���[�^�[�e�[�u���̃��[�h
	hAccel = LoadAccelerators(hInst,MAKEINTRESOURCE(IDR_ACCELERATOR));

	if(hAccel == NULL) 
		MyMessageBox(NULL,"�A�N�Z���[�^�[�e�[�u���̃n���h��\n�擾�Ɏ��s���܂����B"
		,"error",MB_OK|MB_ICONERROR);
	else 
		while (GetMessage(&msg, NULL, 0, 0)) {	// ���b�Z�[�W���[�v
			if(!TranslateAccelerator(hWnd,hAccel,&msg))  // �A�N�Z���[�^�[
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}


	// �ݒ�t�@�C���ۑ�
	SaveIniFile(&ewcData,ewcData.szIniDatFile);

	// �������J��
	free(ewcData.szTrackName[0]);

	
	return 0;
}


//EOF