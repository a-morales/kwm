#include <fstream>
#include <iostream>
#include <string>

#include "help.h"

#include <libproc.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define KwmDaemonPort 3020

int KwmcSockFD;

void Fatal(const std::string &err)
{
    std::cout << err << std::endl;
    exit(1);
}

std::string ReadFromSocket(int SockFD)
{
    std::string Message;
    char Cur;

    while(recv(SockFD, &Cur, 1, 0))
        Message += Cur;

    return Message;
}

void KwmcForwardMessageThroughSocket(int argc, char **argv)
{
    std::string Msg;
    for(int i = 1; i < argc; ++i)
    {
        Msg += argv[i];

        if(i < argc - 1)
            Msg += " ";
    }
    Msg += "\n";

    send(KwmcSockFD, Msg.c_str(), Msg.size(), 0);

    std::string Response = ReadFromSocket(KwmcSockFD);
    if(!Response.empty())
        std::cout << Response << std::endl;
}

void KwmcConnectToDaemon()
{
    struct sockaddr_in srv_addr;
    struct hostent *server;

    if((KwmcSockFD = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        Fatal("Could not create socket!");

    server = gethostbyname("localhost");
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(KwmDaemonPort);
    std::memcpy(&srv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    std::memset(&srv_addr.sin_zero, '\0', 8);

    if(connect(KwmcSockFD, (struct sockaddr*) &srv_addr, sizeof(struct sockaddr)) == -1)
        Fatal("Connection failed! Make sure the Kwm daemon is running...");
}

int main(int argc, char **argv)
{
    if(argc >= 2)
    {
        std::string Command = argv[1];
        if(Command == "help")
        {
            std::cout << helpText() << std::endl;
        }
        else
        {
            KwmcConnectToDaemon();
            KwmcForwardMessageThroughSocket(argc, argv);
            close(KwmcSockFD);
        }
    }
    else
    {
        std::cout << helpText() << std::endl;
    }

    return 0;
}
