#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "types.h"

void KwmConfigCommand(std::vector<std::string> &Tokens);
void KwmQueryCommand(std::vector<std::string> &Tokens, int ClientSockFD);
void KwmBindCommand(std::vector<std::string> &Tokens, bool Passthrough);
void KwmWindowCommand(std::vector<std::string> &Tokens);
void KwmSpaceCommand(std::vector<std::string> &Tokens);
void KwmDisplayCommand(std::vector<std::string> &Tokens);
void KwmTreeCommand(std::vector<std::string> &Tokens);

void KwmInterpretCommand(std::string Message, int ClientSockFD);

#endif
