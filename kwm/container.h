#ifndef CONTAINER_H
#define CONTAINER_H

#include "types.h"

node_container LeftVerticalContainerSplit(screen_info *Screen, tree_node *Node);
node_container RightVerticalContainerSplit(screen_info *Screen, tree_node *Node);
node_container UpperHorizontalContainerSplit(screen_info *Screen, tree_node *Node);
node_container LowerHorizontalContainerSplit(screen_info *Screen, tree_node *Node);
void CreateNodeContainer(screen_info *Screen, tree_node *Node, int ContainerType);
void CreateNodeContainerPair(screen_info *Screen, tree_node *LeftNode, tree_node *RightNode, split_type SplitMode);
void SetRootNodeContainer(screen_info *Screen, tree_node *Node);
void SetLinkNodeContainer(screen_info *Screen, link_node *Link);
void ResizeNodeContainer(screen_info *Screen, tree_node *Node);
void ResizeLinkNodeContainers(tree_node *Root);
void CreateNodeContainers(screen_info *Screen, tree_node *Node, bool OptimalSplit);
void CreateDeserializedNodeContainer(tree_node *Node);

#endif
