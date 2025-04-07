#ifndef KEYBOARD_LISTENER_H
#define KEYBOARD_LISTENER_H 

#include "windowsManager.h"





namespace lib {

	class KeyboardListener {
		public:
			KeyboardListener(DoorsWindowManager* dwm);	
			// KeyboardListener(DoorsWindowManager dwm);
			~KeyboardListener();
		
			// Initialize the keyboard listener (if needed)
			void StartListening();

		
		private:
			DoorsWindowManager* mDwm;
			HHOOK keyboardHook;  // To hold the hook handle
		};

}

#endif