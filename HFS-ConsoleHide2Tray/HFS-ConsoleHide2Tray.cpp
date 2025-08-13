#include <windows.h>
#include <cstring>
#include <string>
#include <cstdio>
#include "resource.h"

#pragma comment(lib, "user32.lib")

HWND g_hwnd = nullptr;
HMENU g_hMenu = nullptr;
NOTIFYICONDATA g_trayData = {};
HWND g_hfsMainWindow = nullptr;
bool g_hidden = false;
HICON g_hIcon = nullptr;

HWND ParseHwnd(const std::string& hwndStr) {
    try {
        return reinterpret_cast<HWND>(std::stoull(hwndStr, nullptr, 16));
    } catch (...) {
        return nullptr;
    }
}

HWND RequestHwndFromNamedPipe() {
    const char* pipeName = "\\\\.\\pipe\\HFS_WINDOW_HWNDS";
    HANDLE hPipe = CreateFile(
        pipeName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        return nullptr;
    }

    const char* request = "GET_HWNDS";
    DWORD bytesWritten;
    if (!WriteFile(hPipe, request, (DWORD)strlen(request), &bytesWritten, NULL)) {
        CloseHandle(hPipe);
        return nullptr;
    }

    char buffer[64];
    DWORD bytesRead;
    if (!ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        CloseHandle(hPipe);
        return nullptr;
    }

    CloseHandle(hPipe);
    buffer[bytesRead] = '\0';
    
    return ParseHwnd(buffer);
}

bool UpdateMainWindow() {
    HWND newWindow = RequestHwndFromNamedPipe();
    if (newWindow && IsWindow(newWindow)) {
        if (g_hfsMainWindow != newWindow) {
            g_hfsMainWindow = newWindow;
            return true;
        }
        return false;
    }
    
    g_hfsMainWindow = nullptr;
    return false;
}

void ToggleVisibility() {
    if (!g_hfsMainWindow || !IsWindow(g_hfsMainWindow)) {
        if (!UpdateMainWindow() || !g_hfsMainWindow) {
            return;
        }
    }
    
    g_hidden = !g_hidden;
    
    ShowWindow(g_hfsMainWindow, g_hidden ? SW_HIDE : SW_SHOW);
    if (!g_hidden) {
        SetForegroundWindow(g_hfsMainWindow);
        BringWindowToTop(g_hfsMainWindow);
    }
    
    ModifyMenu(g_hMenu, 1, MF_BYCOMMAND | MF_STRING, 1, 
              g_hidden ? "Show HFS console" : "Hide HFS console");
    DrawMenuBar(g_hwnd);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_USER + 1:
            if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(g_hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, nullptr);
            }
            else if (lParam == WM_LBUTTONDBLCLK) {
                ToggleVisibility();
            }
            break;
        
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                ToggleVisibility();
            }
            else if (LOWORD(wParam) == 2) {
                DestroyWindow(hwnd);
            }
            break;
            
        case WM_DESTROY:
            Shell_NotifyIcon(NIM_DELETE, &g_trayData);
            if (g_hIcon) DestroyIcon(g_hIcon);
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HICON CreateTrayIcon() {
    HICON hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_TRAY_ICON));
    if (!hIcon) {
        hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }
    return hIcon;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "HFS_ConsoleHide2Tray";
    if (!RegisterClass(&wc)) {
        return 1;
    }
    
    g_hwnd = CreateWindow(wc.lpszClassName, "HFS-ConsoleHide2Tray", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInst, nullptr);
    if (!g_hwnd) {
        return 1;
    }
    
    ShowWindow(g_hwnd, SW_HIDE);
    
    g_hMenu = CreatePopupMenu();
    AppendMenu(g_hMenu, MF_STRING, 1, "Hide HFS console");
    AppendMenu(g_hMenu, MF_STRING, 2, "Exit");
    
    g_hIcon = CreateTrayIcon();
    if (!g_hIcon) {
        return 1;
    }
    
    ZeroMemory(&g_trayData, sizeof(g_trayData));
    g_trayData.cbSize = sizeof(NOTIFYICONDATA);
    g_trayData.hWnd = g_hwnd;
    g_trayData.uID = 1;
    g_trayData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_trayData.uCallbackMessage = WM_USER + 1;
    g_trayData.hIcon = g_hIcon;
    strncpy_s(g_trayData.szTip, "HFS-ConsoleHide2Tray", sizeof(g_trayData.szTip));
    
    if (!Shell_NotifyIcon(NIM_ADD, &g_trayData)) {
        return 1;
    }
    
    if (UpdateMainWindow() && g_hfsMainWindow) {
        g_hidden = true;
        ShowWindow(g_hfsMainWindow, SW_HIDE);
        ModifyMenu(g_hMenu, 1, MF_BYCOMMAND | MF_STRING, 1, "Show HFS console");
        DrawMenuBar(g_hwnd);
    }
    
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}