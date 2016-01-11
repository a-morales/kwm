#include "kwm.h"

extern kwm_path KWMPath;
extern kwm_screen KWMScreen;

node_container LeftVerticalContainerSplit(screen_info *Screen, tree_node *Node)
{
    space_info *Space = &Screen->Space[Screen->ActiveSpace];
    node_container LeftContainer;

    LeftContainer.X = Node->Container.X;
    LeftContainer.Y = Node->Container.Y;
    LeftContainer.Width = (Node->Container.Width * Node->SplitRatio) - (Space->Offset.VerticalGap / 2);
    LeftContainer.Height = Node->Container.Height;
    
    return LeftContainer;
}

node_container RightVerticalContainerSplit(screen_info *Screen, tree_node *Node)
{
    space_info *Space = &Screen->Space[Screen->ActiveSpace];
    node_container RightContainer;

    RightContainer.X = Node->Container.X + (Node->Container.Width * Node->SplitRatio) + (Space->Offset.VerticalGap / 2);
    RightContainer.Y = Node->Container.Y;
    RightContainer.Width = (Node->Container.Width * (1 - Node->SplitRatio)) - (Space->Offset.VerticalGap / 2);
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
    UpperContainer.Height = (Node->Container.Height * Node->SplitRatio) - (Space->Offset.HorizontalGap / 2);

    return UpperContainer;
}

node_container LowerHorizontalContainerSplit(screen_info *Screen, tree_node *Node)
{
    space_info *Space = &Screen->Space[Screen->ActiveSpace];
    node_container LowerContainer;

    LowerContainer.X = Node->Container.X;
    LowerContainer.Y = Node->Container.Y + (Node->Container.Height * Node->SplitRatio) + (Space->Offset.HorizontalGap / 2);
    LowerContainer.Width = Node->Container.Width;
    LowerContainer.Height = (Node->Container.Height * (1 - Node->SplitRatio)) - (Space->Offset.HorizontalGap / 2);

    return LowerContainer;
}

void CreateNodeContainer(screen_info *Screen, tree_node *Node, int ContainerType)
{
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
    RootNode->SplitRatio = KWMScreen.SplitRatio;
    RootNode->SplitMode = 0;

    return RootNode;
}

void SetRootNodeContainer(screen_info *Screen, tree_node *Node)
{
    space_info *Space = &Screen->Space[Screen->ActiveSpace];

    Node->Container.X = Screen->X + Space->Offset.PaddingLeft;
    Node->Container.Y = Screen->Y + Space->Offset.PaddingTop;
    Node->Container.Width = Screen->Width - Space->Offset.PaddingLeft - Space->Offset.PaddingRight;
    Node->Container.Height = Screen->Height - Space->Offset.PaddingTop - Space->Offset.PaddingBottom;
    Node->SplitMode = GetOptimalSplitMode(Node);

    Node->Container.Type = 0;
}

void CreateLeafNodePair(screen_info *Screen, tree_node *Parent, int LeftWindowID, int RightWindowID, int SplitMode)
{
    Parent->WindowID = -1;
    Parent->SplitMode = SplitMode;
    Parent->SplitRatio = KWMScreen.SplitRatio;

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

tree_node *GetFirstLeafNode(tree_node *Node)
{
    if(Node)
    {
        while(Node->LeftChild)
            Node = Node->LeftChild;

        return Node;
    }

    return NULL;
}

tree_node *GetLastLeafNode(tree_node *Node)
{
    if(Node)
    {
        while(Node->RightChild)
            Node = Node->RightChild;

        return Node;
    }

    return NULL;
}

tree_node *GetFirstPseudoLeafNode(tree_node *Node)
{
    tree_node *Leaf = GetFirstLeafNode(Node);
    while(Leaf && Leaf->WindowID != -1)
        Leaf = GetNearestNodeToTheRight(Leaf, SpaceModeBSP);

    return Leaf;
}

bool IsLeftChild(tree_node *Node)
{
    if(Node && IsLeafNode(Node))
    {
        tree_node *Parent = Node->Parent;
        return Parent->LeftChild == Node;
    }

    return false;
}

bool IsRightChild(tree_node *Node)
{
    if(Node && IsLeafNode(Node))
    {
        tree_node *Parent = Node->Parent;
        return Parent->RightChild == Node;
    }

    return false;
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
    else if(Space->Mode == SpaceModeMonocle)
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

bool CreateMonocleTree(tree_node *RootNode, screen_info *Screen, std::vector<window_info*> *WindowsPtr)
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

void ChangeSplitRatio(double Value)
{
    if(Value > 0.0 && Value < 1.0)
    {
        DEBUG("ChangeSplitRatio() New Split-Ratio is " << Value)
        KWMScreen.SplitRatio = Value;
    }
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

tree_node *GetNearestLeafNeighbour(tree_node *Node, space_tiling_option Mode)
{
    if(Node && IsLeafNode(Node))
    {
        if(Mode == SpaceModeBSP)
            return IsLeftChild(Node) ? GetNearestNodeToTheRight(Node, Mode) : GetNearestNodeToTheLeft(Node, Mode);
        else if(Mode == SpaceModeMonocle)
        {
            return Node->LeftChild ? Node->LeftChild : Node->RightChild;
        }
    }

    return NULL;
}

tree_node *GetNodeFromWindowID(tree_node *Node, int WindowID, space_tiling_option Mode)
{
    if(Node)
    {
        tree_node *CurrentNode = GetFirstLeafNode(Node);;
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
        else if(Mode == SpaceModeMonocle)
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
        else if(Mode == SpaceModeMonocle)
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
        Node->SplitRatio = 1 - Node->SplitRatio;
    }

    if(Deg != 180)
        Node->SplitMode = Node->SplitMode == 2 ? 1 : 2;

    RotateTree(Node->LeftChild, Deg);
    RotateTree(Node->RightChild, Deg);
}

void CreateDeserializedNodeContainer(tree_node *Node)
{
    int SplitMode = Node->Parent->SplitMode;
    int ContainerType = 0;

    if(SplitMode == 1)
        ContainerType = IsLeftChild(Node) ? 1 : 2;
    else
        ContainerType = IsLeftChild(Node) ? 3 : 4;

    CreateNodeContainer(KWMScreen.Current, Node, ContainerType);
}

void FillDeserializedTree(tree_node *RootNode)
{
    std::vector<window_info*> Windows = GetAllWindowsOnDisplay(KWMScreen.Current->ID);

    tree_node *Current = GetFirstLeafNode(RootNode);
    tree_node *Root = RootNode;

    int Counter = 0, Leafs = 0;
    while(Current)
    {
        if(Counter < Windows.size())
            Current->WindowID = Windows[Counter++]->WID;

        Current = GetNearestNodeToTheRight(Current, SpaceModeBSP);
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

            DEBUG("FillDeserializedTree() Create pair of leafs")
            CreateLeafNodePair(KWMScreen.Current, Root, Root->WindowID, Windows[Counter]->WID, GetOptimalSplitMode(Root));
            Root = RootNode;
        }
    }
}

int ConvertStringToInt(std::string Integer)
{
    int SplitMode;
    std::stringstream Stream(Integer);
    Stream >> SplitMode;
    return SplitMode;
}

double ConvertStringToDouble(std::string Double)
{
    double SplitRatio;
    std::stringstream Stream(Double);
    Stream >> SplitRatio;
    return SplitRatio;
}

void SerializeParentNode(tree_node *Parent, std::string Role, std::vector<std::string> &Serialized)
{
    Serialized.push_back("kwmc tree root create " + Role);
    Serialized.push_back("kwmc tree split-mode " + std::to_string(Parent->SplitMode));
    Serialized.push_back("kwmc tree split-ratio " + std::to_string(Parent->SplitRatio));

    if(IsLeafNode(Parent->LeftChild))
    {
        Serialized.push_back("kwmc tree child");
        Serialized.push_back("kwmc tree leaf create left");
    }
    else
    {
        Serialized.push_back("kwmc tree child");
        SerializeParentNode(Parent->LeftChild, "left", Serialized);
    }

    if(IsLeafNode(Parent->RightChild))
    {
        Serialized.push_back("kwmc tree child");
        Serialized.push_back("kwmc tree leaf create right");
    }
    else
    {
        Serialized.push_back("kwmc tree child");
        SerializeParentNode(Parent->RightChild, "right", Serialized);
    }
}

int DeserializeParentNode(tree_node *Parent, std::vector<std::string> &Serialized, int Index)
{
    int LineNumber = Index;
    for(;LineNumber < Serialized.size(); ++LineNumber)
    {
        std::string Line = Serialized[LineNumber];
        std::vector<std::string> Tokens = SplitString(Line, ' ');

        if(Tokens[2] == "split-mode")
        {
            Parent->SplitMode = ConvertStringToInt(Tokens[3]);
            DEBUG("Root: SplitMode Found " + Tokens[3])
        }
        else if(Tokens[2] == "split-ratio")
        {
            Parent->SplitRatio = ConvertStringToDouble(Tokens[3]);
            DEBUG("Root: SplitRatio Found " + Tokens[3])
        }
        else if(Tokens[2] == "child")
        {
            DEBUG("Root: Child Found")
            DEBUG("Parent: " << Parent->SplitMode << "|" << Parent->SplitRatio)
            LineNumber = DeserializeChildNode(Parent, Serialized, LineNumber+1);
        }

        if(Parent->RightChild)
            return LineNumber;
    }

    return LineNumber;
}

int DeserializeChildNode(tree_node *Parent, std::vector<std::string> &Serialized, int Index)
{
    int LineNumber = Index;
    for(;LineNumber < Serialized.size(); ++LineNumber)
    {
        std::string Line = Serialized[LineNumber];
        if(Line == "kwmc tree root create left")
        {
            DEBUG("Child: Create root")
            Parent->LeftChild = CreateLeafNode(KWMScreen.Current, Parent, -1, 1);
            CreateDeserializedNodeContainer(Parent->LeftChild);
            LineNumber = DeserializeParentNode(Parent->LeftChild, Serialized, LineNumber+1);
            return LineNumber;
        }
        else if(Line == "kwmc tree root create right")
        {
            DEBUG("Child: Create root")
            Parent->RightChild = CreateLeafNode(KWMScreen.Current, Parent, -1, 2);
            CreateDeserializedNodeContainer(Parent->RightChild);
            LineNumber = DeserializeParentNode(Parent->RightChild, Serialized, LineNumber+1);
            return LineNumber;
        }
        else if(Line == "kwmc tree leaf create left")
        {
            DEBUG("Child: Create left leaf")
            Parent->LeftChild = CreateLeafNode(KWMScreen.Current, Parent, -1, 1);
            CreateDeserializedNodeContainer(Parent->LeftChild);
            return LineNumber;
        }
        else if(Line == "kwmc tree leaf create right")
        {
            DEBUG("Child: Create right leaf")
            Parent->RightChild = CreateLeafNode(KWMScreen.Current, Parent, -1, 2);
            CreateDeserializedNodeContainer(Parent->RightChild);
            return LineNumber;
        }
    }

    return LineNumber;
}

tree_node *DeserializeNodeTree(std::vector<std::string> &Serialized)
{
    if(Serialized.empty() || Serialized[0] != "kwmc tree root create parent")
        return NULL;

    DEBUG("Deserialize: Create Master")
    tree_node *RootNode = CreateRootNode();
    SetRootNodeContainer(KWMScreen.Current, RootNode);
    DeserializeParentNode(RootNode, Serialized, 1);
    return RootNode;
}

void SaveBSPTreeToFile(screen_info *Screen, std::string Name)
{
    if(IsSpaceInitializedForScreen(Screen))
    {
        space_info *Space = &Screen->Space[Screen->ActiveSpace];
        if(Space->Mode != SpaceModeBSP || IsLeafNode(Space->RootNode))
            return;

        std::string TempPath = KWMPath.EnvHome + "/" + KWMPath.ConfigFolder;
        std::ofstream OutFD(TempPath + "/" + Name);
        if(OutFD.fail())
            return;

        tree_node *Root = Space->RootNode;
        std::vector<std::string> SerializedTree;
        SerializeParentNode(Root, "parent", SerializedTree);

        for(int LineNumber = 0; LineNumber < SerializedTree.size(); ++LineNumber)
            OutFD << SerializedTree[LineNumber] << std::endl;

        OutFD.close();
    }
}

void LoadBSPTreeFromFile(screen_info *Screen, std::string Name)
{
    if(IsSpaceInitializedForScreen(Screen))
    {
        space_info *Space = &Screen->Space[Screen->ActiveSpace];
        if(Space->Mode != SpaceModeBSP)
            return;

        std::string TempPath = KWMPath.EnvHome + "/" + KWMPath.ConfigFolder;
        std::ifstream InFD(TempPath + "/" + Name);
        if(InFD.fail())
            return;

        std::string Line;
        std::vector<std::string> SerializedTree;
        while(std::getline(InFD, Line))
            SerializedTree.push_back(Line);

        DestroyNodeTree(Space->RootNode, SpaceModeBSP);
        Space->RootNode = DeserializeNodeTree(SerializedTree);
        FillDeserializedTree(Space->RootNode);
        ApplyNodeContainer(Space->RootNode, SpaceModeBSP);
    }
}
