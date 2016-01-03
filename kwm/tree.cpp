#include "kwm.h"

node_container LeftVerticalContainerSplit(screen_info *Screen, tree_node *Node)
{
    space_info *Space = &Screen->Space[Screen->ActiveSpace];
    node_container LeftContainer;

    LeftContainer.X = Node->Container.X;
    LeftContainer.Y = Node->Container.Y;
    LeftContainer.Width = (Node->Container.Width / 2) - (Space->VerticalGap / 2);
    LeftContainer.Height = Node->Container.Height;
    
    return LeftContainer;
}

node_container RightVerticalContainerSplit(screen_info *Screen, tree_node *Node)
{
    space_info *Space = &Screen->Space[Screen->ActiveSpace];
    node_container RightContainer;

    RightContainer.X = Node->Container.X + (Node->Container.Width / 2) + (Space->VerticalGap / 2);
    RightContainer.Y = Node->Container.Y;
    RightContainer.Width = (Node->Container.Width / 2) - (Space->VerticalGap / 2);
    RightContainer.Height = Node->Container.Height;

    return RightContainer;
}

node_container UpperHorizontalContainerSplit(screen_info *Screen, tree_node *Node)
{
    space_info *Space = &Screen->Space[Screen->ActiveSpace];
    node_container UpperContainer;

    UpperContainer.X = Node->Container.X;
    UpperContainer.Y = Node->Container.Y;
    UpperContainer.Width = Node->Container.Width;
    UpperContainer.Height = (Node->Container.Height / 2) - (Space->HorizontalGap / 2);

    return UpperContainer;
}

node_container LowerHorizontalContainerSplit(screen_info *Screen, tree_node *Node)
{
    space_info *Space = &Screen->Space[Screen->ActiveSpace];
    node_container LowerContainer;

    LowerContainer.X = Node->Container.X;
    LowerContainer.Y = Node->Container.Y + (Node->Container.Height / 2) + (Space->HorizontalGap / 2);
    LowerContainer.Width = Node->Container.Width;
    LowerContainer.Height = (Node->Container.Height / 2) - (Space->HorizontalGap / 2);

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

    Node->SplitMode = 0;
    if(ContainerType != -1)
        Node->Container.Type = ContainerType;
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
    RootNode->SplitMode = 0;

    return RootNode;
}

void SetRootNodeContainer(screen_info *Screen, tree_node *Node)
{
    space_info *Space = &Screen->Space[Screen->ActiveSpace];

    Node->Container.X = Screen->X + Space->PaddingLeft;
    Node->Container.Y = Screen->Y + Space->PaddingTop;
    Node->Container.Width = Screen->Width - Space->PaddingLeft - Space->PaddingRight;
    Node->Container.Height = Screen->Height - Space->PaddingTop - Space->PaddingBottom;

    Node->Container.Type = 0;
}

void CreateLeafNodePair(screen_info *Screen, tree_node *Parent, int LeftWindowID, int RightWindowID, int SplitMode)
{
    Parent->WindowID = -1;
    Parent->SplitMode = SplitMode;

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

tree_node *CreateTreeFromWindowIDList(screen_info *Screen, std::vector<window_info*> *WindowsPtr)
{
    if(IsSpaceFloating(Screen->ActiveSpace))
        return NULL;

    tree_node *RootNode = CreateRootNode();
    SetRootNodeContainer(Screen, RootNode);

    bool Result = false;
    space_info *Space = &Screen->Space[Screen->ActiveSpace];

    if(Space->Mode == SpaceModeBSP)
        Result = CreateBSPTree(RootNode, Screen, WindowsPtr);
    else if(Space->Mode == SpaceModeStacking)
        Result = CreateStackingTree(RootNode, Screen, WindowsPtr);

    if(!Result)
    {
        free(RootNode);
        RootNode = NULL;
    }

    return RootNode;
}

bool CreateBSPTree(tree_node *RootNode, screen_info *Screen, std::vector<window_info*> *WindowsPtr)
{
    bool Result = false;
    std::vector<window_info*> &Windows = *WindowsPtr;

    if(Windows.size() >= 2)
    {
        tree_node *Root = RootNode;
        int FirstIndex;
        bool FoundValidWindow = false;
        for(FirstIndex = 0; FirstIndex < Windows.size(); ++FirstIndex)
        {
            if(!IsWindowFloating(Windows[FirstIndex]->WID, NULL))
            {
                Root->WindowID = Windows[FirstIndex]->WID;
                FoundValidWindow = true;
                break;
            }
        }

        if(!FoundValidWindow)
            return false;

        for(int WindowIndex = FirstIndex + 1; WindowIndex < Windows.size(); ++WindowIndex)
        {
            if(!IsWindowFloating(Windows[WindowIndex]->WID, NULL))
            {
                while(!IsLeafNode(Root))
                {
                    if(!IsLeafNode(Root->LeftChild) && IsLeafNode(Root->RightChild))
                        Root = Root->RightChild;
                    else
                        Root = Root->LeftChild;
                }

                DEBUG("CreateBSPTree() Create pair of leafs")
                CreateLeafNodePair(Screen, Root, Root->WindowID, Windows[WindowIndex]->WID, GetOptimalSplitMode(Root));
                Root = RootNode;
            }
        }

        Result = true;
    }
    else if(Windows.size() == 1 && !IsWindowFloating(Windows[0]->WID, NULL))
    {
        RootNode->WindowID = Windows[0]->WID;
        Result = true;
    }

    return Result;
}

bool CreateStackingTree(tree_node *RootNode, screen_info *Screen, std::vector<window_info*> *WindowsPtr)
{
    bool Result = false;
    std::vector<window_info*> &Windows = *WindowsPtr;

    if(!Windows.empty())
    {
        tree_node *Root = RootNode;
        Root->WindowID = Windows[0]->WID;

        for(int WindowIndex = 1; WindowIndex < Windows.size(); ++WindowIndex)
        {
            tree_node *Next = CreateRootNode();
            SetRootNodeContainer(Screen, Next);
            Next->WindowID = Windows[WindowIndex]->WID;

            Root->RightChild = Next;
            Next->LeftChild = Root;
            Root = Next;
        }

        Result = true;
    }

    return Result;
}

int GetOptimalSplitMode(tree_node *Node)
{
    return (Node->Container.Width / Node->Container.Height) >= 1.618 ? 1 : 2;
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

tree_node *GetNodeFromWindowID(tree_node *Node, int WindowID, space_tiling_option Mode)
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

            CurrentNode = GetNearestNodeToTheRight(CurrentNode, Mode);
        }
    }

    return NULL;
}

void ResizeNodeContainer(screen_info *Screen, tree_node *Node)
{
    if(Node)
    {
        if(Node->LeftChild)
        {
            CreateNodeContainer(Screen, Node->LeftChild, Node->LeftChild->Container.Type);
            ResizeNodeContainer(Screen, Node->LeftChild);
        }

        if(Node->RightChild)
        {
            CreateNodeContainer(Screen, Node->RightChild, Node->RightChild->Container.Type);
            ResizeNodeContainer(Screen, Node->RightChild);
        }
    }
}

tree_node *GetNearestNodeToTheLeft(tree_node *Node, space_tiling_option Mode)
{
    if(Node)
    {
        if(Mode == SpaceModeBSP)
        {
            if(Node->Parent)
            {
                tree_node *Root = Node->Parent;
                if(Root->LeftChild == Node)
                    return GetNearestNodeToTheLeft(Root, Mode);

                if(IsLeafNode(Root->LeftChild))
                    return Root->LeftChild;

                Root = Root->LeftChild;
                while(!IsLeafNode(Root->RightChild))
                    Root = Root->RightChild;

                return Root->RightChild;
            }
        }
        else if(Mode == SpaceModeStacking)
        {
            return Node->LeftChild;
        }
    }

    return NULL;
}

tree_node *GetNearestNodeToTheRight(tree_node *Node, space_tiling_option Mode)
{
    if(Node)
    {
        if(Mode == SpaceModeBSP)
        {
            if(Node->Parent)
            {
                tree_node *Root = Node->Parent;
                if(Root->RightChild == Node)
                    return GetNearestNodeToTheRight(Root, Mode);

                if(IsLeafNode(Root->RightChild))
                    return Root->RightChild;

                Root = Root->RightChild;
                while(!IsLeafNode(Root->LeftChild))
                    Root = Root->LeftChild;

                return Root->LeftChild;
            }
        }
        else if(Mode == SpaceModeStacking)
        {
            return Node->RightChild;
        }
    }

    return NULL;
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

void ToggleNodeSplitMode(screen_info *Screen, tree_node *Node)
{
    if(!Node || IsLeafNode(Node))
        return;

    Node->SplitMode = Node->SplitMode == 1 ? 2 : 1;
    CreateNodeContainers(Screen, Node, false);
    ApplyNodeContainer(Node, SpaceModeBSP);
}

void ApplyNodeContainer(tree_node *Node, space_tiling_option Mode)
{
    if(Node)
    {
        if(Node->WindowID != -1)
            ResizeWindowToContainerSize(Node);

        if(Mode == SpaceModeBSP && Node->LeftChild)
            ApplyNodeContainer(Node->LeftChild, Mode);

        if(Node->RightChild)
            ApplyNodeContainer(Node->RightChild, Mode);
    }
}

void DestroyNodeTree(tree_node *Node, space_tiling_option Mode)
{
    if(Node)
    {
        if(Mode == SpaceModeBSP && Node->LeftChild)
            DestroyNodeTree(Node->LeftChild, Mode);

        if(Node->RightChild)
            DestroyNodeTree(Node->RightChild, Mode);

        free(Node);
    }
}

void RotateTree(tree_node *Node, int Deg)
{
    if (Node == NULL || IsLeafNode(Node))
        return;

    DEBUG("RotateTree() " << Deg << " degrees")

    if((Deg == 90 && Node->SplitMode == 1) ||
       (Deg == 270 && Node->SplitMode == 2) ||
       Deg == 180)
    {
        tree_node *Temp = Node->LeftChild;
        Node->LeftChild = Node->RightChild;
        Node->RightChild = Temp;
    }

    if(Deg != 180)
        Node->SplitMode = Node->SplitMode == 2 ? 1 : 2;

    RotateTree(Node->LeftChild, Deg);
    RotateTree(Node->RightChild, Deg);
}
