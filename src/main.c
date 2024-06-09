#include "types.h"
#include "parser.h"
#include "html.h"
#include "args.h"

#include <stdio.h>

int main(int argc, char* argv[]) {
    // Parse the command line arguments
    config_t cfg = parse_args(argc, argv);

    // Create a lexer
    lexer_t lexer;
    if (new_lexer(&lexer, cfg.input_file) != 0) {
        fprintf(stderr, "Couldn't create a new lexer for file: %s\n", cfg.input_file);
    }

    // Tokenize
    token_array_t token_array = tokenize(&lexer);

    // Build AST
    node_t* root = build_ast(&token_array);
    print_ast(root, 0);

    // Generate HTML from AST
    generate_html(root, cfg.output_file, cfg.title, cfg.css);

    return 0;
}
