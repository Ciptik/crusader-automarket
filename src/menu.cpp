#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <chrono>

#include <windowsx.h>

#include "Menu.hpp"

namespace Menu
{
    HBRUSH hBackgroundBrush;
    HBRUSH hFieldBrush;
    HBRUSH hButtonBrush;
    HBRUSH hPressedButtonBrush;
    HFONT hBoldFont;
    HFONT hFont;

    HDC hdc;

    HWND g_hwnd;

    int x, y;
    int screenWidth, screenHeight;
    int menuWidth, menuHeight;

    RECT menuCanvas;

    bool isMenuActive;

    // Variables for working with the currently held button
    bool isLeftMouseDown;
    int xLeftMouseDown;
    int yLeftMouseDown;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTimeLeftMouseDown;
    bool isMouseTracking;
    std::chrono::milliseconds lastUpdateTime;
    std::chrono::milliseconds updateIntervalMs = std::chrono::milliseconds(10);

    void Init(HWND hwnd) {
        hBackgroundBrush = CreateSolidBrush(RGB(239, 195, 115));
        hFieldBrush = CreateSolidBrush(RGB(239, 228, 176));
        hButtonBrush = CreateSolidBrush(RGB(185, 122, 87));
        hPressedButtonBrush = CreateSolidBrush(RGB(122, 77, 52));

        hBoldFont = CreateFont(
            22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Monotype Corsiva")
        );
        hFont = CreateFont(
            24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Monotype Corsiva")
        );

        x = 0;
        y = 0;

        isMouseTracking = false;
        lastUpdateTime = std::chrono::milliseconds(0);
        isMenuActive = false;

        g_hwnd = hwnd;
    }

    void Uninit() {
        DeleteObject(hBackgroundBrush);
        DeleteObject(hFieldBrush);
        DeleteObject(hButtonBrush);
        DeleteObject(hPressedButtonBrush);
        DeleteObject(hBoldFont);
        DeleteObject(hFont);
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

    bool IsPointInRect(int x, int y, const RECT &rect) {
        return (x >= rect.left && x <= rect.right &&
                y >= rect.top && y <= rect.bottom);
    }

    bool IsRectPressed(RECT rect) {
        return IsPointInRect(xLeftMouseDown, yLeftMouseDown, rect) && isLeftMouseDown;
    }

    std::chrono::milliseconds GetTimeLeftMouseDown() {
        auto start = startTimeLeftMouseDown;
        auto current = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(current - start);
    }

    void DrawFilledRect(RECT rect, HBRUSH hBrush) {
        OffsetRect(&rect, x, y);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
        FillRect(hdc, &rect, hBrush);
        SelectObject(hdc, hOldBrush);
    }

    void DrawTextInRect(RECT rect, HFONT hFont, const wchar_t* text, TextAlign horizontalAlign, TextAlign verticalAlign, int marginLeft=0) {
        OffsetRect(&rect, x, y);
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        int format = DT_NOPREFIX;
        if (horizontalAlign == TEXT_ALIGN_CENTER) {
            format |= DT_CENTER;
        } else if (horizontalAlign == TEXT_ALIGN_LEFT) {
            format |= DT_LEFT;
            rect.left += marginLeft; // Добавляем отступ слева
        }

        if (verticalAlign == TEXT_ALIGN_CENTER) {
            format |= DT_VCENTER | DT_SINGLELINE;
        }

        DrawTextW(hdc, text, -1, &rect, format);
        SelectObject(hdc, hOldFont);
    }

    LRESULT ImplWin32_WndProcHandler(WNDPROC oWndProc, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_LBUTTONDOWN: {
                xLeftMouseDown = GET_X_LPARAM(lParam) - x;
                yLeftMouseDown = GET_Y_LPARAM(lParam) - y;

                if (!IsOpenMenu())
                    break;

                if (xLeftMouseDown < 0 || yLeftMouseDown < 0)
                    break;

                if (!IsPointInRect(xLeftMouseDown, yLeftMouseDown, menuCanvas))
                    break;

                isLeftMouseDown = true;
                startTimeLeftMouseDown = std::chrono::high_resolution_clock::now();

                return 0;

                break;
            }

            case WM_LBUTTONUP: {
                isMouseTracking = false;
                lastUpdateTime = std::chrono::milliseconds(0);

                if (!isLeftMouseDown)
                    break;

                isLeftMouseDown = false;
                return 0;

                break;
            }

            case WM_KEYDOWN: {
                if (wParam == 'M') {
                    isMenuActive = !isMenuActive;
                    return 0;
                }
            break;
        }
        }
        
        return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
    }

    void DrawButton(RECT rect, const wchar_t* text, void (*onClick)() = nullptr) {
        bool isButtonPressed = IsRectPressed(rect);
        DrawFilledRect(rect, isButtonPressed ? hPressedButtonBrush : hButtonBrush);
        DrawTextInRect(rect, hFont, text, TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER);

        // You need to call the callback function 1 time in one click of the left mouse button
        if (!isMouseTracking && isButtonPressed) {
            isMouseTracking = true;
            onClick();
        }
    }

    void DrawNumericField(RECT rect, RECT rectPlusButton, RECT rectMinusButton, int &counter, int minValue, int maxValue) {
        bool isPlusButtonPressed = IsRectPressed(rectPlusButton);
        bool isMinusButtonPressed = IsRectPressed(rectMinusButton);
        DrawFilledRect(rect, hFieldBrush);
        DrawFilledRect(rectPlusButton, isPlusButtonPressed ? hPressedButtonBrush : hButtonBrush);
        DrawFilledRect(rectMinusButton, isMinusButtonPressed ? hPressedButtonBrush : hButtonBrush);
        DrawTextInRect(rect, hFont, std::to_wstring(counter).c_str(), TEXT_ALIGN_LEFT, TEXT_ALIGN_CENTER, 4);
        DrawTextInRect(rectPlusButton, hFont, L"+", TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER);
        DrawTextInRect(rectMinusButton, hFont, L"-", TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER);

        if (isPlusButtonPressed || isMinusButtonPressed) {
            int delta = isPlusButtonPressed ? 1 : -1;

            if (!isMouseTracking) {
                isMouseTracking = true;
                int newValue = counter + delta;
                if (minValue <= newValue && newValue <= maxValue)
                    counter = newValue;
            } else {
                std::chrono::milliseconds timeLeftMouseDown = GetTimeLeftMouseDown();

                if (timeLeftMouseDown > std::chrono::milliseconds(300)) {
                    if ((timeLeftMouseDown - lastUpdateTime) >= updateIntervalMs) {
                        int newValue = counter + delta;
                        if (minValue <= newValue && newValue <= maxValue)
                            counter = newValue;
                        lastUpdateTime = timeLeftMouseDown;
                    }
                }
            }
        }
    }

    void DrawTextField(RECT rect, const wchar_t* text, TextAlign horizontalAlign=TEXT_ALIGN_CENTER, TextAlign verticalAlign=TEXT_ALIGN_CENTER) {
        DrawFilledRect(rect, hFieldBrush);
        DrawTextInRect(rect, hFont, text, horizontalAlign, verticalAlign, 4);
    }

    void DrawBackground(RECT rect) {
        GetWindowSize(screenWidth, screenHeight);
        menuCanvas = rect;
        menuWidth = rect.right - rect.left;
        menuHeight = rect.bottom - rect.top;
        x = screenWidth / 2 - menuWidth / 2;
        y = screenHeight / 2 - menuHeight / 2;

        DrawFilledRect(rect, hBackgroundBrush);
    }

    void SetContext(HDC newHdc) {
        hdc = newHdc;
    }

    bool IsOpenMenu() {
        return isMenuActive;
    }
}
