#include "kwm.h"

extern window_info *FocusedWindow;

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
        std::string Message;
        std::stringstream Stream(KwmReadFromSocket(ClientSockFD));
        std::getline(Stream, Message, ' ');

        if(Message == "focused")
        {
            if(FocusedWindow)
                KwmWriteToSocket(ClientSockFD, FocusedWindow->Owner + " - " + FocusedWindow->Name);
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
