#pragma once

#include "DevServerState.h"

namespace MemeUI {
    void initNativeDesktopApp();
    void showNativeDesktopWindow(DevServerState &state);
    void hideNativeDesktopWindow();
    bool isNativeDesktopWindowOpen();
    void pumpNativeDesktopEvents(double timeoutSeconds = 0.016);
    void triggerNativeDesktopRepaint();
}
