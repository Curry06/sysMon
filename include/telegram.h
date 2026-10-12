#ifndef TELEGRAM_H
#define TELEGRAM_H

#include "config.h"

int SendTextToTelegram(const char* message);
int SendPhotoToTelegram(const BYTE* imageData, DWORD imageSize);
BYTE* DownloadURL(const char* url, DWORD* outSize);
void url_encode(const char* src, char* dst, size_t dstSize);

#endif // TELEGRAM_H
