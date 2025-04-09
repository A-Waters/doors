#ifndef KEYBOARD_LISTENER_H
#define KEYBOARD_LISTENER_H 

#include "windowsManager.h"





namespace lib {
	
	static LRESULT KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
	

	class KeyboardListener {
		public:
			
			KeyboardListener(DoorsWindowManager* dwm);
			// KeyboardListener(DoorsWindowManager dwm);
			~KeyboardListener();
		
			// Initialize the keyboard listener (if needed)
			void StartListening();
			DoorsWindowManager* mDwm;
		
		private:

			HHOOK keyboardHook;  // To hold the hook handle
		};

}

#endif