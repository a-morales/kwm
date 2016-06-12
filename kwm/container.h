#ifndef CONTAINER_H
#define CONTAINER_H

#include "types.h"
#include "axlib/display.h"

/* NOTE(koekeishiya): The following functions are working as expected. */

/* NOTE(koekeishiya): The following functions are under testing. */
void SetRootNodeContainer(ax_display *Display, tree_node *Node);
void CreateNodeContainer(ax_display *Display, tree_node *Node, int ContainerType);
void SetLinkNodeContainer(ax_display *Display, link_node *Link);
void CreateNodeContainers(ax_display *Display, tree_node *Node, bool OptimalSplit);
void CreateNodeContainerPair(ax_display *Display, tree_node *LeftNode, tree_node *RightNode, split_type SplitMode);
void CreateDeserializedNodeContainer(ax_display *Display, tree_node *Node);

/* NOTE(koekeishiya): The following functions still need to be investigated. */
void CreateNodeContainer(screen_info *Screen, tree_node *Node, int ContainerType);
void CreateNodeContainerPair(screen_info *Screen, tree_node *LeftNode, tree_node *RightNode, split_type SplitMode);
void SetRootNodeContainer(screen_info *Screen, tree_node *Node);
void SetLinkNodeContainer(screen_info *Screen, link_node *Link);
void ResizeNodeContainer(screen_info *Screen, tree_node *Node);
void ResizeLinkNodeContainers(tree_node *Root);
void CreateNodeContainers(screen_info *Screen, tree_node *Node, bool OptimalSplit);
void CreateDeserializedNodeContainer(tree_node *Node);

#endif
