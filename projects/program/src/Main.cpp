#include "lib/Lib.h"
#include <iostream>
#include <string>

int main(int, char**) {
    lib::doorsWindowManager DOORS;
    std::cout << "all done";
    std::flush(std::cout);
    std::string userInput;
    getline(std::cin, userInput);
    Sleep(1);
    return 0;
}
