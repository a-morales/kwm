#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "types.h"

void KwmInterpretCommand(std::string Message, int ClientSockFD);
void KwmConfigCommand(std::vector<std::string> &Tokens);
void KwmReadCommand(std::vector<std::string> &Tokens, int ClientSockFD);
void KwmMarkCommand(std::vector<std::string> &Tokens);
void KwmTreeCommand(std::vector<std::string> &Tokens);
void KwmFocusCommand(std::vector<std::string> &Tokens);
void KwmSwapCommand(std::vector<std::string> &Tokens);
void KwmZoomCommand(std::vector<std::string> &Tokens);
void KwmFloatCommand(std::vector<std::string> &Tokens);
void KwmRefreshCommand(std::vector<std::string> &Tokens);
void KwmNodeCommand(std::vector<std::string> &Tokens);
void KwmMoveCommand(std::vector<std::string> &Tokens);
void KwmMoveCommand(std::vector<std::string> &Tokens);
void KwmSplitCommand(std::vector<std::string> &Tokens);
void KwmPaddingCommand(std::vector<std::string> &Tokens);
void KwmGapCommand(std::vector<std::string> &Tokens);
void KwmBindCommand(std::vector<std::string> &Tokens);

#endif
