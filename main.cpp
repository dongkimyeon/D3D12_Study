#include "stdafx.h"
#include "Framework.h"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
extern bool debugMode = true;
Framework* gFramework = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_ENTERSIZEMOVE:
		if (gFramework) gFramework->SetResizing(true);
		return 0;

	case WM_EXITSIZEMOVE:
		if (gFramework)
		{
			gFramework->SetResizing(false);
			RECT rc = {};
			GetClientRect(hwnd, &rc);
			int w = rc.right - rc.left;
			int h = rc.bottom - rc.top;
			if (w > 0 && h > 0)
				gFramework->OnResize(w, h);
		}
		return 0;

	case WM_SIZE:
		if (gFramework && wparam != SIZE_MINIMIZED && !gFramework->IsResizing())
		{

			int w = LOWORD(lparam);
			int h = HIWORD(lparam);
			if (w > 0 && h > 0)
				gFramework->OnResize(w, h);
		}
		return 0;

	case WM_KEYDOWN:
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	WNDCLASSEXW winClass = { sizeof(WNDCLASSEXW) };
	winClass.style = CS_HREDRAW | CS_VREDRAW;
	winClass.lpfnWndProc = WndProc;
	winClass.hInstance = hInstance;
	winClass.hCursor = LoadCursor(0, IDC_ARROW);
	winClass.lpszClassName = L"DirectX_12";
	RegisterClassExW(&winClass);

	RECT initialRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
	HWND hwnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, winClass.lpszClassName, L"DirectX_12",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
		initialRect.right - initialRect.left, initialRect.bottom - initialRect.top,
		0, 0, hInstance, 0);
	ShowWindow(hwnd, nCmdShow);

	{
		Framework framework(WINDOW_WIDTH, WINDOW_HEIGHT);
		gFramework = &framework;
		framework.Initialize(hwnd);
		framework.Run();
	} 

	system("pause");
	FreeConsole();
	return 0;
}