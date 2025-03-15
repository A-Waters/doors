#include "Lib.h"


// Structure to hold window information

namespace lib {
    // Callback function to enumerate windows
    static BOOL CALLBACK enumWindowCallback(HWND windowHandle, LPARAM wm) {
        int length = GetWindowTextLength(windowHandle);
        
        // Add visible windows with non-empty title to the window manager
        if (IsWindowVisible(windowHandle) && length != 0) {
            // Convert LPARAM into windowManger* (pointer to class instance)
            doorsWindowManager* wmptr = reinterpret_cast<doorsWindowManager*>(wm);


            wchar_t* buffer = new wchar_t[length + 1];
            GetWindowText(windowHandle, buffer, length + 1);


            std::wstring your_wchar_in_ws(buffer);
            std::string your_wchar_in_str(your_wchar_in_ws.begin(), your_wchar_in_ws.end());
            const char* your_wchar_in_char = your_wchar_in_str.c_str();

            std::string windowTitle(your_wchar_in_char);
            delete[] buffer;

            // A pointer to a variable that receives the process identifier. 
            // If this parameter is not NULL, GetWindowThreadProcessId copies the identifier of
            // the process to the variable; otherwise, it does not. If the function fails, 
            // the value of the variable is unchanged.
            DWORD processId = 0;
            
            // Declare a DWORD variable to store the process ID
            DWORD threadId = GetWindowThreadProcessId(windowHandle, &processId);  // Pass the address of processId            

            DWORD error = GetLastError();
            if (error) {
                std::cout << "windowHandle: " << windowHandle << " | GetLastError: " << error << " | windowTitle: " << windowTitle << std::endl;
            }
            wmptr->_addWindowToWM(windowHandle, windowTitle, processId);
        }

        return TRUE;
    }

    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-monitorenumproc
    // wtf
    static BOOL CALLBACK enumMonitorCallback(HMONITOR monitorHandle, HDC deviceContextHandle, LPRECT cords, LPARAM wm) {

        doorsWindowManager* wmptr = reinterpret_cast<doorsWindowManager*>(wm);

        wmptr->_addMonitorToWM(monitorHandle, deviceContextHandle, cords);

        return TRUE;
    }


    // Constructor for windowManger
    doorsWindowManager::doorsWindowManager() {
        this->loadWindowInfo();
        this->loadMonitorInfo();
        this->printWindows();   
    }

    doorsWindowManager::~doorsWindowManager()
    {
        for (auto monitor : this->mActiveMonitors) {
            delete monitor;
        }

        for (auto window : this->mActiveWindows) {
            delete window;
        }
    }



    bool doorsWindowManager::loadWindowInfo() {
        EnumWindows(enumWindowCallback, reinterpret_cast<LPARAM>(this));
        return true;
    }
    
    bool doorsWindowManager::loadMonitorInfo() {
        EnumDisplayMonitors(NULL, NULL, enumMonitorCallback, reinterpret_cast<LPARAM>(this));
        return true;
    }


    // Method to add a window to the member list
    bool doorsWindowManager::_addWindowToWM(HWND fhwid, const std::string& ftitle, DWORD flpdwProcessId) {

        DoorWindowInfo* DWI = new DoorWindowInfo();
        DWI->hwnd = fhwid;
        DWI->title = ftitle;
        DWI->lpdwProcessId = flpdwProcessId;
        this->mActiveWindows.push_back(DWI);
        return true;
    }

    bool doorsWindowManager::_addMonitorToWM(HMONITOR monitorHandle, HDC deviceContextHandle, LPRECT cords) {

      lib::DoorMonitorInfo* DoorMonitorInfo = new lib::DoorMonitorInfo();
      DoorMonitorInfo->mMonitorHandle = monitorHandle;
      DoorMonitorInfo->mDeviceContextHandle = deviceContextHandle;
      DoorMonitorInfo->mCords = cords;

      MONITORINFO mi;
      mi.cbSize = sizeof(MONITORINFO);  // Initialize the structure size

      GetMonitorInfo(monitorHandle, &mi);
      
      DoorMonitorInfo->mMonitorInfo = mi;
      this->mActiveMonitors.push_back(DoorMonitorInfo);
      return true;
    }




    // Method to print out all windows stored
    void doorsWindowManager::printWindows() const {
        std::cout << "Windows enumerated:" << std::endl;
        
        for (DoorWindowInfo* window : this->mActiveWindows) {
            std::cout << window->hwnd << ": " << window->title << " --- pid:"<< window->lpdwProcessId << std::endl;
        }


        for (DoorMonitorInfo* monitor : this->mActiveMonitors) {
            monitor->mMonitorInfo;
            // Successfully retrieved monitor info
            std::cout << "Monitor Rectangle: (" << monitor->mMonitorInfo.rcMonitor.left << ", "
                << monitor->mMonitorInfo.rcMonitor.top << ") to (" << monitor->mMonitorInfo.rcMonitor.right << ", "
                << monitor->mMonitorInfo.rcMonitor.bottom << ")\n";
            std::cout << "Work Area Rectangle: (" << monitor->mMonitorInfo.rcWork.left << ", "
                << monitor->mMonitorInfo.rcWork.top << ") to (" << monitor->mMonitorInfo.rcWork.right << ", "
                << monitor->mMonitorInfo.rcWork.bottom << ")\n";
        }
        
    }

    bool doorsWindowManager::moveWindow(HWND hwnd, int x, int y, int width, int height)
    {
        return MoveWindow(hwnd, x, y, width, height, TRUE);
    }



}
