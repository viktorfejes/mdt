#include "tokenizer.h"

#include "lib/mem.h"
#include "lib/str.h"

#include <stdio.h>
#include <stdlib.h>

#define INITIAL_CAPACITY 128

const char* token_str[] = {
    "TOKEN_HEADER",
    "TOKEN_LINEBREAK",
    "TOKEN_LIST",
    "TOKEN_WHITESPACE",
    "TOKEN_EMPHASIS",
    "TOKEN_PAREN_OPEN",
    "TOKEN_PAREN_CLOSE",
    "TOKEN_SQBR_OPEN",
    "TOKEN_SQBR_CLOSE",
    "TOKEN_EXCLAMATION",
    "TOKEN_BACKSLASH",
    "TOKEN_NUMERICAL",
    "TOKEN_BLOCKQUOTE",
    "TOKEN_TEXT",
    "TOKEN_EOF",
    "TOKEN_NONE"
};

b8 tokenizer_init(tokenizer_t* t, const char* path) {
    // Save file path
    t->file_path = path;

    // Determine the source's size
    i64 file_size = load_source(path, 0);
    if (file_size == -1) {
        fprintf(stderr, "Couldn't open file: %s\n", path);
        return false;
    }

    // For future reference
    t->source_size = file_size;

    // Allocate memory for source
    t->source = malloc(file_size + 1);
    if (t->source == NULL) {
        fprintf(stderr, "Failed to allocate memory for source.\n");
        return false;
    }

    // Load source into allocated memory
    load_source(path, t->source);
    t->source[file_size] = '\0';

    // Point cursor to start of source
    t->cursor = t->source;

    t->current_type = TOKEN_NONE;
    t->previous_type = TOKEN_NONE;
    t->open_type = TOKEN_NONE;
    t->current_length = 0;

    // Set debug defaults (1 indexed)
    t->current_line = 1;
    t->current_char = 1;

    // Allocate and setup token array
    t->token_array.tokens = malloc(sizeof(token_t) * INITIAL_CAPACITY);
    t->token_array.capacity = INITIAL_CAPACITY;
    t->token_array.count = 0;

    return true;
}

void tokenizer_shutdown(tokenizer_t* t) {
    free(t->source);
    free(t->token_array.tokens);

    t->source = nullptr;
    t->cursor = nullptr;
}

b8 next_token(tokenizer_t* t) {
    // End of file, flush and return false
    if (*t->cursor == '\0' || (t->cursor - t->source) >= (i64)t->source_size) {
        flush_token(t);
        return false;
    }

    // Ever encountering a tab, we just ignore
    // by incrementing our current position.
    if (*t->cursor == '\t') {
        t->current_char++;
        t->cursor++;
    }

    // Register the type we are looking at
    token_type_t type = char_to_token(*t->cursor, t);

    // If it's a new kind of token and we haven't flushed
    if (type != t->current_type && t->current_type != TOKEN_NONE) {
        flush_token(t);
    }

    // If we flushed, we register the new token...
    if (t->current_type == TOKEN_NONE) {
        t->current_type = type;
        t->start = t->cursor;
        t->current_length = 0;
    }

    t->current_length++;
    t->current_char++;
    t->cursor++;

    while (char_to_token(*t->cursor, t) == type) {
        t->current_length++;
        t->current_char++;
        t->cursor++;
    }

    // Handle signals for open and close brackets
    if (type == TOKEN_PAREN_OPEN || type == TOKEN_SQBR_OPEN) {
        t->open_type = type;
    } else if (type == TOKEN_PAREN_CLOSE || type == TOKEN_SQBR_CLOSE) {
        t->open_type = TOKEN_NONE;
    }

    // If we hit a linebreak, increment the line number
    if (type == TOKEN_LINEBREAK) t->current_line++;

    // Store the current type as previous for next iteration
    t->previous_type = type;

    return true;
}

void flush_token(tokenizer_t* t) {
    if (t->current_length > 0) {
        // Add it to the token array
        if (t->token_array.count < t->token_array.capacity) {
            t->token_array.tokens[t->token_array.count].type = t->current_type;
            t->token_array.tokens[t->token_array.count].value = string_view(t->start, t->current_length);
            t->token_array.count++;
        } else {
            fprintf(stderr, "Token array is full!\n");
        }

        t->previous_type = t->current_type;
        t->current_type = TOKEN_NONE;
        t->current_length = 0;
    }
}

token_type_t char_to_token(char c, tokenizer_t* t) {
    switch (c) {
        case '\n':
            return TOKEN_LINEBREAK;

        case '#':
            return (t->previous_type == TOKEN_LINEBREAK || t->current_char <= 1) ? TOKEN_HEADER : TOKEN_TEXT;

        case '-':
        case '+':
            return (t->previous_type == TOKEN_LINEBREAK) ? TOKEN_LIST : TOKEN_TEXT; 

        case '*':
        case '_':
            return TOKEN_EMPHASIS;

        case '[':
            return TOKEN_SQBR_OPEN;
        case ']': {
            t->open_type = TOKEN_SQBR_CLOSE;
            return TOKEN_SQBR_CLOSE;
        }

        case '(': {
            if (t->previous_type == TOKEN_SQBR_CLOSE) {
                t->open_type = TOKEN_PAREN_OPEN;
                return TOKEN_PAREN_OPEN;
            }
            return TOKEN_TEXT;
        }

        case ')': {
            if (t->open_type == TOKEN_PAREN_OPEN) {
                return TOKEN_PAREN_CLOSE;
            }
            return TOKEN_TEXT;
        }

        // For this we need to look ahead 1
        case '!': {
            if (((t->cursor + 1) - t->source) <= (i64)t->source_size &&  *(t->cursor + 1) == '[') {
                return TOKEN_EXCLAMATION;
            }
            return TOKEN_TEXT;
        }

        case '>':
            return (t->previous_type == TOKEN_LINEBREAK || t->current_char <= 1) ? TOKEN_BLOCKQUOTE : TOKEN_TEXT;

        case ' ':
            return (t->previous_type != TOKEN_TEXT) ? TOKEN_WHITESPACE : TOKEN_TEXT;
            
        default:
            // Handle digit
            if ((is_char_digit(c) || c == '.') && t->previous_type == TOKEN_LINEBREAK) {
                return TOKEN_NUMERICAL;
            }
            // Otherwise, text.
            return TOKEN_TEXT;
    }
}

b8 is_char_digit(char c) {
    return c <= '9' && c >= '0';
}

i64 load_source(const char* path, char* out) {
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

void print_tokens(tokenizer_t* t) {
    for (u64 i = 0; i < t->token_array.count; ++i) {
        token_t token = t->token_array.tokens[i];
        
        printf("[%s]: %.*s\n",
            token_str[token.type],
            token.value.data ? (int)token.value.length : 0,
            token.value.data ? token.value.data : ""
        );
    }
}
