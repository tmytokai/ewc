// easy Wave Cutter
// Copyright (c) 1999-2015 Tomoya Tokairin
// WAVE 関係(共通)
//

#include <windows.h>
#include <mmsystem.h>
#include <math.h>

#define USEWIN32API 
#define CHR_BUF 256


#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 0x0001
#endif
#ifndef WAVE_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_IEEE_FLOAT 0x0003
#endif

// メッセージボックス
int MyMessageBox(HWND,LPSTR,LPSTR,UINT);



//-------------------------------------------------------------------
// set WAVE format
void SetWaveFmt(LPWAVEFORMATEX lpWaveFmt,WORD waveChn,
				DWORD waveRate,	WORD waveBit, WORD wTag)
{
	
	lpWaveFmt->wFormatTag = wTag;
	lpWaveFmt->nChannels = waveChn;  
	lpWaveFmt->nSamplesPerSec = waveRate; 
	lpWaveFmt->nAvgBytesPerSec = (DWORD)(waveRate*waveChn*waveBit/8); // byte/sec
	lpWaveFmt->nBlockAlign = (WORD)(waveChn*waveBit/8); // byte/block
	lpWaveFmt->wBitsPerSample = waveBit; 
	lpWaveFmt->cbSize = (WORD)0; // no extention
}


//-------------------------------------------------------------------
// wave ヘッダのバッファをセットする関数
BOOL SetWaveHdr(HWND hWnd,LPWAVEHDR lpWaveHdr, LONG nHdrNumber,DWORD dwWaveBlockByte)
{
	LONG i;
	LPBYTE lpWaveData;
	
	// バッファメモリを確保
	lpWaveData =(LPBYTE)GlobalAlloc(GPTR,sizeof(BYTE)*dwWaveBlockByte*nHdrNumber);
	
	if(lpWaveData == NULL){
		MyMessageBox(hWnd, "メモリの確保に失敗しました。",
			"Error", MB_OK|MB_SETFOREGROUND|MB_ICONERROR);
		return FALSE;
	}
	
	// wave ヘッダー設定
	for(i=0;i<nHdrNumber;i++){
		memset(&lpWaveHdr[i], 0, sizeof(WAVEHDR));
		lpWaveHdr[i].lpData  = (LPSTR)lpWaveData+dwWaveBlockByte*i;	// データ格納領域確保
		lpWaveHdr[i].dwBufferLength = dwWaveBlockByte; // データサイズ
	}
	
	return TRUE;
}



//-------------------------------------------------------------------
// wave ヘッダのバッファ削除関数
BOOL DelWaveHdr(HWND hWnd,LPWAVEHDR lpWaveHdr)
{
	
	if(lpWaveHdr[0].lpData != NULL){
		if(GlobalFree(lpWaveHdr[0].lpData)!=NULL){
			MyMessageBox(hWnd, "メモリの開放に失敗しました。",
				"Error", MB_OK|MB_SETFOREGROUND|MB_ICONERROR);
			return FALSE;
		}
		else lpWaveHdr[0].lpData=NULL;
	}
	
	return TRUE;
}





//-------------------------------------------------------------------
// change byte-data to double-data 
void WaveLevel(double dLevel[2],  // output of left and right
			   BYTE* lpWaveData,  // input
			   WAVEFORMATEX waveFmt){ 
	
	long i;
	short data[2];
	long nData[2];
	double dData[2];
	float fData[2];

	nData[1] = 0;
	
	if(waveFmt.wBitsPerSample==8){ // 8 bit
		for(i=0;i<waveFmt.nChannels;i++) nData[i] = (long)lpWaveData[i]-0x80;
	}
	else if(waveFmt.wBitsPerSample==16){	// 16 bit
		memcpy(data,lpWaveData,sizeof(short)*waveFmt.nChannels);
		nData[0] = (long)data[0];
		nData[1] = (long)data[1];
	}
	else if(waveFmt.wBitsPerSample==24){	// 24 bit
		for(i=0;i<waveFmt.nChannels;i++){
			nData[i] = 0;
			memcpy((LPBYTE)(nData+i)+1,lpWaveData+3*i,3);
			nData[i] /= 256;
		}
	}
	else if(waveFmt.wBitsPerSample==32 && waveFmt.wFormatTag == WAVE_FORMAT_PCM){	// 32 bit long
		memcpy(nData,lpWaveData,sizeof(long)*waveFmt.nChannels);
	}
	else if(waveFmt.wBitsPerSample==32 && waveFmt.wFormatTag == 3){	// 32 bit float
		memcpy(fData,lpWaveData,sizeof(double)*waveFmt.nChannels);
		dLevel[0] = fData[0];
		dLevel[1] = fData[1];
		return;
	}	
	else if(waveFmt.wBitsPerSample==64){	// 64 bit double
		memcpy(dData,lpWaveData,sizeof(double)*waveFmt.nChannels);
		dLevel[0] = dData[0];
		dLevel[1] = dData[1];
		return;
	}

	dLevel[0] = (double)nData[0];
	dLevel[1] = (double)nData[1];
}


//---------------------------------------------------------
// 音量の平均をとる関数
void WaveLevelAverage(double dLevel[2],  // output of left and right
			   BYTE* lpWaveData,  // input
			   WAVEFORMATEX waveFmt,
			   DWORD dwSize
			   ){

	DWORD i;
	double dWaveLevel[2];

	dLevel[0]=dLevel[1]=0;
	for(i=0;i<dwSize;i+=waveFmt.nBlockAlign){
		WaveLevel(dWaveLevel,lpWaveData+i,waveFmt);
		dLevel[0]+=fabs(dWaveLevel[0]);
		dLevel[1]+=fabs(dWaveLevel[1]);
	}

	dLevel[0] /= (double)(dwSize/waveFmt.nBlockAlign);
	dLevel[1] /= (double)(dwSize/waveFmt.nBlockAlign);
}



//-----------------------------------------------
// get max level of wave 
double GetMaxWaveLevel(WAVEFORMATEX waveFmt){

	double dRet;

	switch(waveFmt.wBitsPerSample){
	case 8: dRet = 127; break;
	case 16: dRet = 32767; break;
	case 24: dRet = 8388607; break;
	case 64: dRet = 1; break;
	case 32: 
		if(waveFmt.wFormatTag == WAVE_FORMAT_PCM) dRet = 2147483647;
		else dRet = 1;
		break;
	}

	return dRet;
}



//--------------------------------------------------------
// copy byte data to double data
void CopyBufferBtoD(BYTE* lpBuffer,  // input, buffer (BYTE*)
						DWORD dwByte, // size of lpBuffer

						double* lpFilterBuf[2], // output, buffer of wave data (double*) (L-R)
						LPDWORD lpdwPointsInBuf, // points of data in lpFilterBuf
						WAVEFORMATEX waveFmt
						){
	
	DWORD i,dwChn,dwPos; 
	BYTE* lpBuffer2;
	LONG nVal;
	DWORD dwFilterBufSize; 

	dwFilterBufSize = dwByte / waveFmt.nBlockAlign;
	
	if(waveFmt.wBitsPerSample == 16){  // 16 bit
		
		for(dwChn = 0 ; dwChn < waveFmt.nChannels  ; dwChn++){
			
			dwPos = 0;
			lpBuffer2 = lpBuffer+sizeof(short)*dwChn;
			memset(lpFilterBuf[dwChn],0,sizeof(double)*dwFilterBufSize);
			
			for(i=0;i< dwByte;i+=waveFmt.nBlockAlign){
				lpFilterBuf[dwChn][dwPos] = (double)(*((short*)lpBuffer2));
				dwPos++;
				lpBuffer2 += waveFmt.nBlockAlign;
			}

		}
	}
	else if(waveFmt.wBitsPerSample == 8){ // 8 bit
		
		for(dwChn = 0 ; dwChn < waveFmt.nChannels  ; dwChn++){
			
			dwPos = 0;
			memset(lpFilterBuf[dwChn],0,sizeof(double)*dwFilterBufSize);
			
			for(i=0;i< dwByte;i+=waveFmt.nBlockAlign){
				lpFilterBuf[dwChn][dwPos] = (double)((int)lpBuffer[i+dwChn]-0x80);
				dwPos++;
			}
		}

	}
	else if(waveFmt.wBitsPerSample == 24){  // 24 bit
		
		for(dwChn = 0 ; dwChn < waveFmt.nChannels  ; dwChn++){
			
			dwPos = 0;
			lpBuffer2 = lpBuffer+3*dwChn;
			memset(lpFilterBuf[dwChn],0,sizeof(double)*dwFilterBufSize);
			
			for(i=0;i< dwByte;i+=waveFmt.nBlockAlign){

				nVal = 0;
				memcpy((LPBYTE)(&nVal)+1,lpBuffer2,3);
				nVal /= 256;

				lpFilterBuf[dwChn][dwPos] = (double)nVal;
				dwPos ++;
				lpBuffer2 += waveFmt.nBlockAlign;
			}

		}
	}
	else if(waveFmt.wBitsPerSample == 32 && waveFmt.wFormatTag == WAVE_FORMAT_PCM){  // 32 bit long
		
		for(dwChn = 0 ; dwChn < waveFmt.nChannels  ; dwChn++){
			
			dwPos = 0;
			lpBuffer2 = lpBuffer+sizeof(long)*dwChn;
			memset(lpFilterBuf[dwChn],0,sizeof(double)*dwFilterBufSize);
			
			for(i=0;i< dwByte;i+=waveFmt.nBlockAlign){
				lpFilterBuf[dwChn][dwPos] = (double)(*((long*)lpBuffer2));
				dwPos ++;
				lpBuffer2 += waveFmt.nBlockAlign;
			}

		}
	}
	else if(waveFmt.wBitsPerSample == 32 && waveFmt.wFormatTag == WAVE_FORMAT_IEEE_FLOAT){  // 32 bit float
		
		for(dwChn = 0 ; dwChn < waveFmt.nChannels  ; dwChn++){
			
			dwPos = 0;
			lpBuffer2 = lpBuffer+sizeof(float)*dwChn;
			memset(lpFilterBuf[dwChn],0,sizeof(double)*dwFilterBufSize);
			
			for(i=0;i< dwByte;i+=waveFmt.nBlockAlign){
				lpFilterBuf[dwChn][dwPos] = (double)(*((float*)lpBuffer2));
				dwPos ++;
				lpBuffer2 += waveFmt.nBlockAlign;
			}

		}
	}
	else if(waveFmt.wBitsPerSample == 64){  // 64 bit(double)
		
		for(dwChn = 0 ; dwChn < waveFmt.nChannels  ; dwChn++){
			
			dwPos = 0;
			lpBuffer2 = lpBuffer+sizeof(double)*dwChn;
			memset(lpFilterBuf[dwChn],0,sizeof(double)*dwFilterBufSize);
			
			for(i=0;i< dwByte;i+=waveFmt.nBlockAlign){
				lpFilterBuf[dwChn][dwPos] = (double)(*((double*)lpBuffer2));
				dwPos ++;
				lpBuffer2 += waveFmt.nBlockAlign;
			}

		}
	}

	*lpdwPointsInBuf = dwByte/waveFmt.nBlockAlign;
}


//----------------------------
// copy double data to byte buffer
void CopyBufferDtoB(BYTE* lpBuffer,  // output, buffer(BYTE*)

					double* lpFilterBuf[2], // input, buffer(double*)
				  DWORD dwPointsInBuf, // points in lpFilterBuf
				  WAVEFORMATEX waveFmt,

				  DWORD* lpdwSaturate  // number of saturation
				  ){
	
	DWORD i,dwChn,dwPos; 
	BYTE* lpBuffer2;
	double dFoo,dFoo2;
	double dMaxLevel;
	long nOut;
	float fOut;
	
	// if bit == 64 or 32-float, then don't check saturation
	if(waveFmt.wBitsPerSample == 64){  // 64 bit
		
		for(dwChn = 0 ; dwChn < waveFmt.nChannels  ; dwChn++){
			dwPos = 0;
			lpBuffer2 = lpBuffer + waveFmt.wBitsPerSample/8 * dwChn;

			for(i=0;i< dwPointsInBuf;i++,dwPos++){
				memcpy(lpBuffer2,lpFilterBuf[dwChn]+dwPos,sizeof(double));
				lpBuffer2 += waveFmt.nBlockAlign;
			}
		}
		
	}
	else if(waveFmt.wBitsPerSample == 32 && waveFmt.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) // 32-float
	{ 
		for(dwChn = 0 ; dwChn < waveFmt.nChannels  ; dwChn++){
			dwPos = 0;
			lpBuffer2 = lpBuffer + waveFmt.wBitsPerSample/8 * dwChn;

			for(i=0;i< dwPointsInBuf;i++,dwPos++){
				fOut = (float)(lpFilterBuf[dwChn][dwPos]);
				memcpy(lpBuffer2,&fOut,sizeof(float));
				lpBuffer2 += waveFmt.nBlockAlign;
			}
		}
	}
	else  // 8,16,24,32-long
	{
		dMaxLevel = GetMaxWaveLevel(waveFmt);
		
		for(dwChn = 0 ; dwChn < waveFmt.nChannels  ; dwChn++){

			dwPos = 0;
			lpBuffer2 = lpBuffer + waveFmt.wBitsPerSample/8 *dwChn;

			for(i=0;i< dwPointsInBuf;i++,dwPos++){
				
				// check saturation
				dFoo = lpFilterBuf[dwChn][dwPos];
				dFoo2 = fabs(dFoo);
				if(dFoo2 > dMaxLevel){
					if(lpdwSaturate) (*lpdwSaturate)++;
					dFoo = (dFoo > 0) ? (dMaxLevel-1) : -(dMaxLevel-1);
				}
				nOut = (LONG)dFoo;

				// copy data to buffer
				if(waveFmt.wBitsPerSample == 16) *((short*)lpBuffer2) = (short)nOut;
				else if(waveFmt.wBitsPerSample == 8) *lpBuffer2 = (BYTE)(nOut+0x80);
				else if(waveFmt.wBitsPerSample == 24) memcpy(lpBuffer2,&nOut,3);
				else if(waveFmt.wBitsPerSample == 32 && waveFmt.wFormatTag == WAVE_FORMAT_PCM) 
					memcpy(lpBuffer2,&nOut,sizeof(long)); // 32 bit long
				
				lpBuffer2 += waveFmt.nBlockAlign;
			}

			
		}
		
	}
	
}



//-------------------------------------
// change bits of data
DWORD ChangeBits(BYTE* lpBuf,  // input,output, buffer(BYTE*)
				DWORD dwByte, // byte, size of lpBuf
				WAVEFORMATEX waveFmtIn,
				WAVEFORMATEX waveFmtOut,
				double* lpFilterBuf[2]) // buffer
{

	DWORD dwPoints,i,i2,dwSat;
	double dChangeBit;

	dChangeBit = GetMaxWaveLevel(waveFmtOut);
	dChangeBit /= GetMaxWaveLevel(waveFmtIn);

	CopyBufferBtoD(lpBuf,dwByte,lpFilterBuf,&dwPoints,waveFmtIn);
	
	for(i=0;i<waveFmtIn.nChannels;i++){
		for(i2=0;i2<dwPoints;i2++){
			lpFilterBuf[i][i2] *= dChangeBit;
		}
	}

	CopyBufferDtoB(lpBuf,lpFilterBuf,dwPoints,waveFmtOut,&dwSat);


	return (dwByte/waveFmtIn.nBlockAlign*waveFmtOut.nBlockAlign);
}




//-------------------------------------
// search for head of chunk, 
BOOL SearchChunk(HANDLE hdFile,char* szChunk, DWORD dwSize, 
				 BOOL bGetChkSize, // get chunk size
				 BOOL bStdin, // no seek
				 LPDWORD lpdwChkSize,LPDWORD lpdwOffset){

	DWORD dwPoint;
	DWORD dwByte;
	BYTE byteRead;
	char szCheck[5];
#ifdef USEWIN32API
	LARGE_INTEGER LI; 
#endif

	memset(szCheck,0,5);
	for(dwPoint = 0; dwPoint < dwSize; dwPoint++){
		
		if(!bStdin){
#ifdef USEWIN32API 
			LI.QuadPart = dwPoint;
			SetFilePointer(hdFile,LI.LowPart, &LI.HighPart,FILE_BEGIN);
#else
			fseek(hdFile,dwPoint,SEEK_SET);
#endif
		}
		
#ifdef USEWIN32API 
		ReadFile(hdFile,&byteRead,1,&dwByte, NULL);
#else  
		dwByte = fread(&byteRead,1,1,hdFile);
#endif
		if(dwByte != 1) return false;
		*lpdwOffset += dwByte;  

		if(byteRead == szChunk[0]){
			
#ifdef USEWIN32API 
			ReadFile(hdFile,szCheck,3,&dwByte, NULL);
#else  
			dwByte = fread(szCheck,1,3,hdFile);
#endif
			if(dwByte != 3) return false;
			*lpdwOffset += dwByte;  
			
			if(strcmp(szCheck,szChunk+1)==0) break;
		}

	}
	
	if(dwPoint == dwSize) return false;

	// chunk size
	if(bGetChkSize){
#ifdef USEWIN32API 
		ReadFile(hdFile,lpdwChkSize,sizeof(DWORD),&dwByte, NULL);
#else  
		dwByte = fread(lpdwChkSize,1,sizeof(DWORD),hdFile);
#endif
		if(dwByte != sizeof(DWORD)) return false;
		*lpdwOffset += dwByte;  
	}

	if(!bStdin){
		*lpdwOffset = dwPoint + 4;
		if(bGetChkSize) *lpdwOffset += sizeof(DWORD);
	}


	return true;
}



//------------------------------------------------------------------
// check whether format is valid
BOOL CheckWaveFormat(LPWAVEFORMATEX lpWaveFmt,char* lpszErr){

	DWORD waveRate,avgByte;	
	WORD waveChn,waveBit;

	if(lpWaveFmt->wFormatTag != WAVE_FORMAT_IEEE_FLOAT 
		|| lpWaveFmt->wFormatTag != WAVE_FORMAT_PCM // 
		){
		if(lpszErr) strcpy(lpszErr,"This data is not PCM format.\n");
	}
	
	waveChn = lpWaveFmt->nChannels;  
	waveRate = lpWaveFmt->nSamplesPerSec; 
	waveBit = lpWaveFmt->wBitsPerSample; 
	avgByte = lpWaveFmt->nAvgBytesPerSec; 

	if(waveChn != 1 && waveChn != 2){
		if(lpszErr) wsprintf(lpszErr,"'%d channel' is not supported.\n",waveChn);
		return false;
	}	

	if(waveBit != 8 && waveBit != 16 && waveBit != 24 && waveBit != 32 && waveBit != 64){
		if(lpszErr) wsprintf(lpszErr,"'%d bit' is not supported.\n",waveBit);
		return false;
	}
	
	if(avgByte != waveChn*waveRate*waveBit/8){
		if(lpszErr) strcpy(lpszErr,"Wave format is broken.\n");
		return false;
	}

	return true;
}


//-------------------------------------------------------------------
// get WAVE format and time, etc.   5/18/2002
BOOL GetWaveFormat(char* lpszFileName, // file name or 'stdin'
				   LPWAVEFORMATEX lpWaveFmt, 
				   LONGLONG* lpn64WaveDataSize, // size of data
				   LONGLONG* lpn64WaveOffset, // offset to data chunk
				   char* lpszErr 
				   )
{
	
#ifdef USEWIN32API
	HANDLE hdFile;
#else
	FILE* hdFile;
#endif
	DWORD dwCkSize; // chunk size
	DWORD dwByte;
	DWORD dwOffset; // offset to data
		
	char szErr[CHR_BUF];
	BOOL bStdin = false;
	LONGLONG n64DataSize;

	n64DataSize = 0;
	
	// open file
	if(strcmp(lpszFileName,"stdin")==0) bStdin = true;
	
	if(bStdin){ // stdin
		
#ifdef USEWIN32API
		hdFile	= GetStdHandle(STD_INPUT_HANDLE); 
#else
		hdFile = stdin;
#ifdef WIN32
		_setmode(_fileno(stdin),_O_BINARY); // binary mode
#endif
		
#endif
	}
	else{ // file
		
#ifdef USEWIN32API
		hdFile = CreateFile(lpszFileName,GENERIC_READ, 0, 0,OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
		if(hdFile == INVALID_HANDLE_VALUE){
			 wsprintf(szErr,"Could not open '%s'\n",lpszFileName);
			goto L_ERR;
		}						
#else
		hdFile = fopen(lpszFileName,"rb");
		if(hdFile == NULL) {
			wsprintf(szErr,"Could not open '%s'\n",lpszFileName);
			goto L_ERR;
		}	
#endif		
	}
	

	//------------------------------------------------------

	dwOffset = 0;
	
	// is this file 'RIFF' format?
	if(!SearchChunk(hdFile,"RIFF",4,true,bStdin,&dwCkSize,&dwOffset)){
		strcpy(szErr,"Could not find 'RIFF'. This is not wave file.\n");
		goto L_ERR;	
	}


	//--------------------------
	
	// search for 'WAVE'
	if(!SearchChunk(hdFile,"WAVE",128,false,bStdin,&dwCkSize,&dwOffset)){
		strcpy(szErr,"Wave header may be broken.\n");
		goto L_ERR;	
	}
	
	//--------------------------
	// format chunk

	// search for 'fmt_'
	if(!SearchChunk(hdFile,"fmt ",128,true,bStdin,&dwCkSize,&dwOffset)){
		strcpy(szErr,"Wave header may be broken.\n");
		goto L_ERR;	
	}

	// get format
#ifdef USEWIN32API 
	ReadFile(hdFile,lpWaveFmt,dwCkSize,&dwByte, NULL);
#else  
	dwByte = fread(lpWaveFmt,1,dwCkSize,hdFile);
#endif
	if(dwByte != dwCkSize){
		strcpy(szErr,"Wave header may be broken.\n");
		goto L_ERR;
	}
	dwOffset += dwByte;  
	
	// set cbSize to  0 
	lpWaveFmt->cbSize = 0; 
	
	// check format tag
	if(!CheckWaveFormat(lpWaveFmt,szErr)) goto L_ERR;

	//--------------------------
	// waveflt chunk (extra chunk)
	
	if(!bStdin){ // when stdin, endless mode is set, so ignore data size

		// search for 'wflt'
		if(SearchChunk(hdFile,"wflt",128,true,bStdin,&dwCkSize,&dwOffset)){

			// get file size
#ifdef USEWIN32API 
			ReadFile(hdFile,&n64DataSize,sizeof(LONGLONG),&dwByte, NULL);
#else  
			dwByte = fread(&n64DataSize,1,sizeof(LONGLONG),hdFile);
#endif
			if(dwByte != sizeof(LONGLONG)){
				strcpy(szErr,"Wave header may be broken.\n");
				goto L_ERR;
			}
			dwOffset += dwByte;  
			
			*lpn64WaveDataSize = n64DataSize;
		}
	}


	//--------------------------
	// data chunk

	// search for 'data'
	if(!SearchChunk(hdFile,"data",128,true,bStdin,&dwCkSize,&dwOffset)){
		strcpy(szErr,"Wave header may be broken.\n");
		goto L_ERR;	
	}
	
	if(lpn64WaveDataSize && n64DataSize == 0) *lpn64WaveDataSize = dwCkSize;
	
	// offset to data
	if(lpn64WaveOffset) *lpn64WaveOffset = dwOffset;

	// close handle
	if(!bStdin){
#ifdef USEWIN32API 
		CloseHandle(hdFile);
#else
		fclose(hdFile);
#endif
	}

	return true;
	
	//------------------------

L_ERR: 
	
	if(!bStdin){
#ifdef USEWIN32API 
		CloseHandle(hdFile);
#else
		fclose(hdFile);
#endif
	}
	if(lpszErr) strcpy(lpszErr,szErr);
	
	return false;
	
}



//-----------------------------------------------
// 再生サウンドデバイス情報取得
// 戻り値はデバイスの数
// 情報の取得に失敗したデバイスの wChannels は 0 にセットされる
// また、lpWaveOutCaps[uMaxNum-1] には WAVE_MAPPER の情報が入る
UINT GetWaveOutDevCap(LPWAVEOUTCAPS lpWaveOutCaps,UINT uMaxNum){
	
	UINT uDevNum,i;
	
	uDevNum = min(uMaxNum,waveOutGetNumDevs());
	if(uDevNum >= uMaxNum) uDevNum = uMaxNum-1;
	
	for(i=0;i<uDevNum;i++){
		if(waveOutGetDevCaps(i,&lpWaveOutCaps[i],sizeof(WAVEOUTCAPS))
			!= MMSYSERR_NOERROR) lpWaveOutCaps[i].wChannels = 0;
	}
	
	if(waveOutGetDevCaps(WAVE_MAPPER,&lpWaveOutCaps[uMaxNum-1],sizeof(WAVEOUTCAPS))
		!= MMSYSERR_NOERROR) lpWaveOutCaps[uMaxNum-1].wChannels = 0;
	
	return uDevNum;
}




//EOF