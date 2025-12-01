// symtab.c
// Implements symbol table API declared in symtab.h
// Uses singly-linked list of symbol_t nodes.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

static symbol_t *sym_head = NULL;

/* create and return a new symbol node (dynamically allocated) */
symbol_t *create_symbol(char *name, int val) {
    if (!name) return NULL;
    symbol_t *s = malloc(sizeof(*s));
    if (!s) {
        perror("create_symbol malloc");
        return NULL;
    }
    s->var_name = strdup(name);
    if (!s->var_name) {
        perror("create_symbol strdup");
        free(s);
        return NULL;
    }
    s->val = val;
    s->next = NULL;
    // append at head for simplicity
    s->next = sym_head;
    sym_head = s;
    return s;
}

/* lookup by name; return pointer or NULL */
symbol_t *lookup_table(char *variable) {
    if (!variable) return NULL;
    symbol_t *cur = sym_head;
    while (cur) {
        if (strcmp(cur->var_name, variable) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

/* build_table: read file and create symbol entries; exits on file open error */
void build_table(char *filename) {
    if (!filename) return; // no file -> leave empty table as spec says
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    char buf[BUFLEN];
    while (fgets(buf, sizeof(buf), f)) {
        // skip leading whitespace
        char *s = buf;
        while (*s && isspace((unsigned char)*s)) ++s;
        if (*s == '#' || *s == '\0' || *s == '\n') continue;
        // parse name and integer value
        char name[BUFLEN];
        int val;
        if (sscanf(s, "%1023s %d", name, &val) == 2) {
            // create_symbol will add to list
            if (!create_symbol(name, val)) {
                fprintf(stderr, "Error creating symbol %s\n", name);
                fclose(f);
                exit(EXIT_FAILURE);
            }
        } else {
            // malformed line — per header comment, file is guaranteed correct, but handle defensively
            fprintf(stderr, "Malformed symbol table line: %s", buf);
            fclose(f);
            exit(EXIT_FAILURE);
        }
    }
    fclose(f);
}

/* dump_table prints in required format; do nothing if empty */
void dump_table(void) {
    if (!sym_head) return;
    printf("SYMBOL TABLE:\n");
    // The sample output prints newest last; our list is head-inserted — order may differ but spec allows order variation
    symbol_t *cur = sym_head;
    while (cur) {
        printf("\tName: %s, Value: %d\n", cur->var_name, cur->val);
        cur = cur->next;
    }
}

/* free_table frees all nodes */
void free_table(void) {
    symbol_t *cur = sym_head;
    while (cur) {
        symbol_t *next = cur->next;
        free(cur->var_name);
        free(cur);
        cur = next;
    }
    sym_head = NULL;
}
