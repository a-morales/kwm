#include "kwm.h"

tree_node * CreateNode(int WindowID, node_container *Container, 
                       tree_node *Parent, tree_node *LeftChild, tree_node *RightChild)
{
    return CreateNode(WindowID, 
                      Container->X, Container->Y, Container->Width, Container->Height, 
                      Parent, LeftChild, RightChild);
}

tree_node * CreateNode(int WindowID, int X, int Y, int Width, int Height, 
                       tree_node *Parent, tree_node *LeftChild, tree_node *RightChild)
{
    tree_node *NewNode = (tree_node*) malloc(sizeof(tree_node));

    NewNode->Container.X = X;
    NewNode->Container.Y = Y;
    NewNode->Container.Width = Width;
    NewNode->Container.Height = Height;

    NewNode->WindowID = WindowID;
    NewNode->Parent = Parent;
    NewNode->LeftChild = LeftChild;
    NewNode->RightChild = RightChild;
    
    return NewNode;
}

void DestroyNode(tree_node *Node)
{
    tree_node *ParentNode = Node->Parent;
    tree_node *LeftChildNode = Node->LeftChild;
    tree_node *RightChildNode = Node->RightChild;
    free(Node);
}

node_container LeftVerticalContainerSplit(screen_info *Screen, tree_node *Node)
{
    node_container LeftContainer;

    LeftContainer.X = Node->Container.X;
    LeftContainer.Y = Node->Container.Y;
    LeftContainer.Width = (Node->Container.Width / 2) - (Screen->PaddingLeft / 2);
    LeftContainer.Height = Node->Container.Height;
    
    return LeftContainer;
}

node_container RightVerticalContainerSplit(screen_info *Screen, tree_node *Node)
{
    node_container RightContainer;

    RightContainer.X = (Node->Container.Width / 2) + (Screen->PaddingLeft);
    RightContainer.Y = Node->Container.Y;
    RightContainer.Width = Node->Container.Width / 2 - (Screen->PaddingRight);
    RightContainer.Height = Node->Container.Height;

    return RightContainer;
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

    if(ContainerType == 0)
        Leaf->Container = FullscreenContainer(Screen, Leaf->Parent);
    else if(ContainerType == 1)
        Leaf->Container = LeftVerticalContainerSplit(Screen, Leaf->Parent);
    else if(ContainerType == 2)
        Leaf->Container = RightVerticalContainerSplit(Screen, Leaf->Parent);

    Leaf->LeftChild = NULL;
    Leaf->RightChild = NULL;

    return Leaf;
}

tree_node *CreateRootNode(screen_info *Screen)
{
    tree_node *RootNode = (tree_node*) malloc(sizeof(tree_node));
    std::memset(RootNode, '\0', sizeof(tree_node));

    RootNode->Parent = NULL;
    RootNode->WindowID = -1;
    RootNode->Container.X = Screen->PaddingLeft;
    RootNode->Container.Y = Screen->PaddingTop,
    RootNode->Container.Width = Screen->Width - Screen->PaddingRight;
    RootNode->Container.Height = Screen->Height - Screen->PaddingTop - Screen->PaddingBottom;

    return RootNode;
}

tree_node *CreateTreeFromWindowIDList(screen_info *Screen, std::vector<int> Windows)
{
    tree_node *RootNode = CreateRootNode(Screen);

    if(Windows.size() == 1)
    {
        RootNode->LeftChild = CreateLeafNode(Screen, RootNode, Windows[0], 0);
    }
    else if(Windows.size() == 2)
    {
        RootNode->LeftChild = CreateLeafNode(Screen, RootNode, Windows[0], 1);
        RootNode->RightChild = CreateLeafNode(Screen, RootNode, Windows[1], 2);
    }
    else
    {

    }


    ApplyNodeContainer(RootNode);

    return RootNode;
}

void ApplyNodeContainer(tree_node *Node)
{
    if(Node)
    {
        ResizeWindow(Node);
        if(Node->LeftChild)
        {
            ApplyNodeContainer(Node->LeftChild);
        }

        if(Node->RightChild)
        {
            ApplyNodeContainer(Node->RightChild);
        }
    }
}
