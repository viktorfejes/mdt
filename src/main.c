#include "types.h"

#include "lib/str.h"
#include "lib/mem.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum token_type {
    TOKEN_HEADER,
    TOKEN_TEXT,
    TOKEN_DASH,
    TOKEN_PLUS,
    TOKEN_NUMBER,
    TOKEN_STAR,
    TOKEN_UNDERSCORE,
    TOKEN_BIGGER_THAN,
    TOKEN_PAREN_OPEN,
    TOKEN_PAREN_CLOSE,
    TOKEN_SQBR_OPEN,
    TOKEN_SQBR_CLOSE,
    TOKEN_BACKTICKS_OPEN,
    TOKEN_BACKTICKS_CLOSE,
    TOKEN_LINE_BREAK,
    TOKEN_ESCAPE,
    TOKEN_EOF,
} token_type_t;

const char* token_str[] = {
    "TOKEN_HEADER",
    "TOKEN_TEXT",
    "TOKEN_DASH",
    "TOKEN_PLUS",
    "TOKEN_NUMBER",
    "TOKEN_STAR",
    "TOKEN_UNDERSCORE",
    "TOKEN_BIGGER_THAN",
    "TOKEN_PAREN_OPEN",
    "TOKEN_PAREN_CLOSE",
    "TOKEN_SQBR_OPEN",
    "TOKEN_SQBR_CLOSE",
    "TOKEN_BACKTICKS_OPEN",
    "TOKEN_BACKTICKS_CLOSE",
    "TOKEN_LINE_BREAK",
    "TOKEN_ESCAPE",
    "TOKEN_EOF",
};

typedef enum {
    ELEMENT_HEADER,
    ELEMENT_PARAGRAPH,
    ELEMENT_LINE_BREAK,
    ELEMENT_BOLD,
    ELEMENT_ITALIC,
    ELEMENT_UNDERLINED,
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

const char* element_type_str[] = {
    "ELEMENT_HEADER",
    "ELEMENT_PARAGRAPH",
    "ELEMENT_LINE_BREAK",
    "ELEMENT_BOLD",
    "ELEMENT_ITALIC",
    "ELEMENT_UNDERLINED",
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

#define INITIAL_CAPACITY 128

typedef struct token {
    token_type_t type;
    char* value;
    u8 level;
    u64 line;
    u64 character;
} token_t;

typedef struct token_array {
    token_t* tokens;
    u64 capacity;
    u64 count;
} token_array_t;

typedef struct element {
    element_type_t type;
    char* value;
    u8 depth; // like h1, h2, h3... (or it could help with nesting?)
} element_t;

typedef struct lexer {
    const char* file_path;
    char* source;
    u64 cur;
    u64 line_num;
    u64 char_num;
} lexer_t;

typedef struct node {
    element_t element;
    struct node* next;
    struct node* children;
} node_t;

i64 load_file(const char* path, char* out) {
    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Couldn't open file: %s\n", path);
        return -1;
    }
    
    // Get size of file
    // NOTE: Maximum file size is around 2GB because of fseek returning int
    // Also, this returns more on Win than Unix if using binary mode because of '\r\n'
    fseek(file, 0L, SEEK_END);
    i64 file_size = ftell(file);
    // Seek back to beginning... but do we have to for fgets?
    rewind(file);

    // If out is allocated, we shove the text in.
    if (out) {
        // But first we zero out the memory so we don't have to deal with
        // null termination.
        mem_set(out, 0, file_size);
        fread(out, 1, file_size, file);
    }

    // Closing the file
    fclose(file);

    return file_size;
}

i8 new_lexer(lexer_t* lexer, const char* src_path) {
    // Read in the file's size
    i64 file_size = load_file(src_path, 0);
    if (file_size == -1) {
        fprintf(stderr, "Couldn't open file: %s\n", src_path);
        return -1;
    }

    // Allocate memory in lexer
    lexer->source = malloc(file_size);
    if (lexer->source == NULL) {
        fprintf(stderr, "Failed to allocate memory for source.\n");
        return -2;
    }

    // Load into the allocated source variable
    load_file(src_path, lexer->source);

    // Save file path
    lexer->file_path = str_dup(src_path);
    // Init cursor position to 0
    lexer->cur = 0;
    // Init line to 0 (zero indexing everything!!!!)
    lexer->line_num = 0;
    // Init character number to 0
    lexer->char_num = 0;

    return 0;
}

char* read_while(b8 (*predicate)(u8), lexer_t* lexer) {
    u64 start = lexer->cur;
    while (predicate(lexer->source[lexer->cur])) {
        lexer->char_num++;
        lexer->cur++;
    }
    u64 length = lexer->cur - start;
    char* result = malloc(length + 1);
    str_ncpy(result, &lexer->source[start], length);
    result[length] = '\0';

    return result;
}

b8 is_not_newline_or_eof(u8 c) {
    return c != '\n' && c != '\0';
}

// All the possible tokens that can appear inline
b8 is_not_inline_token(u8 c) {
    // Emphasis
    b8 em = c != '*' && c != '_';
    // Line break or EOF
    b8 neof = c != '\n' && c != '\0';
    // Links or images
    b8 li = c != '[';
    // Backticks and escape
    b8 bt = c != '`' && c != '\\';

    return em && neof && li && bt;
}

token_t* next_token(lexer_t* lexer) {
    // Ignore whitespace and tab
    while (lexer->source[lexer->cur] == ' ' || lexer->source[lexer->cur] == '\t') {
        lexer->char_num++;
        lexer->cur++;
    }

    // EOF
    if (lexer->source[lexer->cur] == 0) {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_EOF;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++; // also incrementing

        return token;
    }

    // # (Header token)
    if (lexer->source[lexer->cur] == '#') {
        // Store in case we need to recover
        u64 cursor = lexer->cur;
        
        u8 header_level = 0;
        while (lexer->source[lexer->cur] == '#') {
            lexer->cur++;
            header_level++;
        }

        // If next character is not a space after #,
        // we let the function continue.
        if (lexer->source[lexer->cur] != ' ') {
            lexer->cur = cursor;
        } else {
            // we calculate the character number we need to add
            // Adding this in the loop might lead to incorrect count if we bail
            lexer->char_num += lexer->cur - cursor;
            lexer->cur++; // Skip space

            // If we haven't bailed, we are returning the header token
            // where the value represents the depth/number of hashes
            token_t* token = malloc(sizeof(token_t));
            token->type = TOKEN_HEADER;
            token->value = NULL;
            token->level = header_level;
            token->line = lexer->line_num;
            token->character = lexer->char_num;

            return token;
        }
    }

    // Line break
    if (lexer->source[lexer->cur] == '\n') {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_LINE_BREAK;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num;

        // Increment cursor
        lexer->cur++;
        // Increment line number
        lexer->line_num++;
        // Reset character number
        lexer->char_num = 0;

        return token;
    }

    // Escape character
    if (lexer->source[lexer->cur] == '\\') {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_ESCAPE;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        lexer->cur++;

        return token;
    }

    // Unordered list tokens
    if ((lexer->source[lexer->cur] == '-' ||
        lexer->source[lexer->cur] == '*' ||
        lexer->source[lexer->cur] == '+') &&
        lexer->source[lexer->cur + 1] == ' ') {
        
        token_t* token = malloc(sizeof(token_t));
        token->type = lexer->source[lexer->cur] == '-' ? TOKEN_DASH :
                        lexer->source[lexer->cur] == '*' ? TOKEN_STAR : TOKEN_PLUS;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num;

        lexer->char_num += 2;
        lexer->cur += 2;
        return token;
    }

    // Ordered list
    if (lexer->source[lexer->cur] >= 0 && lexer->source[lexer->cur] <= 9) {
        // Store in case we need to recover
        u64 cursor = lexer->cur;
        
        u32 list_number = 0;
        // Check if we have more numbers
        while (lexer->source[lexer->cur] >= '0' && lexer->source[lexer->cur] <= '9') {
            list_number = list_number * 10 + (lexer->source[lexer->cur] - '0');
            lexer->cur++;
        }

        // If next character is not a dot after numbers and then a space,
        // we let the function continue.
        if (lexer->source[lexer->cur] != '.' && lexer->source[lexer->cur + 1] != ' ') {
            // Restore the cursor
            lexer->cur = cursor;
        } else {
            // we calculate the character number we need to add
            // Adding this in the loop might lead to incorrect count if we bail
            lexer->char_num += lexer->cur - cursor + 2;  // +2 for '.' and ' '
            lexer->cur += 2;  // Move past '.' and ' '

            // If we haven't bailed, we are returning the ordered list token
            // where the value represents the number used
            token_t* token = malloc(sizeof(token_t));
            token->type = TOKEN_NUMBER;
            token->value = NULL;
            token->level = list_number;
            token->line = lexer->line_num;
            token->character = lexer->char_num;

            return token;
        }
    }

    // Open parenthesis
    if (lexer->source[lexer->cur] == '(') {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_PAREN_OPEN;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        lexer->cur++;

        return token;
    }

    // Close parenthesis
    if (lexer->source[lexer->cur] == ')') {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_PAREN_CLOSE;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        lexer->cur++;

        return token;
    }

    // Open square bracket
    if (lexer->source[lexer->cur] == '[') {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_SQBR_OPEN;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        lexer->cur++;

        return token;
    }

    // Close square bracket
    if (lexer->source[lexer->cur] == ']') {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_SQBR_CLOSE;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        lexer->cur++;

        return token;
    }

    // Inline * and _ for potential emphasis
    if (lexer->source[lexer->cur] == '*' ||
        lexer->source[lexer->cur] == '_') {

        token_t* token = malloc(sizeof(token_t));
        token->type = lexer->source[lexer->cur] == '*' ? TOKEN_STAR : TOKEN_UNDERSCORE;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        lexer->cur++;

        return token;
    }

    // Otherwise, read text until we find an inline token
    char* text_value = read_while(is_not_inline_token, lexer);

    token_t* token = malloc(sizeof(token_t));
    token->type = TOKEN_TEXT;
    token->value = text_value;
    token->line = lexer->line_num;
    token->character = lexer->char_num;
    // Characters are increased inside read_while...

    // Return text token
    return token;
}

token_array_t tokenize(lexer_t* lexer) {
    token_array_t token_array = { 
        malloc(sizeof(token_t) * INITIAL_CAPACITY),
        INITIAL_CAPACITY,
        0
    };

    while ((token_array.tokens[token_array.count] = *next_token(lexer)).type != TOKEN_EOF) {
        printf("[Token type]: %s, [Token value]: %s, [Token level]: %d\n", token_str[token_array.tokens[token_array.count].type], token_array.tokens[token_array.count].value, token_array.tokens[token_array.count].level);
        token_array.count++;
    }

    return token_array;
}

// node_t* create_node(token_type_t token_type, char* token_value) {
//     node_t* node = malloc(sizeof(node_t));
//     node->token_type = token_type;
//     node->value = token_value ? strdup(token_value) : NULL;
//     node->next = NULL;
//     node->children = NULL;

//     return node;
// }

// void add_child(node_t* parent, node_t* child) {
//     if (!parent->children) {
//         parent->children = child;
//     } else {
//         node_t* sibling = parent->children;
//         while (sibling->next) {
//             sibling = sibling->next;
//         }
//         sibling->next = child;
//     }
// }

// char* read_while(b8 (*predicate)(u8), lexer_t* lexer) {
//     u64 start = lexer->cur;
//     while (predicate(lexer->source[lexer->cur])) {
//         lexer->cur++;
//     }
//     u64 length = lexer->cur - start;
//     char* result = malloc(length + 1);
//     strncpy(result, &lexer->source[start], length);
//     result[length] = '\0';

//     return result;
// }

// b8 is_not_newline_or_eof(u8 c) {
//     return c != '\n' && c != '\0';
// }

// node_t* parse_md(lexer_t* lexer) {
//     node_t* root = malloc(sizeof(node_t));
//     root->token_type = TOKEN_ROOT;
//     root->value = NULL;
//     root->next = NULL;
//     root->children = NULL;
    
//     token_t* token = NULL;
//     node_t* ul_root = NULL;
//     while ((token = next_token(lexer))->type != TOKEN_EOF) {
//         printf("Token Type: %s, Value: %s\n", token_type_str[token->type], token->value);

//         switch (token->type) {
//             case TOKEN_HEADER1:
//             case TOKEN_HEADER2:
//             case TOKEN_HEADER3:
//             case TOKEN_HEADER4:
//             case TOKEN_HEADER5:
//             case TOKEN_HEADER6:
//             case TOKEN_TEXT: {
//                 ul_root = NULL;
//                 node_t* child = create_node(token->type, token->value);
//                 add_child(root, child);
//             } break;

//             case TOKEN_UL: {
//                 if (ul_root == NULL) {
//                     ul_root = create_node(TOKEN_UL_ROOT, NULL);
//                     add_child(root, ul_root);
//                 }

//                 node_t* ul_item = create_node(token->type, token->value);
//                 add_child(ul_root, ul_item);
//             } break;

//             default:
//             break;
//         }

//         free(token->value);
//         free(token);
//     }
//     free(token); // free EOF token
    
//     return root;
// }

// char* slugify(char* input) {
//     // Hello, World! -> hello-world
    
//     // Currently, this functions wastes a bit of memory,
//     // because it allocates memory equal to source string's length,
//     // but then skips some characters, hence doesn't always use
//     // all the allocated memory.

//     i64 length = 0;
//     // strlen
//     while (input[length++] != '\0');

//     // Allocate memory for output string
//     char* out_str = malloc(length + 1);
//     if (out_str == NULL) {
//         // Silent error
//         return input;
//     }

//     i64 o = 0;
//     for (i64 i = 0; i < length; ++i) {
//         // Uppercase characters
//         if (input[i] >= 'A' && input[i] <= 'Z') {
//             // Shift them to the lower case counterpart
//             out_str[o++] = input[i] + 32;
//         } else if (input[i] >= 'a' && input[i] <= 'z') {
//             // Lowercase characters -- no need to change anything.
//             out_str[o++] = input[i];
//         } else if (input[i] == ' ') {
//             // Space becomes hyphen
//             out_str[o++] = '-';
//         }
//     }

//     out_str[o] = '\0';

//     return out_str;
// }

// u64 token_to_html_tag(node_t* node, char* buffer, u64 buffer_cursor) {
//     switch (node->token_type) {
//         case TOKEN_HEADER1:
//         case TOKEN_HEADER2:
//         case TOKEN_HEADER3:
//         case TOKEN_HEADER4:
//         case TOKEN_HEADER5:
//         case TOKEN_HEADER6:
//             // Possible leak from slugify here
//             return buffer_cursor + sprintf(&buffer[buffer_cursor], "<h%d id=\"%s\">%s</h%d>", node->token_type + 1, slugify(node->value), node->value, node->token_type + 1);
//         break;

//         case TOKEN_TEXT: 
//             return buffer_cursor + sprintf(&buffer[buffer_cursor], "<p>%s</p>", node->value);
//         break;

//         case TOKEN_UL_ROOT: {
//             buffer_cursor += sprintf(&buffer[buffer_cursor], "<ul>");
//             node_t* cur_item = node->children;
//             while (cur_item) {
//                 buffer_cursor += sprintf(&buffer[buffer_cursor], "<li>%s</li>", cur_item->value);

//                 cur_item = cur_item->next;
//             }
//             return buffer_cursor + sprintf(&buffer[buffer_cursor], "</ul>");

//         } break;

//         default: break;
//     }

//     return 0;
// }

// i8 generate_html(node_t* root_node, char* buffer) {
//     u64 buffer_cursor = 0;

//     node_t* current = root_node;
//     while (current) {
//         buffer_cursor = token_to_html_tag(current, buffer, buffer_cursor);

//         node_t* sibling = current->next;
//         while (sibling) {
//             buffer_cursor = token_to_html_tag(sibling, buffer, buffer_cursor);
//             sibling = sibling->next;
//         }

//         current = current->children;
//     }
    
//     return 0;
// }

int main(void) {
    
    // char* input_file = NULL;
    // char* output_file = NULL;
    // char* css_file = NULL;
    // char* title = NULL;

    // for (i64 i = 1; i < argc; ++i) {
    //     printf("%s\n", argv[i]);
    //     if (str_cmp(argv[i], "--output=") == 0) {
    //         output_file = argv[i] + 9;
    //     } else  if (str_cmp(argv[i], "-o") == 0) {
    //         output_file = argv[++i];
    //     } else if (str_cmp(argv[i], "--css=") == 0) {
    //         css_file = argv[i] + 6;
    //     } else if (str_cmp(argv[i], "-c") == 0) {
    //         css_file = argv[++i];
    //     } else if (str_cmp(argv[i], "--title=") == 0) {
    //         title = argv[i] + 8;
    //     } else if (str_cmp(argv[i], "-t") == 0) {
    //         title = argv[++i];
    //     } else if (argv[i][0] == '-') {
    //         fprintf(stderr, "Unknown options: %s\n", argv[i]);
    //     } else {
    //         input_file = str_dup(argv[i]);
    //     }
    // }

    // if (!output_file) {
    //     fprintf(stderr, "Output file was not specified!\n");
    //     return -1;
    // }

    // printf("Input file: %s\n", input_file);
    // printf("Output file: %s\n", output_file);
    // printf("CSS file: %s\n", css_file ? css_file : "none");
    // printf("HTML title: %s\n", title ? title : "none");

    // Create a lexer
    const char* resume_path = "tests/resume.md";
    lexer_t lexer;
    if (new_lexer(&lexer, resume_path) != 0) {
        fprintf(stderr, "Couldn't create a new lexer for file: %s\n", resume_path);
    }

    token_array_t token_array = tokenize(&lexer);

    printf("Token amount by token_array: %lld\n", token_array.count);
    printf("Accessing 2nd token:\n");
    printf("Token type: %s, Token value: %s\n", token_str[token_array.tokens[1].type], token_array.tokens[1].value);

    // // Create new lexer
    // lexer_t lexer;
    // if (new_lexer(&lexer, resume_path) != 0) {
    //     fprintf(stderr, "Couldn't create a new lexer for file: %s\n", resume_path);
    // }

    // // Parse source
    // node_t* root = parse_md(&lexer);
    // if (root == NULL) {
    //     fprintf(stderr, "Couldn't parse markdown file.\n");
    // }

    // // Print out the Tree
    // node_t* current = root;
    // while (current) {
    //     printf("- %s\n", token_type_str[current->token_type]);

    //     node_t* sibling = current->next;
    //     while (sibling) {
    //         printf("- %s\n", token_type_str[sibling->token_type]);
    //         sibling = sibling->next;
    //     }

    //     current = current->children;
    // }

    // // Generate html
    // char buffer[34000];
    // memset(buffer, 0, 34000);
    // if (generate_html(root, buffer) != 0) {
    //     fprintf(stderr, "Couldn't generate html from tree!\n");
    // }

    // const char* html_header = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";

    // const char* html_title = "Resume";
    // const char* css = "resume.css";

    // const char* save_file_path = "tests/test.html";
    // FILE* file = fopen(save_file_path, "w");
    // fprintf(file, "%s<title>%s</title><link rel=\"stylesheet\" href=\"%s\"></head><body>%s</body></html>", html_header, html_title, css, buffer);
    // fclose(file);

    // printf("%s\n", buffer);

    // char test_str[] = "Hello, World!";
    // printf("%s\n", slugify(test_str));

    // // Free all links
    // node_t* current_node = root;
    // node_t* next_node = NULL;
    // node_t* sibling_next = NULL;
    // while (current_node) {
    //     next_node = current_node->next;

    //     node_t* sibling = current_node->next;
    //     while (sibling) {
    //         sibling_next = sibling->next;
    //         free(sibling);
    //         sibling = sibling_next;
    //     }

    //     free(current_node);
    //     current_node = next_node;
    // }

    return 0;
}
