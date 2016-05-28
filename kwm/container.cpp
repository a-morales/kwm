#include "container.h"
#include "node.h"
#include "space.h"

#define internal static

extern kwm_screen KWMScreen;

internal node_container
LeftVerticalContainerSplit(screen_info *Screen, tree_node *Node)
{
    Assert(Node);
    space_info *Space = GetActiveSpaceOfScreen(Screen);
    node_container LeftContainer;

    LeftContainer.X = Node->Container.X;
    LeftContainer.Y = Node->Container.Y;
    LeftContainer.Width = (Node->Container.Width * Node->SplitRatio) - (Space->Settings.Offset.VerticalGap / 2);
    LeftContainer.Height = Node->Container.Height;

    return LeftContainer;
}

internal node_container
RightVerticalContainerSplit(screen_info *Screen, tree_node *Node)
{
    Assert(Node);
    space_info *Space = GetActiveSpaceOfScreen(Screen);
    node_container RightContainer;

    RightContainer.X = Node->Container.X + (Node->Container.Width * Node->SplitRatio) + (Space->Settings.Offset.VerticalGap / 2);
    RightContainer.Y = Node->Container.Y;
    RightContainer.Width = (Node->Container.Width * (1 - Node->SplitRatio)) - (Space->Settings.Offset.VerticalGap / 2);
    RightContainer.Height = Node->Container.Height;

    return RightContainer;
}

internal node_container
UpperHorizontalContainerSplit(screen_info *Screen, tree_node *Node)
{
    Assert(Node);
    space_info *Space = GetActiveSpaceOfScreen(Screen);
    node_container UpperContainer;

    UpperContainer.X = Node->Container.X;
    UpperContainer.Y = Node->Container.Y;
    UpperContainer.Width = Node->Container.Width;
    UpperContainer.Height = (Node->Container.Height * Node->SplitRatio) - (Space->Settings.Offset.HorizontalGap / 2);

    return UpperContainer;
}

internal node_container
LowerHorizontalContainerSplit(screen_info *Screen, tree_node *Node)
{
    Assert(Node);
    space_info *Space = GetActiveSpaceOfScreen(Screen);
    node_container LowerContainer;

    LowerContainer.X = Node->Container.X;
    LowerContainer.Y = Node->Container.Y + (Node->Container.Height * Node->SplitRatio) + (Space->Settings.Offset.HorizontalGap / 2);
    LowerContainer.Width = Node->Container.Width;
    LowerContainer.Height = (Node->Container.Height * (1 - Node->SplitRatio)) - (Space->Settings.Offset.HorizontalGap / 2);

    return LowerContainer;
}

void SetRootNodeContainer(screen_info *Screen, tree_node *Node)
{
    Assert(Node);

    space_info *Space = GetActiveSpaceOfScreen(Screen);

    Node->Container.X = Screen->X + Space->Settings.Offset.PaddingLeft;
    Node->Container.Y = Screen->Y + Space->Settings.Offset.PaddingTop;
    Node->Container.Width = Screen->Width - Space->Settings.Offset.PaddingLeft - Space->Settings.Offset.PaddingRight;
    Node->Container.Height = Screen->Height - Space->Settings.Offset.PaddingTop - Space->Settings.Offset.PaddingBottom;
    Node->SplitMode = GetOptimalSplitMode(Node);

    Node->Container.Type = 0;
}

void SetLinkNodeContainer(screen_info *Screen, link_node *Link)
{
    Assert(Link);

    space_info *Space = GetActiveSpaceOfScreen(Screen);

    Link->Container.X = Screen->X + Space->Settings.Offset.PaddingLeft;
    Link->Container.Y = Screen->Y + Space->Settings.Offset.PaddingTop;
    Link->Container.Width = Screen->Width - Space->Settings.Offset.PaddingLeft - Space->Settings.Offset.PaddingRight;
    Link->Container.Height = Screen->Height - Space->Settings.Offset.PaddingTop - Space->Settings.Offset.PaddingBottom;
}

void CreateNodeContainer(screen_info *Screen, tree_node *Node, int ContainerType)
{
    Assert(Node);

    if(Node->SplitRatio == 0)
        Node->SplitRatio = KWMScreen.SplitRatio;

    switch(ContainerType)
    {
        case 1:
        {
            Node->Container = LeftVerticalContainerSplit(Screen, Node->Parent);
        } break;
        case 2:
        {
            Node->Container = RightVerticalContainerSplit(Screen, Node->Parent);
        } break;
        case 3:
        {
            Node->Container = UpperHorizontalContainerSplit(Screen, Node->Parent);
        } break;
        case 4:
        {
            Node->Container = LowerHorizontalContainerSplit(Screen, Node->Parent);
        } break;
    }

    Node->SplitMode = GetOptimalSplitMode(Node);
    Node->Container.Type = ContainerType;
}

void CreateNodeContainerPair(screen_info *Screen, tree_node *LeftNode, tree_node *RightNode, split_type SplitMode)
{
    Assert(LeftNode);
    Assert(RightNode);

    if(SplitMode == SPLIT_VERTICAL)
    {
        CreateNodeContainer(Screen, LeftNode, 1);
        CreateNodeContainer(Screen, RightNode, 2);
    }
    else
    {
        CreateNodeContainer(Screen, LeftNode, 3);
        CreateNodeContainer(Screen, RightNode, 4);
    }
}

void ResizeNodeContainer(screen_info *Screen, tree_node *Node)
{
    if(Node)
    {
        if(Node->LeftChild)
        {
            CreateNodeContainer(Screen, Node->LeftChild, Node->LeftChild->Container.Type);
            ResizeNodeContainer(Screen, Node->LeftChild);
            ResizeLinkNodeContainers(Node->LeftChild);
        }

        if(Node->RightChild)
        {
            CreateNodeContainer(Screen, Node->RightChild, Node->RightChild->Container.Type);
            ResizeNodeContainer(Screen, Node->RightChild);
            ResizeLinkNodeContainers(Node->RightChild);
        }
    }
}

void ResizeLinkNodeContainers(tree_node *Root)
{
    if(Root)
    {
        link_node *Link = Root->List;
        while(Link)
        {
            Link->Container = Root->Container;
            Link = Link->Next;
        }
    }
}

void CreateNodeContainers(screen_info *Screen, tree_node *Node, bool OptimalSplit)
{
    if(Node && Node->LeftChild && Node->RightChild)
    {
        Node->SplitMode = OptimalSplit ? GetOptimalSplitMode(Node) : Node->SplitMode;
        CreateNodeContainerPair(Screen, Node->LeftChild, Node->RightChild, Node->SplitMode);

        CreateNodeContainers(Screen, Node->LeftChild, OptimalSplit);
        CreateNodeContainers(Screen, Node->RightChild, OptimalSplit);
    }
}

void CreateDeserializedNodeContainer(tree_node *Node)
{
    int SplitMode = Node->Parent->SplitMode;
    int ContainerType = 0;

    if(SplitMode == SPLIT_VERTICAL)
        ContainerType = IsLeftChild(Node) ? 1 : 2;
    else
        ContainerType = IsLeftChild(Node) ? 3 : 4;

    CreateNodeContainer(KWMScreen.Current, Node, ContainerType);
}
