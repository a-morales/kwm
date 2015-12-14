#include "kwm.h"

extern window_info *FocusedWindow;
extern export_table ExportTable;

int KwmSockFD;
bool KwmDaemonIsRunning;
int KwmDaemonPort = 3020;

std::string KwmReadFromSocket(int ClientSockFD)
{
    char Cur;
    std::string Message;
    while(recv(ClientSockFD, &Cur, 1, 0))
    {
        if(Cur == '\n')
            break;
        Message += Cur;
    }
    return Message;
}

void KwmWriteToSocket(int ClientSockFD, std::string Msg)
{
    send(ClientSockFD, Msg.c_str(), Msg.size(), 0);
}

std::vector<std::string> SplitString(std::string Line, char Delim)
{
    std::vector<std::string> Elements;
    std::stringstream Stream(Line);
    std::string Temp;

    while(std::getline(Stream, Temp, Delim))
        Elements.push_back(Temp);

    return Elements;
}

void * KwmDaemonHandleConnectionBG(void *)
{
    while(KwmDaemonIsRunning)
        KwmDaemonHandleConnection();
}

void KwmDaemonHandleConnection()
{
    int ClientSockFD;
    struct sockaddr_in ClientAddr;
    socklen_t SinSize = sizeof(struct sockaddr);

    ClientSockFD = accept(KwmSockFD, (struct sockaddr*)&ClientAddr, &SinSize);
    if(ClientSockFD != -1)
    {
        std::string Message = KwmReadFromSocket(ClientSockFD);
        std::vector<std::string> Tokens = SplitString(Message, ' ');

        if(Tokens[0] == "restart")
        {
            KwmRestart();
        }
        else if(Tokens[0] == "focused")
        {
            if(FocusedWindow)
                KwmWriteToSocket(ClientSockFD, FocusedWindow->Owner + " - " + FocusedWindow->Name);
        }
        else if(Tokens[0] == "focus")
        {
            if(Tokens[1] == "-t")
            {
                if(Tokens[2] == "toggle")
                {
                    if(ExportTable.KwmFocusMode == FocusModeDisabled)
                        ExportTable.KwmFocusMode = FocusModeAutofocus;
                    else if(ExportTable.KwmFocusMode == FocusModeAutofocus)
                        ExportTable.KwmFocusMode = FocusModeAutoraise;
                    else if(ExportTable.KwmFocusMode == FocusModeAutoraise)
                        ExportTable.KwmFocusMode = FocusModeDisabled;
                }
                else if(Tokens[2] == "autofocus")
                    ExportTable.KwmFocusMode = FocusModeAutofocus;
                else if(Tokens[2] == "autoraise")
                    ExportTable.KwmFocusMode = FocusModeAutoraise;
                else if(Tokens[2] == "disabled")
                    ExportTable.KwmFocusMode = FocusModeDisabled;
            }
        }
        else if(Tokens[0] == "window") 
        {
            if(Tokens[1] == "-t")
            {
                if(Tokens[2] == "fullscreen")
                    ToggleFocusedWindowFullscreen();
                else if(Tokens[2] == "parent")
                    ToggleFocusedWindowParentContainer();
                else if(Tokens[2] == "float")
                    ToggleFocusedWindowFloating();
                else if(Tokens[2] == "mark")
                    MarkWindowContainer();
            }
            else if(Tokens[1] == "-f")
            {
                if(Tokens[2] == "prev")
                    ShiftWindowFocus(-1);
                else if(Tokens[2] == "next")
                    ShiftWindowFocus(1);
            }
            else if(Tokens[1] == "-s")
            {
                if(Tokens[2] == "prev")
                    SwapFocusedWindowWithNearest(-1);
                else if(Tokens[2] == "next")
                    SwapFocusedWindowWithNearest(1);
            }
        }
        else if(Tokens[0] == "screen")
        {
            if(Tokens[1] == "-s")
            {
                if(Tokens[2] == "optimal")
                    ExportTable.KwmSplitMode = -1;
                else if(Tokens[2] == "vertical")
                    ExportTable.KwmSplitMode = 1;
                else if(Tokens[2] == "horizontal")
                    ExportTable.KwmSplitMode = 2;
            }
            else if(Tokens[1] == "-m")
            {
                if(Tokens[2] == "reflect")
                    ReflectWindowNodeTreeVertically();
                else if(Tokens[2] == "left")
                    MoveContainerSplitter(1, -10);
                else if(Tokens[2] == "right")
                    MoveContainerSplitter(1, 10);
                else if(Tokens[2] == "up")
                    MoveContainerSplitter(2, -10);
                else if(Tokens[2] == "down")
                    MoveContainerSplitter(2, 10);
                else if(Tokens[2] == "prev")
                    CycleFocusedWindowDisplay(-1);
                else if(Tokens[2] == "next")
                    CycleFocusedWindowDisplay(1);
            }
            else if(Tokens[1] == "-p")
            {
                if(Tokens[3] == "left" || Tokens[3] == "right" ||
                   Tokens[3] == "top" || Tokens[3] == "bottom")
                {
                    int Value = 0;
                    if(Tokens[2] == "increase")
                        Value = 10;
                    else if(Tokens[2] == "decrease")
                        Value = -10;

                    ChangePaddingOfDisplay(Tokens[3], Value);
                }

            }
            else if(Tokens[1] == "-g")
            {
                if(Tokens[3] == "vertical" || Tokens[3] == "horizontal")
                {
                    int Value = 0;
                    if(Tokens[2] == "increase")
                        Value = 10;
                    else if(Tokens[2] == "decrease")
                        Value = -10;

                    ChangeGapOfDisplay(Tokens[3], Value);
                }
            }
        }

        close(ClientSockFD);
    }
}

void KwmTerminateDaemon()
{
    KwmDaemonIsRunning = false;
    close(KwmSockFD);
}

bool KwmStartDaemon()
{
    struct sockaddr_in SrvAddr;
    int _True = 1;

    if((KwmSockFD = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        return false;

    if(setsockopt(KwmSockFD, SOL_SOCKET, SO_REUSEADDR, &_True, sizeof(int)) == -1)
        std::cout << "Could not set socket option: SO_REUSEADDR!" << std::endl;

    SrvAddr.sin_family = AF_INET;
    SrvAddr.sin_port = htons(KwmDaemonPort);
    SrvAddr.sin_addr.s_addr = 0;
    std::memset(&SrvAddr.sin_zero, '\0', 8);

    if(bind(KwmSockFD, (struct sockaddr*)&SrvAddr, sizeof(struct sockaddr)) == -1)
        return false;

    if(listen(KwmSockFD, 10) == -1)
        return false;

    KwmDaemonIsRunning = true;
    DEBUG("Local Daemon is now running.." << std::endl)
    return true;
}
