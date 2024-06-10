#pragma once

#include "types.h"
#include "tokenizer.h"

typedef enum {
    NODE_HEADER,
    NODE_PARAGRAPH,
    NODE_ITALIC,
    NODE_BOLD,
    NODE_ITALIC_BOLD,
    NODE_LINEBREAK,
    NODE_BLOCKQUOTE,
    NODE_ORDERED_LIST,
    NODE_UNORDERED_LIST,
    NODE_LIST_ITEM,
    NODE_LINK,
    NODE_IMAGE,
    NODE_URL,
    NODE_TITLE,
    NODE_INNER_TEXT,
    NODE_ROOT
} node_type_t;

typedef struct node {
    node_type_t type;
    str_view_t value;
    u8 depth;

    struct node* next;
    struct node* children;
} node_t;

node_t* parse_md(tokenizer_t* t);
void parse_block(node_t* parent, token_array_t* tokens, u64* i);

void parse_header(node_t* parent, token_array_t* tokens, u64* i);
void parse_list(node_t* parent, token_array_t* tokens, u64* i);
void parse_text(node_t* parent, token_array_t* tokens, u64* i);
void parse_inline_text(node_t* parent, token_array_t* tokens, u64* i);

token_t* peek_ahead(token_array_t* tokens, u64* i, u64 ahead);
b8 consume_token(token_array_t* tokens, u64* i, token_type_t expected);

node_t* create_node(node_type_t node_type, str_view_t* value, u8 depth);
void add_child(node_t* parent, node_t* child);

void print_nodes(node_t* node, i32 indent);

void nodes_free(node_t* root);
