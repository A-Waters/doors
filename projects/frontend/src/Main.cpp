#include "lib/keyboardListener.h"
#include <iostream>
#include <string>

int main(int, char**) {
    lib::DoorsWindowManager doors;
    lib::KeyboardListener listener = lib::KeyboardListener(&doors);
    listener.StartListening();
    std::cout << "done" << std::endl;    
    std::flush(std::cout);
    std::cout << "fluhed" << std::endl;    
    return 0;
}
