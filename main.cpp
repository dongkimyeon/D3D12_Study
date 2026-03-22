#include "stdafx.h"
#include "Framework.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    // ImGui가 입력을 캡처하도록
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;

    switch (msg)
    {
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE) DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{

    // 윈도우 생성 로직
    WNDCLASSEXW winClass = { sizeof(WNDCLASSEXW) };
    winClass.style = CS_HREDRAW | CS_VREDRAW;
    winClass.lpfnWndProc = WndProc;
    winClass.hInstance = hInstance;
    winClass.hCursor = LoadCursor(0, IDC_ARROW);
    winClass.lpszClassName = L"D3D12WindowClass";

    RegisterClassExW(&winClass);

    RECT initialRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);

    HWND hwnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, winClass.lpszClassName, L"D3D12 Study Engine",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        initialRect.right - initialRect.left, initialRect.bottom - initialRect.top,
        0, 0, hInstance, 0);

    ShowWindow(hwnd, nCmdShow);

    // 프레임워크 구동
    Framework framework(WINDOW_WIDTH, WINDOW_HEIGHT);
    framework.Initialize(hwnd);
    framework.Run();

    FreeConsole();
    return 0;
}