#include "Lib.h"



// Structure to hold window information

namespace lib {

    LRESULT CALLBACK DoorsKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION) {
            KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;

            // Only process the key down event (when a key is pressed)
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                int keyCode = pKeyBoard->vkCode;

                // Handle the key press based on the keyCode
                handleKeyPress(keyCode);
            }
        }

        return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
    }

    void handleKeyPress(int keyCode) {
        switch (keyCode) {
        case VK_UP:
            std::cout << "Up arrow key pressed." << std::endl;
            // Add your custom logic to handle this key press here
            break;
        case VK_DOWN:
            std::cout << "Down arrow key pressed." << std::endl;
            // Add your custom logic to handle this key press here
            break;
        case 'A': // For letter 'A'
            std::cout << "'A' key pressed." << std::endl;
            // Add your custom logic to handle this key press here
            break;
        case 'B': // For letter 'B'
            std::cout << "'B' key pressed." << std::endl;
            // Add your custom logic to handle this key press here
            break;
        default:
            std::cout << "Other key pressed: " << keyCode << std::endl;
            break;
        }
    }

    // Callback function to enumerate windows
    static BOOL CALLBACK enumWindowCallback(HWND windowHandle, LPARAM wm) {
        int length = GetWindowTextLength(windowHandle);
        // A pointer to a variable that receives the process identifier. 
            // If this parameter is not NULL, GetWindowThreadProcessId copies the identifier of
            // the process to the variable; otherwise, it does not. If the function fails, 
            // the value of the variable is unchanged.
        DWORD processId = 0;

        // Declare a DWORD variable to store the process ID
        DWORD threadId = GetWindowThreadProcessId(windowHandle, &processId);

        printf("checking, %i \n", processId);
        // Add visible windows with non-empty title to the window manager
        if (IsWindowVisible(windowHandle) && length != 0) {
            printf("inside this thiny for window, %i \n", processId);
            // Convert LPARAM into windowManger* (pointer to class instance)
            doorsWindowManager* wmptr = reinterpret_cast<doorsWindowManager*>(wm);


            wchar_t* buffer = new wchar_t[length + 1];
            GetWindowText(windowHandle, buffer, length + 1);

            std::wstring your_wchar_in_ws(buffer);
            std::string your_wchar_in_str(your_wchar_in_ws.begin(), your_wchar_in_ws.end());
            const char* your_wchar_in_char = your_wchar_in_str.c_str();

            std::string windowTitle(your_wchar_in_char);
            delete[] buffer;

            printf("window is %s \n", windowTitle.c_str());

              // Pass the address of processId            

            DWORD error = GetLastError();
            if (error) {
                std::cout << "windowHandle: " << windowHandle << " | GetLastError: " << error << " | windowTitle: " << windowTitle << std::endl;
            }
            // std::cout << "windowHandle: " << windowHandle << " | windowTitle.compare(Settings): " << (windowTitle != "Settings") << " | windowTitle: " << windowTitle << std::endl;
            if (windowTitle != "Settings" && windowTitle != "Program Manager" && processId != 13988 && windowTitle != "Widgits") {
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
        // Get which windows are on what monitors and create regions for each one
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
        int setId = 0;
        // set the region locations and ids
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
                
                
                curr->id = setId;
                setId += 1;
                curr->mWidth = WidthPerRegion - this->gaps;
                curr->mHeight = workAreaHeight - this->gaps;
                curr->mX = (workAreaX + this->gaps) + (WidthPerRegion * regionsPlaced);
                curr->mY = workAreaY + this->gaps;
                regionsPlaced += 1;

                // Output for debugging
                std::cout << "BUILDING - " << curr->DWI->title << " : " << " [" << regionsPlaced << "] " << curr->mX << "x" << curr->mY << " " << curr->mWidth << "x" << curr->mHeight << std::endl;

                // Move the window to its new position
                // MoveWindow(curr->DWI->hwnd, curr->mX, curr->mY, curr->mWidth, curr->mHeight, TRUE);
            }
        }

        // Ensure that all windows are matched to their new region

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
        // update locations
        int monitorsSize = this->mActiveMonitors.size();

        for (DoorMonitorInfo* monitor : this->mActiveMonitors) {
            // std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~"  << monitor->mMonitorHandle << std::endl;
            std::vector<Region*> onMonitorRegions = this->mRegions[monitor->mMonitorHandle];

            if (onMonitorRegions.size() != 0) {
                
                int workAreaWidth = monitor->mMonitorInfo.rcWork.right - monitor->mMonitorInfo.rcWork.left;
                int workAreaHeight = monitor->mMonitorInfo.rcWork.bottom - monitor->mMonitorInfo.rcWork.top;

                int workAreaX = monitor->mMonitorInfo.rcWork.left;
                int workAreaY = monitor->mMonitorInfo.rcWork.top;
                int WidthPerRegion = workAreaWidth / onMonitorRegions.size();
                int regionsPlaced = 0;
                for (Region* region : onMonitorRegions) {
                    region->mWidth = WidthPerRegion - this->gaps;
                    region->mHeight = workAreaHeight - this->gaps;
                    region->mX = (workAreaX + this->gaps) + (WidthPerRegion * regionsPlaced);
                    region->mY = workAreaY + this->gaps;
                    regionsPlaced += 1;
                }
            }
            else {
                std::cout << "no regions on montitor" << monitor->mMonitorHandle << std::endl;
            }
            
        }
        // move items 
        for (const auto& pair : this->mRegions) {
            std::vector<Region*> regionsForWindow = pair.second;
            for (Region* region : regionsForWindow) {
                std::cout << "MOVING  " << region->DWI->title
                    << " With ID [" << region->id
                    << "] to (" << region->mX
                    << ", " << region->mY
                    << ") with size of (" 
                    << region->mWidth << ","
                    << region->mHeight << ")\n";

                // add a check to check if full screen or minmized and then undoit if requreid
                // SetWindowLongPtr(region->DWI->hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
                moveWindow(region->DWI->hwnd, region->mX, region->mY, region->mWidth, region->mHeight);
                DWORD err = GetLastError();
                if (err) {
                    printf("ERROR: %i \n", err);
                }
                //SetWindowPos(region->DWI->hwnd, nullptr, 0, 0, region->mWidth, region->mHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
                // ShowWindow(region->DWI->hwnd, SW_RESTORE);
                
            }
        };
        return true;
    }






    // Constructor for windowManger
    doorsWindowManager::doorsWindowManager() {

        keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, DoorsKeyboardProc, NULL, 0);
        if (keyboardHook == NULL) {
            std::cerr << "Failed to install keyboard hook!" << std::endl;
        }
        this->mIsAdmin = isRunningAsAdmin();
        this->loadWindowInfo();
        this->loadMonitorInfo();
        this->printInfo();   
        this->moveMouse(100, 100);
        this->buildRegions();
        matchWindowsToRegions();
        printf("====================== waiting ===================== \n");
        Sleep(3000);
        matchWindowsToRegions();
        this->printInfo();
        // Remove the keyboard hook

        if (keyboardHook != NULL) {
            UnhookWindowsHookEx(keyboardHook);
            keyboardHook = NULL;
        }
        // this->swapRegionsByID(3, 4);
    }

    doorsWindowManager::~doorsWindowManager()
    {
        for (auto monitor : this->mActiveMonitors) {
            for (Region* region : this->mRegions[monitor->mMonitorHandle]) {
                delete region;
            }
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

    bool doorsWindowManager::focusRegion(Region* regionToFocus)
    {
        SetFocus(regionToFocus->DWI->hwnd);
        DWORD error = GetLastError();
        if (error) {
            return false;
        }
        this->mFocused = regionToFocus;
        return true;
    }

    bool doorsWindowManager::swapRegionsByID(int regionAid, int regionBid)
    {
        Region* regionA = getRegionsByID(regionAid); 
        Region* regionB = getRegionsByID(regionBid);

        if (regionA == nullptr || regionB == nullptr || regionA == regionB) {
            printf("cant swap region with id %i with %i", regionAid, regionBid);
        };

        return swapRegions(regionA, regionB);
    }

    Region* doorsWindowManager::getRegionsByID(int regionID)
    {

        for (const auto& entry : mRegions) {
            HMONITOR monitor = entry.first;  // This is the key (HMONITOR)
            const std::vector<Region*>& regions = entry.second;  // This is the value (std::vector<Region*>)

            // Perform an action with each monitor and its regions
            std::cout << "Monitor: " << monitor << std::endl;

            for (Region* region : regions) {
                std::cout << "  Region ID: " << region->id << std::endl;
                if (region->id == regionID) return region;
            }
        }

        return nullptr;
    }

    bool doorsWindowManager::swapRegions(Region* regionA, Region* regionB)
    {
        printf("SWAPPING %s and %s \n", regionA->DWI->title.c_str(), regionB->DWI->title.c_str());

        DoorWindowInfo* temp = regionA->DWI;
        regionA->DWI = regionB->DWI;
        regionB->DWI = temp;
        matchWindowsToRegions();
        return true;
    }

    bool doorsWindowManager::createRegion(HMONITOR monitor, int x, int y, int length, int height, DoorWindowInfo* dwi)
    {
        Region* region = new Region(x, y, length, height);
        region->id = this->mRegions.size();
        region->DWI = dwi;
        this->mRegions[monitor].push_back(region);

        return true;
    }

    bool doorsWindowManager::deleteRegion(HMONITOR monitor, Region* regionToRemove)
    {
        // Get the vector of regions from the 'from' monitor
        std::vector<Region*>& monitorRegions = this->mRegions[monitor];

        // Iterate through the regions in 'fromMonitor'
        auto foudnRegion = std::find(monitorRegions.begin(), monitorRegions.end(), regionToRemove);

        // If region is found
        if (foudnRegion != monitorRegions.end()) {
            // Remove the region from the 'fromMonitor' vector
            monitorRegions.erase(foudnRegion);

            return true;
        }

        return false;
    }

    bool doorsWindowManager::moveRegion(HMONITOR from, HMONITOR to, Region* regionToMove) {
        // Check if the region is valid
        if (regionToMove == nullptr) {
            std::cout << "Error: regionToMove is null!" << std::endl;
            return false;
        }

        if (regionToMove->DWI == nullptr) {
            std::cout << "Error: DWI is null for region " << regionToMove->id << std::endl;
            return false;
        }

        // Safe logging if DWI is valid
        std::cout << "Moving region " << regionToMove->id << " (" << regionToMove->DWI->title.c_str() << ") from monitor " << from << " to monitor " << to << std::endl;

        // Check if the 'from' and 'to' monitors exist in the map
        auto fromIt = this->mRegions.find(from);
        auto toIt = this->mRegions.find(to);

        if (fromIt == this->mRegions.end()) {
            std::cout << "Error: 'from' monitor " << from << " does not exist in mRegions!" << std::endl;
            return false;
        }

        if (toIt == this->mRegions.end()) {
            std::cout << "Error: 'to' monitor " << to << " does not exist in mRegions!" << std::endl;
            return false;
        }

        std::vector<Region*>& fromMonitor = fromIt->second;
        std::vector<Region*>& toMonitor = toIt->second;
        
        if (fromMonitor.empty()) {
            std::cout << "No regions to move from monitor " << from << std::endl;
            return false;
        }

        // Find the region in the 'fromMonitor' vector
        auto foundRegion = std::find(fromMonitor.begin(), fromMonitor.end(), regionToMove);

        if (foundRegion != fromMonitor.end()) {
            // Remove the region from the 'fromMonitor' vector
            fromMonitor.erase(foundRegion);

            // Add the region to the 'toMonitor' vector
            toMonitor.push_back(regionToMove);

            std::cout << "Successfully moved region " << regionToMove->id << " to monitor " << to << std::endl;
            return true; // Return true to indicate success
        }
        else {
            std::cout << "Failed to find region " << regionToMove->id << " on monitor " << from << std::endl;
            return false; // Return false if the region wasn't found
        }
    }


    // Method to print out all windows stored
    void doorsWindowManager::printInfo() const {        
        std::cout << "Is Admin?: " << (this->mIsAdmin ? "yes" : "no") << std::endl;

        std::cout << "---------------------------------INFO---------------------------------" << std::endl;
        for (DoorMonitorInfo* monitor : this->mActiveMonitors) {
            std::cout << "================= " << monitor->mMonitorHandle << " =================" << std::endl;
            HMONITOR mhand = monitor->mMonitorHandle;

            // Safely find monitor regions for this monitor handle
            auto it = this->mRegions.find(mhand);
            if (it != this->mRegions.end()) {
                for (Region* region : it->second) {
                    if (region->DWI != nullptr) {
                        std::cout << "Window: " << region->DWI->title << " - Location: ("
                            << region->mX << ", " << region->mY << ") - Size: ("
                            << region->mWidth << ", " << region->mHeight << ")" << std::endl;
                    }
                    else {
                        std::cout << "Region " << region->id << " has no associated DoorWindowInfo." << std::endl;
                    }
                }
            }
            else {
                std::cout << "No regions found for monitor handle " << mhand << std::endl;
            }
        }
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
