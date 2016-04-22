#include "tree.h"
#include "node.h"
#include "container.h"
#include "helpers.h"
#include "display.h"
#include "space.h"
#include "window.h"
#include "border.h"

tree_node *CreateTreeFromWindowIDList(screen_info *Screen, std::vector<window_info*> *WindowsPtr)
{
    if(IsSpaceFloating(Screen->ActiveSpace))
        return NULL;

    tree_node *RootNode = CreateRootNode();
    SetRootNodeContainer(Screen, RootNode);

    bool Result = false;
    space_info *Space = GetActiveSpaceOfScreen(Screen);

    if(Space->Settings.Mode == SpaceModeBSP)
        Result = CreateBSPTree(RootNode, Screen, WindowsPtr);
    else if(Space->Settings.Mode == SpaceModeMonocle)
        Result = CreateMonocleTree(RootNode, Screen, WindowsPtr);

    if(!Result)
    {
        free(RootNode);
        RootNode = NULL;
    }

    return RootNode;
}

bool CreateBSPTree(tree_node *RootNode, screen_info *Screen, std::vector<window_info*> *WindowsPtr)
{
    Assert(RootNode);

    bool Result = false;
    std::vector<window_info*> &Windows = *WindowsPtr;

    if(!Windows.empty())
    {
        tree_node *Root = RootNode;
        Root->WindowID = Windows[0]->WID;
        for(std::size_t WindowIndex = 1; WindowIndex < Windows.size(); ++WindowIndex)
        {
            while(!IsLeafNode(Root))
            {
                if(!IsLeafNode(Root->LeftChild) && IsLeafNode(Root->RightChild))
                    Root = Root->RightChild;
                else
                    Root = Root->LeftChild;
            }

            DEBUG("CreateBSPTree() Create pair of leafs");
            CreateLeafNodePair(Screen, Root, Root->WindowID, Windows[WindowIndex]->WID, GetOptimalSplitMode(Root));
            Root = RootNode;
        }

        Result = true;
    }

    return Result;
}

bool CreateMonocleTree(tree_node *RootNode, screen_info *Screen, std::vector<window_info*> *WindowsPtr)
{
    Assert(RootNode);

    bool Result = false;
    std::vector<window_info*> &Windows = *WindowsPtr;

    if(!Windows.empty())
    {
        tree_node *Root = RootNode;
        Root->List = CreateLinkNode();

        SetLinkNodeContainer(Screen, Root->List);
        Root->List->WindowID = Windows[0]->WID;

        link_node *Link = Root->List;
        for(std::size_t WindowIndex = 1; WindowIndex < Windows.size(); ++WindowIndex)
        {
            link_node *Next = CreateLinkNode();
            SetLinkNodeContainer(Screen, Next);
            Next->WindowID = Windows[WindowIndex]->WID;

            Link->Next = Next;
            Next->Prev = Link;
            Link = Next;
        }

        Result = true;
    }

    return Result;
}

tree_node *GetNearestLeafNodeNeighbour(tree_node *Node)
{
    if(Node && IsLeafNode(Node))
        return IsLeftChild(Node) ? GetNearestTreeNodeToTheRight(Node) : GetNearestTreeNodeToTheLeft(Node);

    return NULL;
}

tree_node *GetTreeNodeFromWindowID(tree_node *Node, int WindowID)
{
    if(Node)
    {
        tree_node *CurrentNode = NULL;
        GetFirstLeafNode(Node, (void**)&CurrentNode);
        while(CurrentNode)
        {
            if(CurrentNode->WindowID == WindowID)
                return CurrentNode;

            CurrentNode = GetNearestTreeNodeToTheRight(CurrentNode);
        }
    }

    return NULL;
}

tree_node *GetTreeNodeFromWindowIDOrLinkNode(tree_node *Node, int WindowID)
{
    tree_node *Result = NULL;
    Result = GetTreeNodeFromWindowID(Node, WindowID);
    if(!Result)
    {
        link_node *Link = GetLinkNodeFromWindowID(Node, WindowID);
        Result = GetTreeNodeFromLink(Node, Link);
    }

    return Result;
}

link_node *GetLinkNodeFromWindowID(tree_node *Root, int WindowID)
{
    if(Root)
    {
        tree_node *Node = NULL;
        GetFirstLeafNode(Root, (void**)&Node);
        while(Node)
        {
            link_node *Link = GetLinkNodeFromTree(Node, WindowID);
            if(Link)
                return Link;

            Node = GetNearestTreeNodeToTheRight(Node);
        }
    }

    return NULL;
}

link_node *GetLinkNodeFromTree(tree_node *Root, int WindowID)
{
    if(Root)
    {
        link_node *Link = Root->List;
        while(Link)
        {
            if(Link->WindowID == WindowID)
                return Link;

            Link = Link->Next;
        }
    }

    return NULL;
}

tree_node *GetTreeNodeFromLink(tree_node *Root, link_node *Link)
{
    if(Root && Link)
    {
        tree_node *Node = NULL;
        GetFirstLeafNode(Root, (void**)&Node);
        while(Node)
        {
            if(GetLinkNodeFromTree(Node, Link->WindowID) == Link)
                return Node;

            Node = GetNearestTreeNodeToTheRight(Node);
        }
    }

    return NULL;
}

tree_node *GetNearestTreeNodeToTheLeft(tree_node *Node)
{
    if(Node)
    {
        if(Node->Parent)
        {
            tree_node *Root = Node->Parent;
            if(Root->LeftChild == Node)
                return GetNearestTreeNodeToTheLeft(Root);

            if(IsLeafNode(Root->LeftChild))
                return Root->LeftChild;

            Root = Root->LeftChild;
            while(!IsLeafNode(Root->RightChild))
                Root = Root->RightChild;

            return Root->RightChild;
        }
    }

    return NULL;
}

tree_node *GetNearestTreeNodeToTheRight(tree_node *Node)
{
    if(Node)
    {
        if(Node->Parent)
        {
            tree_node *Root = Node->Parent;
            if(Root->RightChild == Node)
                return GetNearestTreeNodeToTheRight(Root);

            if(IsLeafNode(Root->RightChild))
                return Root->RightChild;

            Root = Root->RightChild;
            while(!IsLeafNode(Root->LeftChild))
                Root = Root->LeftChild;

            return Root->LeftChild;
        }
    }

    return NULL;
}

void GetFirstLeafNode(tree_node *Node, void **Result)
{
    if(Node)
    {
        if(Node->Type == NodeTypeLink)
            *Result = Node->List;

        else if(Node->Type == NodeTypeTree)
        {
            while(Node->LeftChild)
                Node = Node->LeftChild;

            *Result = Node;
        }
    }
}

void GetLastLeafNode(tree_node *Node, void **Result)
{
    if(Node)
    {
        if(Node->Type == NodeTypeLink)
        {
            link_node *Link = Node->List;
            while(Link->Next)
                Link = Link->Next;

            *Result = Link;
        }
        else if(Node->Type == NodeTypeTree)
        {
            while(Node->RightChild)
                Node = Node->RightChild;

            *Result = Node;
        }
    }
}

tree_node *GetFirstPseudoLeafNode(tree_node *Node)
{
    tree_node *Leaf = NULL;
    GetFirstLeafNode(Node, (void**)&Leaf);
    while(Leaf && Leaf->WindowID != -1)
        Leaf = GetNearestTreeNodeToTheRight(Leaf);

    return Leaf;
}

void ApplyLinkNodeContainer(link_node *Link)
{
    if(Link)
    {
        ResizeWindowToContainerSize(Link);
        if(Link->Next)
            ApplyLinkNodeContainer(Link->Next);
    }
}

void ApplyTreeNodeContainer(tree_node *Node)
{
    if(Node)
    {
        if(Node->WindowID != -1)
            ResizeWindowToContainerSize(Node);

        if(Node->List)
            ApplyLinkNodeContainer(Node->List);

        if(Node->LeftChild)
            ApplyTreeNodeContainer(Node->LeftChild);

        if(Node->RightChild)
            ApplyTreeNodeContainer(Node->RightChild);
    }
}

void DestroyLinkList(link_node *Link)
{
    if(Link)
    {
        if(Link->Next)
            DestroyLinkList(Link->Next);

        free(Link);
        Link = NULL;
    }
}

void DestroyNodeTree(tree_node *Node)
{
    if(Node)
    {
        if(Node->List)
            DestroyLinkList(Node->List);

        if(Node->LeftChild)
            DestroyNodeTree(Node->LeftChild);

        if(Node->RightChild)
            DestroyNodeTree(Node->RightChild);

        free(Node);
        Node = NULL;
    }
}

void RotateTree(tree_node *Node, int Deg)
{
    if (Node == NULL || IsLeafNode(Node))
        return;

    DEBUG("RotateTree() " << Deg << " degrees");

    if((Deg == 90 && Node->SplitMode == SPLIT_VERTICAL) ||
       (Deg == 270 && Node->SplitMode == SPLIT_HORIZONTAL) ||
       Deg == 180)
    {
        tree_node *Temp = Node->LeftChild;
        Node->LeftChild = Node->RightChild;
        Node->RightChild = Temp;
        Node->SplitRatio = 1 - Node->SplitRatio;
    }

    if(Deg != 180)
        Node->SplitMode = Node->SplitMode == SPLIT_HORIZONTAL ? SPLIT_VERTICAL : SPLIT_HORIZONTAL;

    RotateTree(Node->LeftChild, Deg);
    RotateTree(Node->RightChild, Deg);
}

void FillDeserializedTree(tree_node *RootNode)
{
    std::vector<window_info*> Windows = GetAllWindowsOnDisplay(KWMScreen.Current->ID);
    tree_node *Current = NULL;
    GetFirstLeafNode(RootNode, (void**)&Current);

    std::size_t Counter = 0, Leafs = 0;
    while(Current)
    {
        if(Counter < Windows.size())
            Current->WindowID = Windows[Counter++]->WID;

        Current = GetNearestTreeNodeToTheRight(Current);
        ++Leafs;
    }

    if(Leafs < Windows.size() && Counter < Windows.size())
    {
        tree_node *Root = RootNode;
        for(; Counter < Windows.size(); ++Counter)
        {
            while(!IsLeafNode(Root))
            {
                if(!IsLeafNode(Root->LeftChild) && IsLeafNode(Root->RightChild))
                    Root = Root->RightChild;
                else
                    Root = Root->LeftChild;
            }

            DEBUG("FillDeserializedTree() Create pair of leafs");
            CreateLeafNodePair(KWMScreen.Current, Root, Root->WindowID, Windows[Counter]->WID, GetOptimalSplitMode(Root));
            Root = RootNode;
        }
    }
}

void ChangeSplitRatio(double Value)
{
    if(Value > 0.0 && Value < 1.0)
    {
        DEBUG("ChangeSplitRatio() New Split-Ratio is " << Value);
        KWMScreen.SplitRatio = Value;
    }
}

