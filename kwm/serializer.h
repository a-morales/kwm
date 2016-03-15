#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "types.h"

void SerializeParentNode(tree_node *Parent, std::string Role, std::vector<std::string> &Serialized);
unsigned int DeserializeParentNode(tree_node *Parent, std::vector<std::string> &Serialized, unsigned int Index);
unsigned int DeserializeChildNode(tree_node *Parent, std::vector<std::string> &Serialized, unsigned int Index);
tree_node *DeserializeNodeTree(std::vector<std::string> &Serialized);

void SaveBSPTreeToFile(screen_info *Screen, std::string Name);
void LoadBSPTreeFromFile(screen_info *Screen, std::string Name);

#endif
