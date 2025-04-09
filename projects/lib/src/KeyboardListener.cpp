#include "../include/lib/keyboardListener.h"

#include "../include/lib/commonDoors.h"
#include <iostream>
#include <windows.h>

namespace lib
{
	u_int keysPressed = 0;
	int Q_KEY = 0x51;
	int J_KEY = 0x4A;
	int K_KEY = 0x4B;

	bool isitSet(int number, int index) {
		int mask = 1 << index;
		// Use bitwise AND to check if both bits are set
		return ((number & mask) != 0);
	}
	
	// Static pointer to DoorsWindowManager
	static KeyboardListener* globalKeyboardListener = nullptr;

	static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
	{

		
		if (nCode == HC_ACTION)
		{
			KBDLLHOOKSTRUCT *pKey = (KBDLLHOOKSTRUCT *)lParam;

			if (wParam == WM_KEYDOWN)
			{
				keysPressed = keysPressed | (1 << pKey->vkCode);

				std::cout << "Key pressed: " << keysPressed << std::endl;

				// Check if the key pressed is 'q' (virtual key code for 'q' is 0x51)
				if (pKey->vkCode == 0x51)
				{
					std::cout << "'q' key pressed!" << std::endl;
					PostQuitMessage(0);
				}

				auto windowManager = globalKeyboardListener->mDwm;
				
				if (isitSet(keysPressed, K_KEY) && isitSet(keysPressed, VK_LWIN) && isitSet(keysPressed, VK_LSHIFT)) {
					std::cout << "Sending Move Region to right!" << std::endl;
					windowManager->shiftRegionToDirection(windowManager->getFocus(), lib::Direction::right);
				} else if (isitSet(keysPressed, K_KEY) && isitSet(keysPressed, VK_LWIN)) {
					std::cout << "Sending Move Focus to Right!" << std::endl;
					windowManager->shiftFocusToDirection(lib::Direction::right);
				}
				else if (isitSet(keysPressed, J_KEY) && isitSet(keysPressed, VK_LWIN) && isitSet(keysPressed, VK_LSHIFT)) {
					std::cout << "Sending Move Region to left!" << std::endl;
					windowManager->shiftRegionToDirection(windowManager->getFocus(), lib::Direction::left);
				}
				else if (isitSet(keysPressed, J_KEY) && isitSet(keysPressed, VK_LWIN)) {
					std::cout << "Sending Move Focus to left!" << std::endl;
					windowManager->shiftFocusToDirection(lib::Direction::left);
				} 

			
			
			}
			else if (wParam == WM_KEYUP)
			{
				keysPressed = keysPressed & ~(1 << pKey->vkCode);
				std::cout << "Key released: " << pKey->vkCode << std::endl;
			}

		}
		std::cout << "---------------------------------------------------------" << std::endl;
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	KeyboardListener::KeyboardListener(DoorsWindowManager* dwm) : keyboardHook(nullptr)
	{
		this->mDwm = dwm;
		globalKeyboardListener = this;
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
