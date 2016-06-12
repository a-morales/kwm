#ifndef RULES_H
#define RULES_H

#include "types.h"
#include "axlib/axlib.h"

void ApplyWindowRules(ax_window *Window);

void KwmAddRule(std::string RuleSym);
void CheckWindowRules(window_info *Window);
bool EnforceWindowRules(window_info *Window);

#endif
