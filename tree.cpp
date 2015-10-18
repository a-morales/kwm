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

tree_node *CreateTreeFromWindowIDList(std::vector<int> *Windows)
{
    tree_node *RootNode = (tree_node*) malloc(sizeof(tree_node));
    std::memset(&RootNode, '\0', sizeof(tree_node));
    RootNode->Parent = NULL;

    tree_node *ParentNode = NULL;


    return RootNode;
}

void ApplyNodeContainer(tree_node *Node)
{
    ResizeWindow(Node);
}
