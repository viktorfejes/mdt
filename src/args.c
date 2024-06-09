#include "args.h"

#include "lib/str.h"

#include <stdio.h>

config_t parse_args(i32 argc, char** argv) {
    config_t config = {
        .input_file = nullptr,
        .output_file = "out.html",
        .title = "Markdown",
        .css = nullptr
    };

    for (i32 i = 1; i < argc; ++i) {
        // Output file (optional)
        // Usage: -o [filename] || --output=[filename]
        if (str_ncmp(argv[i], "--output=", 9) == 0) {
            config.output_file = argv[i] + 9;
        } else  if (str_cmp(argv[i], "-o") == 0) {
            config.output_file = argv[++i];

        // CSS file (optional)
        // Usage: -c [filename] || --css=[filename]
        } else if (str_ncmp(argv[i], "--css=", 6) == 0) {
            config.css = argv[i] + 6;
        } else if (str_cmp(argv[i], "-c") == 0) {
            config.css = argv[++i];

        // HTML title (optional)
        // Usage: -t [filename] || --title=[filename]
        } else if (str_ncmp(argv[i], "--title=", 8) == 0) {
            config.title = argv[i] + 8;
        } else if (str_cmp(argv[i], "-t") == 0) {
            config.title = argv[++i];

        // Unknown option!
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown options: %s\n", argv[i]);

        // Input file*
        // Usage: [filename]
        } else {
            config.input_file = argv[i];
        }
    }

    if (config.input_file == nullptr) {
        fprintf(stderr, "Input file is required. Use --help or -h to get more information.\n");
    }

    return config;
}
