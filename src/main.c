#include "types.h"
#include "tokenizer.h"
#include "parser.h"
#include "html.h"
#include "args.h"

#include <stdio.h>

int main(int argc, char* argv[]) {
    // Parse the command line arguments
    config_t cfg = parse_args(argc, argv);

    // Start tokenizer
    tokenizer_t tokenizer;
    if (!tokenizer_init(&tokenizer, cfg.input_file)) {
        fprintf(stderr, "Failed to initialize tokenizer!\n");
        return -1;
    }

    while (next_token(&tokenizer));
    print_tokens(&tokenizer);

    // Parse and build the AST!
    node_t* root = parse_md(&tokenizer);
    print_nodes(root, 0);

    // Generate html
    generate_html(root, cfg.output_file, cfg.title, cfg.css);

    // Success!
    printf("All is good.\n");

    // Shutdown tokenizer
    tokenizer_shutdown(&tokenizer);

    return 0;
}
