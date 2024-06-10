#include "parser.h"

#include "lib/str.h"
#include <stdlib.h>
#include <stdio.h>

const char* node_str[] = {
    "NODE_HEADER",
    "NODE_PARAGRAPH",
    "NODE_ITALIC",
    "NODE_BOLD",
    "NODE_ITALIC_BOLD",
    "NODE_LINEBREAK",
    "NODE_BLOCKQUOTE",
    "NODE_ORDERED_LIST",
    "NODE_UNORDERED_LIST",
    "NODE_LIST_ITEM",
    "NODE_LINK",
    "NODE_IMAGE",
    "NODE_URL",
    "NODE_TITLE",
    "NODE_INNER_TEXT",
    "NODE_ROOT"
};

node_t* parse_md(tokenizer_t* t) {
    // Create the root node
    node_t* root = create_node(NODE_ROOT, nullptr, 0);

    // Loop through all the tokens
    u64 i = 0;
    while (i < t->token_array.count) {
        parse_block(root, &t->token_array, &i);

        // TODO: remove this, and always consume in parsing!
        // This has tripped me up sooo many times...
        i++;
    }

    return root;
}

void parse_block(node_t* parent, token_array_t* tokens, u64* i) {
    token_t token = tokens->tokens[*i];

    switch (token.type) {
        case TOKEN_HEADER:
            parse_header(parent, tokens, i);
        break;

        case TOKEN_LIST:
            parse_list(parent, tokens, i);
        break;

        case TOKEN_TEXT:
            parse_text(parent, tokens, i);
        break;

        default: break;
    }
}

void parse_header(node_t* parent, token_array_t* tokens, u64* i) {
    u64 cached_i = *i;
    
    if (!consume_token(tokens, i, TOKEN_HEADER)) {
        // Something went wrong?
        return;
    }

    if (!consume_token(tokens, i, TOKEN_WHITESPACE)) {
        // Invalid header, no white space after
        // parse_inline();
        return;
    }
    
    node_t* header = create_node(
        NODE_HEADER,
        nullptr,
        tokens->tokens[cached_i].value.length);
    
    // Add this to parent
    add_child(parent, header);

    // Consume text
    if (tokens->tokens[*i].type == TOKEN_TEXT) {
        node_t* inner_text = create_node(
            NODE_INNER_TEXT,
            &tokens->tokens[*i].value,
            0
        );
        add_child(header, inner_text);

        // (*i)++;
    }

    // consume_token(tokens, i, TOKEN_LINEBREAK);
}

void parse_list(node_t* parent, token_array_t* tokens, u64* i) {
    u64 cached_i = *i;
    (*i)++;

    if (!consume_token(tokens, i, TOKEN_WHITESPACE)) {
        // We are throwing this back...
        return;
    }

    // Creating the main list node
    node_t* list = create_node(NODE_UNORDERED_LIST, nullptr, 0);
    add_child(parent, list);

    // Roll back to original `i`
    *i = cached_i;
    while (*i < tokens->count) {
        if (!consume_token(tokens, i, TOKEN_LIST)) {
            // No more list items, break out of the loop
            break;
        }

        consume_token(tokens, i, TOKEN_WHITESPACE);

        // List item node
        node_t* li = create_node(NODE_LIST_ITEM, nullptr, 0);
        add_child(list, li);

        // Parse inner text
        parse_inline_text(li, tokens, i);

        // Check if we need to go another round because the next item is a list as well.
        if ((*i) + 1 >= tokens->count || tokens->tokens[(*i) + 1].type != TOKEN_LIST) {
            break;
        }

        (*i)++;
    }
}

void parse_text(node_t* parent, token_array_t* tokens, u64* i) {

    node_t* paragraph = create_node(
        NODE_PARAGRAPH,
        nullptr,
        0
    );
    add_child(parent, paragraph);

    // Parse inline!
    parse_inline_text(paragraph, tokens, i);
}

void parse_inline_text(node_t* parent, token_array_t* tokens, u64* i) {
    // Iterate over all the texts, until we hit something
    // that cancels the loop.
    while (*i < tokens->count) {

        if (tokens->tokens[*i].type == TOKEN_TEXT) {
            // Add TOKEN_TEXT as inner text
            node_t* inner_text = create_node(
                NODE_INNER_TEXT,
                &tokens->tokens[*i].value,
                0
            );
            add_child(parent, inner_text);

        } else if (tokens->tokens[*i].type == TOKEN_EMPHASIS) {
            u8 em_count = tokens->tokens[*i].value.length;


            // Look ahead to see if the pattern is correct
            if (tokens->tokens[*i + 1].type == TOKEN_TEXT &&
                str_ncmp(tokens->tokens[*i].value.data, tokens->tokens[*i + 2].value.data, em_count - 1) == 0 &&
                em_count <= 3) {
                
                // Emphasis pattern seems OK.
                node_t* em_open = create_node(NODE_ITALIC + em_count, nullptr, em_count);
                add_child(parent, em_open);

                (*i)++;

                // Add the inner text
                parse_inline_text(em_open, tokens, i);
            } else {
                // TODO:
                // This is where we would save it as simple text somehow
            }

        } else if (tokens->tokens[*i].type == TOKEN_LINEBREAK) {
            u8 lb_count = tokens->tokens[*i].value.length;

            // Linebreaks only cause immediate exit, if there are
            // two consecutive ones.
            if (lb_count >= 2) {
                // Exit loop
                break;
            }

            // If it's only a single line break, it's either an html(<br>) or
            // some other important element that we need to exit on.
            // e.g. (Header, list...)
            if ((*i) + 1 < tokens->count && (tokens->tokens[(*i) + 1].type == TOKEN_HEADER ||
                                        tokens->tokens[(*i) + 1].type == TOKEN_EMPHASIS ||
                                        tokens->tokens[(*i) + 1].type == TOKEN_LIST ||
                                        tokens->tokens[(*i) + 1].type == TOKEN_NUMERICAL)) {
                // Exit loop.
                break;
            }

            node_t* lb = create_node(
                NODE_LINEBREAK,
                nullptr,
                0
            );
            add_child(parent, lb);

        }

        (*i)++;
    }
}

token_t* peek_ahead(token_array_t* tokens, u64* i, u64 ahead) {
    return &tokens->tokens[(*i) + ahead];
}

b8 consume_token(token_array_t* tokens, u64* i, token_type_t expected) {
    if (*i >= tokens->count) {
        return false;
    }

    token_t token = tokens->tokens[*i];
    if (token.type == expected) {
        (*i)++;
        return true;
    }

    return false;
}

node_t* create_node(node_type_t node_type, str_view_t* value, u8 depth) {
    node_t* node = malloc(sizeof(node_t));

    node->type = node_type;
    node->value = string_view(
        value ? value->data : nullptr,
        value ? value->length : 0);
    node->depth = depth;

    // Set children to nullptr
    node->next = nullptr;
    node->children = nullptr;

    return node;
}

void add_child(node_t* parent, node_t* child) {
    if (!parent->children) {
        parent->children = child;
    } else {
        node_t* sibling = parent->children;
        while (sibling->next) {
            sibling = sibling->next;
        }
        sibling->next = child;
    }
}

void print_nodes(node_t* node, i32 indent) {
    if (node == nullptr) {
        return;
    }

    // Print indent
    for (int i = 0; i < indent; ++i) {
        printf("    ");
    }

    // Print current node
    printf(
        "|- %s (%d): %.*s\n",
        node_str[node->type],
        node->depth,
        node->value.data ? (int)node->value.length : 0,
        node->value.data ? node->value.data : ""
    );

    // Recursively print children
    node_t* child = node->children;
    while (child) {
        print_nodes(child, indent + 1);
        child = child->next;
    }

    // Recursively print siblings of root
    if (indent == 0) {
        node_t* sibling = node->next;
        while (sibling) {
            print_nodes(sibling, indent);
            sibling = sibling->next;
        }
    }
}

