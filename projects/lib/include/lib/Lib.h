#pragma once
#include <windows.h>
#include <winuser.h>
#include <winnt.h>
#include <iostream>
#include <vector>





namespace lib {

    struct WindowInfo {
        HWND hwnd;
        std::string title;
        const LPDWORD& lpdwProcessId;
    };

    static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam);

    class windowManager {
        std::vector <WindowInfo> mActiveWindows;
    public:
        windowManager();
        
        bool addWindowToWM(HWND hWnd, const std::string& title, const LPDWORD& lpdwProcessId);
        void printWindows() const;
        bool moveWindow(HWND hwnd, int x, int y, int width, int height);

    };

}
