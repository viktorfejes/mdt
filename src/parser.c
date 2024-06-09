#include "parser.h"

#include "lib/str.h"

#include <stdlib.h>
#include <stdio.h>

const char* element_str[] = {
    "ELEMENT_HEADER",
    "ELEMENT_PARAGRAPH",
    "ELEMENT_LINE_BREAK",
    "ELEMENT_EMPHASIS",
    "ELEMENT_BLOCKQUOTE",
    "ELEMENT_ORDERED_LIST",
    "ELEMENT_UNORDERED_LIST",
    "ELEMENT_CODE",
    "ELEMENT_DIVIDER",
    "ELEMENT_LINK",
    "ELEMENT_TITLE",
    "ELEMENT_URL",
    "ELEMENT_IMAGE",
    "ELEMENT_INNER_TEXT",

    "_ROOT",
    "_EOF"
};

node_t* build_ast(token_array_t* tokens) {
    // Create the root node
    node_t* root_node = create_node(_ROOT, 0, nullptr);

    // Let's loop through all the tokens
    for (u64 i = 0; i < tokens->count; ++i) {
        
        // Create header node
        if (tokens->tokens[i].type == TOKEN_HEADER) {
            
            // Create header node.
            node_t* header_node = create_node(
                ELEMENT_HEADER,
                tokens->tokens[i].level,
                nullptr);

            // Make header child of root
            add_child(root_node, header_node);

            // Look for inner text
            if(tokens->tokens[i + 1].type == TOKEN_TEXT) {
                // Increment i because we consume next token
                i++;

                // Create inner text node and make it the header's child
                node_t* inner_text_node = create_node(
                    ELEMENT_INNER_TEXT,
                    0,
                    tokens->tokens[i].value
                );
                add_child(header_node, inner_text_node);
            }

            continue;
        }

        // ... at the end we capture the simple text as paragraph
        if (tokens->tokens[i].type == TOKEN_TEXT) {
            // NOTE: to create paragraph we need to consume text and other
            // tokens until we hit a double line break... right?

            // TODO: bold, italic, links...etc

            // Creating a paragraph element to hold inner text and others...
            node_t* paragraph = create_node(
                ELEMENT_PARAGRAPH,
                0,
                nullptr
            );

            // Make root the parent
            add_child(root_node, paragraph);

            // Iterate through all the remaining tokens,
            // unless we encounter a token that cancels the loop.
            while (i < tokens->count) {

                if (tokens->tokens[i].type == TOKEN_TEXT) {
                    // Add TOKEN_TEXT as inner_text
                    node_t* inner_text = create_node(
                        ELEMENT_INNER_TEXT,
                        0,
                        tokens->tokens[i].value
                    );
                    add_child(paragraph, inner_text);

                } else if (tokens->tokens[i].type == TOKEN_LINE_BREAK) {
                    // Line break only causes immediate exit, if it's
                    // two consecutive ones.
                    if (i + 1 < tokens->count && tokens->tokens[i + 1].type == TOKEN_LINE_BREAK) {
                        // Skip the second line break
                        i++;
                        // Exit loop.
                        break;
                    }

                    // If it's only a single line break, it's either an html(<br>) or
                    // some other important element that we need to exit on.
                    // e.g. (Header, list...)
                    if (i + 1 < tokens->count && (tokens->tokens[i + 1].type == TOKEN_HEADER ||
                                                tokens->tokens[i + 1].type == TOKEN_STAR ||
                                                tokens->tokens[i + 1].type == TOKEN_UNDERSCORE ||
                                                tokens->tokens[i + 1].type == TOKEN_NUMBER)) {
                        // Skip the second line break
                        i++;
                        // Exit loop.
                        break;
                    }

                    node_t* br = create_node(
                        ELEMENT_LINE_BREAK,
                        0,
                        nullptr
                    );
                    add_child(paragraph, br);

                } else if (tokens->tokens[i].type == TOKEN_STAR ||
                        tokens->tokens[i].type == TOKEN_UNDERSCORE) {

                    // Caching `i` in case we need to restore
                    // like when this is not a unique star or underscore
                    // token, only text...
                    u64 cached_i = i;
                            
                    // Store which token it is
                    token_type_t token_type = tokens->tokens[i].type;
                    // Let's count the number of tokens we need to match
                    u8 count_before = 0;
                    u8 count_after = 0;

                    // Count stars or underscores
                    while (tokens->tokens[i].type == token_type) {
                        i++;
                        count_before++;
                    }

                    // Look ahead to see if the pattern and count is correct
                    // Let's only accept a pattern of:
                    // (TOKEN_X * y)TOKEN_TEXT(TOKEN_X * y)
                    if (tokens->tokens[i].type == TOKEN_TEXT &&
                        tokens->tokens[i + 1].type == token_type) {
                        u64 temp_i = i + 1;

                        // Count tokens
                        while (tokens->tokens[temp_i].type == token_type) {
                            temp_i++;
                            count_after++;
                        }
                    }

                    // See if before/after is equal and not too many
                    if (count_before == count_after && count_before <= 3) {
                        // We have a good emphasis pattern!
                        
                        // Add opening emphasis
                        node_t* em_open = create_node(
                            ELEMENT_EMPHASIS,
                            count_before,
                            nullptr
                        );
                        add_child(paragraph, em_open);

                        // Add inner text
                        node_t* inner_text = create_node(
                            ELEMENT_INNER_TEXT,
                            0,
                            tokens->tokens[i].value
                        );
                        add_child(em_open, inner_text);

                        i += count_after;

                    } else {
                        // Otherwise, we revert and save all of this as text...
                        i = cached_i;
                        node_t* txt = create_node(
                            ELEMENT_INNER_TEXT,
                            0,
                            tokens->tokens[i].value
                        );
                        add_child(paragraph, txt);
                    }

                }
                // else if (tokens->tokens[i].type == TOKEN_SQBR_OPEN) {
                //     // This can be a link or an image
                //     // Let's check if it's a link first
                //     // TODO: I'm not really checking a lot when looking ahead
                //     // if we still have tokens to consume. Should probably do that...
                //     if (i + 5 < tokens->count && tokens->tokens[i + 1].type == TOKEN_TEXT) {
                //         // It's probably a link, or just plain text
                //         u64 cached_i = i;
                //         // Skip text
                //         i += 2;

                //         if (tokens->tokens[i].type == TOKEN_SQBR_CLOSE &&
                //             tokens->tokens[i + 1].type == TOKEN_PAREN_OPEN) {

                //             }

                //     } else if (i + 5 < tokens->count && tokens->tokens[i + 1].type == TOKEN_EXCLAMATION) {
                //         // It's probably an image, or just plain text...
                //     }
                // }

                i++;
            }
        }
    }

    // Create the end of file node
    node_t* eof_node = create_node(_EOF, 0, nullptr);
    add_child(root_node, eof_node);

    return root_node;
}

node_t* create_node(element_type_t element_type, u16 element_level, char* element_value) {
    node_t* node = malloc(sizeof(node_t));

    node->element.type = element_type;
    node->element.value = element_value ? str_dup(element_value) : nullptr;
    node->element.depth = element_level;

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

void print_ast(node_t* node, int indent) {
    if (node == nullptr) {
        return;
    }

    // Print indent
    for (int i = 0; i < indent; ++i) {
        printf("    ");
    }

    // Print current node
    printf("|- %s (%d): %s\n", element_str[node->element.type], node->element.depth, node->element.value);

    // Recursively print children
    node_t* child = node->children;
    while (child) {
        print_ast(child, indent + 1);
        child = child->next;
    }

    // Recursively print siblings of root
    if (indent == 0) {
        node_t* sibling = node->next;
        while (sibling) {
            print_ast(sibling, indent);
            sibling = sibling->next;
        }
    }
}
