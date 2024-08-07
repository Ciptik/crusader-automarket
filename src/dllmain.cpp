#include <algorithm>
#include <cstring>
#include <format>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <locale>
#include <codecvt>
#include <map>
#include <chrono>

#include <Windows.h>
#include <ddraw.h>

#include "MinHook.h"

#include "log.hpp"
#include "internal.hpp"
#include "menu.hpp"

typedef HWND (WINAPI *CreateWindowExWFn)(
    DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
    DWORD dwStyle, int x, int y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam
);

typedef HRESULT (WINAPI *DirectDrawCreateFn)(GUID*, IDirectDraw**, IUnknown*);
typedef HRESULT (WINAPI *CreateSurfaceFn)(IDirectDraw*, LPDDSURFACEDESC, LPDIRECTDRAWSURFACE*, IUnknown*);
typedef HRESULT (WINAPI *ReleaseDCFn)(IDirectDrawSurface*, HDC);

CreateWindowExWFn pCreateWindowExW = nullptr;
CreateWindowExWFn oCreateWindowExW = nullptr;
HWND WINAPI MyCreateWindowExW(
    DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
    DWORD dwStyle, int x, int y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam
);

WNDPROC oWndProc = nullptr;
LRESULT __stdcall MyWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

DirectDrawCreateFn pDirectDrawCreate = nullptr;
DirectDrawCreateFn oDirectDrawCreate = nullptr;
HRESULT WINAPI MyDirectDrawCreate(GUID* lpGUID, IDirectDraw** lplpDD, IUnknown* pUnkOuter);

CreateSurfaceFn pCreateSurface = nullptr;
CreateSurfaceFn oCreateSurface = nullptr;
HRESULT STDMETHODCALLTYPE MyCreateSurface(IDirectDraw* pDirectDraw, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter);

ReleaseDCFn pReleaseDC = nullptr;
ReleaseDCFn oReleaseDC = nullptr;
HRESULT STDMETHODCALLTYPE MyReleaseDC(IDirectDrawSurface* pSurface, HDC hdc);

UDPLogger logger("192.168.1.169", 7878);
GameInterface driver;

static HWND g_hwnd;

std::map<std::string, std::map<std::string, int>> productSettings;
std::vector<std::string> availableProducts;
int indexCurrentProduct;

std::chrono::time_point<std::chrono::high_resolution_clock> lastTradeTime;
std::chrono::milliseconds tradingFrequency = std::chrono::milliseconds(100);

std::wstring ConvertToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

bool ContainsWordIgnoreCase(const std::string& str, const std::string& word) {
    std::string lowerStr = str;
    std::string lowerWord = word;
    
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);
    
    return lowerStr.find(lowerWord) != std::string::npos;
}

void GetWindowSize(int& width, int& height) {
    RECT rect;
    if (GetClientRect(g_hwnd, &rect)) {
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
    } else {
        width = 0;
        height = 0;
    }
}

HRESULT WINAPI MyDirectDrawCreate(GUID* lpGUID, IDirectDraw** lplpDD, IUnknown* pUnkOuter) {
    HRESULT hr = oDirectDrawCreate(lpGUID, lplpDD, pUnkOuter);

    if (!SUCCEEDED(hr) || !lplpDD || !*lplpDD) {
        logger.sendMessage("Failed to create IDirectDraw object!");
        return hr;
    }

    void** vTable = *reinterpret_cast<void***>(*lplpDD);

    // Creating a hook on CreateSurface
    pCreateSurface = (CreateSurfaceFn)vTable[6];

    if (MH_CreateHook(pCreateSurface, &MyCreateSurface, reinterpret_cast<void**>(&oCreateSurface)) != MH_OK) {
        logger.sendMessage("Failed to create hook for CreateSurface!");
        return hr;
    }

    if (MH_EnableHook(pCreateSurface) != MH_OK) {
        logger.sendMessage("Failed to enable hook for CreateSurface!");
        MH_RemoveHook(pCreateSurface);
        return hr;
    }

    logger.sendMessage("CreateSurface hook enabled!");

    MH_DisableHook(pDirectDrawCreate);
    MH_RemoveHook(pDirectDrawCreate);

    return hr;
}

HRESULT STDMETHODCALLTYPE MyCreateSurface(IDirectDraw* pDirectDraw, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter) {
    HRESULT hr = oCreateSurface(pDirectDraw, lpDDSurfaceDesc, lplpDDSurface, pUnkOuter);

    if (!SUCCEEDED(hr) || !lplpDDSurface || !*lplpDDSurface) {
        logger.sendMessage("Failed to create IDirectDrawSurface object!");
        return hr;
    }

    void** vTable = *reinterpret_cast<void***>(*lplpDDSurface);

    // Creating a hook on ReleaseDC
    pReleaseDC = (ReleaseDCFn)vTable[26];

    if (MH_CreateHook(pReleaseDC, &MyReleaseDC, reinterpret_cast<void**>(&oReleaseDC)) != MH_OK) {
        logger.sendMessage("Failed to create hook for ReleaseDC!");
        return hr;
    }

    if (MH_EnableHook(pReleaseDC) != MH_OK) {
        logger.sendMessage("Failed to enable hook for ReleaseDC!");
        MH_RemoveHook(pReleaseDC);
        return hr;
    }

    logger.sendMessage("ReleaseDC hook enabled!");

    MH_DisableHook(pCreateSurface);
    MH_RemoveHook(pCreateSurface);

    return hr;
}

void ChooseNextProduct() {
    if (availableProducts.empty()) {
        logger.sendMessage("No products available.");
        return;
    }
    indexCurrentProduct = (indexCurrentProduct + 1) % availableProducts.size();
}

void ChoosePrevProduct() {
    if (availableProducts.empty()) {
        logger.sendMessage("No products available.");
        return;
    }
    indexCurrentProduct = (indexCurrentProduct - 1 + availableProducts.size()) % availableProducts.size();
}

void ResetAllSettings() {
    indexCurrentProduct = 0;

    for (const auto& product : availableProducts) {
        productSettings[product]["sale"] = 500;
        productSettings[product]["buy"] = 0;
    }
}

std::chrono::milliseconds GetTimeSinceLastTtrade() {
    auto start = lastTradeTime;
    auto current = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(current - start);
}

void executeTrade() {
    if (GetTimeSinceLastTtrade() >= tradingFrequency) {
        for (const auto& [productName, info] : productSettings) {
            int sale = info.at("sale");
            int buy = info.at("buy");

            int numberProduct = driver.getNumberProducts(productName);

            if (numberProduct > sale) {
                driver.sellProduct(productName);
            }

            if (numberProduct < buy) {
                driver.buyProduct(productName);
            }
        }

        lastTradeTime = std::chrono::high_resolution_clock::now();
    }
}

HRESULT STDMETHODCALLTYPE MyReleaseDC(IDirectDrawSurface* pSurface, HDC hdc) {
    int width, height;
    GetWindowSize(width, height);
    
    DDSURFACEDESC ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    HRESULT hr = pSurface->GetSurfaceDesc(&ddsd);

    if (FAILED(hr) || ddsd.dwWidth != width || ddsd.dwHeight != height)
        return oReleaseDC(pSurface, hdc);

    if (Menu::IsOpenMenu()) {
        std::string currentProductName = availableProducts[indexCurrentProduct];

        Menu::SetContext(hdc);
        Menu::DrawBackground(RECT{0, 0, 300, 139});
        Menu::DrawTextField(RECT{5, 5, 95, 34}, L"product");
        Menu::DrawTextField(RECT{99, 5, 294, 34}, ConvertToWString(currentProductName).c_str(), TEXT_ALIGN_LEFT);

        Menu::DrawTextField(RECT{5, 38, 95, 67}, L"sale >");
        Menu::DrawNumericField(RECT{99, 38, 294, 67}, RECT{210, 40, 249, 65}, RECT{253, 40, 292, 65}, productSettings[currentProductName]["sale"], 0, 500);

        Menu::DrawTextField(RECT{5, 71, 95, 100}, L"buy <");
        Menu::DrawNumericField(RECT{99, 71, 294, 100}, RECT{210, 73, 249, 98}, RECT{253, 73, 292, 98}, productSettings[currentProductName]["buy"], 0, 500);

        Menu::DrawButton(RECT{5, 104, 186, 133}, L"reset all settings", ResetAllSettings);
        Menu::DrawButton(RECT{190, 104, 239, 133}, L"←", ChoosePrevProduct);
        Menu::DrawButton(RECT{243, 104, 292, 133}, L"→", ChooseNextProduct);
    }

    executeTrade();

    // Call the original ReleaseDC
    return oReleaseDC(pSurface, hdc);
}

bool MainInit(HWND hwnd) {
    Menu::Init(g_hwnd);
    availableProducts = driver.getAvailableProducts();
    ResetAllSettings();
    lastTradeTime = std::chrono::high_resolution_clock::now();

    logger.sendMessage("Setting new window procedure...");
    oWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)MyWndProc);
    if (!oWndProc) {
        logger.sendMessage("Failed to set new window procedure.");
        return false;
    }

    logger.sendMessage("Initialization completed successfully.");
    return true;
}

HWND WINAPI MyCreateWindowExW(
    DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
    DWORD dwStyle, int x, int y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam
) {
    HWND hwnd = oCreateWindowExW(dwExStyle, lpClassName, lpWindowName,
                                 dwStyle, x, y, nWidth, nHeight,
                                 hWndParent, hMenu, hInstance, lpParam);

    char windowTitle[256];
    GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

    if (ContainsWordIgnoreCase(windowTitle, "Crusader")) {
        g_hwnd = hwnd;

        MainInit(hwnd);
    }

    return hwnd;
}

bool SetupHooks() {
    if (MH_Initialize() != MH_OK)
        return false;

    // Creating a hook on DirectDrawCreate
    HMODULE hDdraw = LoadLibraryA("ddraw.dll");

    if (!hDdraw)
        return false;
    
    pDirectDrawCreate = (DirectDrawCreateFn)GetProcAddress(hDdraw, "DirectDrawCreate");

    if (!pDirectDrawCreate)
        return false;
    
    if (MH_CreateHook(pDirectDrawCreate, &MyDirectDrawCreate, reinterpret_cast<void**>(&oDirectDrawCreate)) != MH_OK)
        return false;
    
    if (MH_EnableHook(pDirectDrawCreate) != MH_OK) {
        MH_RemoveHook(pDirectDrawCreate);
        return false;
    }
    
    // Creating a window creation hook
    HMODULE hUser32 = GetModuleHandleA("user32.dll");

    if (!hUser32)
        return false;

    pCreateWindowExW = (CreateWindowExWFn)GetProcAddress(hUser32, "CreateWindowExW");

    if (!pCreateWindowExW)
        return false;

    if (MH_CreateHook(pCreateWindowExW, &MyCreateWindowExW, reinterpret_cast<void**>(&oCreateWindowExW)) != MH_OK)
        return false;

    if (MH_EnableHook(pCreateWindowExW) != MH_OK) {
        MH_RemoveHook(pCreateWindowExW);
        return false;
    }

    return true;
}

void CleanupHooks() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

LRESULT __stdcall MyWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return Menu::ImplWin32_WndProcHandler(oWndProc, hWnd, uMsg, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        SetupHooks();
        break;
    case DLL_PROCESS_DETACH:
        CleanupHooks();
        break;
    }
    return TRUE;
}
