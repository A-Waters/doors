#pragma once
#include <windows.h>
#include <winuser.h>
#include <winnt.h>
#include <iostream>
#include <vector>





namespace lib {

    struct DoorMonitorInfo
    {
        HMONITOR mMonitorHandle;
        HDC mDeviceContextHandle;
        LPRECT mCords;
        MONITORINFO mMonitorInfo;

        DoorMonitorInfo() : mMonitorHandle(0), mDeviceContextHandle(0), mCords(0), mMonitorInfo(tagMONITORINFO()) {} // weird but it works
    };


    struct DoorWindowInfo {
        HWND hwnd;
        std::string title;
        DWORD lpdwProcessId; // will be 0 if not running as admin or sudo

        DoorWindowInfo() : hwnd(0), title(""), lpdwProcessId(DWORD()) {};
    };

    static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam);
    static BOOL CALLBACK enumMonitorCallback(HMONITOR monitorHandle, HDC deviceContextHandle, LPRECT cords, LPARAM);




    class doorsWindowManager {
        std::vector <DoorWindowInfo*> mActiveWindows;
        std::vector <DoorMonitorInfo*> mActiveMonitors;
    public:
        doorsWindowManager();
        ~doorsWindowManager();
        void printWindows() const;
        bool moveWindow(HWND hwnd, int x, int y, int width, int height);
        bool _addWindowToWM(HWND hWnd, const std::string& title, DWORD lpdwProcessId);
        bool _addMonitorToWM(HMONITOR monitorHandle, HDC deviceContextHandle, LPRECT cords);

    private:    
        bool loadMonitorInfo();
        bool loadWindowInfo();

    };
    
}
