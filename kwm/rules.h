#ifndef RULES_H
#define RULES_H

#include "types.h"

bool ParseIdentifier(tokenizer *Tokenizer, std::string *Member);
bool ParseProperties(tokenizer *Tokenizer, window_properties *Properties);
bool KwmParseRule(std::string RuleSym, window_rule *Rule);

void KwmAddRule(std::string RuleSym);
bool CheckWindowRule(window_rule *Rule, window_info *Window);
bool ApplyWindowRules(window_info *Window);

#endif
