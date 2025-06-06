#ifndef WINDOWS_MANAGER_H
#define WINDOWS_MANAGER_H

#include "commonDoors.h"
#include <iostream>
#include <vector>
#include <map>


/*
        HOW EVERYTHING RELATES

        std::vector <DoorWindowInfo*> mActiveWindows;
        std::vector <DoorMonitorInfo*> mActiveMonitors;
        std::map<HMONITOR, std::vector<Region*>> mRegions;


                                      +----------------------+
                                      |  DoorMonitorInfo     | <-------------
                                      +----------------------+              |
                               ------ | - mMonitorHandle     |              |
                               |      | - mDeviceContextHandle|             | 
                               |      | - mCords             |              |
                               |      | - mMonitorInfo       |              |
                               |      +----------------------+              |
                               |                                            | getMonitorInfoFromMonitorHandle()
                               |      +----------------------+              |
                               |      |  DoorWindowInfo      | <---         |
                               |      +----------------------+    |         |
     mRegions[mMonitorHandle]  |      | - hwnd               |    |         |
                               |      | - title              |    |         |
                               |      | - lpdwProcessId      |    |         |
                               |      | - minSizes           |    |         |
                               |      | - monitorHandle      | ---+----------
                               |      +----------------------+    |
                               |                                  |
                               |      +----------------------+    |
                               ------>|      Region          |    |
                                      +----------------------+    |
                                      | - id                 |    |
                                      | - mX                 |    |
                                      | - mY                 |    |
                                      | - mWidth             |    |
                                      | - mHeight            |    |
                                      | - DWI*               | ----
                                      +----------------------+


*/



namespace lib {
    
    enum Direction {
        left,
        right,
        up,
        down,
    };

    class DoorsWindowManager {

        


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
            HMONITOR monitorHandle;

            DoorWindowInfo() : hwnd(0), title(""), lpdwProcessId(DWORD()), minSizes(), monitorHandle() {};
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


    public:
        bool _addWindowToWM(HWND hwnd, const std::string& title, DWORD lpdwProcessId);
        bool _addMonitorToWM(HMONITOR monitorHandle, HDC deviceContextHandle, LPRECT cords);
        DoorsWindowManager();
        ~DoorsWindowManager();
        void printInfo();
        bool moveWindow(DoorWindowInfo* dwi, int x, int y, int width, int height);
        bool moveMouse(int x, int y);
        Region* getFocus();
        // DoorsWindowManager::Region* getRegionByWindowHandle(HWND hwnd);
        bool shiftRegionToDirection(Region* region, Direction dir);
        bool shiftFocusToDirection(Direction dir);

        bool swapRegionsByID(int regionAid, int regionBid);
        bool swapRegions(Region* regionA, Region* regionB);
        bool createRegion(HMONITOR monitor, int x, int y, int length, int height, DoorWindowInfo* dwi);
        bool deleteRegion(HMONITOR monitor, Region* regionToRemove);
        // bool moveRegion(HMONITOR from, HMONITOR to, Region* regionToMove);
        bool moveRegionToMonitor(HMONITOR from, HMONITOR to, Region* regionToMove, Direction dir);
        bool loadMonitorInfo();
        bool loadWindowInfo();
        bool buildRegions();

    private:
        DoorMonitorInfo* mFocusedMonitor;
        Region* mFocusedRegion;
        int sideGaps = 20;
        int topGap = 20;
        int botGap = 20;
        int innerGap = 20;
        std::vector <DoorWindowInfo*> mActiveWindows;
        std::vector <DoorMonitorInfo*> mActiveMonitors;
        std::map<HMONITOR, std::vector<Region*>> mRegions;

        bool mIsAdmin;

        bool isRunningAsAdmin();

        DoorMonitorInfo* getMonitorInfoFromMonitorHandle(HMONITOR monitorHandle);
        std::vector<Region*> calculateRegionsForMonitor(DoorMonitorInfo* monitor);

        bool matchWindowsToRegions();
        Region* getRegionsByID(int regionAid);
        bool focusLocation(Region* regionToFocus, DoorMonitorInfo* monitorToFocus);
        MINMAXINFO getSizeConstrains(HWND hwnd);
        WindowType GetWindowType(HWND hwnd);
        void highlightRegion(Region* regionToHighlight);
        std::vector<Region*> getRegionsFromMonitorInfo(const DoorMonitorInfo* handle);

    };
    
}

#endif