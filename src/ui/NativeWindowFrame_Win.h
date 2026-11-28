#pragma once

#include <QRect>
#include <QtGlobal>
#include <vector>

class QWindow;

namespace auvio {

class NativeWindowFrame {
public:
    static void enable(QWindow* window);

    static bool handleNativeEvent(QWindow* window, void* message, qintptr* result,
                                   const QRect& captionRect,
                                   const std::vector<QRect>& excludedRects,
                                   int resizeMargin);
};

}
