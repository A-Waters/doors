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
            // std::cout << "windowHandle: " << windowHandle << " | windowTitle.compare(Settings): " << (windowTitle != "Settings") << " | windowTitle: " << windowTitle << std::endl;
            if (windowTitle != "Settings" && windowTitle != "Program Manager" && windowTitle != "program.exe") {
                wmptr->_addWindowToWM(windowHandle, windowTitle, processId);
            }
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


    bool doorsWindowManager::isRunningAsAdmin() {
        BOOL isElevated = FALSE;
        HANDLE hToken = NULL;

        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elevation;
            DWORD dwSize;
            if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
                isElevated = elevation.TokenIsElevated;
            }
            CloseHandle(hToken);
        }

        return isElevated == TRUE;
    }

    bool doorsWindowManager::buildRegions()
    {
        // Get which windows are on what monitors
        for (DoorWindowInfo* DWIptr : mActiveWindows)
        {
            HMONITOR monitorHandle = MonitorFromWindow(DWIptr->hwnd, MONITOR_DEFAULTTONEAREST);
            if (this->mRegions.count(monitorHandle) == 0) {
                // If no entry for this monitor, create a new entry
                std::vector<Region*> newMonitors;
                Region* newRegion = new Region(0, 0, 0, 0);
                newRegion->DWI = DWIptr;
                newMonitors.push_back(newRegion);
                this->mRegions[monitorHandle] = newMonitors;
            }
            else {
                // If monitor already has a vector, add the new window
                Region* newRegion = new Region(0, 0, 0, 0);
                newRegion->DWI = DWIptr;
                this->mRegions[monitorHandle].push_back(newRegion);
            }
        }

        // Create regions for each monitor
        for (const auto& pair : this->mRegions) {

            std::vector<Region*> regionsForMonitor = pair.second; // Get windows on this monitor
            HMONITOR monitorHandle = pair.first;
            DoorMonitorInfo* monitor = getMonitorInfoFromHandle(monitorHandle);

            // Work area (excluding taskbar)
            int workAreaWidth = monitor->mMonitorInfo.rcWork.right - monitor->mMonitorInfo.rcWork.left;
            int workAreaHeight = monitor->mMonitorInfo.rcWork.bottom - monitor->mMonitorInfo.rcWork.top;
            int workAreaX = monitor->mMonitorInfo.rcWork.left;
            int workAreaY = monitor->mMonitorInfo.rcWork.top;

            size_t numberOfWindows = regionsForMonitor.size();

            // Ensure no division by zero or odd behavior
            int WidthPerRegion = workAreaWidth / numberOfWindows;


            std::cout << "==================== Monitor : " << monitorHandle << " | Width : " << WidthPerRegion <<" | Windows: " << numberOfWindows <<" ===============\n";
            int regionsPlaced = 0;
            for (Region* curr : regionsForMonitor) {
                regionsPlaced += 1;

                curr->mWidth = WidthPerRegion - this->gaps;
                curr->mHeight = workAreaHeight - this->gaps;
                curr->mX = (workAreaX + this->gaps) + (WidthPerRegion * (regionsPlaced-1));
                curr->mY = workAreaY + this->gaps;

                // Output for debugging
                std::cout << curr->DWI->title << " : " << " [" << regionsPlaced << "] " << curr->mX << "x" << curr->mY << " " << curr->mWidth << "x" << curr->mHeight << std::endl;

                // Move the window to its new position
                // MoveWindow(curr->DWI->hwnd, curr->mX, curr->mY, curr->mWidth, curr->mHeight, TRUE);
            }
        }

        // Ensure that all windows are matched to their new regions
        matchWindowsToRegions();

        return true;
    }

    DoorMonitorInfo* doorsWindowManager::getMonitorInfoFromHandle(HMONITOR monitorHandle) {
        auto monitorIt = std::find_if(this->mActiveMonitors.begin(), this->mActiveMonitors.end(), [monitorHandle](const DoorMonitorInfo* curr) {
            return monitorHandle == curr->mMonitorHandle;
            });

        if (monitorIt != this->mActiveMonitors.end()) {
            return *monitorIt; // Return the element pointed by the iterator
        }
        return nullptr; // or handle the case when the monitor isn't found
    }

    bool doorsWindowManager::matchWindowsToRegions()
    {
        for (const auto& pair : this->mRegions) {
            std::vector<Region*> regionsForWindow = pair.second;
            for (Region* region : regionsForWindow) {
                std::cout << "moving " << region->DWI->title 
                    << " to (" << region->mX
                    << ", " << region->mY
                    << ") with size of (" 
                    << region->mWidth << ","
                    << region->mHeight << ")\n";

                // add a check to check if full screen or minmized and then undoit if requreid
                // SetWindowLongPtr(region->DWI->hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
                moveWindow(region->DWI->hwnd, region->mX, region->mY, region->mWidth, region->mHeight);
                
                // SetWindowPos(region->DWI->hwnd, nullptr, 0, 0, region->mWidth, region->mHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
                ShowWindow(region->DWI->hwnd, SW_RESTORE);
                
            }
        };

        return true;
    }






    // Constructor for windowManger
    doorsWindowManager::doorsWindowManager() {
        this->mIsAdmin = isRunningAsAdmin();
        this->loadWindowInfo();
        this->loadMonitorInfo();
        this->printInfo();   
        this->moveMouse(100, 100);
        this->buildRegions();
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

    bool doorsWindowManager::selectWindow(DWORD pid)
    {
        
        for (DoorWindowInfo* DWI : this->mActiveWindows) {
            if (DWI->lpdwProcessId == pid) {
                MonitorFromWindow(DWI->hwnd, MONITOR_DEFAULTTONEAREST);
            }

        }

        return false;
    }

    // Method to print out all windows stored
    void doorsWindowManager::printInfo() const {        
        std::cout << "Is Admin?: " << (this->mIsAdmin ? "yes" : "no") << std::endl;

        std::cout << "Windows enumerated:" << std::endl;
        for (DoorWindowInfo* window : this->mActiveWindows) {
            std::cout << window->hwnd << ": " << window->title << " --- pid:"<< window->lpdwProcessId << std::endl;
        }

        /*
        for (DoorMonitorInfo* monitor : this->mActiveMonitors) {
            std::cout << monitor->mMonitorHandle << std::endl;
            // Successfully retrieved monitor info
            std::cout << "Monitor Rectangle: (" << monitor->mMonitorInfo.rcMonitor.left << ", "
                << monitor->mMonitorInfo.rcMonitor.top << ") to (" << monitor->mMonitorInfo.rcMonitor.right << ", "
                << monitor->mMonitorInfo.rcMonitor.bottom << ")\n";
            std::cout << "Work Area Rectangle: (" << monitor->mMonitorInfo.rcWork.left << ", "
                << monitor->mMonitorInfo.rcWork.top << ") to (" << monitor->mMonitorInfo.rcWork.right << ", "
                << monitor->mMonitorInfo.rcWork.bottom << ")\n";
        }*/
        
    }

    bool doorsWindowManager::moveWindow(HWND hwnd, int x, int y, int width, int height)
    {
        return MoveWindow(hwnd, x, y, width, height, TRUE);
    }

    bool doorsWindowManager::moveMouse(int x, int y) {
        if (this->mIsAdmin) {
            SetCursorPos(x, y);
            return true;
        }
        else {
            return false;
        }
    }



}
