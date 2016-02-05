#ifndef KWM_H
#define KWM_H

#include "types.h"

CGEventRef CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon);
void * KwmWindowMonitor(void*);

void KwmExecuteConfig();
void KwmExecuteFile(std::string File);
void KwmReloadConfig();
void KwmClearSettings();

bool IsKwmAlreadyAddedToLaunchd();
void AddKwmToLaunchd();
void RemoveKwmFromLaunchd();

void KwmInit();
void KwmQuit();
bool GetKwmFilePath();
bool CheckPrivileges();
bool CheckArguments(int argc, char **argv);
void SignalHandler(int Signum);
void Fatal(const std::string &Err);
int main(int argc, char **argv);

#endif
