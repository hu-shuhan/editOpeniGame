#include <QCoreApplication>

#if defined(_WIN32) && defined(NDEBUG)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace {
void DisableReleaseConsoleQuickEdit()
{
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (input == nullptr || input == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(input, &mode)) {
        // No console, or stdin is redirected: leave the handle unchanged.
        return;
    }

    // Avoid mouse selection pausing console output. Keep keyboard input,
    // line editing and all unrelated console mode flags unchanged.
    mode |= ENABLE_EXTENDED_FLAGS;
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(input, mode);
}
} // namespace

// Runs during QApplication construction, before the main window/model loading.
Q_COREAPP_STARTUP_FUNCTION(DisableReleaseConsoleQuickEdit)
#endif
