#include "command.h"
#include "helpers.h"

extern kwm_thread KWMThread;

void KwmExecuteSystemCommand(std::string Command)
{
    int ChildPID = fork();
    if(ChildPID == 0)
    {
        DEBUG("Exec: FORK SUCCESS");
        std::vector<std::string> Tokens = SplitString(Command, ' ');
        const char **ExecArgs = new const char*[Tokens.size()+1];
        for(int Index = 0; Index < Tokens.size(); ++Index)
        {
            ExecArgs[Index] = Tokens[Index].c_str();
            DEBUG("Exec argument " << Index << ": " << ExecArgs[Index]);
        }

        ExecArgs[Tokens.size()] = NULL;
        int StatusCode = execvp(ExecArgs[0], (char **)ExecArgs);
        DEBUG("Exec failed with code: " << StatusCode);
        exit(StatusCode);
    }
}

void KwmExecuteThreadedSystemCommand(std::string Command)
{
    std::string *HCommand = new std::string(Command);
    pthread_create(&KWMThread.SystemCommand, NULL, &KwmStartThreadedSystemCommand, HCommand);
}

void * KwmStartThreadedSystemCommand(void *Args)
{
    std::string Command = *((std::string*)Args);
    KwmExecuteSystemCommand(Command);
    delete (std::string*)Args;
    return NULL;
}
