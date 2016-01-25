#ifndef TREE_H
#define TREE_H

#include "types.h"

node_container LeftVerticalContainerSplit(screen_info *Screen, tree_node *Node);
node_container RightVerticalContainerSplit(screen_info *Screen, tree_node *Node);
node_container UpperHorizontalContainerSplit(screen_info *Screen, tree_node *Node);
node_container LowerHorizontalContainerSplit(screen_info *Screen, tree_node *Node);

void CreateNodeContainer(screen_info *Screen, tree_node *Node, int ContainerType);
void CreateNodeContainerPair(screen_info *Screen, tree_node *LeftNode, tree_node *RightNode, int SplitMode);
void SetRootNodeContainer(screen_info *Screen, tree_node *Node);
void ResizeNodeContainer(screen_info *Screen, tree_node *Node);
void CreateNodeContainers(screen_info *Screen, tree_node *Node, bool OptimalSplit);
void ApplyNodeContainer(tree_node *Node, space_tiling_option Mode);

tree_node *CreateLeafNode(screen_info *Screen, tree_node *Parent, int WindowID, int ContainerType);
void CreateLeafNodePair(screen_info *Screen, tree_node *Parent, int FirstWindowID, int SecondWindowID, int SplitMode);
tree_node *CreateRootNode();

bool IsLeafNode(tree_node *Node);
bool IsLeftChild(tree_node *Node);
bool IsRightChild(tree_node *Node);

tree_node *GetFirstLeafNode(tree_node *Node);
tree_node *GetLastLeafNode(tree_node *Node);
tree_node *GetFirstPseudoLeafNode(tree_node *Node);
tree_node *GetNearestLeafNeighbour(tree_node *Node, space_tiling_option Mode);
tree_node *GetNearestNodeToTheLeft(tree_node *Node, space_tiling_option Mode);
tree_node *GetNearestNodeToTheRight(tree_node *Node, space_tiling_option Mode);
tree_node *GetNodeFromWindowID(tree_node *Node, int WindowID, space_tiling_option Mode);
void SwapNodeWindowIDs(tree_node *A, tree_node *B);
int GetOptimalSplitMode(tree_node *Node);
void ToggleNodeSplitMode(screen_info *Screen, tree_node *Node);
void ChangeSplitRatio(double Value);

tree_node *CreateTreeFromWindowIDList(screen_info *Screen, std::vector<window_info*> *WindowsPtr);
bool CreateBSPTree(tree_node *RootNode, screen_info *Screen, std::vector<window_info*> *WindowsPtr);
bool CreateMonocleTree(tree_node *RootNode, screen_info *Screen, std::vector<window_info*> *WindowsPtr);
void DestroyNodeTree(tree_node *Node, space_tiling_option Mode);
void RotateTree(tree_node *Node, int Deg);

void SaveBSPTreeToFile(screen_info *Screen, std::string Name);
void LoadBSPTreeFromFile(screen_info *Screen, std::string Name);
void SerializeParentNode(tree_node *Parent, std::string Role, std::vector<std::string> &Serialized);
unsigned int DeserializeParentNode(tree_node *Parent, std::vector<std::string> &Serialized, unsigned int Index);
unsigned int DeserializeChildNode(tree_node *Parent, std::vector<std::string> &Serialized, unsigned int Index);
tree_node *DeserializeNodeTree(std::vector<std::string> &Serialized);
void CreateDeserializedNodeContainer(tree_node *Node);
void FillDeserializedTree(tree_node *RootNode);

#endif
