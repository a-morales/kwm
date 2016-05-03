#ifndef KWM_H
#define KWM_H

#include "types.h"

extern "C" void NSApplicationLoad(void);
extern void CreateWorkspaceWatcher(void *Watcher);

CGEventRef CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon);
void * KwmWindowMonitor(void*);

void KwmExecuteConfig();
void KwmExecuteInitScript();
void KwmReloadConfig();
void KwmClearSettings();

void KwmInit();
void KwmQuit();
bool GetKwmFilePath();
bool CheckPrivileges();
bool CheckArguments(int argc, char **argv);
void SignalHandler(int Signum);
void Fatal(const std::string &Err);
int main(int argc, char **argv);

#endif
