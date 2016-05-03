#ifndef COMMAND_H
#define COMMAND_H

#include "types.h"

void KwmExecuteSystemCommand(std::string Command);
void KwmExecuteThreadedSystemCommand(std::string Command);
void * KwmStartThreadedSystemCommand(void *Args);

#endif
