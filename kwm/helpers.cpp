#include "helpers.h"

bool IsPrefixOfString(std::string &Line, std::string Prefix)
{
    bool Result = false;

    if(Line.substr(0, Prefix.size()) == Prefix)
    {
        Line = Line.substr(Prefix.size()+1);
        Result = true;
    }

    return Result;
}

std::string CreateStringFromTokens(std::vector<std::string> Tokens, int StartIndex)
{
    std::string Text = "";
    for(std::size_t TokenIndex = StartIndex; TokenIndex < Tokens.size(); ++TokenIndex)
    {
        Text += Tokens[TokenIndex];
        if(TokenIndex < Tokens.size() - 1)
            Text += " ";
    }

    return Text;
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

int ConvertStringToInt(std::string Integer)
{
    int IntResult;
    std::stringstream Stream(Integer);
    Stream >> IntResult;
    return IntResult;
}

unsigned int ConvertHexStringToInt(std::string HexString)
{
    unsigned int HexResult = 0xffffff;
    std::stringstream Stream;
    Stream << std::hex << HexString;
    Stream >> HexResult;
    return HexResult;
}

double ConvertStringToDouble(std::string Double)
{
    double DoubleResult;
    std::stringstream Stream(Double);
    Stream >> DoubleResult;
    return DoubleResult;
}
