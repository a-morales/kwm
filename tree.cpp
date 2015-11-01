#include "kwm.h"

extern std::vector<window_info> WindowLst;

node_container LeftVerticalContainerSplit(screen_info *Screen, tree_node *Node)
{
    node_container LeftContainer;

    LeftContainer.X = Node->Container.X;
    LeftContainer.Y = Node->Container.Y;
    LeftContainer.Width = (Node->Container.Width / 2) + (Screen->PaddingLeft / 2);
    LeftContainer.Height = Node->Container.Height;
    
    return LeftContainer;
}

node_container RightVerticalContainerSplit(screen_info *Screen, tree_node *Node)
{
    node_container RightContainer;

    RightContainer.X = Node->Container.X + (Node->Container.Width / 2) + (Screen->PaddingLeft);
    RightContainer.Y = Node->Container.Y;
    RightContainer.Width = (Node->Container.Width / 2) - (Screen->PaddingRight);
    RightContainer.Height = Node->Container.Height;

    return RightContainer;
}

node_container UpperHorizontalContainerSplit(screen_info *Screen, tree_node *Node)
{
    node_container UpperContainer;

    UpperContainer.X = Node->Container.X;
    UpperContainer.Y = Node->Container.Y;
    UpperContainer.Width = Node->Container.Width;
    UpperContainer.Height = (Node->Container.Height / 2);

    return UpperContainer;
}

node_container LowerHorizontalContainerSplit(screen_info *Screen, tree_node *Node)
{
    node_container LowerContainer;

    LowerContainer.X = Node->Container.X;
    LowerContainer.Y = Node->Container.Y + (Node->Container.Height / 2) + (Screen->PaddingBottom / 2);
    LowerContainer.Width = Node->Container.Width;
    LowerContainer.Height = (Node->Container.Height / 2) - (Screen->PaddingBottom / 2);

    return LowerContainer;
}

node_container FullscreenContainer(screen_info *Screen, tree_node *Node)
{
    node_container FullscreenContainer;

    FullscreenContainer.X = Node->Container.X;
    FullscreenContainer.Y = Node->Container.Y;
    FullscreenContainer.Width = Node->Container.Width - Screen->PaddingRight;
    FullscreenContainer.Height = Node->Container.Height;

    return FullscreenContainer;
}

tree_node *CreateLeafNode(screen_info *Screen, tree_node *Parent, int WindowID, int ContainerType)
{
    tree_node *Leaf = (tree_node*) malloc(sizeof(tree_node));
    Leaf->Parent = Parent;
    Leaf->WindowID = WindowID;

    switch(ContainerType)
    {
        case 0:
        {
            Leaf->Container = FullscreenContainer(Screen, Leaf->Parent);
        } break;
        case 1:
        {
            Leaf->Container = LeftVerticalContainerSplit(Screen, Leaf->Parent);
        } break;
        case 2:
        {
            Leaf->Container = RightVerticalContainerSplit(Screen, Leaf->Parent);
        } break;
        case 3:
        {
            Leaf->Container = UpperHorizontalContainerSplit(Screen, Leaf->Parent);
        } break;
        case 4:
        {
            Leaf->Container = LowerHorizontalContainerSplit(Screen, Leaf->Parent);
        } break;
    }

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

void SetRootNodeContainer(tree_node *Node, int X, int Y, int Width, int Height)
{
    Node->Container.X = X;
    Node->Container.Y = Y,
    Node->Container.Width = Width;
    Node->Container.Height = Height;
}

void CreateLeafNodePair(screen_info *Screen, tree_node *Parent, int LeftWindowID, int RightWindowID, int SplitMode)
{
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
    SetRootNodeContainer(RootNode, Screen->X + Screen->PaddingLeft, Screen->Y + Screen->PaddingTop,
                         Screen->Width - Screen->PaddingLeft - Screen->PaddingRight,
                         Screen->Height - Screen->PaddingTop - Screen->PaddingBottom);

    if(Windows.size() == 1)
    {
        RootNode->WindowID = Windows[0];
    }
    else
    {
        int splitmode = 1;
        tree_node *Root = RootNode;
        Root->WindowID = Windows[0];
        for(int WindowIndex = 1; WindowIndex < Windows.size(); ++WindowIndex)
        {
            while(!IsLeafNode(Root))
            {
                if(!IsLeafNode(Root->LeftChild) && IsLeafNode(Root->RightChild))
                    Root = Root->RightChild;
                else
                    Root = Root->LeftChild;
            }

            DEBUG("CreateTreeFromWindowIDList() Create pair of leafs")
            CreateLeafNodePair(Screen, Root, Root->WindowID, Windows[WindowIndex], splitmode++ % 3);
            Root->WindowID = -1;
            Root = RootNode;
        }
    }

    return RootNode;
}

void SwapNodeWindowIDs(tree_node *A, tree_node *B)
{
    if(A && B)
    {
        DEBUG("SwapNodeWindowIDs() " << A->WindowID << " with " << B->WindowID)
        int TempWindowID = A->WindowID;
        A->WindowID = B->WindowID;
        B->WindowID = TempWindowID;
        ResizeWindow(A);
        ResizeWindow(B);
    }
}

tree_node *GetNodeFromWindowID(tree_node *Node, int WindowID)
{
    tree_node *Result = NULL;

    if(Node)
    {
        if(Node->WindowID == WindowID)
        {
            DEBUG("GetNodeFromWindowID() " << WindowID)
            return Node;
        }

        if(Node->LeftChild)
        {
            Result = GetNodeFromWindowID(Node->LeftChild, WindowID);
            if(Result == NULL)
                return GetNodeFromWindowID(Node->RightChild, WindowID);
        }

        if(Node->RightChild)
        {
            Result = GetNodeFromWindowID(Node->RightChild, WindowID);
            if(Result == NULL)
                return GetNodeFromWindowID(Node->LeftChild, WindowID);
        }
    }

    return Result;
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
                if(Root->LeftChild->WindowID == -1)
                    return Root->LeftChild->RightChild;

                return Root->LeftChild;
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
                if(Root->RightChild->WindowID == -1)
                    return Root->RightChild->LeftChild;

                return Root->RightChild;
            }
            else
            {
                return GetNearestNodeToTheRight(Root);
            }
        }
    }

    return NULL;
}

void ApplyNodeContainer(tree_node *Node)
{
    if(Node)
    {
        if(Node->LeftChild)
            ApplyNodeContainer(Node->LeftChild);

        if(Node->RightChild)
            ApplyNodeContainer(Node->RightChild);

        if(Node->WindowID != -1)
            ResizeWindow(Node);
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
