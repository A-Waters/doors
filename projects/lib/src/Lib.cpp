#include "Lib.h"


// Structure to hold window information

namespace lib {
    // Callback function to enumerate windows
    static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam) {
        int length = GetWindowTextLength(hWnd);
        wchar_t* buffer = new wchar_t[length + 1];
        GetWindowText(hWnd, buffer, length + 1);

        const LPDWORD& winID = nullptr;
        GetWindowThreadProcessId(hWnd, winID);

        std::wstring your_wchar_in_ws(buffer);
        std::string your_wchar_in_str(your_wchar_in_ws.begin(), your_wchar_in_ws.end());
        const char* your_wchar_in_char = your_wchar_in_str.c_str();

        std::string windowTitle(your_wchar_in_char);
        delete[] buffer;

        // Add visible windows with non-empty title to the window manager
        if (IsWindowVisible(hWnd) && length != 0) {
            // Convert LPARAM into windowManger* (pointer to class instance)
            windowManager* wm = reinterpret_cast<windowManager*>(lparam);
            wm->addWindowToWM(hWnd, windowTitle, winID);
        }

        return TRUE;
    }

    // Constructor for windowManger
    lib::windowManager::windowManager() {
        std::cout << "Enumerating windows..." << std::endl;
        EnumWindows(enumWindowCallback, reinterpret_cast<LPARAM>(this));
        std::cout << "Done" << std::endl;
        this->printWindows();  // Print the windows after enumeration
        moveWindow(this->mActiveWindows[0].hwnd, 100, 100, 100, 100);
 
    }

    // Method to add a window to the member list
    bool windowManager::addWindowToWM(HWND hWnd, const std::string& title, const LPDWORD& lpdwProcessId) {
        this->mActiveWindows.push_back({ hWnd, title, lpdwProcessId });
        return true;
    }

    // Method to print out all windows stored
    void windowManager::printWindows() const {
        std::cout << "Windows enumerated:" << std::endl;
        for (const WindowInfo& window : this->mActiveWindows) {
            std::cout << window.hwnd << ": " << window.title << " --- pid:"<< window.lpdwProcessId << std::endl;
        }
    }

    bool windowManager::moveWindow(HWND hwnd, int x, int y, int width, int height)
    {
        return MoveWindow(hwnd, x, y, width, height, TRUE);
    }



}
