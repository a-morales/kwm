#ifndef AXLIB_ELEMENT_H
#define AXLIB_ELEMENT_H

#include <Carbon/Carbon.h>
#include <string>

extern "C" AXError _AXUIElementGetWindow(AXUIElementRef, int *);

bool AXLibIsWindowResizable(AXUIElementRef WindowRef);
bool AXLibIsWindowMovable(AXUIElementRef WindowRef);

bool AXLibSetWindowPosition(AXUIElementRef WindowRef, int X, int Y);
bool AXLibSetWindowSize(AXUIElementRef WindowRef, int Width, int Height);
int AXLibGetWindowID(AXUIElementRef WindowRef);

CFTypeRef AXLibGetWindowProperty(AXUIElementRef WindowRef, CFStringRef Property);
AXError AXLibSetWindowProperty(AXUIElementRef WindowRef, CFStringRef Property, CFTypeRef Value);

std::string AXLibGetWindowTitle(AXUIElementRef WindowRef);
CGPoint AXLibGetWindowPosition(AXUIElementRef WindowRef);
CGSize AXLibGetWindowSize(AXUIElementRef WindowRef);

bool AXLibGetWindowRole(AXUIElementRef WindowRef, CFTypeRef *Role);
bool AXLibGetWindowSubrole(AXUIElementRef WindowRef, CFTypeRef *Subrole);
// void AXLibParseWindowInfo(const void *Key, const void *Value, void *Context);
std::string GetUTF8String(CFStringRef Temp);

#endif
