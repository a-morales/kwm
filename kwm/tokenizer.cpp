#include "tokenizer.h"

void EatAllWhiteSpace(tokenizer *Tokenizer)
{
    while(Tokenizer->At[0])
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

std::string GetTextTilEndOfLine(tokenizer *Tokenizer)
{
    EatAllWhiteSpace(Tokenizer);

    token Token = {};
    Token.TextLength = 1;
    Token.Text = Tokenizer->At;

    while(Tokenizer->At[0] && !IsEndOfLine(Tokenizer->At[0]))
        ++Tokenizer->At;

    Token.Type = Token_String;
    Token.TextLength = Tokenizer->At - Token.Text;

    return std::string(Token.Text, Token.TextLength);
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
        case '/':
        {
            if(Tokenizer->At[0] == '*')
            {
                ++Tokenizer->At;
                Token.Text = Tokenizer->At;

                while(Tokenizer->At[0] &&
                      Tokenizer->At[1] &&
                      !((Tokenizer->At[0] == '*') &&
                        (Tokenizer->At[1] == '/')))
                    ++Tokenizer->At;

                Token.Type = Token_Comment;
                Token.TextLength = Tokenizer->At - Token.Text;

                if(Tokenizer->At[0] == '*')
                    ++Tokenizer->At;
                if(Tokenizer->At[0] == '/')
                    ++Tokenizer->At;
            }
            else if(Tokenizer->At[0] == '/')
            {
                ++Tokenizer->At;
                Token.Text = Tokenizer->At;

                while(Tokenizer->At[0] && !IsEndOfLine(Tokenizer->At[0]))
                    ++Tokenizer->At;

                Token.Type = Token_Comment;
                Token.TextLength = Tokenizer->At - Token.Text;
            }
            else
            {
                Token.Type = Token_Unknown;
            }
        } break;
        default:
        {
            if(IsAlpha(C))
            {
                while(IsAlpha(Tokenizer->At[0]) ||
                      IsNumeric(Tokenizer->At[0]) ||
                      (Tokenizer->At[0] == '+') ||
                      (Tokenizer->At[0] == '_'))
                    ++Tokenizer->At;

                Token.Type = Token_Identifier;
                Token.TextLength = Tokenizer->At - Token.Text;
            }
            else if(IsNumeric(C))
            {
                if(C == '0' && (Tokenizer->At[0] == 'x' || Tokenizer->At[0] == 'X'))
                {
                    ++Tokenizer->At;
                    while(IsHexadecimal(Tokenizer->At[0]))
                        ++Tokenizer->At;

                    Token.Type = Token_Hex;
                    Token.TextLength = Tokenizer->At - Token.Text;
                }
                else
                {
                    while(IsNumeric(Tokenizer->At[0]) ||
                          IsDot(Tokenizer->At[0]))
                        ++Tokenizer->At;

                    Token.Type = Token_Digit;
                    Token.TextLength = Tokenizer->At - Token.Text;
                }
            }
            else
            {
                Token.Type = Token_Unknown;
            }
        } break;
    }

    return Token;
}
