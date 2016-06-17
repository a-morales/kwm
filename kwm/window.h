#ifndef WINDOW_H
#define WINDOW_H

#include "types.h"
#include "axlib/display.h"
#include "axlib/window.h"

void CreateWindowNodeTree(ax_display *Display);
void CreateInactiveWindowNodeTree(ax_display *Display, std::vector<uint32_t> *Windows);
void LoadWindowNodeTree(ax_display *Display, std::string Layout);
void ResetWindowNodeTree(ax_display *Display, space_tiling_option Mode);
void AddWindowToNodeTree(ax_display *Display, uint32_t WindowID);
void RemoveWindowFromNodeTree(ax_display *Display, uint32_t WindowID);
void RebalanceNodeTree(ax_display *Display);
void AddWindowToInactiveNodeTree(ax_display *Display, uint32_t WindowID);

ax_window *GetWindowByID(uint32_t WindowID);
void GetCenterOfWindow(ax_window *Window, int *X, int *Y);
bool WindowIsInDirection(ax_window *WindowA, ax_window *WindowB, int Degrees);
bool FindClosestWindow(int Degrees, ax_window **ClosestWindow, bool Wrap);
void CenterWindow(ax_display *Display, ax_window *Window);

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
void SetWindowDimensions(AXUIElementRef WindowRef, int X, int Y, int Width, int Height);
bool IsWindowFullscreen(ax_window *Window);
bool IsWindowParentContainer(ax_window *Window);
void LockWindowToContainerSize(ax_window *Window);
void ShiftSubTreeWindowFocus(int Shift);

/* TODO(koekeishiya): NOT YET LOOKED AT */
void MoveFloatingWindow(int X, int Y);

#endif
