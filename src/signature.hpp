#ifndef SIGNATURE_HPP
#define SIGNATURE_HPP

#include <string>

namespace signature
{
    uintptr_t getAddressBySignatureInFile(const std::string& filePath, const std::string& signature);
}

#endif // SIGNATURE_HPP