#include "DoorsKeyboardManger.h"

bool lib::DoorsKeyboardManger::startKeyboardHook()
{
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (keyboardHook == NULL) {
        std::cerr << "Failed to install keyboard hook!" << std::endl;
    }
}

bool lib::DoorsKeyboardManger::stopKeyboardHook()
{
    if (keyboardHook != NULL) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = NULL;
    }
}

void lib::DoorsKeyboardManger::startKeyboardListener()
{
    startKeyboardHook();

    // Message loop to keep the application running and process messages
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    stopKeyboardHook();
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;

        // Only process key down and key up events
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            int keyCode = pKeyBoard->vkCode;

            // Check if the pressed key is Windows key (left or right) or K key
            if (keyCode == VK_LWIN || keyCode == VK_RWIN) {
                lib::isWindowsKeyPressed = true; // Windows key pressed
            }
            if (keyCode == 'K') {
                lib::isKKeyPressed = true; // K key pressed
            }

            // Check if both Windows Key and K are pressed simultaneously
            if (lib::isWindowsKeyPressed && lib::isKKeyPressed) {
                std::cout << "Windows Key + K pressed!" << std::endl;
                // Perform your action here for Windows Key + K press
            }
        }
        // Handle key release events (when a key is released)
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            int keyCode = pKeyBoard->vkCode;
            if (keyCode == VK_LWIN || keyCode == VK_RWIN) {
                lib::isWindowsKeyPressed = false; // Windows key released
            }
            if (keyCode == 'K') {
                lib::isKKeyPressed = false; // K key released
            }
        }
    }

    return CallNextHookEx(lib::keyboardHook, nCode, wParam, lParam);
}

