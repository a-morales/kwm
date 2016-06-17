#ifndef TREE_H
#define TREE_H

#include "types.h"
#include "axlib/display.h"

tree_node *CreateTreeFromWindowIDList(ax_display *Display, std::vector<uint32_t> *Windows);
bool CreateBSPTree(tree_node *RootNode, ax_display *Display, std::vector<uint32_t> *WindowsPtr);
bool CreateMonocleTree(tree_node *RootNode, ax_display *Display, std::vector<uint32_t> *WindowsPtr);
void FillDeserializedTree(tree_node *RootNode, ax_display *Display, std::vector<uint32_t> *WindowsPtr);
void RotateBSPTree(int Deg);
tree_node *GetNearestLeafNodeNeighbour(tree_node *Node);
tree_node *GetTreeNodeFromWindowID(tree_node *Node, uint32_t WindowID);
tree_node *GetTreeNodeFromWindowIDOrLinkNode(tree_node *Node, uint32_t WindowID);
link_node *GetLinkNodeFromWindowID(tree_node *Root, uint32_t WindowID);
link_node *GetLinkNodeFromTree(tree_node *Root, uint32_t WindowID);
tree_node *GetTreeNodeFromLink(tree_node *Root, link_node *Link);
tree_node *GetNearestTreeNodeToTheLeft(tree_node *Node);
tree_node *GetNearestTreeNodeToTheRight(tree_node *Node);
void GetFirstLeafNode(tree_node *Node, void **Result);
void GetLastLeafNode(tree_node *Node, void **Result);
tree_node *GetFirstPseudoLeafNode(tree_node *Node);
void ApplyLinkNodeContainer(link_node *Link);
void ApplyTreeNodeContainer(tree_node *Node);
void DestroyLinkList(link_node *Link);
void DestroyNodeTree(tree_node *Node);
void ChangeSplitRatio(double Value);

#endif
