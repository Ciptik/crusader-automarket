#include <iostream>
#include "psutils.hpp"

namespace psutils
{
    // Utility function to get the base address of the current process
    uintptr_t getBaseAddress() {
        HMODULE hModule = GetModuleHandle(NULL);
        if (hModule == NULL) {
            MessageBoxW(NULL, L"Failed to get handle for main module.", L"Error", MB_OK | MB_ICONERROR);
            return 0;
        }
        return reinterpret_cast<uintptr_t>(hModule);
    }

    // Utility function to get the process name
    std::string getProcessName() {
        wchar_t processName[MAX_PATH] = L"<unknown>";
        DWORD processID = GetCurrentProcessId();
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
        if (hProcess) {
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProcess, 0, processName, &size)) {
                wchar_t* fileName = wcsrchr(processName, L'\\');
                if (fileName != NULL) {
                    std::wstring wstr(fileName + 1);
                    std::string str(wstr.begin(), wstr.end());
                    CloseHandle(hProcess);
                    return str;
                }
            }
            CloseHandle(hProcess);
        }
        return "<unknown>";
    }

    // Utility function to validate if an address is valid in the process memory
    bool isValidAddress(uintptr_t address) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi))) {
            DWORD mask = (PAGE_GUARD | PAGE_NOACCESS);
            bool isValid = !(mbi.Protect & mask) && (mbi.State == MEM_COMMIT);
            return isValid;
        }
        return false;
    }
}
