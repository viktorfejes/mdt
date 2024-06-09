#pragma once

#include "types.h"

typedef struct config {
    char* input_file;
    char* output_file;
    char* title;
    char* css;
} config_t;

config_t parse_args(i32  argc, char** argv);
