#ifndef RULES_H
#define RULES_H

#include "types.h"

void KwmAddRule(std::string RuleSym);
void CheckWindowRules(window_info *Window);
bool EnforceWindowRules(window_info *Window);

#endif
