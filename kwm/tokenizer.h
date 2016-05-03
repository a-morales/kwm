#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "types.h"
inline bool
IsDot(char C)
{
    bool Result = ((C == '.') || (C == ','));
    return Result;
}

inline bool
IsEndOfLine(char C)
{
    bool Result = ((C == '\n') ||
                   (C == '\r'));

    return Result;
}

inline bool
IsWhiteSpace(char C)
{
    bool Result = ((C == ' ') ||
                   (C == '\t') ||
                   IsEndOfLine(C));

    return Result;
}

inline bool
IsAlpha(char C)
{
    bool Result = (((C >= 'a') && (C <= 'z')) ||
                   ((C >= 'A') && (C <= 'Z')));

    return Result;
}

inline bool
IsNumeric(char C)
{
    bool Result = ((C >= '0') && (C <= '9'));
    return Result;
}

inline bool
TokenEquals(token Token, const char *Match)
{
    const char *At = Match;
    for(int Index = 0; Index < Token.TextLength; ++Index, ++At)
    {
        if((*At == 0) || (Token.Text[Index] != *At))
            return false;
    }

    bool Result = (*At == 0);
    return Result;
}

std::string GetTextTilEndOfLine(tokenizer *Tokenizer);
token GetToken(tokenizer *Tokenizer);
void EatAllWhiteSpace(tokenizer *Tokenizer);
bool RequireToken(tokenizer *Tokenizer, token_type DesiredType);

#endif
