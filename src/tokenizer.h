#pragma once

#include "types.h"
#include "lib/str.h"

typedef enum {
    TOKEN_HEADER,
    TOKEN_LINEBREAK,
    TOKEN_LIST,
    TOKEN_WHITESPACE,
    TOKEN_EMPHASIS,
    TOKEN_PAREN_OPEN,
    TOKEN_PAREN_CLOSE,
    TOKEN_SQBR_OPEN,
    TOKEN_SQBR_CLOSE,
    TOKEN_EXCLAMATION,
    TOKEN_BACKSLASH,
    TOKEN_NUMERICAL,
    TOKEN_BLOCKQUOTE,
    TOKEN_TEXT,
    TOKEN_EOF,

    TOKEN_NONE
} token_type_t;

typedef struct {
    token_type_t type;
    str_view_t value;
} token_t;

typedef struct token_array {
    token_t* tokens;
    u64 capacity;
    u64 count;
} token_array_t;

typedef struct {
    // Source file and data
    const char* file_path;
    char* source;
    u64 source_size;

    // Token array
    token_array_t token_array;

    // Cursor position and info
    char* cursor;
    char* start;
    token_type_t current_type;
    token_type_t previous_type;
    token_type_t open_type;
    u64 current_length;

    // Debug info
    u64 current_line;
    u64 current_char;
} tokenizer_t;

b8 tokenizer_init(tokenizer_t* t, const char* path);
void tokenizer_shutdown(tokenizer_t* t);

b8 next_token(tokenizer_t* t);
void flush_token(tokenizer_t* t);
token_type_t char_to_token(char c, tokenizer_t* t);

// Helper functions
b8 is_char_digit(char c);
i64 load_source(const char* path, char* out);
void print_tokens(tokenizer_t* t);
