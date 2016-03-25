#ifndef TREE_H
#define TREE_H

#include "types.h"

tree_node *CreateTreeFromWindowIDList(screen_info *Screen, std::vector<window_info*> *WindowsPtr);
bool CreateBSPTree(tree_node *RootNode, screen_info *Screen, std::vector<window_info*> *WindowsPtr);
bool CreateMonocleTree(tree_node *RootNode, screen_info *Screen, std::vector<window_info*> *WindowsPtr);
tree_node *GetNearestLeafNodeNeighbour(tree_node *Node);
tree_node *GetTreeNodeFromWindowID(tree_node *Node, int WindowID);
tree_node *GetTreeNodeFromWindowIDOrLinkNode(tree_node *Node, int WindowID);
link_node *GetLinkNodeFromWindowID(tree_node *Root, int WindowID);
link_node *GetLinkNodeFromTree(tree_node *Root, int WindowID);
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
void RotateTree(tree_node *Node, int Deg);
void FillDeserializedTree(tree_node *RootNode);
void ChangeSplitRatio(double Value);

#endif
