#include "types.h"
#include "parser.h"
#include "html.h"

#include <stdio.h>

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

    node_t* root = build_ast(&token_array);
    print_ast(root, 0);

    generate_html(root, "tests/out.html", "Resume", "style.css");

    return 0;
}
