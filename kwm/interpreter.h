#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "types.h"

void KwmInterpretCommand(std::string Message, int ClientSockFD);
void KwmConfigCommand(std::vector<std::string> &Tokens);
void KwmReadCommand(std::vector<std::string> &Tokens, int ClientSockFD);
void KwmWindowCommand(std::vector<std::string> &Tokens);
void KwmTreeCommand(std::vector<std::string> &Tokens);
void KwmScreenCommand(std::vector<std::string> &Tokens);
void KwmSpaceCommand(std::vector<std::string> &Tokens);
void KwmBindCommand(std::vector<std::string> &Tokens);

#endif
