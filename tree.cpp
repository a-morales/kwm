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

tree_node *CreateTreeFromWindowIDList(screen_info *Screen, std::vector<int> *Windows)
{
    tree_node *RootNode = (tree_node*) malloc(sizeof(tree_node));
    std::memset(RootNode, '\0', sizeof(tree_node));
    
    RootNode->Parent = NULL;
    RootNode->WindowID = -1;
    RootNode->Container.X = Screen->PaddingLeft;
    RootNode->Container.Y = Screen->PaddingTop,
    RootNode->Container.Width = Screen->Width - Screen->PaddingRight;
    RootNode->Container.Height = Screen->Height - Screen->PaddingTop - Screen->PaddingBottom;

    node_container LeftContainer;
    LeftContainer.X = RootNode->Container.X;
    LeftContainer.Y = RootNode->Container.Y;
    LeftContainer.Width = (RootNode->Container.Width / 2) - (Screen->PaddingLeft / 2);
    LeftContainer.Height = RootNode->Container.Height;

    tree_node *LeftChild = (tree_node*) malloc(sizeof(tree_node));
    LeftChild->Parent = RootNode;
    LeftChild->WindowID = (*Windows)[0];
    LeftChild->Container = LeftContainer;
    LeftChild->LeftChild = NULL;
    LeftChild->RightChild = NULL;

    node_container RightContainer;
    RightContainer.X = (RootNode->Container.Width / 2) + (Screen->PaddingLeft);
    RightContainer.Y = RootNode->Container.Y;
    RightContainer.Width = RootNode->Container.Width / 2 - (Screen->PaddingRight);
    RightContainer.Height = RootNode->Container.Height;

    tree_node *RightChild = (tree_node*) malloc(sizeof(tree_node));
    RightChild->Parent = RootNode;
    RightChild->WindowID = (*Windows)[1];
    RightChild->Container = RightContainer;
    RightChild->LeftChild = NULL;
    RightChild->RightChild = NULL;

    RootNode->LeftChild = LeftChild;
    RootNode->RightChild = RightChild;
    return RootNode;
}

void ApplyNodeContainer(tree_node *Node)
{
    ResizeWindow(Node);
}
