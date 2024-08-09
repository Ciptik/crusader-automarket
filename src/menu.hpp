#ifndef MENU_HPP
#define MENU_HPP

#include <Windows.h>

enum TextAlign {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER
};

namespace Menu
{
    void Init(HWND hwnd);
    void Uninit();
    LRESULT ImplWin32_WndProcHandler(WNDPROC oWndProc, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void DrawButton(RECT rect, const wchar_t* text, void (*onClick)() = nullptr);
    void DrawNumericField(RECT rect, RECT rectPlusButton, RECT rectMinusButton, int &counter, int minValue, int maxValue);
    void DrawTextField(RECT rect, const wchar_t* text, TextAlign horizontalAlign=TEXT_ALIGN_CENTER, TextAlign verticalAlign=TEXT_ALIGN_CENTER);
    void DrawBackground(RECT rect);
    void SetContext(HDC newHdc);
    bool IsOpenMenu();
    RECT GetClipRect();
}

#endif // MENU_HPP
