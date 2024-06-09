#pragma once

#include "types.h"
#include "lexer.h"

typedef enum {
    ELEMENT_HEADER,
    ELEMENT_PARAGRAPH,
    ELEMENT_LINE_BREAK,
    ELEMENT_EMPHASIS,
    ELEMENT_BLOCKQUOTE,
    ELEMENT_ORDERED_LIST,
    ELEMENT_UNORDERED_LIST,
    ELEMENT_CODE,
    ELEMENT_DIVIDER,
    ELEMENT_LINK,
    ELEMENT_TITLE,
    ELEMENT_URL,
    ELEMENT_IMAGE,
    ELEMENT_INNER_TEXT,

    _ROOT,
    _EOF
} element_type_t;

typedef struct element {
    element_type_t type;
    char* value;
    u16 depth; // like h1, h2, h3... (or it could help with nesting?)
} element_t;

typedef struct node {
    element_t element;
    struct node* next;
    struct node* children;
} node_t;

node_t* build_ast(token_array_t* tokens);

node_t* create_node(element_type_t element_type, u16 element_level, char* element_value);
void add_child(node_t* parent, node_t* child);

void print_ast(node_t* node, int indent);
