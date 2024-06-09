#include "html.h"

#include "lib/str.h"
#include <stdlib.h>

void node_to_html(node_t* node, FILE* file) {
    if (node == nullptr) {
        return;
    }

    switch (node->element.type) {
        case _ROOT: {
            node_t* child = node->children;
            while (child) {
                node_to_html(child, file);
                child = child->next;
            }
        } break;
        
        case ELEMENT_HEADER: {
            // For now, let's assume header can only have a single inner_text
            node_t* inner_text = node->children;

            fprintf(file,
            "<h%d id=\"%s\">%s</h%d>",
            node->element.depth,
            slugify(inner_text->element.value),
            inner_text->element.value,
            node->element.depth);
        } break;
        
        case ELEMENT_PARAGRAPH: {
            fprintf(file, "<p>");
            node_t* child = node->children;
            while (child) {
                node_to_html(child, file);
                child = child->next;
            }
            fprintf(file, "</p>");
        } break;

        case ELEMENT_LINE_BREAK: {
            fprintf(file, "<br>");
        } break;

        case ELEMENT_EMPHASIS: {
            char* open_tag;
            char* close_tag;
            switch (node->element.depth) {
                // italic
                case 1: {
                    const char open[] = "<em>";
                    const char close[] = "</em> ";
                    open_tag = str_dup(open);
                    close_tag = str_dup(close);
                } break;
                // bold
                case 2: {
                    const char open[] = "<strong>";
                    const char close[] = "</strong> ";
                    open_tag = str_dup(open);
                    close_tag = str_dup(close);
                } break;
                // italic-bold
                case 3: {
                    const char open[] = "<em><strong>";
                    const char close[] = "</strong></em> ";
                    open_tag = str_dup(open);
                    close_tag = str_dup(close);
                }
            }

            fprintf(file, "%s", open_tag);
            node_t* child = node->children;
            while (child) {
                node_to_html(child, file);
                child = child->next;
            }
            fprintf(file, "%s", close_tag);

            free(open_tag);
            free(close_tag);
        } break;

        case ELEMENT_INNER_TEXT: {
            fprintf(file, "%s", node->element.value);

            node_t* child = node->children;
            while (child) {
                node_to_html(child, file);
                child = child->next;
            }
        } break;

        default:
            fprintf(stderr, "Unknown node type: %d\n", node->element.type);
        break;
    }
}

void generate_html(node_t* root, const char* out_file, const char* title, const char* css) {
    FILE* file = fopen(out_file, "w");
    if (!file) {
        fprintf(stderr, "Couldn't open file for writing: %s\n", out_file);
        return;
    }

    fprintf(file,
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "\t<meta charset=\"UTF-8\">\n"
    "\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "\t<title>%s</title>\n"
    "\t<link rel=\"stylesheet\" href=\"%s\">\n"
    "</head>\n"
    "<body>\n",
    title ? title : "Markdown",
    css ? css : "");

    node_to_html(root, file);

    fprintf(file, "</body>\n</html>\n");
    fclose(file);
}

char* slugify(char* input) {
    // Hello, World! -> hello-world
    
    // Currently, this functions wastes a bit of memory,
    // because it allocates memory equal to source string's length,
    // but then skips some characters, hence doesn't always use
    // all the allocated memory.

    i64 length = 0;
    // strlen
    while (input[length++] != '\0');

    // Allocate memory for output string
    char* out_str = malloc(length + 1);
    if (out_str == nullptr) {
        // Silent error
        return input;
    }

    i64 o = 0;
    for (i64 i = 0; i < length; ++i) {
        // Uppercase characters
        if (input[i] >= 'A' && input[i] <= 'Z') {
            // Shift them to the lower case counterpart
            out_str[o++] = input[i] + 32;
        } else if (input[i] >= 'a' && input[i] <= 'z') {
            // Lowercase characters -- no need to change anything.
            out_str[o++] = input[i];
        } else if (input[i] == ' ') {
            // Space becomes hyphen
            out_str[o++] = '-';
        }
    }

    out_str[o] = '\0';

    return out_str;
}
