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
int GetFocusedWindowID();
bool FocusWindowOfOSX();
void FocusWindowBelowCursor();
void FocusFirstLeafNode();
void FocusLastLeafNode();

void UpdateWindowTree();
std::vector<window_info> FilterWindowListAllDisplays();
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

void ToggleWindowFloating(int WindowID, bool Center);
void ToggleFocusedWindowFloating();
void ToggleFocusedWindowParentContainer();
void ToggleFocusedWindowFullscreen();

void DetachAndReinsertWindow(int WindowID, int Degrees);
void SwapFocusedWindowWithMarked();
void SwapFocusedWindowDirected(int Degrees);
void SwapFocusedWindowWithNearest(int Shift);
void MoveCursorToCenterOfWindow(window_info *Window);
void MoveCursorToCenterOfFocusedWindow();
void FocusWindowByID(int WindowID);
void ShiftWindowFocus(int Shift);
void ShiftWindowFocusDirected(int Degrees);
bool FindClosestWindow(int Degrees, window_info *Target, bool Wrap);
double GetWindowDistance(window_info *A, window_info *B);
void GetCenterOfWindow(window_info *Window, int *X, int *Y);
bool WindowIsInDirection(window_info *A, window_info *B, int Degrees, bool Wrap);

void ClearMarkedWindow();
void MarkWindowContainer(window_info *Window);
void MarkFocusedWindowContainer();

void SetWindowRefFocus(AXUIElementRef WindowRef, window_info *Window);
void SetWindowFocus(window_info *Window);
void SetWindowFocusByNode(tree_node *Node);

bool IsWindowNonResizable(AXUIElementRef WindowRef, window_info *Window, CFTypeRef NewWindowPos, CFTypeRef NewWindowSize);
void CenterWindowInsideNodeContainer(AXUIElementRef WindowRef, int *Xptr, int *Yptr, int *Wptr, int *Hptr);

void SetWindowDimensions(AXUIElementRef WindowRef, window_info *Window, int X, int Y, int Width, int Height);
void CenterWindow(screen_info *Screen, window_info *Window);
void MoveFloatingWindow(int X, int Y);
void ModifyContainerSplitRatio(double Offset);
void ResizeWindowToContainerSize(tree_node *Node);
void ResizeWindowToContainerSize(window_info *Window);
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
