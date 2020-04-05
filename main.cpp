/*! @file main.cpp
 *  @brief Исполняемый файл
 */
 
#include "bayan.h"

int main(int argc, char *argv[])
{
    auto bayan = ParseArgs(argc, argv);

    bayan.Run();

    return 0;
}
