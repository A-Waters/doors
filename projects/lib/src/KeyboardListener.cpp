#include "../include/lib/keyboardListener.h"

#include "../include/lib/commonDoors.h"
#include <iostream>
#include <windows.h>

namespace lib
{

	static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode == HC_ACTION)
		{
			KBDLLHOOKSTRUCT *pKey = (KBDLLHOOKSTRUCT *)lParam;

			if (wParam == WM_KEYDOWN)
			{
				std::cout << "Key pressed: " << pKey->vkCode << std::endl;

				// Check if the key pressed is 'q' (virtual key code for 'q' is 0x51)
				if (pKey->vkCode == 0x51)
				{
					std::cout << "'q' key pressed!" << std::endl;
					PostQuitMessage(0);
				}
			}
			else if (wParam == WM_KEYUP)
			{
				std::cout << "Key released: " << pKey->vkCode << std::endl;
			}
		}
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	KeyboardListener::KeyboardListener(DoorsWindowManager* dwm) : keyboardHook(nullptr)
	{
		this->mDwm = dwm;
	}

	KeyboardListener::~KeyboardListener()
	{
		if (keyboardHook)
		{
			if (!UnhookWindowsHookEx(keyboardHook))
			{
				// If UnhookWindowsHookEx fails, log the error using GetLastError
				DWORD errorCode = GetLastError();
				std::cerr << "Failed to remove hook! Error code: " << errorCode << std::endl;
			}
			else
			{
				keyboardHook = NULL;
				std::cout << "Hook successfully removed." << std::endl;
			}
		}
	}

	void KeyboardListener::StartListening()
	{
		// Set up the keyboard hook to listen for key events globally
		keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
		if (keyboardHook == NULL)
		{
			std::cerr << "Failed to install hook!" << std::endl;
			return;
		}

		std::cout << "Listening for keyboard events..." << std::endl;
		// Run a message loop to keep the program alive and handle events
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) != 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

	}
}
