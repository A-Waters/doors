#pragma once
#include <windows.h>
#include <winuser.h>
#include <winnt.h>
#include <iostream>
#include <vector>
#include <map>




namespace lib {
   


    enum WindowType {
        TopLevel,
        Child,
        Popup,
        Dialog,
        ToolWindow,
        dMessageBox,
        Overlay,
        Invisible,
        Unknown
    };

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
        MINMAXINFO minSizes;
        HMONITOR monitor;

        DoorWindowInfo() : hwnd(0), title(""), lpdwProcessId(DWORD()), minSizes(), monitor() {};
    };

    struct Region {
        int id;
        int mX;
        int mY;
        int mWidth;
        int mHeight;
        DoorWindowInfo* DWI; 

        Region(int x, int y, int width, int height) : mX(x), mY(y), mWidth(width), mHeight(height), DWI(nullptr), id(-1) {};
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
        bool _addWindowToWM(HWND hwnd, const std::string& title, DWORD lpdwProcessId);
        bool _addMonitorToWM(HMONITOR monitorHandle, HDC deviceContextHandle, LPRECT cords);
        bool swapRegionsByID(int regionAid, int regionBid);
        bool swapRegions(Region* regionA, Region* regionB);
        bool createRegion(HMONITOR monitor, int x, int y, int length, int height, DoorWindowInfo* dwi);
        bool deleteRegion(HMONITOR monitor, Region* regionToRemove);
        bool moveRegion(HMONITOR from, HMONITOR to, Region* regionToMove);

    private:
        Region* mFocused;
        int gaps = 0;
        std::vector <DoorWindowInfo*> mActiveWindows;
        std::vector <DoorMonitorInfo*> mActiveMonitors;
        std::map<HMONITOR, std::vector<Region*>> mRegions;

        bool mIsAdmin;
        bool loadMonitorInfo();
        bool loadWindowInfo();
        bool isRunningAsAdmin();
        bool buildRegions();
        DoorMonitorInfo* getMonitorInfoFromMonitorHandle(HMONITOR monitorHandle);
        std::vector<Region*> doorsWindowManager::calculateRegionsForMonitor(DoorMonitorInfo* monitor);
        bool matchWindowsToRegions();
        Region* getRegionsByID(int regionAid);
        bool focusRegion(Region* regionToFocus);
        MINMAXINFO getMinSize(HWND hwnd);

        WindowType GetWindowType(HWND hwnd);

    };
    
}
