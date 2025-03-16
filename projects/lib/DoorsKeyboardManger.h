#pragma once
#include <windows.h>
#include <iostream>


namespace lib {
	HHOOK keyboardHook;
	bool isWindowsKeyPressed = false;
	bool isKKeyPressed = false;
	LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

	class DoorsKeyboardManger
	{
		bool startKeyboardHook();
		bool stopKeyboardHook();
		void startKeyboardListener();

	};

}