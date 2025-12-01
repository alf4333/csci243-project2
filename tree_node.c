// tree_node.c
// Implements make_interior and make_leaf per tree_node.h
// Also implements free_tree and print_infix helpers used by parser/interp.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree_node.h"

/* Create a leaf node (stores type in leaf_node_s). We use the tree_node_t->node
   pointer to point to a leaf_node_t structure. */
tree_node_t *make_leaf(exp_type_t exp_type, char *token) {
    if (!token) return NULL;
    tree_node_t *tn = malloc(sizeof(*tn));
    if (!tn) {
        perror("make_leaf malloc tn");
        return NULL;
    }
    tn->type = LEAF;
    tn->token = strdup(token);
    if (!tn->token) { free(tn); return NULL; }

    leaf_node_t *leaf = malloc(sizeof(*leaf));
    if (!leaf) { free(tn->token); free(tn); return NULL; }
    leaf->exp_type = exp_type;
    tn->node = leaf;
    return tn;
}

/* Create an interior node: allocate interior_node_t and set fields
   token string is stored in tree_node_t->token. */
tree_node_t *make_interior(op_type_t op, char *token,
                           tree_node_t *left, tree_node_t *right) {
    if (!token) return NULL;
    tree_node_t *tn = malloc(sizeof(*tn));
    if (!tn) {
        perror("make_interior malloc tn");
        return NULL;
    }
    tn->type = INTERIOR;
    tn->token = strdup(token);
    if (!tn->token) { free(tn); return NULL; }

    interior_node_t *in = malloc(sizeof(*in));
    if (!in) { free(tn->token); free(tn); return NULL; }
    in->op = op;
    in->left = left;
    in->right = right;
    tn->node = in;
    return tn;
}

/* recursively free a tree_node_t */
void free_tree(tree_node_t *root) {
    if (!root) return;
    if (root->type == INTERIOR) {
        interior_node_t *in = (interior_node_t*)root->node;
        if (in->left) free_tree(in->left);
        if (in->right) free_tree(in->right);
        free(in);
    } else if (root->type == LEAF) {
        leaf_node_t *ln = (leaf_node_t*)root->node;
        free(ln);
    }
    free(root->token);
    free(root);
}

/* print fully-parenthesized infix representation (no whitespace) */
void print_infix(tree_node_t *root) {
    if (!root) return;
    if (root->type == LEAF) {
        printf("%s", root->token);
    } else {
        interior_node_t *in = (interior_node_t*)root->node;
        printf("(");
        print_infix(in->left);
        printf("%s", root->token);
        print_infix(in->right);
        printf(")");
    }
}
