// e-WC  editwave.cpp �̃O���[�o���֐�

#include "common.h"

// �O���[�o���ϐ�

HINSTANCE hInst;

CHAR SzInfo[4192];  // �C���t�H���[�V����


DWORD DwWaveTime; // WAVE �̒���(�~���b)

LONGLONG N64OrgFileSize; // �t�@�C���̌��T�C�Y
LONGLONG N64OrgDataSize; // �t�@�C���̌��f�[�^�T�C�Y
LONGLONG N64OrgDataOffset; // �t�@�C���̌��I�t�Z�b�g

DWORD DwZoomX; // �Y�[���T�C�Y�A��
DWORD DwZoomY; // �c


// �������T�[�`�p
double DbNoSoundBound; // �������T�[�`�̂������l
DWORD dwNSoundCount; // �������T�[�`�̃J�E���g��
WORD wNSoundPos; // �T�[�`��̈ʒu

// �X�N���[���o�[�p
HWND hScrWnd; // �X�N���[���o�[�̃n���h��
LONG nScrMax; // �X�N���[���o�[�̃}�b�N�X�l
SCROLLINFO scrInfo; // �X�N���[�����\����
LONGLONG N64MaxBlock; // n64WaveDataSize�� FRAMESIZE*waveFmt.nBlockAlign ����������

// �}�[�N�p
LONG nMarkedPos; // �}�[�N�����|�W�V����
LONGLONG N64MarkedPosByte; // �I�t�Z�b�g����}�[�N�����_�܂ł̃o�C�g��(>=0 < n64WaveDataSize)
double DbMarkedLevel[2]; // db, ��}�[�N�̃��x��

LONG nSubMarkedPos; // ���}�[�N�̃|�W�V����
LONGLONG N64SubMarkedPosByte; // ���}�[�N�̃o�C�g��
double DbSubMarkedLevel[2]; // db, ���}�[�N�̃��x��
	

// �X�e�[�^�X�AUNDO �p
WORD wCurStatus;	 // ���݂̏��
BOOL bUpdate; // �X�V������
UNDODATA undoData; // UNDO �̃f�[�^

// �ҏW�A�T�[�`�p
DWORD DwSearchTime = 0; // ���ԃT�[�`�p(sec)
LPEWCDATA lpEwcData;


// �`��p
HDC hBufDC = NULL; 
HBITMAP hBufBit;
RECT RedrawRect;
HICON hIcon[14]; // �{�^���̃A�C�R��
HWND hPreEwcWnd;

//EOF