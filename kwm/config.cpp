#include "config.h"
#include "tokenizer.h"
#include "interpreter.h"
#include "rules.h"
#include "helpers.h"
#include "command.h"

extern kwm_path KWMPath;

void ReportInvalidCommand(std::string Command)
{
    std::cerr << "  Parse error: " << Command << std::endl;
}

void KwmParseConfigOptionTiling(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "bsp"))
        KwmInterpretCommand("config tiling bsp", 0);
    else if(TokenEquals(Token, "monocle"))
        KwmInterpretCommand("config tiling monocle", 0);
    else if(TokenEquals(Token, "float"))
        KwmInterpretCommand("config tiling float", 0);
    else if(TokenEquals(Token, "off"))
        KwmInterpretCommand("config tiling off", 0);
    else
        ReportInvalidCommand("Unknown command 'config tiling " + std::string(Token.Text, Token.TextLength) + "'");
}

void KwmParseConfigOptionHotkeys(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "on"))
        KwmInterpretCommand("config hotkeys on", 0);
    else if(TokenEquals(Token, "off"))
        KwmInterpretCommand("config hotkeys off", 0);
    else
        ReportInvalidCommand("Unknown command 'config hotkeys " + std::string(Token.Text, Token.TextLength) + "'");
}

void KwmParseConfigOptionPadding(tokenizer *Tokenizer)
{
    bool IsValid = true;
    token TokenTop = GetToken(Tokenizer);
    token TokenBottom = GetToken(Tokenizer);
    token TokenLeft = GetToken(Tokenizer);
    token TokenRight = GetToken(Tokenizer);

    if(TokenTop.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown config padding top value '" + std::string(TokenTop.Text, TokenTop.TextLength) + "'");
        IsValid = false;
    }
    if(TokenBottom.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown config padding bottom value '" + std::string(TokenBottom.Text, TokenBottom.TextLength) + "'");
        IsValid = false;
    }
    if(TokenLeft.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown config padding left value '" + std::string(TokenLeft.Text, TokenLeft.TextLength) + "'");
        IsValid = false;
    }
    if(TokenRight.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown config padding right value '" + std::string(TokenRight.Text, TokenRight.TextLength) + "'");
        IsValid = false;
    }

    if(IsValid)
    {
        KwmInterpretCommand("config padding " + std::string(TokenTop.Text, TokenTop.TextLength) +
                            " " + std::string(TokenBottom.Text, TokenBottom.TextLength) +
                            " " + std::string(TokenLeft.Text, TokenLeft.TextLength) +
                            " " + std::string(TokenRight.Text, TokenRight.TextLength), 0);
    }
}

void KwmParseConfigOptionGap(tokenizer *Tokenizer)
{
    bool IsValid = true;
    token TokenVertical = GetToken(Tokenizer);
    token TokenHorizontal = GetToken(Tokenizer);

    if(TokenVertical.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown config gap vertical value '" + std::string(TokenVertical.Text, TokenVertical.TextLength) + "'");
        IsValid = false;
    }
    if(TokenHorizontal.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown config gap horizontal value '" + std::string(TokenHorizontal.Text, TokenHorizontal.TextLength) + "'");
        IsValid = false;
    }

    if(IsValid)
    {
        KwmInterpretCommand("config gap " + std::string(TokenVertical.Text, TokenVertical.TextLength) +
                            " " + std::string(TokenHorizontal.Text, TokenHorizontal.TextLength), 0);
    }
}

void KwmParseConfigOptionSpacesKey(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "key"))
        {
            token TokenKey = GetToken(Tokenizer);
            KwmInterpretCommand("config spaces-key " + std::string(TokenKey.Text, TokenKey.TextLength), 0);
        }
        else
        {
            ReportInvalidCommand("Expected token 'key' after 'config spaces-'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config spaces'");
    }
}

void KwmParseConfigOptionFocusFollowsMouse(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "follows"))
        {
            if(RequireToken(Tokenizer, Token_Dash))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "mouse"))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "autofocus"))
                        KwmInterpretCommand("config focus-follows-mouse autofocus", 0);
                    else if(TokenEquals(Token, "autoraise"))
                        KwmInterpretCommand("config focus-follows-mouse autoraise", 0);
                    else if(TokenEquals(Token, "off"))
                        KwmInterpretCommand("config focus-follows-mouse off", 0);
                    else
                        ReportInvalidCommand("Unknown command 'config focus-follows-mouse " + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                    ReportInvalidCommand("Unknown command 'config focus-follows-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Unknown command 'config focus-" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config focus'");
    }
}

void KwmParseConfigOptionMouseFollowsFocus(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "follows"))
        {
            if(RequireToken(Tokenizer, Token_Dash))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "focus"))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "on"))
                        KwmInterpretCommand("config mouse-follows-focus on", 0);
                    else if(TokenEquals(Token, "off"))
                        KwmInterpretCommand("config mouse-follows-focus off", 0);
                    else
                        ReportInvalidCommand("Unknown command 'config mouse-follows-focus " + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                    ReportInvalidCommand("Unknown command 'config mouse-follows-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Unknown command 'config mouse-" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config mouse'");
    }
}

void KwmParseConfigOptionStandbyOnFloat(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "on"))
        {
            if(RequireToken(Tokenizer, Token_Dash))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "float"))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "on"))
                        KwmInterpretCommand("config standby-on-float on", 0);
                    else if(TokenEquals(Token, "off"))
                        KwmInterpretCommand("config standby-on-float off", 0);
                    else
                        ReportInvalidCommand("Unknown command 'config standby-on-float " + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                    ReportInvalidCommand("Unknown command 'config standby-on-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Unknown command 'config standby-" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config standby'");
    }
}

void KwmParseConfigOptionFloatNonResizable(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "non"))
        {
            if(RequireToken(Tokenizer, Token_Dash))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "resizable"))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "on"))
                        KwmInterpretCommand("config float-non-resizable on", 0);
                    else if(TokenEquals(Token, "off"))
                        KwmInterpretCommand("config float-non-resizable off", 0);
                    else
                        ReportInvalidCommand("Unknown command 'config float-non-resizable " + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                    ReportInvalidCommand("Unknown command 'config float-non-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Unknown command 'config float-" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config float'");
    }
}

void KwmParseConfigOptionLockToContainer(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "to"))
        {
            if(RequireToken(Tokenizer, Token_Dash))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "container"))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "on"))
                        KwmInterpretCommand("config lock-to-container on", 0);
                    else if(TokenEquals(Token, "off"))
                        KwmInterpretCommand("config lock-to-container off", 0);
                    else
                        ReportInvalidCommand("Unknown command 'config lock-to-container " + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                    ReportInvalidCommand("Unknown command 'config lock-to-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Unknown command 'config lock-" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config lock'");
    }
}

void KwmParseConfigOptionCycleFocus(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "focus"))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "screen"))
                KwmInterpretCommand("config cycle-focus screen", 0);
            else if(TokenEquals(Token, "off"))
                KwmInterpretCommand("config cycle-focus off", 0);
            else
                ReportInvalidCommand("Unknown command 'config cycle-focus " + std::string(Token.Text, Token.TextLength) + "'");
        }
        else
            ReportInvalidCommand("Unknown command 'config cycle-" + std::string(Token.Text, Token.TextLength) + "'");
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config cycle'");
    }
}

void KwmParseConfigOptionSplitRatio(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "ratio"))
        {
            token Token = GetToken(Tokenizer);
            switch(Token.Type)
            {
                case Token_Digit:
                {
                    KwmInterpretCommand("config split-ratio " + std::string(Token.Text, Token.TextLength), 0);
                } break;
                default:
                {
                    ReportInvalidCommand("Unknown command 'config split-ratio " + std::string(Token.Text, Token.TextLength) + "'");
                } break;
            }
        }
        else
            ReportInvalidCommand("Unknown command 'config split-" + std::string(Token.Text, Token.TextLength) + "'");
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config cycle'");
    }
}

void KwmParseConfigOptionOptimalRatio(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "ratio"))
        {
            token Token = GetToken(Tokenizer);
            switch(Token.Type)
            {
                case Token_Digit:
                {
                    KwmInterpretCommand("config optimal-ratio " + std::string(Token.Text, Token.TextLength), 0);
                } break;
                default:
                {
                    ReportInvalidCommand("Unknown command 'config optimal-ratio " + std::string(Token.Text, Token.TextLength) + "'");
                } break;
            }
        }
        else
            ReportInvalidCommand("Unknown command 'config optimal-" + std::string(Token.Text, Token.TextLength) + "'");
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config optimal'");
    }
}

void KwmParseConfigOptionSpawn(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "left"))
        KwmInterpretCommand("config spawn left", 0);
    else if(TokenEquals(Token, "right"))
        KwmInterpretCommand("config spawn right", 0);
    else
        ReportInvalidCommand("Unknown command 'config spawn " + std::string(Token.Text, Token.TextLength) + "'");
}

void KwmParseConfigOptionBorder(tokenizer *Tokenizer)
{
    token TokenBorder = GetToken(Tokenizer);
    if((TokenEquals(TokenBorder, "focused")) ||
       (TokenEquals(TokenBorder, "marked")))
    {
        std::string BorderType(TokenBorder.Text, TokenBorder.TextLength);
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "on"))
            KwmInterpretCommand("config border " + BorderType + " on", 0);
        else if(TokenEquals(Token, "off"))
            KwmInterpretCommand("config border " + BorderType + " off", 0);
        else if(TokenEquals(Token, "size"))
        {
            token Token = GetToken(Tokenizer);
            switch(Token.Type)
            {
                case Token_Digit:
                {
                    std::string BorderSize(Token.Text, Token.TextLength);
                    KwmInterpretCommand("config border " + BorderType + " size " + BorderSize, 0);
                } break;
                default:
                {
                    std::string BorderSize(Token.Text, Token.TextLength);
                    ReportInvalidCommand("Unknown command 'config border " + BorderType + " size " + BorderSize + "'");
                } break;
            }
        }
        else if(TokenEquals(Token, "radius"))
        {
            token Token = GetToken(Tokenizer);
            switch(Token.Type)
            {
                case Token_Digit:
                {
                    std::string BorderSize(Token.Text, Token.TextLength);
                    KwmInterpretCommand("config border " + BorderType + " radius " + BorderSize, 0);
                } break;
                default:
                {
                    std::string BorderSize(Token.Text, Token.TextLength);
                    ReportInvalidCommand("Unknown command 'config border " + BorderType + " radius " + BorderSize + "'");
                } break;
            }
        }
        else if(TokenEquals(Token, "color"))
        {
            token Token = GetToken(Tokenizer);
            std::string BorderColor(Token.Text, Token.TextLength);
            KwmInterpretCommand("config border " + BorderType + " color " + BorderColor, 0);
        }
    }
    else
    {
        ReportInvalidCommand("Unknown command 'config border " + std::string(TokenBorder.Text, TokenBorder.TextLength) + "'");
    }
}

void KwmParseConfigOptionSpace(tokenizer *Tokenizer)
{
    token TokenDisplay = GetToken(Tokenizer);
    std::string Display(TokenDisplay.Text, TokenDisplay.TextLength);
    if(TokenDisplay.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown command 'config space " + Display + "'");
        return;
    }

    token TokenSpace = GetToken(Tokenizer);
    std::string Space(TokenSpace.Text, TokenSpace.TextLength);
    if(TokenSpace.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown command 'config space " + Display + " " + Space + "'");
        return;
    }

    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "mode"))
    {
        token Token = GetToken(Tokenizer);
        std::string Mode(Token.Text, Token.TextLength);
        if((TokenEquals(Token, "bsp")) || (TokenEquals(Token, "monocle")) || (TokenEquals(Token, "float")))
            KwmInterpretCommand("config space " + Display + " " + Space + " mode " + Mode, 0);
        else
            ReportInvalidCommand("Unknown command 'config space " + Display + " " + Space + " mode " + Mode + "'");
    }
    else if(TokenEquals(Token, "padding"))
    {
        bool IsValid = true;
        token TokenTop = GetToken(Tokenizer);
        token TokenBottom = GetToken(Tokenizer);
        token TokenLeft = GetToken(Tokenizer);
        token TokenRight = GetToken(Tokenizer);

        if(TokenTop.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding top value '" + std::string(TokenTop.Text, TokenTop.TextLength) + "'");
            IsValid = false;
        }
        if(TokenBottom.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding bottom value '" + std::string(TokenBottom.Text, TokenBottom.TextLength) + "'");
            IsValid = false;
        }
        if(TokenLeft.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding left value '" + std::string(TokenLeft.Text, TokenLeft.TextLength) + "'");
            IsValid = false;
        }
        if(TokenRight.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding right value '" + std::string(TokenRight.Text, TokenRight.TextLength) + "'");
            IsValid = false;
        }

        if(IsValid)
        {
            KwmInterpretCommand("config space " + Display + " " + Space +
                    " padding " + std::string(TokenTop.Text, TokenTop.TextLength) +
                    " " + std::string(TokenBottom.Text, TokenBottom.TextLength) +
                    " " + std::string(TokenLeft.Text, TokenLeft.TextLength) +
                    " " + std::string(TokenRight.Text, TokenRight.TextLength), 0);
        }
    }
    else if(TokenEquals(Token, "gap"))
    {
        bool IsValid = true;
        token TokenVertical = GetToken(Tokenizer);
        token TokenHorizontal = GetToken(Tokenizer);

        if(TokenVertical.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config gap vertical value '" + std::string(TokenVertical.Text, TokenVertical.TextLength) + "'");
            IsValid = false;
        }
        if(TokenHorizontal.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config gap horizontal value '" + std::string(TokenHorizontal.Text, TokenHorizontal.TextLength) + "'");
            IsValid = false;
        }

        if(IsValid)
        {
            KwmInterpretCommand("config space " + Display + " " + Space +
                    " gap " + std::string(TokenVertical.Text, TokenVertical.TextLength) +
                    " " + std::string(TokenHorizontal.Text, TokenHorizontal.TextLength), 0);
        }
    }
    else if(TokenEquals(Token, "name"))
    {
        token Token = GetToken(Tokenizer);
        KwmInterpretCommand("config space " + Display + " " + Space + " name " + std::string(Token.Text, Token.TextLength), 0);
    }
    else if(TokenEquals(Token, "tree"))
    {
        token Token = GetToken(Tokenizer);
        KwmInterpretCommand("config space " + Display + " " + Space + " tree " + std::string(Token.Text, Token.TextLength), 0);
    }
    else
    {
        ReportInvalidCommand("Unknown command 'config space " + Display + " " + Space + " " + std::string(Token.Text, Token.TextLength) + "'");
    }
}

void KwmParseConfigOptionDisplay(tokenizer *Tokenizer)
{
    token TokenDisplay = GetToken(Tokenizer);
    std::string Display(TokenDisplay.Text, TokenDisplay.TextLength);
    if(TokenDisplay.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown command 'config display " + Display + "'");
        return;
    }

    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "mode"))
    {
        token Token = GetToken(Tokenizer);
        std::string Mode(Token.Text, Token.TextLength);
        if((TokenEquals(Token, "bsp")) || (TokenEquals(Token, "monocle")) || (TokenEquals(Token, "float")))
            KwmInterpretCommand("config display " + Display + " mode " + Mode, 0);
        else
            ReportInvalidCommand("Unknown command 'config display " + Display + " mode " + Mode + "'");
    }
    else if(TokenEquals(Token, "padding"))
    {
        bool IsValid = true;
        token TokenTop = GetToken(Tokenizer);
        token TokenBottom = GetToken(Tokenizer);
        token TokenLeft = GetToken(Tokenizer);
        token TokenRight = GetToken(Tokenizer);

        if(TokenTop.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding top value '" + std::string(TokenTop.Text, TokenTop.TextLength) + "'");
            IsValid = false;
        }
        if(TokenBottom.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding bottom value '" + std::string(TokenBottom.Text, TokenBottom.TextLength) + "'");
            IsValid = false;
        }
        if(TokenLeft.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding left value '" + std::string(TokenLeft.Text, TokenLeft.TextLength) + "'");
            IsValid = false;
        }
        if(TokenRight.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding right value '" + std::string(TokenRight.Text, TokenRight.TextLength) + "'");
            IsValid = false;
        }

        if(IsValid)
        {
            KwmInterpretCommand("config display " + Display +
                    " padding " + std::string(TokenTop.Text, TokenTop.TextLength) +
                    " " + std::string(TokenBottom.Text, TokenBottom.TextLength) +
                    " " + std::string(TokenLeft.Text, TokenLeft.TextLength) +
                    " " + std::string(TokenRight.Text, TokenRight.TextLength), 0);
        }
    }
    else if(TokenEquals(Token, "gap"))
    {
        bool IsValid = true;
        token TokenVertical = GetToken(Tokenizer);
        token TokenHorizontal = GetToken(Tokenizer);

        if(TokenVertical.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config gap vertical value '" + std::string(TokenVertical.Text, TokenVertical.TextLength) + "'");
            IsValid = false;
        }
        if(TokenHorizontal.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config gap horizontal value '" + std::string(TokenHorizontal.Text, TokenHorizontal.TextLength) + "'");
            IsValid = false;
        }

        if(IsValid)
        {
            KwmInterpretCommand("config display " + Display +
                    " gap " + std::string(TokenVertical.Text, TokenVertical.TextLength) +
                    " " + std::string(TokenHorizontal.Text, TokenHorizontal.TextLength), 0);
        }
    }
    else
    {
        ReportInvalidCommand("Unknown command 'config display " + Display + " " + std::string(Token.Text, Token.TextLength) + "'");
    }
}

void KwmParseModeOptionActivate(tokenizer *Tokenizer)
{
    token TokenMode = GetToken(Tokenizer);
    std::string Mode(TokenMode.Text, TokenMode.TextLength);
    KwmInterpretCommand("mode activate " + Mode, 0);
}

void KwmParseModeOptionProperties(token *TokenMode, tokenizer *Tokenizer)
{
    std::string Mode(TokenMode->Text, TokenMode->TextLength);
    token Token = GetToken(Tokenizer);

    if(TokenEquals(Token, "prefix"))
    {
        token Token = GetToken(Tokenizer);
        std::string Status(Token.Text, Token.TextLength);
        if((TokenEquals(Token, "on")) || (TokenEquals(Token, "off")))
            KwmInterpretCommand("mode " + Mode + " prefix " + Status, 0);
        else
            ReportInvalidCommand("Unknown command 'mode " + Mode + " prefix " + Status + "'");
    }
    else if(TokenEquals(Token, "timeout"))
    {
        token Token = GetToken(Tokenizer);
        switch(Token.Type)
        {
            case Token_Digit:
            {
                std::string Timeout(Token.Text, Token.TextLength);
                KwmInterpretCommand("mode " + Mode + " timeout " + Timeout, 0);
            } break;
            default:
            {
                std::string Timeout(Token.Text, Token.TextLength);
                ReportInvalidCommand("Unknown command 'mode " + Mode + " timeout " + Timeout + "'");
            } break;
        }
    }
    else if(TokenEquals(Token, "color"))
    {
        token Token = GetToken(Tokenizer);
        std::string Color(Token.Text, Token.TextLength);
        KwmInterpretCommand("mode " + Mode + " color " + Color, 0);
    }
    else if(TokenEquals(Token, "restore"))
    {
        token Token = GetToken(Tokenizer);
        std::string Restore(Token.Text, Token.TextLength);
        KwmInterpretCommand("mode " + Mode + " restore " + Restore, 0);
    }
}

void KwmParseConfigOption(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    switch(Token.Type)
    {
        case Token_EndOfStream:
        {
            ReportInvalidCommand("Unexpected end of file!");
            return;
        } break;
        case Token_Identifier:
        {
            if(TokenEquals(Token, "tiling"))
                KwmParseConfigOptionTiling(Tokenizer);
            else if(TokenEquals(Token, "hotkeys"))
                KwmParseConfigOptionHotkeys(Tokenizer);
            else if(TokenEquals(Token, "padding"))
                KwmParseConfigOptionPadding(Tokenizer);
            else if(TokenEquals(Token, "gap"))
                KwmParseConfigOptionGap(Tokenizer);
            else if(TokenEquals(Token, "spaces"))
                KwmParseConfigOptionSpacesKey(Tokenizer);
            else if(TokenEquals(Token, "focus"))
                KwmParseConfigOptionFocusFollowsMouse(Tokenizer);
            else if(TokenEquals(Token, "mouse"))
                KwmParseConfigOptionMouseFollowsFocus(Tokenizer);
            else if(TokenEquals(Token, "standby"))
                KwmParseConfigOptionStandbyOnFloat(Tokenizer);
            else if(TokenEquals(Token, "float"))
                KwmParseConfigOptionFloatNonResizable(Tokenizer);
            else if(TokenEquals(Token, "lock"))
                KwmParseConfigOptionLockToContainer(Tokenizer);
            else if(TokenEquals(Token, "cycle"))
                KwmParseConfigOptionCycleFocus(Tokenizer);
            else if(TokenEquals(Token, "split"))
                KwmParseConfigOptionSplitRatio(Tokenizer);
            else if(TokenEquals(Token, "optimal"))
                KwmParseConfigOptionOptimalRatio(Tokenizer);
            else if(TokenEquals(Token, "spawn"))
                KwmParseConfigOptionSpawn(Tokenizer);
            else if(TokenEquals(Token, "border"))
                KwmParseConfigOptionBorder(Tokenizer);
            else if(TokenEquals(Token, "space"))
                KwmParseConfigOptionSpace(Tokenizer);
            else if(TokenEquals(Token, "display"))
                KwmParseConfigOptionSpace(Tokenizer);
            else
                ReportInvalidCommand("Unknown command 'config " + std::string(Token.Text, Token.TextLength) + "'");
        } break;
        default:
        {
            ReportInvalidCommand("Unknown token '" + std::string(Token.Text, Token.TextLength) + "'");
            //printf("%d: %.*s\n", Token.Type, Token.TextLength, Token.Text);
        } break;
    }
}

void KwmParseModeOption(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    switch(Token.Type)
    {
        case Token_EndOfStream:
        {
            ReportInvalidCommand("Unexpected end of file!");
            return;
        } break;
        case Token_Identifier:
        {
            if(TokenEquals(Token, "activate"))
                KwmParseModeOptionActivate(Tokenizer);
            else
                KwmParseModeOptionProperties(&Token, Tokenizer);
        } break;
        default:
        {
            ReportInvalidCommand("Unknown token '" + std::string(Token.Text, Token.TextLength) + "'");
            //printf("%d: %.*s\n", Token.Type, Token.TextLength, Token.Text);
        } break;
    }
}

void KwmParseKwmc(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    switch(Token.Type)
    {
        case Token_EndOfStream:
        {
            return;
        } break;
        case Token_Identifier:
        {
            if(TokenEquals(Token, "config"))
                KwmParseConfigOption(Tokenizer);
            else if(TokenEquals(Token, "mode"))
                KwmParseModeOption(Tokenizer);
            else if(TokenEquals(Token, "bindsym") ||
                    TokenEquals(Token, "bindcode") ||
                    TokenEquals(Token, "bindsym-passthrough") ||
                    TokenEquals(Token, "bindcode-passthrough") ||
                    TokenEquals(Token, "rule"))
                KwmInterpretCommand(std::string(Token.Text, Token.TextLength) + " " + GetTextTilEndOfLine(Tokenizer), 0);
            else
                ReportInvalidCommand("Unknown token '" + std::string(Token.Text, Token.TextLength) + "'");
        } break;
        default:
        {
            ReportInvalidCommand("Unknown token '" + std::string(Token.Text, Token.TextLength) + "'");
            // printf("%d: %.*s\n", Token.Type, Token.TextLength, Token.Text);
        } break;
    }
}

void KwmParseInclude(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    KwmParseConfig(std::string(Token.Text, Token.TextLength));
}

void KwmParseDefine(tokenizer *Tokenizer, std::map<std::string, std::string> &Defines)
{
    token Token = GetToken(Tokenizer);
    std::string Variable(Token.Text, Token.TextLength);
    std::string Value = GetTextTilEndOfLine(Tokenizer);
    Defines[Variable] = Value;
}

void KwmExpandVariables(std::map<std::string, std::string> &Defines, std::string &Text)
{
    std::map<std::string, std::string>::iterator It;
    for(It = Defines.begin(); It != Defines.end(); ++It)
    {
        std::size_t Pos = Text.find(It->first);
        while(Pos != std::string::npos)
        {
            Text.replace(Pos, It->first.size(), It->second);
            Pos = Text.find(It->first, Pos + It->second.size() + 1);
        }
    }
}

void KwmPreprocessConfig(std::string &Text)
{
    std::map<std::string, std::string> Defines;
    tokenizer Tokenizer = {};
    Tokenizer.At = const_cast<char*>(Text.c_str());

    bool Parsing = true;
    while(Parsing)
    {
        token Token = GetToken(&Tokenizer);
        switch(Token.Type)
        {
            case Token_EndOfStream:
            {
                Parsing = false;
            } break;
            case Token_Comment:
            {
            } break;
            case Token_Identifier:
            {
                if(TokenEquals(Token, "define"))
                    KwmParseDefine(&Tokenizer, Defines);
            } break;
            default:
            {
            } break;
        }
    }

    KwmExpandVariables(Defines, Text);
}

void KwmParseConfig(std::string File)
{
    tokenizer Tokenizer = {};
    std::string AbsoluteFilePath = KWMPath.EnvHome + "/" + KWMPath.ConfigFolder + "/" + File;
    char *FileContents = ReadFile(AbsoluteFilePath);

    if(FileContents)
    {
        std::string FileContentsString(FileContents);

        KwmPreprocessConfig(FileContentsString);
        Tokenizer.At = const_cast<char*>(FileContentsString.c_str());

        bool Parsing = true;
        while(Parsing)
        {
            token Token = GetToken(&Tokenizer);
            switch(Token.Type)
            {
                case Token_EndOfStream:
                    {
                        Parsing = false;
                    } break;
                case Token_Comment:
                    {
                        // printf("%d Comment: %.*s\n", Token.Type, Token.TextLength, Token.Text);
                    } break;
                case Token_Identifier:
                    {
                        if(TokenEquals(Token, "kwmc"))
                        {
                            KwmParseKwmc(&Tokenizer);
                        }
                        else if(TokenEquals(Token, "exec"))
                        {
                            KwmExecuteThreadedSystemCommand(GetTextTilEndOfLine(&Tokenizer));
                        }
                        else if(TokenEquals(Token, "include"))
                        {
                            KwmParseInclude(&Tokenizer);
                        }
                        else if(TokenEquals(Token, "define"))
                        {
                            GetToken(&Tokenizer);
                            GetTextTilEndOfLine(&Tokenizer);
                        }
                        else
                        {
                            ReportInvalidCommand("Unknown token '" + std::string(Token.Text, Token.TextLength) + "'");
                            // printf("Unknown identifier - %d: %.*s\n", Token.Type, Token.TextLength, Token.Text);
                        }
                    } break;
                default:
                    {
                        ReportInvalidCommand("Unknown token '" + std::string(Token.Text, Token.TextLength) + "'");
                        // printf("%d: %.*s\n", Token.Type, Token.TextLength, Token.Text);
                    } break;
            }
        }

        free(FileContents);
    }
}
