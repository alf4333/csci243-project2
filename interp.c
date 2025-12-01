// interp.c
// Main program: reads optional sym-table filename, prints symbol table,
// then enters REPL for postfix expressions.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "symtab.h"
#include "tree_node.h"
#include "parser.h"
#include "interp.h"

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: interp [sym-table]\n");
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        build_table(argv[1]); // exits on file open failure per header
    }

    dump_table(); // print only if non-empty

    printf("Enter postfix expressions (CTRL-D to exit):\n");

    char linebuf[MAX_LINE + 2]; // allow newline + null
    while (1) {
        // prompt
        printf("> ");
        fflush(stdout);

        if (!fgets(linebuf, sizeof(linebuf), stdin)) {
            // EOF or error
            break;
        }
        // remove leading whitespace
        char *s = linebuf;
        while (*s && isspace((unsigned char)*s)) s++;
        if (*s == '\0') continue;
        // ignore full-line comments starting with '#'
        if (*s == '#') continue;
        // remove inline comments
        char *hash = strchr(s, '#');
        if (hash) *hash = '\0';
        // strip trailing newline
        char *nl = strchr(s, '\n');
        if (nl) *nl = '\0';
        // skip if line empty after stripping
        char *t = s;
        while (*t && isspace((unsigned char)*t)) t++;
        if (*t == '\0') continue;

        // parse
        char *err = NULL;
        tree_node_t *root = parse_line(s, &err);
        if (!root) {
            if (err) { fprintf(stderr, "%s\n", err); free(err); }
            else fprintf(stderr, "Parse error\n");
            continue;
        }
        long value;
        char *eval_err = NULL;
        if (!eval_tree(root, &value, &eval_err)) {
            if (eval_err) { fprintf(stderr, "%s\n", eval_err); free(eval_err); }
            else fprintf(stderr, "Evaluation error\n");
            free_tree(root);
            continue;
        }
        // print infix and value
        print_infix(root);
        printf(" = %ld\n", value);
        free_tree(root);
    }

    dump_table(); // final symbol table
    free_table();
    return 0;
}
