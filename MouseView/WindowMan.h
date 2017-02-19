#pragma once
#include <shellapi.h>
#include <comsvcs.h>
#include <gdiplus.h>
#include <list>
#include <algorithm>
using namespace std;
using namespace Gdiplus;

#define MAX_DIV_NUM		6
#define MAX_ZOOM_NUM	25		// キューイング可能なコマンドの個数
#define MAX_ZOOM_STEP	2		// １回のズームで増やす倍率（％）
#define MAX_ZOOM_RATE	80000	// 最大拡大倍率
#define MIN_ZOOM_RATE	10000	// 最小拡大倍率
#define MOVE_THRESHOLD	10		// 画像切り替えを行うマウス移動量の閾値

#define NOT_DEFINED		-1

typedef enum { MB_LDOWN, MB_LUP, MB_RDOWN, MB_RUP, MB_MDOWN, MB_MUP } MB_TYPE;


class ImageEntry {
public:
	Image * pImage;
	wchar_t wcsFileName[MAX_PATH];

	ImageEntry(Image * pImage, wchar_t *wcsFileName) {
		this->pImage = pImage;
		wcscpy_s(this->wcsFileName, MAX_PATH, wcsFileName);
	};
	~ImageEntry() {
		if (this->pImage != NULL) {
			delete this->pImage;
		}
	}
	bool IsEarly(const ImageEntry & pImageEntry) {
		return wcscmp(this->wcsFileName, pImageEntry.wcsFileName) < 0;
	}
	//bool operator < (const ImageEntry & pImageEntry) {
	//	return wcscmp(this->wcsFileName, pImageEntry.wcsFileName) < 0;
	//}
};

typedef struct {
	ImageEntry * pImageEntry;
	Gdiplus::Rect rctWindow;
	Gdiplus::Rect rctDst;
	Gdiplus::Rect rctSrc;
	POINT ptCentor;
	long lZoomRate;
} ImageRgn;

typedef struct {
	long nIndex;
	bool bZoomIn;
} ZoomCommand;


class WindowMan
{
public:
	WindowMan(HWND hWnd);
public:
	virtual ~WindowMan(void);

	void OnPaint(HDC hdc);
	void OnMouseMove(long nFlags, long lX, long lY);
	void OnMouseButton(MB_TYPE type, long lX, long lY);
	void OnMouseWheel(long nFlags, long lDelta, long lX, long lY) ;
	void OnKeyDown(long lCharCode);
	void OnSize(long nFlags, long lWidth, long lHeight);
	void OnDropFiles(HDROP hDropInfo);
	void OnTimer(void);
		
private:
	HWND _hWnd;

	GdiplusStartupInput _gdiSI;
	ULONG_PTR _gdiToken;

	list<ImageEntry *> _lstImage;
	ImageRgn _ImageRgn[MAX_DIV_NUM * MAX_DIV_NUM];
	bool _catalog;
	long _nDivX;
	long _nDivY;
	RECT _rctClient;

	list<ZoomCommand *> _lstZoom;
	UINT _uTimerID;

	bool _bStayAfterButtonDown;
	POINT _ptAnchor;

	void UpdateSrcDstRect(void);
	void UpdateWindowRect(void);
	long PointToIndex(long lX, long lY);
	ImageEntry * FindImage(ImageEntry * pImage);
	void ClearAllImage(void);
	void ToggleFullScreen(void);
	void ToNextImage(long nIndex);
	void ToPrevImage(long nIndex);
	void ToSameImage(long nIndex, long nSourceIndex);
};
