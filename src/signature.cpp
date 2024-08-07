#include <fstream>
#include <iostream>
#include <vector>
#include "signature.hpp"

namespace signature
{
    using BYTE = unsigned char;

    // Function for checking the signature match
    bool compareByteArray(const BYTE* pData, const BYTE* bMask, const char* szMask) {
        for (; *szMask; ++szMask, ++pData, ++bMask) {
            if (*szMask == 'x' && *pData != *bMask) {
                return false;
            }
        }
        return (*szMask) == 0;
    }

    // A function for searching for a signature in memory
    uintptr_t findSignature(const BYTE* data, size_t size, const BYTE* bMask, const char* szMask) {
        for (size_t i = 0; i < size; i++) {
            if (compareByteArray(data + i, bMask, szMask)) {
                return reinterpret_cast<uintptr_t>(data + i);
            }
        }
        return 0;
    }

    // A function for converting a signature from a string to a byte array
    void parseSignature(const std::string& signature, std::vector<BYTE>& bytes, std::string& mask) {
        size_t len = signature.length();
        for (size_t i = 0; i < len; i++) {
            if (signature[i] == ' ') continue;
            if (signature[i] == '?') {
                bytes.push_back(0);
                mask += '?';
                while (i + 1 < len && signature[i + 1] == '?') {
                    i++;
                }
            }
            else {
                char byte[3] = { signature[i], signature[i + 1], 0 };
                bytes.push_back((BYTE)strtol(byte, nullptr, 16));
                mask += 'x';
                i++;
            }
        }
    }

    uintptr_t getAddressBySignatureInFile(const std::string& filePath, const std::string& signature) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file." << std::endl;
            return 0;
        }

        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<BYTE> fileData(fileSize);
        file.read(reinterpret_cast<char*>(fileData.data()), fileSize);

        if (!file) {
            std::cerr << "Failed to read file." << std::endl;
            return 0;
        }

        std::vector<BYTE> sigBytes;
        std::string mask;
        parseSignature(signature, sigBytes, mask);

        uintptr_t result = findSignature(fileData.data(), fileData.size(), sigBytes.data(), mask.c_str());
        if (result != 0) {
            return result - reinterpret_cast<uintptr_t>(fileData.data()); // Offset relative to the beginning of the file
        }
        return 0;
    }
}
