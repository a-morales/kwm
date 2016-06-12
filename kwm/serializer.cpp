#include "serializer.h"
#include "container.h"
#include "node.h"
#include "tree.h"
#include "space.h"
#include "border.h"
#include "helpers.h"
#include "axlib/display.h"

#define internal static

internal void SerializeParentNode(tree_node *Parent, std::string Role, std::vector<std::string> &Serialized);

internal tree_node * DeserializeNodeTree(std::vector<std::string> &Serialized, ax_display *Display);
internal unsigned int DeserializeParentNode(tree_node *Parent, ax_display *Display, std::vector<std::string> &Serialized, unsigned int Index);
internal unsigned int DeserializeChildNode(tree_node *Parent, ax_display *Display, std::vector<std::string> &Serialized, unsigned int Index);

internal void
SerializeParentNode(tree_node *Parent, std::string Role, std::vector<std::string> &Serialized)
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

internal unsigned int
DeserializeParentNode(tree_node *Parent, ax_display *Display, std::vector<std::string> &Serialized, unsigned int Index)
{
    unsigned int LineNumber = Index;
    for(;LineNumber < Serialized.size(); ++LineNumber)
    {
        std::string Line = Serialized[LineNumber];
        std::vector<std::string> Tokens = SplitString(Line, ' ');

        if(Tokens[2] == "split-mode")
        {
            Parent->SplitMode = (split_type)ConvertStringToInt(Tokens[3]);
            DEBUG("Root: SplitMode Found " + Tokens[3]);
        }
        else if(Tokens[2] == "split-ratio")
        {
            Parent->SplitRatio = ConvertStringToDouble(Tokens[3]);
            DEBUG("Root: SplitRatio Found " + Tokens[3]);
        }
        else if(Tokens[2] == "child")
        {
            DEBUG("Root: Child Found");
            DEBUG("Parent: " << Parent->SplitMode << "|" << Parent->SplitRatio);
            LineNumber = DeserializeChildNode(Parent, Display, Serialized, LineNumber+1);
        }

        if(Parent->RightChild)
            return LineNumber;
    }

    return LineNumber;
}

internal unsigned int
DeserializeChildNode(tree_node *Parent, ax_display *Display, std::vector<std::string> &Serialized, unsigned int Index)
{
    unsigned int LineNumber = Index;
    for(;LineNumber < Serialized.size(); ++LineNumber)
    {
        std::string Line = Serialized[LineNumber];
        if(Line == "kwmc tree root create left")
        {
            DEBUG("Child: Create root");
            Parent->LeftChild = CreateLeafNode(Display, Parent, -1, 1);
            CreateDeserializedNodeContainer(Display, Parent->LeftChild);
            LineNumber = DeserializeParentNode(Parent->LeftChild, Display, Serialized, LineNumber+1);
            return LineNumber;
        }
        else if(Line == "kwmc tree root create right")
        {
            DEBUG("Child: Create root");
            Parent->RightChild = CreateLeafNode(Display, Parent, -1, 2);
            CreateDeserializedNodeContainer(Display, Parent->RightChild);
            LineNumber = DeserializeParentNode(Parent->RightChild, Display, Serialized, LineNumber+1);
            return LineNumber;
        }
        else if(Line == "kwmc tree leaf create left")
        {
            DEBUG("Child: Create left leaf");
            Parent->LeftChild = CreateLeafNode(Display, Parent, -1, 1);
            CreateDeserializedNodeContainer(Display, Parent->LeftChild);
            return LineNumber;
        }
        else if(Line == "kwmc tree leaf create right")
        {
            DEBUG("Child: Create right leaf");
            Parent->RightChild = CreateLeafNode(Display, Parent, -1, 2);
            CreateDeserializedNodeContainer(Display, Parent->RightChild);
            return LineNumber;
        }
    }

    return LineNumber;
}

internal tree_node *
DeserializeNodeTree(std::vector<std::string> &Serialized, ax_display *Display)
{
    if(Serialized.empty() || Serialized[0] != "kwmc tree root create parent")
        return NULL;

    DEBUG("Deserialize: Create Master");
    tree_node *RootNode = CreateRootNode();
    SetRootNodeContainer(Display, RootNode);
    DeserializeParentNode(RootNode, Display, Serialized, 1);
    return RootNode;
}

/* NOTE(koekeishiya): This is outdated and does not work with ax_display. */
void SaveBSPTreeToFile(screen_info *Screen, std::string Name)
{
    if(IsSpaceInitializedForScreen(Screen))
    {
        space_info *Space = GetActiveSpaceOfScreen(Screen);
        if(Space->Settings.Mode != SpaceModeBSP || IsLeafNode(Space->RootNode))
            return;

        struct stat Buffer;
        std::string TempPath = KWMPath.EnvHome + "/" + KWMPath.ConfigFolder + "/" + KWMPath.BSPLayouts;
        if (stat(TempPath.c_str(), &Buffer) == -1)
            mkdir(TempPath.c_str(), 0700);

        std::ofstream OutFD(TempPath + "/" + Name);
        if(OutFD.fail())
            return;

        tree_node *Root = Space->RootNode;
        std::vector<std::string> SerializedTree;
        SerializeParentNode(Root, "parent", SerializedTree);

        for(std::size_t LineNumber = 0; LineNumber < SerializedTree.size(); ++LineNumber)
            OutFD << SerializedTree[LineNumber] << std::endl;

        OutFD.close();
    }
}

void LoadBSPTreeFromFile(screen_info *Screen, std::string Name) { }

void LoadBSPTreeFromFile(ax_display *Display, space_info *SpaceInfo, std::string Name)
{
    if(SpaceInfo->Settings.Mode != SpaceModeBSP)
        return;

    std::string TempPath = KWMPath.EnvHome + "/" + KWMPath.ConfigFolder + "/" + KWMPath.BSPLayouts;
    std::ifstream InFD(TempPath + "/" + Name);
    if(InFD.fail())
        return;

    std::string Line;
    std::vector<std::string> SerializedTree;
    while(std::getline(InFD, Line))
        SerializedTree.push_back(Line);

    DestroyNodeTree(SpaceInfo->RootNode);
    SpaceInfo->RootNode = DeserializeNodeTree(SerializedTree, Display);
}
