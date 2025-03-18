#include "Lib.h"
using namespace std;



// Structure to hold window information

namespace lib {


   

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

        // Add visible windows with non-empty title to the window manager
        if (IsWindowVisible(windowHandle) && length != 0) {
            printf("inside this thiny for window, %i \n", processId);
            // Convert LPARAM into windowManger* (pointer to class instance)
            doorsWindowManager* wmptr = reinterpret_cast<doorsWindowManager*>(wm);


            wchar_t* buffer = new wchar_t[length + 1];
            GetWindowText(windowHandle, buffer, length + 1);

            // https://stackoverflow.com/questions/3019977/convert-wchar-t-to-char
            std::wstring your_wchar_in_ws(buffer);
            std::string your_wchar_in_str(your_wchar_in_ws.begin(), your_wchar_in_ws.end());
            const char* your_wchar_in_char = your_wchar_in_str.c_str();

            std::string windowTitle(your_wchar_in_char);
            delete[] buffer;

            printf("window is %s \n", windowTitle.c_str());

              // Pass the address of processId            

            DWORD error = GetLastError();
            if (error) {
                std::cout << "[ERROR] windowHandle: " << windowHandle << " | GetLastError: " << error << " | windowTitle: " << windowTitle << std::endl;
            }
            else {
                wmptr->_addWindowToWM(windowHandle, windowTitle, processId);
            }
            // std::cout << "windowHandle: " << windowHandle << " | windowTitle.compare(Settings): " << (windowTitle != "Settings") << " | windowTitle: " << windowTitle << std::end
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
                newRegion->DWI->minSizes = getMinSize(DWIptr->hwnd);
                newMonitors.push_back(newRegion);
                this->mRegions[monitorHandle] = newMonitors;
            }
            else {
                // If monitor already has a vector, add the new window
                Region* newRegion = new Region(0, 0, 0, 0);
                newRegion->DWI = DWIptr;
                newRegion->DWI->minSizes = getMinSize(DWIptr->hwnd);
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
            int totalMinWidth = 0;

            // Calculate the total minimum width required for all windows
            for (Region* region : regionsForMonitor) {
                totalMinWidth += region->DWI->minSizes.ptMinTrackSize.x;
                std::cout << region->DWI->title << " | Minimum Track Size: "
                    << region->DWI->minSizes.ptMinTrackSize.x << "x"
                    << region->DWI->minSizes.ptMinTrackSize.y << std::endl;
            }

            // Calculate the total gaps between the regions
            int totalGaps = (this->gaps * numberOfWindows) + this->gaps;

            // Check if the total minimum width exceeds the available width after considering gaps
            int availableWidth = workAreaWidth - totalMinWidth - totalGaps;

            if (availableWidth < 0) {
                std::cout << "Total minimum width exceeds available space. Need to handle this!" << std::endl;
                // Handle scaling or stacking if the total minimum width exceeds available space
                // For now, you could potentially handle this by stacking the windows vertically
                // or scaling them down to fit
                availableWidth = 0;  // No extra space, all windows will take their minimum width
            }

            // Now we calculate the width per region, ensuring it doesn't exceed the available space
            int widthPerRegion = (availableWidth / numberOfWindows);

            std::cout << "Available width for regions: " << availableWidth
                << " | Width per region: " << widthPerRegion << std::endl;

            // Now we need to place each region
            int regionsPlaced = 0;
            int setId = 0; // Initialize setId if not set elsewhere

            // Track the position of the last placed region
            int lastPlacedX = workAreaX + this->gaps;  // Start at the work area X with initial gap
            int lastPlacedY = workAreaY + this->gaps;  // Start at the work area Y with initial gap

            for (Region* region : regionsForMonitor) {
                // Ensure the region’s width respects its minimum size
                region->id = setId;
                region->mWidth = max(widthPerRegion, region->DWI->minSizes.ptMinTrackSize.x);
                region->mHeight = workAreaHeight - this->gaps;

                // Position the region based on the last placed region
                region->mX = lastPlacedX;
                region->mY = lastPlacedY;

                // Update lastPlacedX for the next region
                lastPlacedX = region->mX + region->mWidth + this->gaps;  // Move to the next position after the current region

                regionsPlaced += 1;
                setId += 1;

                // Output for debugging
                std::cout << "BUILDING - " << region->DWI->title << " : "
                    << " [" << regionsPlaced << "] "
                    << region->mX << "x" << region->mY << " "
                    << region->mWidth << "x" << region->mHeight << std::endl;

                // Move the window to its new position
                // MoveWindow(region->DWI->hwnd, region->mX, region->mY, region->mWidth, region->mHeight, TRUE);
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

    // returns regions which can not fit on screen
    std::vector<Region*> doorsWindowManager::calulateRegionForMonitor(DoorMonitorInfo* monitor) {

        std::vector<Region*> removedRegions;

        std::vector<Region*> regions = this->mRegions[monitor->mMonitorHandle];

        if (regions.size() <= 0) {
            return removedRegions;
        }

        // Work area (excluding taskbar)
        int workAreaWidth = monitor->mMonitorInfo.rcWork.right - monitor->mMonitorInfo.rcWork.left;
        int workAreaHeight = monitor->mMonitorInfo.rcWork.bottom - monitor->mMonitorInfo.rcWork.top;
        int workAreaX = monitor->mMonitorInfo.rcWork.left;
        int workAreaY = monitor->mMonitorInfo.rcWork.top;

        int totalMinWidth = 0;
        for (Region* region : regions) {
            totalMinWidth += region->DWI->minSizes.ptMinTrackSize.x;
        }

        if (totalMinWidth > workAreaWidth) {
            printf("too big");
            // TODO
            // future alex issue
        }
        
        int widthPerRegion = (workAreaWidth - totalMinWidth) / regions.size();

        int lastPlacedRegionWidth = 0;
        int lastPlacedRegionX = 0;

        for (Region * region : regions) {
            region->mX = lastPlacedRegionX + lastPlacedRegionWidth + this->gaps;
            region->mY = workAreaY;

            region->mWidth = max(region->DWI->minSizes.ptMinTrackSize.x, totalMinWidth);
            region->mHeight = workAreaHeight;
        }

        return removedRegions;
    }


    bool doorsWindowManager::matchWindowsToRegions()
    {
        // update locations
        int monitorsSize = this->mActiveMonitors.size();

        for (DoorMonitorInfo* monitor : this->mActiveMonitors) {
           
            std::vector<Region*> onMonitorRegions = this->mRegions[monitor->mMonitorHandle];



            if (onMonitorRegions.size() != 0) {
                



                int workAreaWidth = monitor->mMonitorInfo.rcWork.right - monitor->mMonitorInfo.rcWork.left;
                int workAreaHeight = monitor->mMonitorInfo.rcWork.bottom - monitor->mMonitorInfo.rcWork.top;

                int workAreaX = monitor->mMonitorInfo.rcWork.left;
                int workAreaY = monitor->mMonitorInfo.rcWork.top;
                
                
                int widthPerRegion = workAreaWidth / onMonitorRegions.size();
                int regionsPlaced = 0;
                
                // Track the position of the last placed region
                int lastPlacedX = workAreaX + this->gaps;  // Start at the work area X with initial gap
                int lastPlacedY = workAreaY + this->gaps;  // Start at the work area Y with initial gap
    
                for (Region* region : onMonitorRegions) {
                    region->mWidth = max(widthPerRegion, region->DWI->minSizes.ptMinTrackSize.x);
                    region->mHeight = workAreaHeight - this->gaps;

                    // Position the region based on the last placed region
                    region->mX = lastPlacedX;
                    region->mY = lastPlacedY;

                    // Update lastPlacedX for the next region
                    lastPlacedX = region->mX + region->mWidth + this->gaps;  // Move to the next position after the current region

                    regionsPlaced += 1;
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

        WindowType wt = GetWindowType(fhwid);
        
        // why is everything i care about unkown? 
        if (wt == WindowType::Unknown) {
            DoorWindowInfo* DWI = new DoorWindowInfo();
            DWI->hwnd = fhwid;
            DWI->title = ftitle;
            DWI->lpdwProcessId = flpdwProcessId;
            this->mActiveWindows.push_back(DWI);
            return true;
        }
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

    MINMAXINFO doorsWindowManager::getMinSize(HWND hwnd)
    {
        MINMAXINFO minMaxInfo;
        // Send the WM_GETMINMAXINFO message to the window to retrieve the min size
        SendMessage(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&minMaxInfo);

        std::cout << minMaxInfo.ptMinTrackSize.x << std::endl;
        std::cout << minMaxInfo.ptMinTrackSize.y << std::endl;
        // check for bad numbers i.e. -858993460
        if (minMaxInfo.ptMinTrackSize.x > 2000) {
            minMaxInfo.ptMinTrackSize.x = 2000;
        }
        if (minMaxInfo.ptMinTrackSize.x < 0) {
            minMaxInfo.ptMinTrackSize.x = 0;
        }
        if (minMaxInfo.ptMinTrackSize.y > 2000) {
            minMaxInfo.ptMinTrackSize.y = 2000;
        }
        if (minMaxInfo.ptMinTrackSize.y < 1000) {
            minMaxInfo.ptMinTrackSize.y = 0;
        }

        if (minMaxInfo.ptMaxTrackSize.x > 2000) {
            minMaxInfo.ptMaxTrackSize.x = 2000;
        }
        if (minMaxInfo.ptMaxTrackSize.x < 0) {
            minMaxInfo.ptMaxTrackSize.x = 0;
        }
        if (minMaxInfo.ptMaxTrackSize.y > 2000) {
            minMaxInfo.ptMaxTrackSize.y = 2000;
        }
        if (minMaxInfo.ptMaxTrackSize.y < 1000) {
            minMaxInfo.ptMaxTrackSize.y = 0;
        }

        return minMaxInfo;
    }

    WindowType doorsWindowManager::GetWindowType(HWND hwnd)
    {
        // Retrieve window styles
        DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
        DWORD dwExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

        // Check for Popup Window (usually has WS_POPUP style)
        if (dwStyle & WS_POPUP) {
            return Popup;
        }

        std::cout << "Style = " << dwStyle << " Exstyle = " << dwExStyle << std::endl;
        // Check for a MessageBox - These are special modal windows
        // You could also check if the window is a child of the MessageBox class
        wchar_t className[512];
        GetClassName(hwnd, className, sizeof(className));

        // https://stackoverflow.com/questions/3019977/convert-wchar-t-to-char
        std::wstring your_wchar_in_ws(className);
        std::string your_wchar_in_str(your_wchar_in_ws.begin(), your_wchar_in_ws.end());
        const char* your_wchar_in_char = your_wchar_in_str.c_str();


        if (strcmp(your_wchar_in_char, "#32770") == 0) { // The class name for MessageBox is "#32770"
            return WindowType::dMessageBox;
        }

        // Check for Dialog Window (typically windows created using CreateDialog or CreateDialogParam)
        if (dwExStyle & WS_EX_DLGMODALFRAME) {
            return WindowType::Dialog;
        }

        // Check for Tool Window (typically has WS_EX_TOOLWINDOW style)
        if (dwExStyle & WS_EX_TOOLWINDOW) {
            return WindowType::ToolWindow;
        }

        // Check for a Child Window (has WS_CHILD style)
        if (dwStyle & WS_CHILD) {
            return WindowType::Child;
        }

        // Check for Overlay Window (layered window, which typically uses WS_EX_LAYERED)
        if (dwExStyle & WS_EX_LAYERED) {
            return WindowType::Overlay;
        }

        // Check for Invisible Window (this could mean no visible styles or transparency, or minimized window)
        if (dwStyle & WS_MINIMIZE) {
            return WindowType::Invisible;
        }

        // If it's not a special type, we assume it's a Top-Level Window
        if (dwExStyle & WS_EX_TOPMOST) {
            return WindowType::TopLevel; // Could be a top-level window with specific attributes
        }

        // Default to Unknown if no criteria matched
        return WindowType::Unknown;
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


        std::cout << "---------------------------------INFO---------------------------------" << std::endl;
        std::cout << "Is Admin?: " << (this->mIsAdmin ? "yes" : "no") << std::endl;
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

        std::cout << "---------------------------------END---------------------------------" << std::endl;
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
