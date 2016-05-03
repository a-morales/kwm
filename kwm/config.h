#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

void ReportInvalidCommand(std::string Command);
void ReportUnexpectedToken(token Token);

void KwmParseConfigOptionTiling(tokenizer *Tokenizer);
void KwmParseConfigOptionHotkeys(tokenizer *Tokenizer);
void KwmParseConfigOptionPadding(tokenizer *Tokenizer);
void KwmParseConfigOptionGap(tokenizer *Tokenizer);
void KwmParseConfigOptionSpacesKey(tokenizer *Tokenizer);
void KwmParseConfigOptionFocusFollowsMouse(tokenizer *Tokenizer);
void KwmParseConfigOptionMouseFollowsFocus(tokenizer *Tokenizer);
void KwmParseConfigOptionStandbyOnFloat(tokenizer *Tokenizer);
void KwmParseConfigOptionFloatNonResizable(tokenizer *Tokenizer);
void KwmParseConfigOptionLockToContainer(tokenizer *Tokenizer);
void KwmParseConfigOptionCycleFocus(tokenizer *Tokenizer);
void KwmParseConfigOptionSplitRatio(tokenizer *Tokenizer);
void KwmParseConfigOptionOptimalRatio(tokenizer *Tokenizer);
void KwmParseConfigOptionSpawn(tokenizer *Tokenizer);
void KwmParseConfigOptionBorder(tokenizer *Tokenizer);
void KwmParseConfigOptionSpace(tokenizer *Tokenizer);
void KwmParseConfigOptionDisplay(tokenizer *Tokenizer);

void KwmParseModeOptionActivate(tokenizer *Tokenizer);
void KwmParseModeOptionProperties(token *TokenMode, tokenizer *Tokenizer);

void KwmParseConfigOption(tokenizer *Tokenizer);
void KwmParseModeOption(tokenizer *Tokenizer);

void KwmParseKwmc(tokenizer *Tokenizer);
void KwmParseInclude(tokenizer *Tokenizer);
void KwmParseDefine(tokenizer *Tokenizer, std::map<std::string, std::string> &Defines);
void KwmExpandVariables(std::map<std::string, std::string> &Defines, std::string &Text);
void KwmPreprocessConfig(std::string &Text);
void KwmParseConfig(std::string File);

#endif
