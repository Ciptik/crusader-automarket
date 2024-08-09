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

typedef HWND (WINAPI *CreateWindowExAFn)(
    DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
    DWORD dwStyle, int x, int y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam
);
typedef HWND (WINAPI *CreateWindowExWFn)(
    DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
    DWORD dwStyle, int x, int y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam
);

typedef HRESULT (WINAPI *DirectDrawCreateFn)(GUID*, IDirectDraw**, IUnknown*);
typedef HRESULT (WINAPI *CreateSurfaceFn)(IDirectDraw*, LPDDSURFACEDESC, LPDIRECTDRAWSURFACE*, IUnknown*);
typedef HRESULT (WINAPI *BltFn)(IDirectDrawSurface*, LPCRECT, IDirectDrawSurface*, LPCRECT, DWORD, LPDDBLTFX);

CreateWindowExAFn pCreateWindowExA = nullptr;
CreateWindowExAFn oCreateWindowExA = nullptr;
HWND WINAPI MyCreateWindowExA(
    DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
    DWORD dwStyle, int x, int y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam
);
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

BltFn pBlt = nullptr;
BltFn oBlt = nullptr;
HRESULT WINAPI MyBlt(IDirectDrawSurface* pDestSurface, LPCRECT lpDestRect, IDirectDrawSurface* pSrcSurface, LPCRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);

// UDPLogger logger("192.168.1.169", 7878);
GameInterface driver;

HWND g_hwnd;

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
        // logger.sendMessage("Failed to create IDirectDraw object!");
        return hr;
    }

    void** vTable = *reinterpret_cast<void***>(*lplpDD);

    // Creating a hook on CreateSurface
    pCreateSurface = (CreateSurfaceFn)vTable[6];

    if (MH_CreateHook(pCreateSurface, &MyCreateSurface, reinterpret_cast<void**>(&oCreateSurface)) != MH_OK) {
        // logger.sendMessage("Failed to create hook for CreateSurface!");
        return hr;
    }

    if (MH_EnableHook(pCreateSurface) != MH_OK) {
        // logger.sendMessage("Failed to enable hook for CreateSurface!");
        MH_RemoveHook(pCreateSurface);
        return hr;
    }

    // logger.sendMessage("CreateSurface hook enabled!");

    MH_DisableHook(pDirectDrawCreate);
    MH_RemoveHook(pDirectDrawCreate);

    return hr;
}

HRESULT STDMETHODCALLTYPE MyCreateSurface(IDirectDraw* pDirectDraw, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter) {
    HRESULT hr = oCreateSurface(pDirectDraw, lpDDSurfaceDesc, lplpDDSurface, pUnkOuter);

    if (!SUCCEEDED(hr) || !lplpDDSurface || !*lplpDDSurface) {
        // logger.sendMessage("Failed to create IDirectDrawSurface object!");
        return hr;
    }

    void** vTable = *reinterpret_cast<void***>(*lplpDDSurface);

    // Creating a hook on Blt
    pBlt = (BltFn)vTable[5];

    if (MH_CreateHook(pBlt, &MyBlt, reinterpret_cast<void**>(&oBlt)) != MH_OK) {
        // logger.sendMessage("Failed to create hook for Blt!");
        return hr;
    }

    if (MH_EnableHook(pBlt) != MH_OK) {
        // logger.sendMessage("Failed to enable hook for Blt!");
        MH_RemoveHook(pBlt);
        return hr;
    }

    // logger.sendMessage("Blt hook enabled!");

    MH_DisableHook(pCreateSurface);
    MH_RemoveHook(pCreateSurface);

    return hr;
}

void ChooseNextProduct() {
    if (availableProducts.empty()) {
        // logger.sendMessage("No products available.");
        return;
    }
    indexCurrentProduct = (indexCurrentProduct + 1) % availableProducts.size();
}

void ChoosePrevProduct() {
    if (availableProducts.empty()) {
        // logger.sendMessage("No products available.");
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

void DrawRedSquare(IDirectDrawSurface* pSurface, int x, int y, int width, int height) {
    // Блокируем поверхность, чтобы получить доступ к пиксельным данным
    DDSURFACEDESC ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    HRESULT hr = pSurface->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
    if (FAILED(hr)) {
        // Ошибка при блокировке поверхности
        return;
    }

    // Получаем указатель на пиксельный буфер
    uint16_t* pixelBuffer = static_cast<uint16_t*>(ddsd.lpSurface);
    int pitch = ddsd.lPitch / sizeof(uint16_t);  // Количество пикселей в одной строке (Pitch)

    // Определяем цвет красного в формате RGB565
    uint16_t redColor = 0xF800; // RGB565 для красного цвета

    // Рисуем квадрат
    for (int py = y; py < y + height; ++py) {
        if (py < 0 || py >= ddsd.dwHeight) continue;  // Проверяем границы по высоте
        uint16_t* row = pixelBuffer + py * pitch;
        for (int px = x; px < x + width; ++px) {
            if (px < 0 || px >= ddsd.dwWidth) continue;  // Проверяем границы по ширине
            row[px] = redColor;
        }
    }

    // Разблокируем поверхность после работы с буфером
    pSurface->Unlock(NULL);
}

void CopyRenderResult(uint16_t* destBuffer, uint32_t* srcBuffer) {
    RECT rectClip = Menu::GetClipRect();

    // Извлекаем координаты области для копирования
    int xStart = rectClip.left;
    int xEnd = rectClip.right;
    int yStart = rectClip.top;
    int yEnd = rectClip.bottom;

    int width, height;
    GetWindowSize(width, height);

    // Определяем ширину и высоту буферов
    int bufferWidth = width;  // Предполагается, что эта функция возвращает ширину буфера

    for (int y = yStart; y < yEnd; ++y) {
        for (int x = xStart; x < xEnd; ++x) {
            // Индекс текущего пикселя
            int index = y * bufferWidth + x;

            // Извлекаем цвет из srcBuffer (ARGB формат)
            uint32_t argb = srcBuffer[index];
            uint8_t r = (argb >> 16) & 0xFF;
            uint8_t g = (argb >> 8) & 0xFF;
            uint8_t b = argb & 0xFF;

            // Преобразуем цвет в формат RGB565
            uint16_t rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

            // Записываем преобразованный цвет в destBuffer
            destBuffer[index] = rgb565;
        }
    }
}

HRESULT WINAPI MyBlt(IDirectDrawSurface* pDestSurface, LPCRECT lpDestRect, IDirectDrawSurface* pSrcSurface, LPCRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) {
    executeTrade();

    HRESULT hr = oBlt(pDestSurface, lpDestRect, pSrcSurface, lpSrcRect, dwFlags, lpDDBltFx);

    if (FAILED(hr)) {
        return hr;
    }

    if (!Menu::IsOpenMenu())
        return hr;

    DDSURFACEDESC ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    if (FAILED(pDestSurface->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL))) {
        return E_FAIL; // Не удалось заблокировать поверхность
    }

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = ddsd.dwWidth;
    bmi.bmiHeader.biHeight = -static_cast<int>(ddsd.dwHeight); // Отрицательная высота для верхнего левого начала
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32; // 32 бита на пиксель (8 бит на канал RGB + 8 бит на альфа-канал или просто 0)
    bmi.bmiHeader.biCompression = BI_RGB; // BI_RGB означает отсутствие сжатия

    // Создаем HBITMAP и привязываем его к пиксельному буферу
    void* pPixels = NULL; // Передаем NULL для выделения памяти
    HBITMAP hBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pPixels, NULL, 0);
    if (!hBitmap) {
        pDestSurface->Unlock(NULL);
        return E_FAIL; // Не удалось создать HBITMAP
    }

    // Создаем совместимый HDC и выбираем HBITMAP
    HDC hdc = CreateCompatibleDC(NULL);
    if (!hdc) {
        DeleteObject(hBitmap);
        pDestSurface->Unlock(NULL);
        return E_FAIL; // Не удалось создать совместимый HDC
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdc, hBitmap);

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

    CopyRenderResult((uint16_t*)ddsd.lpSurface, (uint32_t*)pPixels);

    SelectObject(hdc, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdc);

    pDestSurface->Unlock(NULL);

    return hr;
}

bool MainInit(HWND hwnd) {
    Menu::Init(g_hwnd);
    availableProducts = driver.getAvailableProducts();
    ResetAllSettings();
    lastTradeTime = std::chrono::high_resolution_clock::now();

    // logger.sendMessage("Setting new window procedure...");
    oWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)MyWndProc);
    if (!oWndProc) {
        // logger.sendMessage("Failed to set new window procedure.");
        return false;
    }

    // logger.sendMessage("Initialization completed successfully.");
    return true;
}

HWND WINAPI MyCreateWindowExA(
    DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
    DWORD dwStyle, int x, int y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam
) {
    HWND hwnd = oCreateWindowExA(dwExStyle, lpClassName, lpWindowName,
                                 dwStyle, x, y, nWidth, nHeight,
                                 hWndParent, hMenu, hInstance, lpParam);

    if (hwnd != NULL && !g_hwnd) {
        char windowTitle[256];
        GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

        std::string titleString(windowTitle);

        // logger.sendMessage("Create window: " + titleString);

        if (ContainsWordIgnoreCase(windowTitle, "Crusader")) {
            g_hwnd = hwnd;

            // logger.sendMessage("Crusader find!");

            MainInit(hwnd);

            MH_DisableHook(pCreateWindowExW);
            MH_RemoveHook(pCreateWindowExW);

            MH_DisableHook(pCreateWindowExA);
            MH_RemoveHook(pCreateWindowExA);
        }
    }

    return hwnd;
}

HWND WINAPI MyCreateWindowExW(
    DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
    DWORD dwStyle, int x, int y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam
) {
    HWND hwnd = oCreateWindowExW(dwExStyle, lpClassName, lpWindowName,
                                 dwStyle, x, y, nWidth, nHeight,
                                 hWndParent, hMenu, hInstance, lpParam);

    if (hwnd != NULL && !g_hwnd) {
        wchar_t windowTitleW[256];
        GetWindowTextW(hwnd, windowTitleW, sizeof(windowTitleW) / sizeof(wchar_t));

        // Преобразование Unicode строки в ANSI строку
        char windowTitle[256];
        WideCharToMultiByte(CP_ACP, 0, windowTitleW, -1, windowTitle, sizeof(windowTitle), NULL, NULL);

        std::string titleString(windowTitle);

        // logger.sendMessage("Create window: " + titleString);

        if (ContainsWordIgnoreCase(windowTitle, "Crusader")) {
            g_hwnd = hwnd;

            // logger.sendMessage("Crusader find!");

            MainInit(hwnd);

            MH_DisableHook(pCreateWindowExW);
            MH_RemoveHook(pCreateWindowExW);

            MH_DisableHook(pCreateWindowExA);
            MH_RemoveHook(pCreateWindowExA);
        }
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

    pCreateWindowExA = (CreateWindowExAFn)GetProcAddress(hUser32, "CreateWindowExA");

    if (!pCreateWindowExA)
        return false;

    if (MH_CreateHook(pCreateWindowExA, &MyCreateWindowExA, reinterpret_cast<void**>(&oCreateWindowExA)) != MH_OK)
        return false;

    if (MH_EnableHook(pCreateWindowExA) != MH_OK) {
        MH_RemoveHook(pCreateWindowExA);
        return false;
    }

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
