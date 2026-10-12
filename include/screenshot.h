#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include "config.h"

BYTE* capScr(DWORD* outSize);
DWORD WINAPI ScreenshotThread(LPVOID lpParam);

#endif // SCREENSHOT_H
