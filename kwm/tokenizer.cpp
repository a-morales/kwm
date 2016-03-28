#include "tokenizer.h"

void EatAllWhiteSpace(tokenizer *Tokenizer)
{
    for(;;)
    {
        if(IsWhiteSpace(Tokenizer->At[0]))
            ++Tokenizer->At;
        else
            break;
    }
}

bool RequireToken(tokenizer *Tokenizer, token_type DesiredType)
{
    token Token = GetToken(Tokenizer);
    bool Result = Token.Type == DesiredType;
    return Result;
}


token GetToken(tokenizer *Tokenizer)
{
    EatAllWhiteSpace(Tokenizer);

    token Token = {};
    Token.TextLength = 1;
    Token.Text = Tokenizer->At;
    char C = Tokenizer->At[0];
    ++Tokenizer->At;

    switch(C)
    {
        case '\0': { Token.Type = Token_EndOfStream; } break;
        case ':': { Token.Type = Token_Colon; } break;
        case ';': { Token.Type = Token_SemiColon; } break;
        case '=': { Token.Type = Token_Equals; } break;
        case '-': { Token.Type = Token_Dash; } break;
        case '(': { Token.Type = Token_OpenParen; } break;
        case ')': { Token.Type = Token_CloseParen; } break;
        case '[': { Token.Type = Token_OpenBracket; } break;
        case ']': { Token.Type = Token_CloseBracket; } break;
        case '{': { Token.Type = Token_OpenBrace; } break;
        case '}': { Token.Type = Token_CloseBrace; } break;

        case '"':
        {
            Token.Text = Tokenizer->At;
            while(Tokenizer->At[0] && Tokenizer->At[0] != '"')
                ++Tokenizer->At;

            Token.Type = Token_String;
            Token.TextLength = Tokenizer->At - Token.Text;

            if(Tokenizer->At[0] == '"')
                ++Tokenizer->At;
        } break;

        default:
        {
            if(IsAlpha(C))
            {
                while(IsAlpha(Tokenizer->At[0]) ||
                      IsNumeric(Tokenizer->At[0]) ||
                      (Tokenizer->At[0] == '_'))
                {
                    ++Tokenizer->At;
                }

                Token.Type = Token_Identifier;
                Token.TextLength = Tokenizer->At - Token.Text;
            }
            else
            {
                Token.Type = Token_Unknown;
            }
        } break;
    }

    return Token;
}
