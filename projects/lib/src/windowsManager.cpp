#include "windowsManager.h"




// Structure to hold window information

namespace lib {
    void LogError(const char* functionName, HWND hwnd, int x, int y, int width, int height) {
        DWORD err = GetLastError();
        if (err) {
            // Get a human-readable error message
            LPVOID msgBuffer;
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                err,
                0, // Default language
                (LPTSTR)&msgBuffer,
                0,
                NULL
            );

            // Print detailed error message with function name and parameters
            printf("%s failed. Error code: %lu, Message: %s\n", functionName, err, (char*)msgBuffer);
            printf("Attempted to move window with HWND: %p, Position: (%d, %d), Size: (%d, %d)\n", hwnd, x, y, width, height);

            // Free the buffer allocated by FormatMessage
            LocalFree(msgBuffer);
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

        // Add visible windows with non-empty title to the window manager
        if (IsWindowVisible(windowHandle) && length != 0) {
            printf("inside this thiny for window, %i \n", processId);
            // Convert LPARAM into windowManger* (pointer to class instance)
            DoorsWindowManager* wmptr = reinterpret_cast<DoorsWindowManager*>(wm);


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
        DoorsWindowManager* wmptr = reinterpret_cast<DoorsWindowManager*>(wm);
        wmptr->_addMonitorToWM(monitorHandle, deviceContextHandle, cords);
        return TRUE;
    }


    bool DoorsWindowManager::isRunningAsAdmin() {
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

    bool DoorsWindowManager::buildRegions()
    {
        // not gonna work when deleting regions and then adding one back as we base the id on the size of the arr meaning there can be multiple regions with the same ID;
        int regionsPlaced = 0;
        // Get which windows are on what monitors and create regions for each one and set IDs
        for (DoorWindowInfo* DWIptr : mActiveWindows)
        {
            HMONITOR monitorHandle = MonitorFromWindow(DWIptr->hwnd, MONITOR_DEFAULTTONEAREST);

            DWIptr->monitorHandle = monitorHandle;


            if (this->mRegions.count(monitorHandle) == 0) {
                // If no entry for this monitor, create a new entry
                std::vector<Region*> newMonitors;
                Region* newRegion = new Region(0, 0, 0, 0);
                newRegion->id = regionsPlaced;
                newRegion->DWI = DWIptr;
                newRegion->DWI->minSizes = getSizeConstrains(DWIptr->hwnd);
                newMonitors.push_back(newRegion);
                this->mRegions[monitorHandle] = newMonitors;
            }
            else {
                // If monitor already has a vector, add the new window
                Region* newRegion = new Region(0, 0, 0, 0);
                newRegion->id = regionsPlaced;
                newRegion->DWI = DWIptr;
                newRegion->DWI->minSizes = getSizeConstrains(DWIptr->hwnd);
                this->mRegions[monitorHandle].push_back(newRegion);
            }
            regionsPlaced += 1;
        }
        // set the region locations
        for (DoorMonitorInfo* monitor : this->mActiveMonitors) {
            calculateRegionsForMonitor(monitor);
        }

        return true;
    }

    DoorsWindowManager::DoorMonitorInfo* DoorsWindowManager::getMonitorInfoFromMonitorHandle(HMONITOR monitorHandle) {
        auto monitorIt = std::find_if(this->mActiveMonitors.begin(), this->mActiveMonitors.end(), [monitorHandle](const DoorMonitorInfo* curr) {
            return monitorHandle == curr->mMonitorHandle;
            });

        if (monitorIt != this->mActiveMonitors.end()) {
            return *monitorIt; // Return the element pointed by the iterator
        }
        return nullptr; // or handle the case when the monitor isn't found
    }



    // returns regions which can not fit on screen
    std::vector<DoorsWindowManager::Region*> DoorsWindowManager::calculateRegionsForMonitor(DoorMonitorInfo* monitor) {

        std::vector<Region*> removedRegions;
        std::vector<Region*>& regions = mRegions[monitor->mMonitorHandle];

        std::cout << " =================================== " << monitor->mMonitorHandle << " =================================== " << std::endl;

        if (regions.empty()) {
            std::cerr << "No regions to process!" << std::endl;
            return removedRegions;  // No regions to process
        }

        // Work area (excluding taskbar)
        int workAreaWidth = monitor->mMonitorInfo.rcWork.right - monitor->mMonitorInfo.rcWork.left;
        int workAreaHeight = monitor->mMonitorInfo.rcWork.bottom - monitor->mMonitorInfo.rcWork.top;
        int workAreaX = monitor->mMonitorInfo.rcWork.left;
        int workAreaY = monitor->mMonitorInfo.rcWork.top;

        // Subtract the gaps from the work area to get the usable area
        int usableWidth = workAreaWidth - 2 * sideGaps;  // minus left and right side gaps
        int usableHeight = workAreaHeight - topGap - botGap;  // minus top and bottom gaps

        std::cout << "Usable Area Dimensions: Width = " << usableWidth << ", Height = " << usableHeight << std::endl;

        // Edge Case: Handle negative or zero usable area
        if (usableHeight <= 0 || usableWidth <= 0) {
            std::cerr << "Error: Invalid usable area dimensions!" << std::endl;
            return removedRegions;
        }

        // Calculate total minimum width and find the smallest region
        int totalMinWidth = 0;
        Region* smallestRegion = nullptr;
        for (Region* region : regions) {
            totalMinWidth += region->DWI->minSizes.ptMinTrackSize.x;
            if (!smallestRegion || region->DWI->minSizes.ptMinTrackSize.x < smallestRegion->DWI->minSizes.ptMinTrackSize.x) {
                smallestRegion = region;
            }
        }

        // If the total min width exceeds the usable width, remove the smallest region
        if (totalMinWidth > usableWidth) {
            std::cerr << "Error: Total minimum width exceeds the available usable space! Removing the smallest region." << std::endl;

            // Add smallest region to removed list and remove it from regions
            removedRegions.push_back(smallestRegion);
            regions.erase(std::remove(regions.begin(), regions.end(), smallestRegion), regions.end());
            mRegions.erase(monitor->mMonitorHandle);  // Remove the monitor handle from regions map

            // Recalculate the total min width after removing the smallest region
            totalMinWidth = 0;
            for (Region* region : regions) {
                totalMinWidth += region->DWI->minSizes.ptMinTrackSize.x;
            }
        }

        // Handle layout calculation depending on the number of regions
        if (regions.size() == 1) {
            regions[0]->mWidth = usableWidth;  // Assign the full usable width to the single region
        }
        else {
            int sizePerRegionBaseline = usableWidth / regions.size();
            int totalExtraWidth = 0;
            int totalMinWidth = 0;
            int numberOfSmallerRegions = 0;
            bool allSmallerThanBaseline = true;

            // Calculate the total minimum width and track larger regions
            for (Region* region : regions) {
                int regionMinWidth = region->DWI->minSizes.ptMinTrackSize.x;
                if (regionMinWidth < sizePerRegionBaseline) {
                    totalMinWidth += regionMinWidth;
                    numberOfSmallerRegions++;
                }
                else {
                    totalExtraWidth += regionMinWidth;
                    allSmallerThanBaseline = false;
                }
                std::cout << "REGION min size for " << region->DWI->title << " | Min Width = " << regionMinWidth << std::endl;
            }

            if (allSmallerThanBaseline) {
                std::cout << "------------Case: All regions smaller than baseline--------------" << std::endl;
                for (Region* region : regions) {
                    region->mWidth = sizePerRegionBaseline;
                    std::cout << "Adjusted width for region: " << region->DWI->title << " | Width = " << region->mWidth << std::endl;
                }
            }
            else {
                int remainingSpace = usableWidth - totalMinWidth - totalExtraWidth;
                std::cout << "Remaining space: " << remainingSpace << std::endl;

                if (remainingSpace > 0 && numberOfSmallerRegions > 0) {
                    int spacePerSmallerRegion = remainingSpace / numberOfSmallerRegions;

                    for (Region* region : regions) {
                        int regionMinWidth = region->DWI->minSizes.ptMinTrackSize.x;
                        if (regionMinWidth < sizePerRegionBaseline) {
                            region->mWidth = regionMinWidth + spacePerSmallerRegion;
                            std::cout << "Adjusted width for smaller region: " << region->DWI->title << " | Width = " << region->mWidth << std::endl;
                        }
                        else {
                            region->mWidth = regionMinWidth;  // Keep regions larger than baseline at their min size
                        }
                    }
                }
            }
        }

        // Adjust positions and sizes for each region
        int nextXStart = workAreaX + sideGaps;
        int nextYStart = workAreaY + topGap;


        // im not convinced this is right 
        // some stuff looks weird....
        for (size_t i = 0; i < regions.size(); ++i) {
            Region* region = regions[i];

            // why care about usableWidth?
            if (nextXStart + region->mWidth + sideGaps > workAreaX + usableWidth) {
                region->mWidth = workAreaX + usableWidth - nextXStart - sideGaps;  // Adjust width to fit within available space
            }

            if (i < regions.size() - 1) {
                Region* nextRegion = regions[i + 1];
                int spaceToReduce = (nextRegion->mWidth + region->mWidth + innerGap) - usableWidth;
                if (spaceToReduce > 0) {
                    // ? why 2
                    int reduceFromCurrent = min(spaceToReduce / 2, region->mWidth - region->DWI->minSizes.ptMinTrackSize.x);
                    int reduceFromNext = spaceToReduce - reduceFromCurrent;

                    region->mWidth -= reduceFromCurrent;
                    nextRegion->mWidth -= reduceFromNext;
                }
            }

            region->mY = nextYStart;
            region->mX = nextXStart;
            region->mHeight = usableHeight;

            std::cout << "Assigned position to " << region->DWI->title << " | X = " << region->mX << ", Y = " << region->mY << ", Width = " << region->mWidth << ", Height = " << region->mHeight << std::endl;

            nextXStart = region->mX + region->mWidth + innerGap;
        }

        return removedRegions;
    }

    

    

    bool DoorsWindowManager::matchWindowsToRegions()
    {
        // update locations
        int monitorsSize = this->mActiveMonitors.size();

        for (DoorMonitorInfo* monitor : this->mActiveMonitors) {
            calculateRegionsForMonitor(monitor);
        }
        // move items 
        for (const auto& pair : this->mRegions) {
            std::vector<Region*> regionsForWindow = pair.second;
            for (Region* region : regionsForWindow) {
                std::cout << "moving to match regions " << region->DWI->title
                    << " With ID [" << region->id
                    << "] to (" << region->mX
                    << ", " << region->mY
                    << ") with size of (" 
                    << region->mWidth << ","
                    << region->mHeight << ")\n";

                // add a check to check if full screen or minmized and then undoit if requreid
                // SetWindowLongPtr(region->DWI->hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
                moveWindow(region->DWI, region->mX, region->mY, region->mWidth, region->mHeight);
                DWORD err = GetLastError();
                if (err) {
                    LogError("moveWindow", region->DWI->hwnd, region->mX, region->mY, region->mWidth, region->mHeight);
                }
                //SetWindowPos(region->DWI->hwnd, nullptr, 0, 0, region->mWidth, region->mHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
                // ShowWindow(region->DWI->hwnd, SW_RESTORE);
                
            }
        };
        return true;
    }






    // Constructor for windowManger
    DoorsWindowManager::DoorsWindowManager() {


        this->mIsAdmin = isRunningAsAdmin();
        this->loadWindowInfo();
        this->loadMonitorInfo();       
        this->printInfo();   
        this->moveMouse(100, 100);
        this->buildRegions();
        matchWindowsToRegions();
        printf("====================== waiting ===================== \n");
        Sleep(2000);
        // Remove the keyboard hook
        if (mActiveMonitors.size() >= 2) {
            
            HMONITOR fromMonitor = mActiveMonitors[0]->mMonitorHandle;  // First monitor
            HMONITOR toMonitor = mActiveMonitors[1]->mMonitorHandle;    // Second monitor

            // Find a region associated with the first monitor (just an example)
            if (!mRegions[fromMonitor].empty()) {
                printf("====================== MOIVING ONE ===================== \n");
                
                Region* regionToMove = mRegions[fromMonitor][0];  // Assume first region on the first monitor
                focusRegion(regionToMove);
                // Move this region to the second monitor
                bool result = moveRegionToMonitor(fromMonitor, toMonitor, regionToMove);

                if (result) {
                    std::cout << "Region moved successfully!" << std::endl;
                }
                else {
                    std::cout << "Failed to move region!" << std::endl;
                }

                
                /*printf("GET READY\n");
                Sleep(3);
                shiftRegionToDirection(regionToMove, DoorsWindowManager::left);
                matchWindowsToRegions();
                Sleep(3);
                shiftRegionToDirection(regionToMove, DoorsWindowManager::right);
                matchWindowsToRegions();
                Sleep(3);
                shiftRegionToDirection(regionToMove, DoorsWindowManager::down);
                matchWindowsToRegions();
                Sleep(3);
                shiftRegionToDirection(regionToMove, DoorsWindowManager::up);
                matchWindowsToRegions();
                */
            }
        }
        matchWindowsToRegions();
        

        // this->swapRegionsByID(3, 4);
    }

    DoorsWindowManager::~DoorsWindowManager()
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

    bool DoorsWindowManager::loadWindowInfo() {
        EnumWindows(enumWindowCallback, reinterpret_cast<LPARAM>(this));
        return true;
    }
    
    bool DoorsWindowManager::loadMonitorInfo() {
        EnumDisplayMonitors(NULL, NULL, enumMonitorCallback, reinterpret_cast<LPARAM>(this));
        return true;
    }


    // Method to add a window to the member list
    bool DoorsWindowManager::_addWindowToWM(HWND fhwid, const std::string& ftitle, DWORD flpdwProcessId) {

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

    bool DoorsWindowManager::_addMonitorToWM(HMONITOR monitorHandle, HDC deviceContextHandle, LPRECT cords) {

      DoorMonitorInfo* DoorMonitorInfo = new DoorsWindowManager::DoorMonitorInfo();
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

    bool DoorsWindowManager::focusRegion(Region* regionToFocus)
    {
        SetFocus(regionToFocus->DWI->hwnd);
        
        DWORD error = GetLastError();
        if (error) {
            return false;
        }
        highlightRegion(regionToFocus);
        this->mFocused = regionToFocus;
        return true;
    }

    MINMAXINFO DoorsWindowManager::getSizeConstrains(HWND hwnd)
    {
        MINMAXINFO minMaxInfo;
        // Send the WM_GETMINMAXINFO message to the window to retrieve the min size
        SendMessage(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&minMaxInfo);

        std::cout << minMaxInfo.ptMinTrackSize.x << std::endl;
        std::cout << minMaxInfo.ptMinTrackSize.y << std::endl;
        // check for bad numbers i.e. -858993460?
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

    void DoorsWindowManager::highlightRegion(Region* regionToHighlight)
    {
        // Get the current window style
        // LONG style = GetWindowLongPtrA(regionToHighlight->DWI->hwnd, GWL_STYLE);

        // Add a border style (WS_BORDER or WS_THICKFRAME)
        // style |= WS_THICKFRAME; // Add a thin border to the window
        // Alternatively, for a thicker, resizable border, use:
        // style |= WS_THICKFRAME;

        // Set the new style
        // SetWindowLong(regionToHighlight->DWI->hwnd, GWL_STYLE, style);

        // Redraw the window to reflect the new border
        // SetWindowPos(regionToHighlight->DWI->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOREPOSITION | SWP_NOSIZE);
    }

    DoorsWindowManager::WindowType DoorsWindowManager::GetWindowType(HWND hwnd)
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

    bool DoorsWindowManager::swapRegionsByID(int regionAid, int regionBid)
    {
        Region* regionA = getRegionsByID(regionAid); 
        Region* regionB = getRegionsByID(regionBid);

        if (regionA == nullptr || regionB == nullptr || regionA == regionB) {
            printf("cant swap region with id %i with %i", regionAid, regionBid);
        };

        return swapRegions(regionA, regionB);
    }

    DoorsWindowManager:: Region* DoorsWindowManager::getRegionsByID(int regionID)
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

    bool DoorsWindowManager::swapRegions(Region* regionA, Region* regionB)
    {
        printf("SWAPPING %s and %s \n", regionA->DWI->title.c_str(), regionB->DWI->title.c_str());

        DoorWindowInfo* temp = regionA->DWI;
        regionA->DWI = regionB->DWI;
        regionB->DWI = temp;
        matchWindowsToRegions();
        return true;
    }

    bool DoorsWindowManager::createRegion(HMONITOR monitor, int x, int y, int length, int height, DoorWindowInfo* dwi)
    {
        Region* region = new Region(x, y, length, height);
        region->id = this->mRegions.size();
        region->DWI = dwi;
        this->mRegions[monitor].push_back(region);

        return true;
    }

    bool DoorsWindowManager::deleteRegion(HMONITOR monitor, Region* regionToRemove)
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

    bool DoorsWindowManager::moveRegionToMonitor(HMONITOR from, HMONITOR to, Region* regionToMove) {
        
        // if you have gone clinicically insane 
        if (mActiveMonitors.size() < 2) {
            std::cout << "Error: less than two monitors!" << mActiveMonitors.size() << std::endl;
            return false;
        }
        
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

            // bad preformance but its C so
            toMonitor.insert(toMonitor.begin(), regionToMove);

            std::cout << "Successfully moved region " << regionToMove->id << " to monitor " << to << std::endl;
            return true; // Return true to indicate success
        }
        else {
            std::cout << "Failed to find region " << regionToMove->id << " on monitor " << from << std::endl;
            return false; // Return false if the region wasn't found
        }
    }
    
    
    bool DoorsWindowManager::shiftRegionToDirection(Region* region, Direction dir) {
        // Get the current monitor's info for the region's monitor
        
        printf("moving %s to the %i direction \n ", region->DWI->title.c_str(), dir);
        
        DoorMonitorInfo* currentMonitorInfo = getMonitorInfoFromMonitorHandle(region->DWI->monitorHandle);
        if (!currentMonitorInfo) {
            return false;  // Failed to get monitor info
        }

        // Get the regions on this monitor
        auto& regions = mRegions[currentMonitorInfo->mMonitorHandle];

        // Find the index of the region to shift
        auto it = std::find(regions.begin(), regions.end(), region);
        if (it == regions.end()) {
            return false;  // Region not found in the monitor's region list
        }

        // Get the current index of the region
        int currentIndex = std::distance(regions.begin(), it);

        bool shifted = false; // Flag to track whether the region has been shifted

        // Handle the shift direction
        switch (dir) {
        case left:
            if (currentIndex > 0) {
                // Swap the region with the one on the left
                swapRegions(regions[currentIndex], regions[currentIndex - 1]);
                shifted = true;
            }
            else {
                // No region to the left on the current monitor, check next monitor
                DoorMonitorInfo* leftMonitor = nullptr;
                for (size_t i = 0; i < mActiveMonitors.size(); ++i) {
                    if (mActiveMonitors[i] == currentMonitorInfo && i > 0) {
                        leftMonitor = mActiveMonitors[i - 1]; // Get monitor to the left
                        break;
                    }
                }

                // If there is a monitor to the left, move the region there
                if (leftMonitor) {
                    shifted = moveRegionToMonitor(currentMonitorInfo->mMonitorHandle, leftMonitor->mMonitorHandle, region);
                }
            }
            break;

        case right:
            if (currentIndex < regions.size() - 1) {
                // Swap the region with the one on the right
                swapRegions(regions[currentIndex], regions[currentIndex + 1]);
                shifted = true;
            }
            else {
                // No region to the right on the current monitor, check next monitor
                DoorMonitorInfo* rightMonitor = nullptr;
                for (size_t i = 0; i < mActiveMonitors.size(); ++i) {
                    if (mActiveMonitors[i] == currentMonitorInfo && i < mActiveMonitors.size() - 1) {
                        rightMonitor = mActiveMonitors[i + 1]; // Get monitor to the right
                        break;
                    }
                }

                // If there is a monitor to the right, move the region there
                if (rightMonitor) {
                    shifted = moveRegionToMonitor(currentMonitorInfo->mMonitorHandle, rightMonitor->mMonitorHandle, region);
                }
            }
            break;

        case up:
            /* if (currentIndex > 0) {
                swapRegions(regions[currentIndex], regions[currentIndex - 1]);
                shifted = true;
            }*/
            break;

        case down:
            /*if (currentIndex < regions.size() - 1) {
                swapRegions(regions[currentIndex], regions[currentIndex + 1]);
                shifted = true;
            }*/
            break;
        }

        // Return true if the region has been successfully shifted
        return shifted;
    }



    // Method to print out all windows stored
    void DoorsWindowManager::printInfo() const {        


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

    bool DoorsWindowManager::moveWindow(DoorWindowInfo* dwi, int x, int y, int width, int height)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos
        return SetWindowPos(dwi->hwnd, HWND_TOP,x, y, width, height, SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
    }

    bool DoorsWindowManager::moveMouse(int x, int y) {
        if (this->mIsAdmin) {
            SetCursorPos(x, y);
            return true;
        }
        else {
            return false;
        }
    }



}
