#pragma once
#include <windows.h>
#include <winuser.h>
#include <winnt.h>
#include <iostream>
#include <vector>
#include <map>




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

    struct Region {
        int mX;
        int mY;
        int mWidth;
        int mHeight;
        DoorWindowInfo* DWI; 

        Region(int x, int y, int width, int height) : mX(x), mY(y), mWidth(width), mHeight(height), DWI(nullptr) {};
    };


    static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam);
    static BOOL CALLBACK enumMonitorCallback(HMONITOR monitorHandle, HDC deviceContextHandle, LPRECT cords, LPARAM);




    class doorsWindowManager {

    public:
        doorsWindowManager();
        ~doorsWindowManager();
        void printInfo() const;
        bool moveWindow(HWND hwnd, int x, int y, int width, int height);
        bool moveMouse(int x, int y);
        bool _addWindowToWM(HWND hWnd, const std::string& title, DWORD lpdwProcessId);
        bool _addMonitorToWM(HMONITOR monitorHandle, HDC deviceContextHandle, LPRECT cords);
        bool selectWindow(DWORD pid);

    private:
        int gaps = 15;
        std::vector <DoorWindowInfo*> mActiveWindows;
        std::vector <DoorMonitorInfo*> mActiveMonitors;
        std::map<HMONITOR, std::vector<Region*>> mRegions;

        bool mIsAdmin;
        bool loadMonitorInfo();
        bool loadWindowInfo();
        bool isRunningAsAdmin();
        bool buildRegions();
        DoorMonitorInfo* getMonitorInfoFromHandle(HMONITOR monitorHandle);
        bool matchWindowsToRegions();


    };
    
}
