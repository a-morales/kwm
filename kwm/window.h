#ifndef WINDOW_H
#define WINDOW_H

#include "types.h"

void AllowRoleForApplication(std::string Application, std::string Role);
bool IsAppSpecificWindowRole(window_info *Window, CFTypeRef Role, CFTypeRef SubRole);
bool IsApplicationCapturedByScreen(window_info *Window);
void CaptureApplication(window_info *Window);

bool IsApplicationFloating(window_info *Window);
bool IsFocusedWindowFloating();
bool IsWindowFloating(int WindowID, int *Index);
bool IsAnyWindowBelowCursor();
bool IsWindowBelowCursor(window_info *Window);
bool IsWindowOnActiveSpace(int WindowID);
bool WindowsAreEqual(window_info *Window, window_info *Match);

void ClearFocusedWindow();
bool ShouldWindowGainFocus(window_info *Window);
bool GetWindowFocusedByOSX(int *WindowWID);
bool FocusWindowOfOSX();
void FocusWindowBelowCursor();
void FocusFirstLeafNode();
void FocusLastLeafNode();

void UpdateWindowTree();
bool FilterWindowList(screen_info *Screen);
void UpdateActiveWindowList(screen_info *Screen);
void CreateWindowNodeTree(screen_info *Screen, std::vector<window_info*> *Windows);
void ShouldWindowNodeTreeUpdate(screen_info *Screen);
void AddWindowToTreeOfUnfocusedMonitor(screen_info *Screen, window_info *Window);

void ShouldBSPTreeUpdate(screen_info *Screen, space_info *Space);
void AddWindowToBSPTree(screen_info *Screen, int WindowID);
void AddWindowToBSPTree();
void RemoveWindowFromBSPTree(screen_info *Screen, int WindowID, bool Center, bool Refresh);
void RemoveWindowFromBSPTree();

void ShouldMonocleTreeUpdate(screen_info *Screen, space_info *Space);
void AddWindowToMonocleTree(screen_info *Screen, int WindowID);
void RemoveWindowFromMonocleTree(screen_info *Screen, int WindowID, bool Center);

void ToggleWindowFloating(int WindowID);
void ToggleFocusedWindowFloating();
void ToggleFocusedWindowParentContainer();
void ToggleFocusedWindowFullscreen();

void SwapFocusedWindowWithMarked();
void SwapFocusedWindowWithNearest(int Shift);
void MoveCursorToCenterOfWindow(window_info *Window);
void MoveCursorToCenterOfFocusedWindow();
void ShiftWindowFocus(int Shift);

void ClearMarkedWindow();
void MarkWindowContainer();

void SetWindowRefFocus(AXUIElementRef WindowRef, window_info *Window);
void SetWindowFocus(window_info *Window);
void SetWindowFocusByNode(tree_node *Node);

void SetWindowDimensions(AXUIElementRef WindowRef, window_info *Window, int X, int Y, int Width, int Height);
void CenterWindow(screen_info *Screen, window_info *Window);
void ModifyContainerSplitRatio(double Offset);
void ResizeWindowToContainerSize(tree_node *Node);
void ResizeWindowToContainerSize();

CGPoint GetCursorPos();
window_info *GetWindowByID(int WindowID);
std::string GetUTF8String(CFStringRef Temp);
std::string GetWindowTitle(AXUIElementRef WindowRef);
CGSize GetWindowSize(AXUIElementRef WindowRef);
CGPoint GetWindowPos(AXUIElementRef WindowRef);
void GetWindowInfo(const void *Key, const void *Value, void *Context);
bool GetWindowRole(window_info *Window, CFTypeRef *Role, CFTypeRef *SubRole);
bool GetWindowRef(window_info *Window, AXUIElementRef *WindowRef);
bool GetWindowRefFromCache(window_info *Window, AXUIElementRef *WindowRef);
bool IsApplicationInCache(int PID, std::vector<AXUIElementRef> *Elements);
void FreeWindowRefCache(int PID);

#endif
