#include "StdAfx.h"
#include "WindowMan.h"
#include <mmsystem.h>
#include <algorithm>
using namespace std;

void CALLBACK timerProc(UINT uiID, UINT uiNo, DWORD dwUser, DWORD dwNo1, DWORD dwNo2)
{
	::SendMessage((HWND)dwUser, WM_TIMER, NULL, NULL);
}

bool IsEarly(const ImageEntry* pImageEntry1, const ImageEntry*  pImageEntry2) {
	return wcscmp((*pImageEntry1).wcsFileName, (*pImageEntry2).wcsFileName) < 0;
}

WindowMan::WindowMan(HWND hWnd)
	: _hWnd(hWnd)
	, _catalog(false)
	, _nDivX(1)
	, _nDivY(1)
	, _uTimerID(NULL)
	, _bStayAfterButtonDown(false)
{
	_ptAnchor.x = NOT_DEFINED;
	_ptAnchor.y = NOT_DEFINED;
	GdiplusStartup(&_gdiToken, &_gdiSI, NULL);
}

WindowMan::~WindowMan(void)
{
	GdiplusShutdown(_gdiToken);
}

/*
* �`��C�x���g�ɑ΂���n���h��
*/
void WindowMan::OnPaint(HDC hdc)
{
	Graphics graphicsMain(hdc);
	Bitmap bmpBack(_rctClient.right - _rctClient.left, _rctClient.bottom - _rctClient.top);
	Graphics * pBack = graphicsMain.FromImage(&bmpBack);

	// �w�i��h��Ԃ�
	SolidBrush BlackBrush(Color(255, 0, 0, 0));
	pBack->FillRectangle(&BlackBrush, _rctClient.left, _rctClient.top,
		_rctClient.right - _rctClient.left, _rctClient.bottom - _rctClient.top);

	if (!_lstImage.empty())
	{
		for (long y = 0; y < _nDivY; y++)
		{
			for (long x = 0; x < _nDivX; x++)
			{
				long nIndex = x + MAX_DIV_NUM * y;
				Gdiplus::Rect rctTarget(
					_ImageRgn[nIndex].rctWindow.X + _ImageRgn[nIndex].rctDst.X,
					_ImageRgn[nIndex].rctWindow.Y + _ImageRgn[nIndex].rctDst.Y,
					_ImageRgn[nIndex].rctDst.Width, _ImageRgn[nIndex].rctDst.Height);
				Image * pImage = _ImageRgn[nIndex].pImageEntry->pImage;
				pBack->DrawImage(pImage, rctTarget,
					_ImageRgn[nIndex].rctSrc.X, _ImageRgn[nIndex].rctSrc.Y,
					_ImageRgn[nIndex].rctSrc.Width, _ImageRgn[nIndex].rctSrc.Height, UnitPixel);

				RectF rectPos(0.0f, 0.0f, 0.0f, 0.0f);
				SolidBrush brushWhite(Color(255, 255, 255, 255));
				pBack->DrawString(_ImageRgn[nIndex].pImageEntry->wcsFileName,
					wcslen(_ImageRgn[nIndex].pImageEntry->wcsFileName),
					NULL, rectPos, NULL, &brushWhite);
			}
		}
	}

	//bmpBack.RotateFlip(Rotate90FlipNone);
	graphicsMain.DrawImage(&bmpBack, 0, 0);

	delete pBack;
	return;
}


/*
* �}�E�X�ړ��R�}���h�ɑ΂���n���h��
*/
void WindowMan::OnMouseMove(long nFlags, long lX, long lY)
{
	if (_lstImage.empty())
	{
		return;
	}
	if (!(nFlags & MK_LBUTTON) && !(nFlags & MK_RBUTTON))
	{
		return;
	}
	// ���S�ʒu���X�V
	long nIndex = PointToIndex(lX, lY);
	if (nIndex < 0 || nIndex >= MAX_DIV_NUM * MAX_DIV_NUM)
	{
		return;
	}
	if (nFlags & MK_LBUTTON)
	{
		_ImageRgn[nIndex].ptCentor.x = 10000 * (lX - _ImageRgn[nIndex].rctWindow.X)
			/ _ImageRgn[nIndex].rctWindow.Width;
		_ImageRgn[nIndex].ptCentor.y = 10000 * (lY - _ImageRgn[nIndex].rctWindow.Y)
			/ _ImageRgn[nIndex].rctWindow.Height;

		// �}�E�X�{�^���_�E����A�K��l�ȏ�̈ړ����s��ꂽ�Ȃ�摜�؂�ւ��͍s��Ȃ��B
		if (_bStayAfterButtonDown)
		{
			if (abs(_ptAnchor.x - lX) > MOVE_THRESHOLD || abs(_ptAnchor.y - lY) > MOVE_THRESHOLD)
			{
				_bStayAfterButtonDown = false;
			}
		}
	}
	else if (nFlags & MK_RBUTTON)
	{
		if (lX - _ptAnchor.x > 50)
		{
			ToNextImage(nIndex);
			_ptAnchor.x = lX;
		}
		else if (_ptAnchor.x - lX > 50)
		{
			ToPrevImage(nIndex);
			_ptAnchor.x = lX;
		}
	}
	UpdateSrcDstRect();
	::InvalidateRect(_hWnd, NULL, FALSE);
	return;
}

/*
* �}�E�X�{�^���R�}���h�ɑ΂���n���h��
*/
void WindowMan::OnMouseButton(MB_TYPE type, long lX, long lY)
{
	if (_lstImage.empty())
	{
		return;
	}
	long nIndex = PointToIndex(lX, lY);
	long nAnchorIndex = PointToIndex(_ptAnchor.x, _ptAnchor.y);
	switch (type)
	{
	case MB_RDOWN:
	case MB_LDOWN:
		_bStayAfterButtonDown = true;
		_ptAnchor.x = lX;
		_ptAnchor.y = lY;
		break;
	case MB_RUP:
	case MB_LUP:
		if (nAnchorIndex == nIndex)
		{
			if (_bStayAfterButtonDown)
			{
				if (type == MB_RUP)	ToNextImage(nIndex);
				if (type == MB_LUP)	ToPrevImage(nIndex);
			}
		}
		else
		{
			ToSameImage(nIndex, nAnchorIndex);
		}
		break;
	case MB_MDOWN:
	{
		ToggleFullScreen();
	}
	default:
		break;
	}

	// ���S�ʒu���X�V����
	POINT ptCursor;
	GetCursorPos(&ptCursor);
	ScreenToClient(_hWnd, &ptCursor);
	::PostMessage(_hWnd, WM_MOUSEMOVE, (WPARAM)MK_LBUTTON, MAKELPARAM(ptCursor.x, ptCursor.y));

	UpdateSrcDstRect();
	::InvalidateRect(_hWnd, NULL, FALSE);
	return;
}

/*
* �}�E�X�z�C�[����]�C�x���g�ɑ΂���n���h��
*/
void WindowMan::OnMouseWheel(long nFlags, long lDelta, long lX, long lY)
{
	// �ő�L���[�C���O���𒴂���Y�[���͊i�[�ł��Ȃ�
	if (_lstZoom.size() > MAX_ZOOM_NUM)
	{
		return;
	}

	if (_lstZoom.empty())
	{
		timeBeginPeriod(1);
		_uTimerID = ::timeSetEvent(20, 1, timerProc, (DWORD_PTR)_hWnd, TIME_PERIODIC);
	}

	for (long i = 0; i < 5; i++)
	{
		ZoomCommand * pZoom = new ZoomCommand;
		pZoom->bZoomIn = lDelta > 0;
		pZoom->nIndex = PointToIndex(lX, lY);
		_lstZoom.push_back(pZoom);
	}
	return;
}

/*
* �L�[�����C�x���g�ɑ΂���n���h��
*/
void WindowMan::OnKeyDown(long lCharCode)
{
	switch (lCharCode)
	{
	case 'F':	ToggleFullScreen();	break;
	case '1':	_nDivX = 1;		_nDivY = 1;		UpdateWindowRect();		_catalog = false;	break;
	case '2':	_nDivX = 2;		_nDivY = 1;		UpdateWindowRect();		_catalog = false;	break;
	case '3':	_nDivX = 3;		_nDivY = 1;		UpdateWindowRect();		_catalog = false;	break;
	case '4':	_nDivX = 2;		_nDivY = 2;		UpdateWindowRect();		_catalog = false;	break;
	case '6':	_nDivX = 3;		_nDivY = 2;		UpdateWindowRect();		_catalog = false;	break;
	case '8':	_nDivX = 4;		_nDivY = 2;		UpdateWindowRect();		_catalog = false;	break;
	case '9':	_nDivX = 3;		_nDivY = 3;		UpdateWindowRect();		_catalog = false;	break;
	case '0':	_nDivX = 4;		_nDivY = 3;		UpdateWindowRect();		_catalog = false;	break;
	case 'C':	_nDivX = 6;		_nDivY = 4;		UpdateWindowRect();		_catalog = true;	break;
	}
	return;
}

/*
* �^�C�}�[�C�x���g�ɑ΂���n���h��
*/
void WindowMan::OnTimer(void)
{
	if (!_lstZoom.empty())
	{
		ZoomCommand * pZoom = _lstZoom.front();
		if (pZoom->bZoomIn)
		{
			_ImageRgn[pZoom->nIndex].lZoomRate = (100 + MAX_ZOOM_STEP) * _ImageRgn[pZoom->nIndex].lZoomRate / 100;
		}
		else
		{
			_ImageRgn[pZoom->nIndex].lZoomRate = 100 * _ImageRgn[pZoom->nIndex].lZoomRate / (100 + MAX_ZOOM_STEP);
		}
		if (_ImageRgn[pZoom->nIndex].lZoomRate < MIN_ZOOM_RATE)
			_ImageRgn[pZoom->nIndex].lZoomRate = MIN_ZOOM_RATE;
		if (_ImageRgn[pZoom->nIndex].lZoomRate > MAX_ZOOM_RATE)
			_ImageRgn[pZoom->nIndex].lZoomRate = MAX_ZOOM_RATE;
		delete pZoom;
		_lstZoom.pop_front();
	}

	if (_lstZoom.empty())
	{
		timeKillEvent(_uTimerID);
		_uTimerID = 0;
	}

	// ���S�ʒu���X�V����
	POINT ptCursor;
	GetCursorPos(&ptCursor);
	::ScreenToClient(_hWnd, &ptCursor);
	::PostMessage(_hWnd, WM_MOUSEMOVE, (WPARAM)MK_LBUTTON, MAKELPARAM(ptCursor.x, ptCursor.y));

	UpdateSrcDstRect();
	::InvalidateRect(_hWnd, NULL, FALSE);
	return;
}

/*
* �E�B���h�E�T�C�Y�ύX�C�x���g�ɑ΂���n���h��
*/
void WindowMan::OnSize(long nFlags, long lX, long lY)
{
	wchar_t szTmp[20] = L"";
	swprintf_s(szTmp, L"%d %d %d\n", nFlags, lX, lY);
	OutputDebugString(szTmp);

	if (nFlags == SIZE_RESTORED)
	{
		_rctClient.left = 0;
		_rctClient.top = 0;
		_rctClient.right = lX;
		_rctClient.bottom = lY;
		UpdateWindowRect();
	}

	::InvalidateRect(_hWnd, NULL, FALSE);
	return;
}

/*
* �t�@�C���̃h���b�v�ɑ΂���n���h��
*/
void WindowMan::OnDropFiles(HDROP hDropInfo)
{
	size_t lPathLgh;
	WCHAR szDrop[MAX_PATH], szPath[MAX_PATH], szWild[MAX_PATH];
	if (DragQueryFile(hDropInfo, 0xFFFFFFFF, szDrop, sizeof(szDrop)) != 1) return;
	DragQueryFile(hDropInfo, 0, szDrop, sizeof(szDrop));

	lstrcpy(szPath, szDrop);
	if (GetFileAttributes(szDrop) == FILE_ATTRIBUTE_DIRECTORY) {
	}
	else {
		// �h���b�v���ꂽ�����񂩂�I�[�v�����郏�C���h�J�[�h�����
		WCHAR *szTmp = _tcsrchr(szPath, (int)('\\'));
		*(szTmp) = '\0';
	}
	lPathLgh = wcslen(szPath);
	lstrcpy(szWild, szPath);
	wcscat_s(szWild, L"\\*.jpg");

	ImageEntry * pDroppedImage = NULL;
	WIN32_FIND_DATA data;
	HANDLE hFindFile = FindFirstFile(szWild, &data);
	if (hFindFile == NULL) return;

	// ���X�g���N���A����
	ClearAllImage();

	do
	{
		wcscat_s(szPath, L"\\");
		wcscat_s(szPath, data.cFileName);
		Image * pImage = new Image(szPath, FALSE);
		ImageEntry * pImageEntry = new ImageEntry(pImage, data.cFileName);
		_lstImage.push_back(pImageEntry);
		if (pDroppedImage == NULL || wcscmp(szPath, szDrop) == 0)
		{
			pDroppedImage = pImageEntry;
		}
		szPath[lPathLgh] = '\0';
	} while (FindNextFile(hFindFile, &data));

	_lstImage.sort(IsEarly);

	if (pDroppedImage != NULL)
	{
		ImageEntry * pImageEntry = FindImage(pDroppedImage);
		for (long it = 0; it < MAX_DIV_NUM * MAX_DIV_NUM; it++)
		{
			_ImageRgn[it].pImageEntry = pImageEntry;
			_ImageRgn[it].ptCentor.x = 5000;
			_ImageRgn[it].ptCentor.y = 5000;
			_ImageRgn[it].lZoomRate = 10000;
		}
		list<ImageEntry*>::iterator i = _lstImage.begin();
		if (!_catalog)
		{
			i = find(_lstImage.begin(), _lstImage.end(), pDroppedImage);
		}
		for (long y = 0; y < _nDivY; y++)
		{
			for (long x = 0; x < _nDivX; x++)
			{
				long nIndex = x + y * MAX_DIV_NUM;
				_ImageRgn[nIndex].pImageEntry = *i;
				if (++i == _lstImage.end()) i = _lstImage.begin();
			}
		}
		UpdateWindowRect();
	}
	else
	{
		ClearAllImage();
	}
	::InvalidateRect(_hWnd, NULL, FALSE);
}

//	����������E�B���h�E��`���X�V����
void WindowMan::UpdateWindowRect(void)
{
	for (long x = 0; x < MAX_DIV_NUM; x++)
	{
		for (long y = 0; y < MAX_DIV_NUM; y++)
		{
			long nIndex = x + y * MAX_DIV_NUM;
			_ImageRgn[nIndex].rctWindow.Width = (_rctClient.right - _rctClient.left) / _nDivX;
			_ImageRgn[nIndex].rctWindow.Height = (_rctClient.bottom - _rctClient.top) / _nDivY;
			_ImageRgn[nIndex].rctWindow.X = x * _ImageRgn[nIndex].rctWindow.Width;
			_ImageRgn[nIndex].rctWindow.Y = y * _ImageRgn[nIndex].rctWindow.Height;
		}
	}
	UpdateSrcDstRect();
}

//	�Y�[�����A�Z���^�[���W����A�R�s�[���E�R�s�[���`���X�V����
void WindowMan::UpdateSrcDstRect(void)
{
	if (_lstImage.empty())
	{
		return;
	}
	for (long y = 0; y < _nDivY; y++)
	{
		for (long x = 0; x < _nDivX; x++)
		{
			long nIndex = x + y * MAX_DIV_NUM;
			Image * pImage = _ImageRgn[nIndex].pImageEntry->pImage;

			// ���S�ʒu�iptCentor�j�𒲐�
			long lMinX = 5000 * _ImageRgn[nIndex].rctSrc.Width / pImage->GetWidth();
			long lMinY = 5000 * _ImageRgn[nIndex].rctSrc.Height / pImage->GetHeight();
			long lMaxX = 10000 - lMinX;
			long lMaxY = 10000 - lMinY;
			if (_ImageRgn[nIndex].ptCentor.x < lMinX) _ImageRgn[nIndex].ptCentor.x = lMinX;
			if (_ImageRgn[nIndex].ptCentor.y < lMinY) _ImageRgn[nIndex].ptCentor.y = lMinY;
			if (_ImageRgn[nIndex].ptCentor.x > lMaxX) _ImageRgn[nIndex].ptCentor.x = lMaxX;
			if (_ImageRgn[nIndex].ptCentor.y > lMaxY) _ImageRgn[nIndex].ptCentor.y = lMaxY;

			long lDstWidth = 0;
			long lDstHeight = 0;
			long lImgWidth = pImage->GetWidth();
			long lImgHeight = pImage->GetHeight();
			long lWndWidth = _ImageRgn[nIndex].rctWindow.Width;
			long lWndHeight = _ImageRgn[nIndex].rctWindow.Height;
			long lCentorX = _ImageRgn[nIndex].ptCentor.x;
			long lCentorY = _ImageRgn[nIndex].ptCentor.y;
			long lZoomRate = _ImageRgn[nIndex].lZoomRate;
			// �摜���E�B���h�E�ɑ΂������̏ꍇ
			if (lImgWidth * lWndHeight < lImgHeight * lWndWidth)
			{
				lDstHeight = lWndHeight;
				lDstWidth = (long)((LONGLONG)(lWndHeight)* lImgWidth * lZoomRate / lImgHeight / 10000);
				// ���E�Ƀ}�[�W�������݂���ꍇ
				if (lDstWidth < lWndWidth)
				{
					_ImageRgn[nIndex].rctSrc.Width = lImgWidth;
					_ImageRgn[nIndex].rctSrc.Height = lImgWidth * lDstHeight / lDstWidth;
				}
				// �}�[�W�����Ƃ�Ȃ��ꍇ
				else
				{
					_ImageRgn[nIndex].rctSrc.Width = lImgWidth * lWndWidth / lDstWidth;
					_ImageRgn[nIndex].rctSrc.Height = _ImageRgn[nIndex].rctSrc.Width * lWndHeight / lWndWidth;
					lDstWidth = lWndWidth;
				}
			}
			// �摜���E�B���h�E�ɑ΂������̏ꍇ
			else
			{
				lDstWidth = lWndWidth;
				lDstHeight = (long)((LONGLONG)(lWndWidth)* lImgHeight * lZoomRate / lImgWidth / 10000);
				// �㉺�Ƀ}�[�W�������݂���ꍇ
				if (lDstHeight < lWndHeight)
				{
					_ImageRgn[nIndex].rctSrc.Height = lImgHeight;
					_ImageRgn[nIndex].rctSrc.Width = lImgHeight * lDstWidth / lDstHeight;
				}
				// �}�[�W�����Ƃ�Ȃ��ꍇ
				else
				{
					_ImageRgn[nIndex].rctSrc.Height = lImgHeight * lWndHeight / lDstHeight;
					_ImageRgn[nIndex].rctSrc.Width = _ImageRgn[nIndex].rctSrc.Height * lWndWidth / lWndHeight;
					lDstHeight = lWndHeight;
				}
			}
			_ImageRgn[nIndex].rctSrc.X = lImgWidth * (10000 - lCentorX) / 10000 - _ImageRgn[nIndex].rctSrc.Width / 2;
			_ImageRgn[nIndex].rctSrc.Y = lImgHeight * (10000 - lCentorY) / 10000 - _ImageRgn[nIndex].rctSrc.Height / 2;
			_ImageRgn[nIndex].rctDst.X = (lWndWidth - lDstWidth) / 2;
			_ImageRgn[nIndex].rctDst.Y = (lWndHeight - lDstHeight) / 2;
			_ImageRgn[nIndex].rctDst.Width = lDstWidth;
			_ImageRgn[nIndex].rctDst.Height = lDstHeight;
		}
	}
}

long WindowMan::PointToIndex(long lX, long lY)
{
	long lWidth = (_rctClient.right - _rctClient.left) / _nDivX;
	long lHeight = (_rctClient.bottom - _rctClient.top) / _nDivY;
	long lRet = (lX / lWidth) + (lY / lHeight) * MAX_DIV_NUM;
	//ASSERT(lRet >= 0);
	//ASSERT(lRet < MAX_DIV_NUM * MAX_DIV_NUM);
	return lRet;

}

ImageEntry * WindowMan::FindImage(ImageEntry * pEntry)
{
	list<ImageEntry*>::iterator it = find(_lstImage.begin(), _lstImage.end(), pEntry);
	return *it;
}

void WindowMan::ClearAllImage(void)
{
	while (!_lstImage.empty())
	{
		list<ImageEntry*>::iterator it = _lstImage.begin();
		delete (*it);
		_lstImage.remove(*it);
	}
}

// �t���X�N���[���؂�ւ�
void WindowMan::ToggleFullScreen(void)
{
	static bool bFullScreen = false;
	static RECT rctWindow = { 0, 0, 0, 0 };
	// �t���X�N���[������
	if (bFullScreen)
	{
		long lCurrentStyle = GetWindowLong(_hWnd, GWL_STYLE);
		long lNewStyle = lCurrentStyle | (WS_CAPTION | WS_THICKFRAME);
		SetWindowLong(_hWnd, GWL_STYLE, lNewStyle);
		SetWindowPos(_hWnd, HWND_NOTOPMOST, rctWindow.left, rctWindow.top,
			rctWindow.right - rctWindow.left, rctWindow.bottom - rctWindow.top, NULL);
		bFullScreen = false;
	}
	// �t���X�N���[���ݒ�
	else
	{
		// �}�E�X�J�[�\���̈ʒu
		POINT point;
		if (!GetCursorPos(&point)) return;
		// ���j�^�[�̃n���h�����擾
		HMONITOR monitor = MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST);
		MONITORINFOEX info;
		info.cbSize = sizeof(info);
		GetMonitorInfo(monitor, &info);
		GetWindowRect(_hWnd, &rctWindow);
		long lCurrentStyle = GetWindowLong(_hWnd, GWL_STYLE);
		long lNewStyle = lCurrentStyle & ~(WS_CAPTION | WS_THICKFRAME);
		SetWindowLong(_hWnd, GWL_STYLE, lNewStyle);
		SetWindowPos(_hWnd, HWND_TOPMOST,
			info.rcMonitor.left, info.rcMonitor.top,
			info.rcMonitor.right - info.rcMonitor.left,
			info.rcMonitor.bottom - info.rcMonitor.top, SWP_FRAMECHANGED);
		bFullScreen = true;
	}
}

void WindowMan::ToNextImage(long nIndex)
{
	if (_catalog)
	{
		nIndex = _nDivX + (_nDivY - 1) * MAX_DIV_NUM - 1;
		ImageEntry * pImageEntry = _ImageRgn[nIndex].pImageEntry;
		list<ImageEntry*>::iterator i = find(_lstImage.begin(), _lstImage.end(), pImageEntry);
		for (long y = 0; y < _nDivY; y++)
		{
			for (long x = 0; x < _nDivX; x++)
			{
				long nIndex = x + y * MAX_DIV_NUM;
				if (++i == _lstImage.end()) i = _lstImage.begin();
				_ImageRgn[nIndex].pImageEntry = *i;
			}
		}
	}
	else
	{
		ImageEntry * pImageEntry = _ImageRgn[nIndex].pImageEntry;
		list<ImageEntry*>::iterator it = find(_lstImage.begin(), _lstImage.end(), pImageEntry);
		if (it != _lstImage.end())
		{
			if (++it == _lstImage.end()) it = _lstImage.begin();
			_ImageRgn[nIndex].pImageEntry = *it;
		}
	}
}

void WindowMan::ToPrevImage(long nIndex)
{
	if (_catalog)
	{
		nIndex = 0;
		ImageEntry * pImageEntry = _ImageRgn[nIndex].pImageEntry;
		list<ImageEntry*>::iterator i = find(_lstImage.begin(), _lstImage.end(), pImageEntry);
		for (int j = 0; j < _lstImage.size() - _nDivX * _nDivY - 1; j++)
		{
			if (++i == _lstImage.end()) i = _lstImage.begin();
		}
		for (long y = 0; y < _nDivY; y++)
		{
			for (long x = 0; x < _nDivX; x++)
			{
				long nIndex = x + y * MAX_DIV_NUM;
				if (++i == _lstImage.end()) i = _lstImage.begin();
				_ImageRgn[nIndex].pImageEntry = *i;
			}
		}
	}
	else
	{
		ImageEntry * pImageEntry = _ImageRgn[nIndex].pImageEntry;
		list<ImageEntry*>::reverse_iterator it = find(_lstImage.rbegin(), _lstImage.rend(), pImageEntry);
		if (it != _lstImage.rend())
		{
			if (++it == _lstImage.rend()) it = _lstImage.rbegin();
			_ImageRgn[nIndex].pImageEntry = *it;
		}
	}
}

void WindowMan::ToSameImage(long nIndex, long nSourceIndex)
{
	ImageEntry * pImageEntry = _ImageRgn[nSourceIndex].pImageEntry;
	list<ImageEntry*>::iterator it = find(_lstImage.begin(), _lstImage.end(), pImageEntry);
	if (it != _lstImage.end())
	{
		_ImageRgn[nIndex].pImageEntry = *it;
	}
}
