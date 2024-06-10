#pragma once

#include "types.h"
#include "parser.h"

#include <stdio.h>

void node_to_html(node_t* node, FILE* file);
void generate_html(node_t* root, const char* out_file, const char* title, const char* css);

char* slugify(char* input);
char* slugifyn(char* input, u64 length);
