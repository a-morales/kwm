#include "kwm.h"

extern std::vector<window_info> WindowLst;

/*

       *
    *     *
  *   * *   *
*   *     *   *

*/

node_container LeftVerticalContainerSplit(screen_info *Screen, tree_node *Node)
{
    node_container LeftContainer;

    LeftContainer.X = Node->Container.X;
    LeftContainer.Y = Node->Container.Y;
    LeftContainer.Width = (Node->Container.Width / 2) - (Screen->VerticalGap / 2);
    LeftContainer.Height = Node->Container.Height;
    
    return LeftContainer;
}

node_container RightVerticalContainerSplit(screen_info *Screen, tree_node *Node)
{
    node_container RightContainer;

    RightContainer.X = Node->Container.X + (Node->Container.Width / 2) + (Screen->VerticalGap / 2);
    RightContainer.Y = Node->Container.Y;
    RightContainer.Width = (Node->Container.Width / 2) - (Screen->VerticalGap / 2);
    RightContainer.Height = Node->Container.Height;

    return RightContainer;
}

node_container UpperHorizontalContainerSplit(screen_info *Screen, tree_node *Node)
{
    node_container UpperContainer;

    UpperContainer.X = Node->Container.X;
    UpperContainer.Y = Node->Container.Y;
    UpperContainer.Width = Node->Container.Width;
    UpperContainer.Height = (Node->Container.Height / 2) - (Screen->HorizontalGap / 2);

    return UpperContainer;
}

node_container LowerHorizontalContainerSplit(screen_info *Screen, tree_node *Node)
{
    node_container LowerContainer;

    LowerContainer.X = Node->Container.X;
    LowerContainer.Y = Node->Container.Y + (Node->Container.Height / 2) + (Screen->HorizontalGap / 2);
    LowerContainer.Width = Node->Container.Width;
    LowerContainer.Height = (Node->Container.Height / 2) - (Screen->HorizontalGap / 2);

    return LowerContainer;
}

void CreateNodeContainer(screen_info *Screen, tree_node *Node, int ContainerType)
{
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

    if(ContainerType != -1)
    {
        Node->Container.Type = ContainerType;
    }
}

void CreateNodeContainerPair(screen_info *Screen, tree_node *LeftNode, tree_node *RightNode, int SplitMode)
{
    if(SplitMode == 1)
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

tree_node *CreateLeafNode(screen_info *Screen, tree_node *Parent, int WindowID, int ContainerType)
{
    tree_node *Leaf = (tree_node*) malloc(sizeof(tree_node));
    Leaf->Parent = Parent;
    Leaf->WindowID = WindowID;

    CreateNodeContainer(Screen, Leaf, ContainerType);

    Leaf->LeftChild = NULL;
    Leaf->RightChild = NULL;

    return Leaf;
}

tree_node *CreateRootNode()
{
    tree_node *RootNode = (tree_node*) malloc(sizeof(tree_node));
    std::memset(RootNode, '\0', sizeof(tree_node));

    RootNode->WindowID = -1;
    RootNode->Parent = NULL;
    RootNode->LeftChild = NULL;
    RootNode->RightChild = NULL;

    return RootNode;
}

void SetRootNodeContainer(screen_info *Screen, tree_node *Node)
{
    Node->Container.X = Screen->X + Screen->PaddingLeft;
    Node->Container.Y = Screen->Y + Screen->PaddingTop;
    Node->Container.Width = Screen->Width - Screen->PaddingLeft - Screen->PaddingRight;
    Node->Container.Height = Screen->Height - Screen->PaddingTop - Screen->PaddingBottom;
    Node->Container.Type = 0;
}

void CreateLeafNodePair(screen_info *Screen, tree_node *Parent, int LeftWindowID, int RightWindowID, int SplitMode)
{
    Parent->WindowID = -1;

    if(SplitMode == 1)
    {
        Parent->LeftChild = CreateLeafNode(Screen, Parent, LeftWindowID, 1);
        Parent->RightChild = CreateLeafNode(Screen, Parent, RightWindowID, 2);
    }
    else
    {
        Parent->LeftChild = CreateLeafNode(Screen, Parent, LeftWindowID, 3);
        Parent->RightChild = CreateLeafNode(Screen, Parent, RightWindowID, 4);
    }
}

bool IsLeafNode(tree_node *Node)
{
    return Node->LeftChild == NULL && Node->RightChild == NULL ? true : false;
}

tree_node *CreateTreeFromWindowIDList(screen_info *Screen, std::vector<int> Windows)
{
    tree_node *RootNode = CreateRootNode();
    SetRootNodeContainer(Screen, RootNode);

    if(Windows.size() > 1)
    {
        tree_node *Root = RootNode;
        Root->WindowID = Windows[0];
        for(int WindowIndex = 1; WindowIndex < Windows.size(); ++WindowIndex)
        {
            if(!IsWindowFloating(Windows[WindowIndex]))
            {
                while(!IsLeafNode(Root))
                {
                    if(!IsLeafNode(Root->LeftChild) && IsLeafNode(Root->RightChild))
                        Root = Root->RightChild;
                    else
                        Root = Root->LeftChild;
                }

                DEBUG("CreateTreeFromWindowIDList() Create pair of leafs")
                CreateLeafNodePair(Screen, Root, Root->WindowID, Windows[WindowIndex], GetOptimalSplitMode(Root));
                Root = RootNode;
            }
        }
    }
    else if(Windows.size() == 1)
    {
        RootNode->WindowID = Windows[0];
    }

    return RootNode;
}

int GetOptimalSplitMode(tree_node *Node)
{
    int SplitMode;

    if(Node->Container.Width / Node->Container.Height >= 1.618)
        SplitMode = 1;
    else
        SplitMode = 2;

    return SplitMode;
}

void SwapNodeWindowIDs(tree_node *A, tree_node *B)
{
    if(A && B)
    {
        DEBUG("SwapNodeWindowIDs() " << A->WindowID << " with " << B->WindowID)
        int TempWindowID = A->WindowID;
        A->WindowID = B->WindowID;
        B->WindowID = TempWindowID;
        ResizeWindowToContainerSize(A);
        ResizeWindowToContainerSize(B);
    }
}

tree_node *GetNodeFromWindowID(tree_node *Node, int WindowID)
{
    if(Node)
    {
        tree_node *CurrentNode = Node;
        while(CurrentNode->LeftChild)
            CurrentNode = CurrentNode->LeftChild;

        while(CurrentNode)
        {
            if(CurrentNode->WindowID == WindowID)
            {
                DEBUG("GetNodeFromWindowID() " << WindowID)
                return CurrentNode;
            }

            CurrentNode = GetNearestNodeToTheRight(CurrentNode);
        }
    }

    return NULL;
}

void ReflectTreeVertically(tree_node *Node)
{
    DEBUG("NYI")
}

void ResizeNodeContainer(screen_info *Screen, tree_node *Node)
{
    if(Node)
    {
        if(Node->LeftChild)
        {
            int ContainerType = Node->LeftChild->Container.Type;
            CreateNodeContainer(Screen, Node->LeftChild, ContainerType);
            ResizeNodeContainer(Screen, Node->LeftChild);
        }

        if(Node->RightChild)
        {
            int ContainerType = Node->RightChild->Container.Type;
            CreateNodeContainer(Screen, Node->RightChild, ContainerType);
            ResizeNodeContainer(Screen, Node->RightChild);
        }
    }
}

tree_node *GetNearestNodeToTheLeft(tree_node *Node)
{
    if(Node)
    {
        if(Node->Parent)
        {
            tree_node *Root = Node->Parent;
            if(Root->LeftChild != Node)
            {
                if(IsLeafNode(Root->LeftChild))
                {
                    return Root->LeftChild;
                }
                else
                {
                    Root = Root->LeftChild;
                    while(!IsLeafNode(Root->RightChild))
                        Root = Root->RightChild;
                }

                return Root->RightChild;
            }
            else
            {
                return GetNearestNodeToTheLeft(Root);
            }
        }
    }

    return NULL;
}

tree_node *GetNearestNodeToTheRight(tree_node *Node)
{
    if(Node)
    {
        if(Node->Parent)
        {
            tree_node *Root = Node->Parent;
            if(Root->RightChild != Node)
            {
                if(IsLeafNode(Root->RightChild))
                {
                    return Root->RightChild;
                }
                else
                {
                    Root = Root->RightChild;
                    while(!IsLeafNode(Root->LeftChild))
                        Root = Root->LeftChild;
                }

                return Root->LeftChild;
            }
            else
            {
                return GetNearestNodeToTheRight(Root);
            }
        }
    }

    return NULL;
}

void CreateNodeContainers(screen_info *Screen, tree_node *Node)
{
    if(Node && Node->LeftChild && Node->RightChild)
    {
        int SplitMode = GetOptimalSplitMode(Node);
        CreateNodeContainerPair(Screen, Node->LeftChild, Node->RightChild, SplitMode);

        CreateNodeContainers(Screen, Node->LeftChild);
        CreateNodeContainers(Screen, Node->RightChild);
    }
}

void ApplyNodeContainer(tree_node *Node)
{
    if(Node)
    {
        if(Node->WindowID != -1)
            ResizeWindowToContainerSize(Node);

        if(Node->LeftChild)
            ApplyNodeContainer(Node->LeftChild);

        if(Node->RightChild)
            ApplyNodeContainer(Node->RightChild);
    }
}

void DestroyNodeTree(tree_node *Node)
{
    if(Node)
    {
        if(Node->LeftChild)
            DestroyNodeTree(Node->LeftChild);

        if(Node->RightChild)
            DestroyNodeTree(Node->RightChild);

        free(Node);
    }
}
