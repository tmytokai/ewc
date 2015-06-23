// WAVE 関係(共通)

#include <windows.h>
#include <mmsystem.h>
#include <math.h>
#include <stdio.h>
#include <io.h> // _setmode
#include <fcntl.h> // _O_BINARY
#include <assert.h>

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




const bool IsWaveFormatValid( WAVEFORMATEX* waveformat, 
						   char* errmsg
						   )
{
	assert( waveformat );
	assert( errmsg );

	const unsigned short tag = waveformat->wFormatTag;
	if( tag != WAVE_FORMAT_IEEE_FLOAT && tag != WAVE_FORMAT_PCM 
		){
		strncpy( errmsg, "Not PCM format.\n", CHR_BUF);
		return false;
	}

	const unsigned short channels = waveformat->nChannels;  
	if( channels != 1 && channels != 2){
		_snprintf( errmsg, CHR_BUF ,"'%d channels' is not supported.\n", channels);
		return false;
	}	

	const unsigned short bit = waveformat->wBitsPerSample; 
	if( bit != 8 && bit != 16 && bit != 24 && bit != 32 && bit != 64){
		_snprintf(errmsg, CHR_BUF, "'%d bit' is not supported.\n", bit);
		return false;
	}
	
	const unsigned int avgbytes = waveformat->nAvgBytesPerSec; 
	const unsigned int rate = waveformat->nSamplesPerSec; 
	if(avgbytes != channels*rate*bit/8){
		strncpy(errmsg,"Invalid wave format.\n", CHR_BUF);
		return false;
	}

	return true;
}



enum
{
	ID_err = 0,
	ID_riff,
	ID_wave,
	ID_fmt,
	ID_data,
	ID_fact,
	ID_bext,
	ID_cue,
	ID_list,
	ID_plst,
	ID_junk,
	ID_wflt, // extension
	ID_unknown
};


const int GetChunkID( FILE* fp,
					 char* chunk,
					 unsigned int* chunksize
					 )
{
	assert( fp );
	assert( chunk );
	assert( chunksize );

	unsigned int byte;
	*chunksize = 0;

	byte = fread( chunk, 1, 4, fp);
	if(byte != 4) return ID_err;

	if( strncmp(chunk,"WAVE",4) ==0 ) return ID_wave;

	byte = fread(chunksize,1,sizeof(unsigned int),fp);
	if(byte != sizeof(unsigned int)) return ID_err;

	if( strncmp(chunk,"RIFF",4) ==0 ) return ID_riff;
	if( strncmp(chunk,"fmt ",4) ==0 ) return ID_fmt;
	if( strncmp(chunk,"data",4) ==0 ) return ID_data;
	if( strncmp(chunk,"fact",4) ==0 ) return ID_fact;
	if( strncmp(chunk,"bext",4) ==0 ) return ID_bext;
	if( strncmp(chunk,"cue ",4) ==0 ) return ID_cue;
	if( strncmp(chunk,"LIST",4) ==0 ) return ID_list;
	if( strncmp(chunk,"plst",4) ==0 ) return ID_plst;
	if( strncmp(chunk,"JUNK",4) ==0 ) return ID_junk;
	if( strncmp(chunk,"wflt",4) ==0 ) return ID_wflt;

	return ID_unknown;
}


const bool GetWaveFormat(const char* filename, // name or 'stdin'
				   WAVEFORMATEX* waveformat, 
				   unsigned long long* datasize, // data size (byte)
				   unsigned long long* offset, // offset to data chunk (byte)
				   char* errmsg 
				   )
{
	assert( filename );
	assert( waveformat );
	assert( datasize );
	assert( offset );
	assert( errmsg );

	FILE* fp = NULL;
	bool ret = false;

	*datasize = 0;
	*offset = 0;
	errmsg[0] = '\0';

	// stdin
	if(strncmp(filename,"stdin", 5)==0){
		fp = stdin;
#ifdef WIN32
		_setmode(_fileno(stdin),_O_BINARY); // set binary mode
#endif
	}
	// file
	else{
		fp = fopen(filename,"rb");
		if(fp == NULL) {
			_snprintf(errmsg, CHR_BUF, "Cannot open '%s'\n", filename);
			goto L_ERR;
		}
	}

	char chunk[5] = {0};
	unsigned int chunksize;

	if( GetChunkID( fp, chunk, &chunksize ) != ID_riff ){
		strcpy(errmsg,"This is not wave file.\n");
		goto L_ERR;	
	}
	*offset += 8;

	while(1){

		unsigned int byte;
		const int id = GetChunkID( fp, chunk, &chunksize );

		if( id == ID_err ){
			strncpy(errmsg,"Invalid wave header.\n", CHR_BUF);
			goto L_ERR;	
		}
		else if( id == ID_unknown ){
			_snprintf(errmsg, CHR_BUF, "Unknown chunk '%s'.\n", chunk);
			goto L_ERR;	
		}
		else if( id == ID_wave ){
			*offset += 4;
			continue;
		}
		else if( id == ID_fmt ){
			*offset += 8;
			byte = fread(waveformat,1,chunksize,fp);
			if(byte != chunksize){
				strncpy(errmsg, "Invalid fmt chunk.\n", CHR_BUF);
				goto L_ERR;
			}
			*offset += byte;  
			waveformat->cbSize = 0;
			if(!IsWaveFormatValid(waveformat, errmsg)) goto L_ERR;
		}
		else if( id == ID_wflt ){
			*offset += 8;
			byte = fread( datasize, 1, sizeof(unsigned long long), fp);
			if(byte != sizeof(unsigned long long)){
				strncpy(errmsg, "Invalid wflt chunk.\n", CHR_BUF);
				goto L_ERR;
			}
			*offset += byte;  
		}
		else if( id == ID_data ){
			*offset += 8;
			if( *datasize == 0) *datasize = chunksize;
			break;
		}
		else{
			*offset += 8 + chunksize;
			__int64 pos64 = *offset;
			_fseeki64( fp , pos64, SEEK_SET);
		}
	}
			
	ret = true;

	L_ERR: 

	// close file
	if( fp && fp != stdin ) fclose(fp);

	return ret;
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
