#pragma once
#include "../linear.h"

struct Node {
    uint8_t type;
    uint8_t value;
    Node** children;
};

void* allocate_node (int num_children, LinearAlloc* arena) {
    return arena->allocate(2 + num_children * sizeof(Node*));
}

void build_graph_lin_alloc (int num_children, int num_layers, LinearAlloc* arena) {

    std::vector<Node*> layer_nodes;
    Node* root = (Node*) allocate_node(num_children, arena);

    layer_nodes.push_back(root);
    for (int j = 0; j < num_layers; j++) {

        std::vector<Node*> next_layer_nodes;
        for (int k = 0; k < layer_nodes.size(); k++) {

            for (int i = 0; i < num_children; i++) {
                Node* child = (Node*) allocate_node(num_children, arena);
                child->type = 1;
                child->value = 1;
                layer_nodes[k]->children[i] = child;
                next_layer_nodes.push_back(child);
            }
        }
        layer_nodes = std::move(next_layer_nodes);
    }
}

void build_graph_malloc () {
    
}