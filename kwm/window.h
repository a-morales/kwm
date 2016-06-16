#ifndef WINDOW_H
#define WINDOW_H

#include "types.h"
#include "axlib/display.h"
#include "axlib/window.h"

void CreateWindowNodeTree(ax_display *Display);
void LoadWindowNodeTree(ax_display *Display, std::string Layout);
void ResetWindowNodeTree(ax_display *Display, space_tiling_option Mode);
void AddWindowToNodeTree(ax_display *Display, uint32_t WindowID);
void RemoveWindowFromNodeTree(ax_display *Display, uint32_t WindowID);
void RebalanceNodeTree(ax_display *Display);

ax_window *GetWindowByID(uint32_t WindowID);
void GetCenterOfWindow(ax_window *Window, int *X, int *Y);
bool WindowIsInDirection(ax_window *WindowA, ax_window *WindowB, int Degrees);
bool FindClosestWindow(int Degrees, ax_window **ClosestWindow, bool Wrap);

/* TODO(koekeishiya): PARTIALLY LOOKED AT */
void FocusWindowByID(uint32_t WindowID);
void ToggleWindowFloating(uint32_t WindowID, bool Center);
void ToggleFocusedWindowFloating();
void ToggleFocusedWindowParentContainer();
void ToggleFocusedWindowFullscreen();
void DetachAndReinsertWindow(unsigned int WindowID, int Degrees);
void SwapFocusedWindowWithMarked();
void SwapFocusedWindowDirected(int Degrees);
void SwapFocusedWindowWithNearest(int Shift);
void ShiftWindowFocus(int Shift);
void ShiftWindowFocusDirected(int Degrees);
void ClearMarkedWindow();
void MarkWindowContainer(ax_window *Window);
void MarkFocusedWindowContainer();
void SetWindowFocusByNode(tree_node *Node);
void SetWindowFocusByNode(link_node *Link);
void CenterWindowInsideNodeContainer(AXUIElementRef WindowRef, int *Xptr, int *Yptr, int *Wptr, int *Hptr);
void SetWindowDimensions(AXUIElementRef WindowRef, window_info *Window, int X, int Y, int Width, int Height);

/* TODO(koekeishiya): NOT YET LOOKED AT */
void AddWindowToTreeOfUnfocusedMonitor(screen_info *Screen, window_info *Window);

void AddWindowToBSPTree(screen_info *Screen, int WindowID);
void RemoveWindowFromBSPTree(screen_info *Screen, int WindowID, bool Center, bool UpdateFocus);

bool IsWindowFullscreen(window_info *Window);
bool IsWindowParentContainer(window_info *Window);
void LockWindowToContainerSize(window_info *Window);

void ShiftSubTreeWindowFocus(int Shift);

void MoveFloatingWindow(int X, int Y);

#endif
