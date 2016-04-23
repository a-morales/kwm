#include "rules.h"
#include "tokenizer.h"
#include "display.h"
#include "space.h"
#include "window.h"
#include "tree.h"
#include "helpers.h"

extern int GetNumberOfSpacesOfDisplay(screen_info *Screen);
extern void AddWindowToSpace(int CGSpaceID, int WindowID);
extern void RemoveWindowFromSpace(int CGSpaceID, int WindowID);
extern bool IsWindowOnSpace(int WindowID, int CGSpaceID);
extern int GetCGSpaceIDFromSpaceNumber(screen_info *Screen, int SpaceID);

extern kwm_screen KWMScreen;
extern kwm_tiling KWMTiling;

/* Current Window Properties:
 *          float = "true" | "false"
 *          display = "id"
 *          space = "id"
 *
 * Examples:
 *          kwmc rule owner="" name="" properties={float=""; display=""; space=""} except=""
 *
 * Apply given properties to all iTerm2 windows that does not have
 * a title containing $except:
 *          kwmc rule owner="iTerm2" properties={float="true"} except="borderless"
 *
 * Apply given properties to all iTerm2 windows that does have a
 * title containing $name and at the same times does not contain $except:
 *          kwmc rule owner="iTerm2" name="borderless" properties={float="true"} except="ignore"
 *
 * Apply given properties to all Steam windows
 *          kwmc rule owner="Steam" properties={float="true"}
 *
 * Apply given properties to all iTunes windows
 *          kwmc rule owner="iTunes" properties={display="1"}
 *
 * Assign iTunes to space 1 of display 1
 *          kwmc rule owner="iTunes" properties={space="1"; display="1"}
*/

bool ParseIdentifier(tokenizer *Tokenizer, std::string *Member)
{
    if(RequireToken(Tokenizer, Token_Equals))
    {
        token Token = GetToken(Tokenizer);
        switch(Token.Type)
        {
            case Token_String:
            {
                std::string String;
                for(int Index = 0; Index < Token.TextLength; ++Index)
                    String += Token.Text[Index];
                *Member = String;
                return true;
            } break;
            default:
            {
                DEBUG("Expected token of type Token_String");
            } break;
        }
    }
    else
    {
        DEBUG("Expected token '='\n");
    }

    return false;
}

bool ParseProperties(tokenizer *Tokenizer, window_properties *Properties)
{
    if(RequireToken(Tokenizer, Token_Equals))
    {
        if(RequireToken(Tokenizer, Token_OpenBrace))
        {
            Properties->Display = -1;
            Properties->Space = -1;
            Properties->Float = -1;
            bool ValidState = true;

            while(ValidState)
            {
                token Token = GetToken(Tokenizer);
                switch(Token.Type)
                {
                    case Token_SemiColon: { continue; } break;
                    case Token_CloseBrace: { ValidState = false; } break;
                    case Token_Identifier:
                    {
                        if(TokenEquals(Token, "float"))
                        {
                            std::string Value;
                            if(ParseIdentifier(Tokenizer, &Value))
                            {
                                if(Value == "true")
                                    Properties->Float = 1;
                                else if(Value == "false")
                                    Properties->Float = 0;
                            }
                        }
                        else if(TokenEquals(Token, "display"))
                        {
                            std::string Value;
                            if(ParseIdentifier(Tokenizer, &Value))
                                Properties->Display = ConvertStringToInt(Value);
                        }
                        else if(TokenEquals(Token, "space"))
                        {
                            std::string Value;
                            if(ParseIdentifier(Tokenizer, &Value))
                                Properties->Space = ConvertStringToInt(Value);
                        }
                    } break;
                    default: { DEBUG("Expected token of type Token_Identifier"); } break;
                }
            }

            return true;
        }
        else
        {
            DEBUG("Expected token '{'");
        }
    }
    else
    {
        DEBUG("Expected token '='");
    }

    return false;
}

bool KwmParseRule(std::string RuleSym, window_rule *Rule)
{
    tokenizer Tokenizer = {};
    Tokenizer.At = const_cast<char*>(RuleSym.c_str());

    bool Result = true;
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
            case Token_Unknown:
            {
            } break;
            case Token_Identifier:
            {
                if(TokenEquals(Token, "owner"))
                    Result = Result && ParseIdentifier(&Tokenizer, &Rule->Owner);
                else if(TokenEquals(Token, "name"))
                    Result = Result && ParseIdentifier(&Tokenizer, &Rule->Name);
                else if(TokenEquals(Token, "properties"))
                    Result = Result && ParseProperties(&Tokenizer, &Rule->Properties);
                else if(TokenEquals(Token, "except"))
                    Result = Result && ParseIdentifier(&Tokenizer, &Rule->Except);
            } break;
            default: { } break;
        }
    }

    return Result;
}

void KwmAddRule(std::string RuleSym)
{
    window_rule Rule = {};
    if(KwmParseRule(RuleSym, &Rule))
        KWMTiling.WindowRules.push_back(Rule);
}

bool MatchWindowRule(window_rule *Rule, window_info *Window)
{
    bool Match = true;
    if(!Rule->Owner.empty())
        Match = Rule->Owner == Window->Owner;

    if(!Rule->Name.empty())
        Match = Match && Window->Name.find(Rule->Name) != std::string::npos;

    if(!Rule->Except.empty())
        Match = Match && Window->Name.find(Rule->Except) == std::string::npos;

    return Match;
}

void CheckWindowRules(window_info *Window)
{
    if(HasRuleBeenApplied(Window))
        return;

    Window->Display = -1;
    Window->Space = -1;
    Window->Float = 0;

    for(int Index = 0; Index < KWMTiling.WindowRules.size(); ++Index)
    {
        window_rule *Rule = &KWMTiling.WindowRules[Index];
        if(MatchWindowRule(Rule, Window))
        {
            if(Rule->Properties.Float != -1)
                Window->Float = Rule->Properties.Float == 1;

            if(Rule->Properties.Display != -1)
                Window->Display = Rule->Properties.Display;

            if(Rule->Properties.Space != -1)
                Window->Space = Rule->Properties.Space;
        }
    }
}

bool EnforceWindowRules(window_info *Window)
{
    bool Result  = false;

    if(Window->Float)
    {
        screen_info *ScreenOfWindow = GetDisplayOfWindow(Window);
        if(ScreenOfWindow)
        {
            space_info *SpaceOfWindow = GetActiveSpaceOfScreen(ScreenOfWindow);
            if(SpaceOfWindow->Settings.Mode == SpaceModeBSP)
                RemoveWindowFromBSPTree(ScreenOfWindow, Window->WID, true, false);
            else if(SpaceOfWindow->Settings.Mode == SpaceModeMonocle)
                RemoveWindowFromMonocleTree(ScreenOfWindow, Window->WID, true, false);
        }

        KWMTiling.FloatingWindowLst.push_back(Window->WID);
    }

    if(Window->Display != -1)
    {
        screen_info *Screen = GetDisplayFromScreenID(Window->Display);
        if(Screen && Screen != GetDisplayOfWindow(Window))
        {
            MoveWindowToDisplay(Window, Window->Display, false);
            Result = true;
        }
    }

    if(Window->Space != -1)
    {
        int Display = Window->Display == -1 ? 0 : Window->Display;
        screen_info *SourceScreen = GetDisplayOfWindow(Window);
        screen_info *DestinationScreen = GetDisplayFromScreenID(Display);
        int TotalSpaces = GetNumberOfSpacesOfDisplay(DestinationScreen);
        if(Window->Space <= TotalSpaces && Window->Space >= 1)
        {
            int SourceCGSpaceID = SourceScreen->ActiveSpace;
            int DestinationCGSpaceID = GetCGSpaceIDFromSpaceNumber(DestinationScreen, Window->Space);
            if(!IsWindowOnSpace(Window->WID, DestinationCGSpaceID))
            {
                AddWindowToSpace(DestinationCGSpaceID, Window->WID);
                RemoveWindowFromSpace(SourceCGSpaceID, Window->WID);
            }
        }
    }

    KWMTiling.EnforcedWindows[Window->WID] = true;
    return Result;
}

bool HasRuleBeenApplied(window_info *Window)
{
    std::map<int, bool>::iterator It = KWMTiling.EnforcedWindows.find(Window->WID);
    return It != KWMTiling.EnforcedWindows.end();
}
