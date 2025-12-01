// parser.c
// Tokenizer, parser, and evaluator. Exposes parse_line, eval_tree, print_infix, free_tree.
// Uses the tree_node.c helpers and symtab functions.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "stack.h"
#include "tree_node.h"
#include "symtab.h"
#include "interp.h"   // for MAX_LINE

/* Helper: duplicate string safely */
static char *strdup_safe(const char *s) {
    if (!s) return NULL;
    char *r = strdup(s);
    return r;
}

/* Tokenizer: split a line by whitespace and push tokens left->right onto stack.
   We push tokens in left-to-right order so the top of stack is the last token (rightmost).
   Caller is responsible for freeing the token strings remaining on the stack or for using them to build tree.
*/
static int tokenize_line(const char *line, stack_t *tokstack) {
    if (!line || !tokstack) return 0;
    char *copy = strdup_safe(line);
    if (!copy) return 0;
    char *saveptr;
    char *tok = strtok_r(copy, " \t\r\n", &saveptr);
    while (tok) {
        char *s = strdup_safe(tok);
        if (!s) { free(copy); return 0; }
        push(tokstack, s);
        tok = strtok_r(NULL, " \t\r\n", &saveptr);
    }
    free(copy);
    return 1;
}

/* classify token */
static int is_op(const char *t) {
    if (!t || strlen(t) != 1) return 0;
    char c = t[0];
    return (c=='+'||c=='-'||c=='*'||c=='/'||c=='%'||c=='='||c=='?');
}
static int is_integer_token(const char *t) {
    if (!t) return 0;
    if (!isdigit((unsigned char)t[0])) return 0;
    for (size_t i=1;i<strlen(t);++i) if (!isdigit((unsigned char)t[i])) return 0;
    return 1;
}
static int is_symbol_token(const char *t) {
    if (!t) return 0;
    if (!isalpha((unsigned char)t[0])) return 0;
    for (size_t i=1;i<strlen(t);++i) if (!isalnum((unsigned char)t[i])) return 0;
    return 1;
}

/* forward declaration */
tree_node_t *parse_from_stack(stack_t *tokstack, char **err);

/* parse_line public: tokenizes and builds parse tree; returns root or NULL and sets err message */
tree_node_t *parse_line(const char *line, char **err) {
    if (!line) {
        if (err) *err = strdup_safe("Invalid expression, not enough tokens");
        return NULL;
    }
    stack_t *tokstack = make_stack();
    if (!tokstack) { if (err) *err = strdup_safe("Out of memory"); return NULL; }
    if (!tokenize_line(line, tokstack)) {
        free_stack(tokstack);
        if (err) *err = strdup_safe("Out of memory");
        return NULL;
    }
    // empty?
    if (empty_stack(tokstack)) {
        free_stack(tokstack);
        if (err) *err = strdup_safe("Invalid expression, not enough tokens");
        return NULL;
    }
    tree_node_t *root = parse_from_stack(tokstack, err);
    if (!root) { // parse error, tokstack top tokens must be freed
        // free remaining token strings
        while (!empty_stack(tokstack)) {
            char *s = top(tokstack);
            free(s);
            pop(tokstack);
        }
        free_stack(tokstack);
        return NULL;
    }
    // after parse, token stack should be empty
    if (!empty_stack(tokstack)) {
        // too many tokens: clean token strings
        while (!empty_stack(tokstack)) {
            char *s = top(tokstack);
            free(s);
            pop(tokstack);
        }
        free_stack(tokstack);
        free_tree(root);
        if (err) *err = strdup_safe("Invalid expression, too many tokens");
        return NULL;
    }
    free_stack(tokstack);
    return root;
}

/* parse_from_stack: the core recursive parser.
   Pops one token; if op -> parse right then left; if '?' parse c,b,a as in spec.
*/
tree_node_t *parse_from_stack(stack_t *tokstack, char **err) {
    if (!tokstack) {
        if (err) *err = strdup_safe("Internal parser error");
        return NULL;
    }
    if (empty_stack(tokstack)) {
        if (err) *err = strdup_safe("Invalid expression, not enough tokens");
        return NULL;
    }
    char *token = top(tokstack);
    pop(tokstack);
    if (!token) {
        if (err) *err = strdup_safe("Invalid expression, not enough tokens");
        return NULL;
    }

    if (is_op(token)) {
        char opch = token[0];
        if (opch == '?') {
            // parse c (false), b (true), a (test) in that order (they are on the stack)
            tree_node_t *c = parse_from_stack(tokstack, err);
            if (!c) { free(token); return NULL; }
            tree_node_t *b = parse_from_stack(tokstack, err);
            if (!b) { free_tree(c); free(token); return NULL; }
            tree_node_t *a = parse_from_stack(tokstack, err);
            if (!a) { free_tree(c); free_tree(b); free(token); return NULL; }
            // build ALT node ":" with ALT_OP
            tree_node_t *alt = make_interior(ALT_OP, ":", b, c);
            if (!alt) { free_tree(a); free_tree(b); free_tree(c); free(token); return NULL; }
            tree_node_t *q = make_interior(Q_OP, "?", a, alt);
            free(token);
            return q;
        } else {
            // binary operator
            tree_node_t *right = parse_from_stack(tokstack, err);
            if (!right) { free(token); return NULL; }
            tree_node_t *left  = parse_from_stack(tokstack, err);
            if (!left) { free_tree(right); free(token); return NULL; }
            op_type_t opt = NO_OP;
            switch (opch) {
                case '+': opt = ADD_OP; break;
                case '-': opt = SUB_OP; break;
                case '*': opt = MUL_OP; break;
                case '/': opt = DIV_OP; break;
                case '%': opt = MOD_OP; break;
                case '=': opt = ASSIGN_OP; break;
                default: opt = NO_OP; break;
            }
            tree_node_t *n = make_interior(opt, token, left, right);
            free(token);
            return n;
        }
    } else {
        // not operator: integer literal or symbol
        if (is_integer_token(token)) {
            tree_node_t *leaf = make_leaf(INTEGER, token);
            free(token);
            return leaf;
        } else if (is_symbol_token(token)) {
            tree_node_t *leaf = make_leaf(SYMBOL, token);
            free(token);
            return leaf;
        } else {
            // illegal token
            size_t m = strlen(token) + 32;
            char *msg = malloc(m);
            if (msg) {
                snprintf(msg, m, "Illegal token");
                if (err) *err = msg;
                else free(msg);
            } else {
                if (err) *err = strdup_safe("Illegal token");
            }
            free(token);
            return NULL;
        }
    }
}

/* Evaluate a tree; returns 1 success and places value in out; returns 0 on error and sets err message.
   Uses symtab functions to lookup/create symbols for assignment.
*/
int eval_tree(tree_node_t *root, long *out, char **err) {
    if (!root) { if (err) *err = strdup_safe("Unknown node type"); return 0; }
    if (root->type == LEAF) {
        leaf_node_t *ln = (leaf_node_t*)root->node;
        if (ln->exp_type == INTEGER) {
            long v = strtol(root->token, NULL, 10);
            *out = v;
            return 1;
        } else if (ln->exp_type == SYMBOL) {
            symbol_t *s = lookup_table(root->token);
            if (!s) {
                size_t m = strlen(root->token) + 32;
                if (err) {
                    char *msg = malloc(m);
                    snprintf(msg, m, "Undefined symbol");
                    *err = msg;
                }
                return 0;
            }
            *out = s->val;
            return 1;
        } else {
            if (err) *err = strdup_safe("Unknown leaf type");
            return 0;
        }
    } else if (root->type == INTERIOR) {
        interior_node_t *in = (interior_node_t*)root->node;
        if (in->op == Q_OP) {
            // left is test, right is ALT_OP node
            long testv;
            if (!eval_tree(in->left, &testv, err)) return 0;
            // right should be an ALT_OP interior node
            if (!in->right || in->right->type != INTERIOR) {
                if (err) *err = strdup_safe("Unknown node type");
                return 0;
            }
            interior_node_t *alt = (interior_node_t*)in->right->node;
            if (testv) {
                return eval_tree(alt->left, out, err);
            } else {
                return eval_tree(alt->right, out, err);
            }
        } else if (in->op == ASSIGN_OP) {
            // left must be a SYMBOL leaf (l-value)
            if (!in->left || in->left->type != LEAF) { if (err) *err = strdup_safe("Missing l-value"); return 0; }
            leaf_node_t *ln = (leaf_node_t*)in->left->node;
            if (ln->exp_type != SYMBOL) { if (err) *err = strdup_safe("Invalid l-value"); return 0; }
            long rval;
            if (!eval_tree(in->right, &rval, err)) return 0;
            // update existing or create new symbol
            symbol_t *ent = lookup_table(in->left->token);
            if (ent) {
                ent->val = (int)rval;
            } else {
                if (!create_symbol(in->left->token, (int)rval)) {
                    if (err) *err = strdup_safe("No room in symbol table");
                    return 0;
                }
            }
            *out = rval;
            return 1;
        } else {
            // binary ops: evaluate left then right
            long lv, rv;
            if (!eval_tree(in->left, &lv, err)) return 0;
            if (!eval_tree(in->right, &rv, err)) return 0;
            switch (in->op) {
                case ADD_OP: *out = lv + rv; return 1;
                case SUB_OP: *out = lv - rv; return 1;
                case MUL_OP: *out = lv * rv; return 1;
                case DIV_OP:
                    if (rv == 0) { if (err) *err = strdup_safe("Division by zero"); return 0; }
                    *out = lv / rv; return 1;
                case MOD_OP:
                    if (rv == 0) { if (err) *err = strdup_safe("Division by zero"); return 0; }
                    *out = lv % rv; return 1;
                default:
                    if (err) *err = strdup_safe("Unknown operation");
                    return 0;
            }
        }
    } else {
        if (err) *err = strdup_safe("Unknown node type");
        return 0;
    }
}
