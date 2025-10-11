#pragma once
#include "../linear.h"

struct Node {
    uint8_t type;
    uint8_t value;
    Node*  children[];
};

size_t inline alignment_helper (size_t act_len, size_t align_len) {
    return ((act_len + (align_len - 1)) & ~(align_len-1));
}

void build_graph_lin_alloc (int num_children, int num_layers, int align_len, LinearAlloc* arena) {

    std::vector<Node*> layer_nodes;
    Node* root = (Node*) arena->allocate(alignment_helper(2 + num_children * sizeof(Node*), align_len), align_len);

    layer_nodes.push_back(root);
    for (int j = 1; j < num_layers; j++) {

        std::vector<Node*> next_layer_nodes;
        for (int k = 0; k < layer_nodes.size(); k++) {

            for (int i = 0; i < num_children; i++) {
                Node* child = (Node*) arena->allocate(alignment_helper(2 + num_children * sizeof(Node*), align_len), align_len);
                layer_nodes[k]->children[i] = child;
                next_layer_nodes.push_back(child);
            }
        }
        layer_nodes = std::move(next_layer_nodes);
    }
}

void build_graph_malloc (int num_children, int num_layers) {
    std::vector<Node*> layer_nodes;
    Node* root = (Node*) malloc(2 + num_children * sizeof(Node*));

    layer_nodes.push_back(root);
    for (int j = 1; j < num_layers; j++) {

        std::vector<Node*> next_layer_nodes;
        for (int k = 0; k < layer_nodes.size(); k++) {

            for (int i = 0; i < num_children; i++) {
                Node* child = (Node*) malloc(2 + num_children * sizeof(Node*));
                layer_nodes[k]->children[i] = child;
                next_layer_nodes.push_back(child);
            }
        }
        layer_nodes = std::move(next_layer_nodes);
    }
}