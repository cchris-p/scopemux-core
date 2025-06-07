#include <stdio.h>
#include <stdlib.h>
#include <tree_sitter/api.h>

// Provide this function from the tree-sitter-c grammar
extern const TSLanguage *tree_sitter_c();

char *source_code = NULL;
size_t size = 0;

void load_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open file");
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    source_code = malloc(size + 1);
    fread(source_code, 1, size, fp);
    source_code[size] = '\0';
    fclose(fp);
}

void print_node(FILE *out, TSNode node, int indent) {
    for (int i = 0; i < indent; i++) fprintf(out, "  ");
    fprintf(out, "%s [%u children] (start: %u:%u, end: %u:%u)\n",
        ts_node_type(node),
        ts_node_child_count(node),
        ts_node_start_point(node).row, ts_node_start_point(node).column,
        ts_node_end_point(node).row, ts_node_end_point(node).column
    );

    uint32_t n = ts_node_child_count(node);
    for (uint32_t i = 0; i < n; i++) {
        print_node(out, ts_node_child(node, i), indent + 1);
    }
}

int main() {
    load_file("example.c");

    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_c());

    TSTree *tree = ts_parser_parse_string(
        parser,
        NULL,
        source_code,
        size
    );

    TSNode root_node = ts_tree_root_node(tree);

    FILE *out = fopen("cst.txt", "w");
    if (!out) {
        perror("Failed to write output");
        return 1;
    }
    print_node(out, root_node, 0);
    fclose(out);

    ts_tree_delete(tree);
    ts_parser_delete(parser);
    free(source_code);

    return 0;
}
