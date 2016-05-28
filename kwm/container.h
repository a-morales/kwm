#ifndef CONTAINER_H
#define CONTAINER_H

#include "types.h"

void CreateNodeContainer(screen_info *Screen, tree_node *Node, int ContainerType);
void CreateNodeContainerPair(screen_info *Screen, tree_node *LeftNode, tree_node *RightNode, split_type SplitMode);
void SetRootNodeContainer(screen_info *Screen, tree_node *Node);
void SetLinkNodeContainer(screen_info *Screen, link_node *Link);
void ResizeNodeContainer(screen_info *Screen, tree_node *Node);
void ResizeLinkNodeContainers(tree_node *Root);
void CreateNodeContainers(screen_info *Screen, tree_node *Node, bool OptimalSplit);
void CreateDeserializedNodeContainer(tree_node *Node);

#endif
