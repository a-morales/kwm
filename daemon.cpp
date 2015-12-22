#include "kwm.h"

extern screen_info *Screen;
extern std::vector<std::string> FloatingAppLst;
extern window_info *FocusedWindow;
extern focus_option KwmFocusMode;
extern int KwmSplitMode;
extern bool KwmUseBuiltinHotkeys;

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

        if(Tokens[0] == "quit")
        {
            KwmQuit();
        }
        else if(Tokens[0] == "config")
        {
            if(Tokens[1] == "hotkeys")
            {
                if(Tokens[2] == "disable")
                    KwmUseBuiltinHotkeys = false;
                else if(Tokens[2] == "enable")
                    KwmUseBuiltinHotkeys = true;
            }
            else if(Tokens[1] == "float")
            {
                std::string AppName;
                for(int TokenIndex = 2; TokenIndex < Tokens.size(); ++TokenIndex)
                {
                    AppName += Tokens[TokenIndex];
                    if(TokenIndex < Tokens.size() - 1)
                        AppName += " ";
                }
                FloatingAppLst.push_back(AppName);
            }
            else if(Tokens[1] == "padding")
            {
                if(Tokens[2] == "left" || Tokens[2] == "right" ||
                   Tokens[2] == "top" || Tokens[2] == "bottom")
                {
                    int Value = 0;
                    std::stringstream Stream(Tokens[3]);
                    Stream >> Value;
                    SetDefaultPaddingOfDisplay(Tokens[2], Value);
                }
            }
            else if(Tokens[1] == "gap")
            {
                if(Tokens[2] == "vertical" || Tokens[2] == "horizontal")
                {
                    int Value = 0;
                    std::stringstream Stream(Tokens[3]);
                    Stream >> Value;
                    SetDefaultGapOfDisplay(Tokens[2], Value);
                }
            }
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
                    if(KwmFocusMode == FocusModeDisabled)
                        KwmFocusMode = FocusModeAutofocus;
                    else if(KwmFocusMode == FocusModeAutofocus)
                        KwmFocusMode = FocusModeAutoraise;
                    else if(KwmFocusMode == FocusModeAutoraise)
                        KwmFocusMode = FocusModeDisabled;
                }
                else if(Tokens[2] == "autofocus")
                    KwmFocusMode = FocusModeAutofocus;
                else if(Tokens[2] == "autoraise")
                    KwmFocusMode = FocusModeAutoraise;
                else if(Tokens[2] == "disabled")
                    KwmFocusMode = FocusModeDisabled;
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
            else if(Tokens[1] == "-c")
            {
                if(Tokens[2] == "split")
                {
                    tree_node *Node = GetNodeFromWindowID(Screen->Space[Screen->ActiveSpace].RootNode, FocusedWindow->WID);
                    ToggleNodeSplitMode(Screen, Node->Parent);
                }
                else if(Tokens[2] == "refresh")
                    ResizeWindowToContainerSize();

            }
            else if(Tokens[1] == "-f")
            {
                if(Tokens[2] == "prev")
                    ShiftWindowFocus(-1);
                else if(Tokens[2] == "next")
                    ShiftWindowFocus(1);
                else if(Tokens[2] == "curr")
                    FocusWindowBelowCursor();
            }
            else if(Tokens[1] == "-s")
            {
                if(Tokens[2] == "prev")
                    SwapFocusedWindowWithNearest(-1);
                else if(Tokens[2] == "next")
                    SwapFocusedWindowWithNearest(1);
                else if(Tokens[2] == "mark")
                    SwapFocusedWindowWithMarked();
            }
        }
        else if(Tokens[0] == "screen")
        {
            if(Tokens[1] == "-s")
            {
                if(Tokens[2] == "optimal")
                    KwmSplitMode = -1;
                else if(Tokens[2] == "vertical")
                    KwmSplitMode = 1;
                else if(Tokens[2] == "horizontal")
                    KwmSplitMode = 2;
            }
            else if(Tokens[1] == "-m")
            {
                if(Tokens[2] == "prev")
                    CycleFocusedWindowDisplay(-1, true);
                else if(Tokens[2] == "next")
                    CycleFocusedWindowDisplay(1, true);
                else
                {
                    int Index = 0;
                    std::stringstream Stream(Tokens[2]);
                    Stream >> Index;
                    CycleFocusedWindowDisplay(Index, false);
                }
            }
        }
        else if(Tokens[0] == "space")
        {
            if(Tokens[1] == "-t")
            {
                if(Tokens[2] == "toggle")
                    ToggleFocusedSpaceFloating();
                else if(Tokens[2] == "float")
                    FloatFocusedSpace();
                else if(Tokens[2] == "tile")
                    TileFocusedSpace();
            }
            else if(Tokens[1] == "-r")
            {
                if(Tokens[2] == "90" || Tokens[2] == "270" || Tokens[2] == "180")
                {
                    int Deg = 0;
                    std::stringstream Stream(Tokens[2]);
                    Stream >> Deg;
                    RotateTree(Screen->Space[Screen->ActiveSpace].RootNode, Deg);
                    CreateNodeContainers(Screen, Screen->Space[Screen->ActiveSpace].RootNode, false);
                    ApplyNodeContainer(Screen->Space[Screen->ActiveSpace].RootNode);
                }
            }
            else if(Tokens[1] == "-m")
            {
                if(Tokens[2] == "left")
                    MoveContainerSplitter(1, -10);
                else if(Tokens[2] == "right")
                    MoveContainerSplitter(1, 10);
                else if(Tokens[2] == "up")
                    MoveContainerSplitter(2, -10);
                else if(Tokens[2] == "down")
                    MoveContainerSplitter(2, 10);
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
