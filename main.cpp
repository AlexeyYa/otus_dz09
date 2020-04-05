/*! @file main.cpp
 *  @brief Исполняемый файл
 */
 
#include "bayan.h"

#include <exception>

int main(int argc, char *argv[])
{
    try {
        auto bayan = ParseArgs(argc, argv);
        bayan.Run();
    } catch (std::exception e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
