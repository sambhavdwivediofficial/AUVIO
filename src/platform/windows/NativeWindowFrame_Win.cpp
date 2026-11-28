#include "NativeWindowFrame_Win.h"

#include <QWindow>

#include <windows.h>
#include <dwmapi.h>
#include <windowsx.h>

#pragma comment(lib, "dwmapi.lib")

namespace auvio {

void NativeWindowFrame::enable(QWindow* window) {
    if (!window) return;
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());

    MARGINS margins{0, 0, 1, 0}; // a 1px bottom margin is enough to keep the real DWM drop shadow
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style |= WS_THICKFRAME | WS_CAPTION;
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

bool NativeWindowFrame::handleNativeEvent(QWindow* window, void* message, qintptr* result,
                                            const QRect& captionRect,
                                            const std::vector<QRect>& excludedRects,
                                            int resizeMargin) {
    MSG* msg = static_cast<MSG*>(message);
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());

    if (msg->message == WM_NCCALCSIZE) {
        if (msg->wParam == TRUE) {
            auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);

            WINDOWPLACEMENT placement{};
            placement.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(hwnd, &placement);
            if (placement.showCmd == SW_SHOWMAXIMIZED) {
                // Maximized windows otherwise overhang past the work area by the
                // standard frame thickness -- pull the client rect back in to match.
                const int frameX = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                const int frameY = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                params->rgrc[0].left += frameX;
                params->rgrc[0].top += frameY;
                params->rgrc[0].right -= frameX;
                params->rgrc[0].bottom -= frameY;
            }

            *result = 0;
            return true;
        }
    }

    if (msg->message == WM_NCHITTEST) {
        *result = 0;

        POINT cursor{GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};
        ScreenToClient(hwnd, &cursor);

        RECT clientRect{};
        GetClientRect(hwnd, &clientRect);

        const int x = cursor.x;
        const int y = cursor.y;
        const int w = clientRect.right - clientRect.left;
        const int h = clientRect.bottom - clientRect.top;

        WINDOWPLACEMENT placement{};
        placement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd, &placement);
        const bool maximized = placement.showCmd == SW_SHOWMAXIMIZED;

        if (!maximized) {
            const bool onLeft = x < resizeMargin;
            const bool onRight = x >= w - resizeMargin;
            const bool onTop = y < resizeMargin;
            const bool onBottom = y >= h - resizeMargin;

            if (onTop && onLeft) { *result = HTTOPLEFT; return true; }
            if (onTop && onRight) { *result = HTTOPRIGHT; return true; }
            if (onBottom && onLeft) { *result = HTBOTTOMLEFT; return true; }
            if (onBottom && onRight) { *result = HTBOTTOMRIGHT; return true; }
            if (onLeft) { *result = HTLEFT; return true; }
            if (onRight) { *result = HTRIGHT; return true; }
            if (onTop) { *result = HTTOP; return true; }
            if (onBottom) { *result = HTBOTTOM; return true; }
        }

        const QPoint point(x, y);
        for (const QRect& excluded : excludedRects) {
            if (excluded.contains(point)) {
                *result = HTCLIENT;
                return true;
            }
        }

        if (captionRect.contains(point)) {
            *result = HTCAPTION;
            return true;
        }

        *result = HTCLIENT;
        return true;
    }

    return false;
}

}
