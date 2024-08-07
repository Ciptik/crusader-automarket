#ifndef PSUTILS_HPP
#define PSUTILS_HPP

#include <windows.h>
#include <string>

namespace psutils
{
    uintptr_t getBaseAddress();
    std::string getProcessName();
    bool isValidAddress(uintptr_t address);
}

#endif // PSUTILS_HPP