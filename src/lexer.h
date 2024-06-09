#pragma once

#include "types.h"

// TODO: maybe introduce TOKEN_WHITESPACE that can work with
// tokens and their in-betweens. However, wouldn't stop inner-text read.

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
    TOKEN_EXCLAMATION,
    TOKEN_EOF,
} token_type_t;

typedef struct lexer {
    const char* file_path;
    char* source;
    u64 cur;
    u64 line_num;
    u64 char_num;
} lexer_t;

typedef struct token {
    token_type_t type;
    char* value;
    u16 level;
    u64 line;
    u64 character;
} token_t;

typedef struct token_array {
    token_t* tokens;
    u64 capacity;
    u64 count;
} token_array_t;

i8 new_lexer(lexer_t* lexer, const char* src_path);

token_t* next_token(lexer_t* lexer);
token_array_t tokenize(lexer_t* lexer);

char* read_while(b8 (*predicate)(u8), lexer_t* lexer);
b8 is_not_inline_token(u8 c);
i64 load_file(const char* path, char* out);
