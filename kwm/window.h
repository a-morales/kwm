#ifndef WINDOW_H
#define WINDOW_H

#include "types.h"

extern int GetSpaceNumberFromCGSpaceID(screen_info *Screen, int CGSpaceID);
extern int GetActiveSpaceOfDisplay(screen_info *Screen);

bool IsFocusedWindowFloating();
bool IsWindowFloating(int WindowID, int *Index);
bool IsAnyWindowBelowCursor();
bool IsWindowBelowCursor(window_info *Window);
bool IsWindowOnActiveSpace(int WindowID);
bool WindowsAreEqual(window_info *Window, window_info *Match);

void ClearFocusedWindow();
bool GetWindowFocusedByOSX(AXUIElementRef *WindowRef);
bool FocusWindowOfOSX();
void FocusWindowBelowCursor();

void UpdateWindowTree();
std::vector<window_info> FilterWindowListAllDisplays();
bool FilterWindowList(screen_info *Screen);
void UpdateActiveWindowList(screen_info *Screen);
void CreateWindowNodeTree(screen_info *Screen, std::vector<window_info*> *Windows);
void ShouldWindowNodeTreeUpdate(screen_info *Screen);
void AddWindowToTreeOfUnfocusedMonitor(screen_info *Screen, window_info *Window, bool UpdateFocus);

std::vector<int> GetAllWindowIDsInTree(space_info *Space);
std::vector<window_info*> GetAllWindowsNotInTree(std::vector<int> &WindowIDsInTree);
std::vector<int> GetAllWindowIDsToRemoveFromTree(std::vector<int> &WindowIDsInTree);

void ShouldBSPTreeUpdate(screen_info *Screen, space_info *Space);
void AddWindowToBSPTree(screen_info *Screen, int WindowID);
void AddWindowToBSPTree();
void RemoveWindowFromBSPTree(screen_info *Screen, int WindowID, bool Center, bool UpdateFocus);
void RemoveWindowFromBSPTree();

void ShouldMonocleTreeUpdate(screen_info *Screen, space_info *Space);
void AddWindowToMonocleTree(screen_info *Screen, int WindowID);
void RemoveWindowFromMonocleTree(screen_info *Screen, int WindowID, bool Center, bool UpdateFocus);

void ToggleWindowFloating(int WindowID, bool Center);
void ToggleFocusedWindowFloating();
void ToggleFocusedWindowParentContainer();
void ToggleFocusedWindowFullscreen();
bool IsWindowFullscreen(window_info *Window);
bool IsWindowParentContainer(window_info *Window);
void LockWindowToContainerSize(window_info *Window);

void DetachAndReinsertWindow(int WindowID, int Degrees);
void SwapFocusedWindowWithMarked();
void SwapFocusedWindowDirected(int Degrees);
void SwapFocusedWindowWithNearest(int Shift);
void MoveCursorToCenterOfWindow(window_info *Window);
void MoveCursorToCenterOfFocusedWindow();
void FocusWindowByID(int WindowID);
void ShiftWindowFocus(int Shift);
void ShiftSubTreeWindowFocus(int Shift);
void ShiftWindowFocusDirected(int Degrees);
bool FindClosestWindow(int Degrees, window_info *Target, bool Wrap);
double GetWindowDistance(window_info *A, window_info *B);
void GetCenterOfWindow(window_info *Window, int *X, int *Y);
bool WindowIsInDirection(window_info *A, window_info *B, int Degrees, bool Wrap);

void ClearMarkedWindow();
void MarkWindowContainer(window_info *Window);
void MarkFocusedWindowContainer();

void SetWindowRefFocus(AXUIElementRef WindowRef);
void SetKwmFocus(AXUIElementRef WindowRef);
void SetWindowFocus(window_info *Window);
void SetWindowFocusByNode(tree_node *Node);
void SetWindowFocusByNode(link_node *Link);

bool IsWindowTilable(window_info *Window);
bool IsWindowTilable(AXUIElementRef WindowRef);
bool IsWindowResizable(AXUIElementRef WindowRef);
bool IsWindowMovable(AXUIElementRef WindowRef);
void CenterWindowInsideNodeContainer(AXUIElementRef WindowRef, int *Xptr, int *Yptr, int *Wptr, int *Hptr);

void SetWindowDimensions(AXUIElementRef WindowRef, window_info *Window, int X, int Y, int Width, int Height);
void CenterWindow(screen_info *Screen, window_info *Window);
void MoveFloatingWindow(int X, int Y);

int GetWindowIDFromRef(AXUIElementRef WindowRef);
window_info GetWindowByRef(AXUIElementRef WindowRef);
window_info *GetWindowByID(int WindowID);
std::string GetWindowTitle(AXUIElementRef WindowRef);
CGSize GetWindowSize(AXUIElementRef WindowRef);
CGPoint GetWindowPos(window_info *Window);
CGPoint GetWindowPos(AXUIElementRef WindowRef);
void GetWindowInfo(const void *Key, const void *Value, void *Context);
bool GetWindowRole(window_info *Window, CFTypeRef *Role, CFTypeRef *SubRole);
bool GetWindowRef(window_info *Window, AXUIElementRef *WindowRef);

#endif
