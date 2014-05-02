#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <ScrnSave.h>

#include <Direct2D.h>
#include <dwrite.h>

#include <vector>
#include <chrono>
#include <algorithm>

#include "resource.h"

#if defined _M_IX86
# pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
# pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
# pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
# pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#pragma comment(lib,"scrnsavw")
#pragma comment(lib,"comctl32")

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

template <class T>
void SafeRelease(T *&o)
{
	if(o != nullptr){
		o->Release();
		o = nullptr;
	}
}

const unsigned timer_id = 1000;

ID2D1Factory *d2factory;
ID2D1HwndRenderTarget *target;
ID2D1SolidColorBrush *brush;

IDWriteFactory *dwfactory;
IDWriteTextFormat *format;

D2D1_SIZE_F size;
unsigned x, beammax, beamc = 1, state;
int duration;

std::vector<wchar_t> string;
std::chrono::system_clock::time_point t;

typedef std::chrono::system_clock sc;

bool CreateResource(HWND hwnd);
void DestroyResource();

bool OnCreate(HWND hwnd, LPCREATESTRUCT lpcs);
void OnDestroy(HWND hwnd);
void OnPaint(HWND hwnd);
void OnTimer(HWND hwnd, UINT id);
void OnSize(HWND hwnd, UINT state, int cx, int cy);

LRESULT CALLBACK ScreenSaverProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg){
		HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
		HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
		HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
		HANDLE_MSG(hwnd, WM_SIZE, OnSize);

	case WM_ERASEBKGND:
		return FALSE;
	}

	return DefScreenSaverProc(hwnd, msg, wParam, lParam);
}

bool CreateResource(HWND hwnd)
{
	if(target != nullptr)
		return true;

	RECT rc;
	::GetClientRect(hwnd, &rc);

	if(FAILED(d2factory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(
				hwnd,
				D2D1::SizeU(rc.right, rc.bottom)
			),
			&target)))
		return false;
	if(FAILED(target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush)))
		return false;
	return true;
}

void DestroyResource()
{
	::SafeRelease(brush);
	::SafeRelease(target);
}

bool OnCreate(HWND hwnd, LPCREATESTRUCT lpcs)
{
	if(FAILED(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2factory)))
		return false;
	if(FAILED(::DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(dwfactory), reinterpret_cast<IUnknown**>(&dwfactory))))
		return false;
	if(FAILED(dwfactory->CreateTextFormat(
			L"Consolas",
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			20.f,
			L"",
			&format)))
		return false;
	format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	size = D2D1::Size(lpcs->cx, lpcs->cy);
	::SetTimer(hwnd, timer_id, 20, nullptr);
	t = sc::now();
	duration = ::GetPrivateProfileIntW(L"Chikubi", L"Speed", 1, L"chikubi.ini");
	return true;
}

void OnDestroy(HWND hwnd)
{
	::DestroyResource();
	::SafeRelease(format);
	::SafeRelease(d2factory);
	::SafeRelease(dwfactory);
}

void OnPaint(HWND hwnd)
{
	if(::CreateResource(hwnd)){
		target->BeginDraw();

		target->Clear();

		auto now = sc::now();
		if(now - t >= std::chrono::milliseconds(duration)){
			t = now;

			string.assign(x, L' ');
			::wcsncpy(&string[0], L"| o   o |", 9);
			string[10] = L' ';

			for(unsigned i = 0; i < beamc; ++i){
				for(unsigned j = 0; j < 3; ++j){
					unsigned idx = j + state + i * 4;
					if(3 <= idx && idx < x)
						string[idx] = L'-';
					idx += 4;
					if(7 <= idx && idx < x)
						string[idx] = L'-';
				}
			}

			if(state == 3)
				beamc = std::min(beamc + 1, beammax);
			state = (state + 1) % 4;
		}
		target->DrawText(&string[0], x, format, D2D1::Rect(0.f, 0.f, size.width, size.height), brush);

		if(target->EndDraw() == D2DERR_RECREATE_TARGET){
			::DestroyResource();
			::InvalidateRect(hwnd, nullptr, FALSE);
		}
	}
}

void OnTimer(HWND hwnd, UINT id)
{
	::InvalidateRect(hwnd, nullptr, FALSE);
}

void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	if(target)
		target->Resize(D2D1::SizeU(cx, cy));
	size = D2D1::Size(cx, cy);
	x = cx / 11;
	beammax = (x - 3) / 4;
	string.resize(x + 1);
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	wchar_t s[10];

	switch(msg){
	case WM_INITDIALOG:
		::GetPrivateProfileStringW(L"Chikubi", L"Speed", L"1", s, 10, L"chikubi.ini");
		::SetWindowTextW(::GetDlgItem(hDlg, IDC_EDITSPEED), s);
		::SendMessageW(::GetDlgItem(hDlg, IDC_SPINSPEED), UDM_SETRANGE32, static_cast<WPARAM>(1), static_cast<LPARAM>(10000));
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDOK:
			::GetWindowTextW(::GetDlgItem(hDlg, IDC_EDITSPEED), s, 10);
			::WritePrivateProfileStringW(L"Chikubi", L"Speed", s, L"chikubi.ini");
			EndDialog(hDlg, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
	return TRUE;
}
