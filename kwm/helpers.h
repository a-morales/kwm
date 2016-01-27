#ifndef HELPERS_H
#define HELPERS_H

#include "types.h"

std::string CreateStringFromTokens(std::vector<std::string> Tokens, int StartIndex);
std::vector<std::string> SplitString(std::string Line, char Delim);

bool IsPrefixOfString(std::string &Line, std::string Prefix);
int ConvertStringToInt(std::string Integer);
double ConvertStringToDouble(std::string Double);
unsigned int ConvertHexStringToInt(std::string Hex);

#endif
