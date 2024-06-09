#include "lexer.h"

#include "lib/mem.h"
#include "lib/str.h"

#include <stdio.h>
#include <stdlib.h>

const char* token_str[] = {
    "TOKEN_HEADER",
    "TOKEN_TEXT",
    "TOKEN_DASH",
    "TOKEN_PLUS",
    "TOKEN_NUMBER",
    "TOKEN_STAR",
    "TOKEN_UNDERSCORE",
    "TOKEN_BIGGER_THAN",
    "TOKEN_PAREN_OPEN",
    "TOKEN_PAREN_CLOSE",
    "TOKEN_SQBR_OPEN",
    "TOKEN_SQBR_CLOSE",
    "TOKEN_BACKTICKS_OPEN",
    "TOKEN_BACKTICKS_CLOSE",
    "TOKEN_LINE_BREAK",
    "TOKEN_ESCAPE",
    "TOKEN_EXCLAMATION",
    "TOKEN_EOF",
};

#define INITIAL_CAPACITY 128

i8 new_lexer(lexer_t* lexer, const char* src_path) {
    // Read in the file's size
    i64 file_size = load_file(src_path, 0);
    if (file_size == -1) {
        fprintf(stderr, "Couldn't open file: %s\n", src_path);
        return -1;
    }

    // Allocate memory in lexer
    lexer->source = malloc(file_size);
    if (lexer->source == NULL) {
        fprintf(stderr, "Failed to allocate memory for source.\n");
        return -2;
    }

    // Load into the allocated source variable
    load_file(src_path, lexer->source);

    // Save file path
    lexer->file_path = str_dup(src_path);
    // Init cursor position to 0
    lexer->cur = 0;
    // Init line to 0 (zero indexing everything!!!!)
    lexer->line_num = 0;
    // Init character number to 0
    lexer->char_num = 0;

    return 0;
}

token_t* next_token(lexer_t* lexer) {
    // Ignore whitespace and tab
    while (lexer->source[lexer->cur] == ' ' || lexer->source[lexer->cur] == '\t') {
        lexer->char_num++;
        lexer->cur++;
    }

    // EOF
    if (lexer->source[lexer->cur] == 0) {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_EOF;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++; // also incrementing

        return token;
    }

    // # (Header token)
    if (lexer->source[lexer->cur] == '#') {
        // Store in case we need to recover
        u64 cursor = lexer->cur;
        
        u16 header_level = 0;
        while (lexer->source[lexer->cur] == '#') {
            lexer->cur++;
            header_level++;
        }

        // If next character is not a space after #,
        // we restore and won't return token.
        if (lexer->source[lexer->cur] != ' ') {
            lexer->cur = cursor;
        } else {
            // we calculate the character number we need to add
            // Adding this in the loop might lead to incorrect count if we bail
            lexer->char_num += lexer->cur - cursor;
            lexer->cur++; // Skip space

            // If we haven't bailed, we are returning the header token
            // where the value represents the depth/number of hashes
            token_t* token = malloc(sizeof(token_t));
            token->type = TOKEN_HEADER;
            token->value = NULL;
            token->level = header_level;
            token->line = lexer->line_num;
            token->character = lexer->char_num;

            return token;
        }
    }

    // Line break
    if (lexer->source[lexer->cur] == '\n') {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_LINE_BREAK;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num;

        // Increment cursor
        lexer->cur++;
        // Increment line number
        lexer->line_num++;
        // Reset character number
        lexer->char_num = 0;

        return token;
    }

    // Escape character
    if (lexer->source[lexer->cur] == '\\') {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_ESCAPE;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        lexer->cur++;

        return token;
    }

    // Unordered list tokens
    if ((lexer->source[lexer->cur] == '-' ||
        lexer->source[lexer->cur] == '*' ||
        lexer->source[lexer->cur] == '+') &&
        lexer->source[lexer->cur + 1] == ' ') {
        
        token_t* token = malloc(sizeof(token_t));
        token->type = lexer->source[lexer->cur] == '-' ? TOKEN_DASH :
                        lexer->source[lexer->cur] == '*' ? TOKEN_STAR : TOKEN_PLUS;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num;

        lexer->char_num += 2;
        lexer->cur += 2;
        return token;
    }

    // Ordered list
    if (lexer->source[lexer->cur] >= '0' && lexer->source[lexer->cur] <= '9') {
        // Store in case we need to recover
        u64 cursor = lexer->cur;
        
        u32 list_number = 0;
        // Check if we have more numbers
        while (lexer->source[lexer->cur] >= '0' && lexer->source[lexer->cur] <= '9') {
            list_number = list_number * 10 + (lexer->source[lexer->cur] - '0');
            lexer->cur++;
        }

        // If next character is not a dot after numbers and then a space,
        // we let the function continue.
        if (lexer->source[lexer->cur] != '.' && lexer->source[lexer->cur + 1] != ' ') {
            // Restore the cursor
            lexer->cur = cursor;
        } else {
            // we calculate the character number we need to add
            // Adding this in the loop might lead to incorrect count if we bail
            lexer->char_num += lexer->cur - cursor + 2;  // +2 for '.' and ' '
            lexer->cur += 2;  // Move past '.' and ' '

            // If we haven't bailed, we are returning the ordered list token
            // where the value represents the number used
            token_t* token = malloc(sizeof(token_t));
            token->type = TOKEN_NUMBER;
            token->value = NULL;
            token->level = list_number;
            token->line = lexer->line_num;
            token->character = lexer->char_num;

            return token;
        }
    }

    // Open parenthesis
    if (lexer->source[lexer->cur] == '(') {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_PAREN_OPEN;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        lexer->cur++;

        return token;
    }

    // Close parenthesis
    if (lexer->source[lexer->cur] == ')') {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_PAREN_CLOSE;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        lexer->cur++;

        return token;
    }

    // Open square bracket
    if (lexer->source[lexer->cur] == '[') {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_SQBR_OPEN;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        lexer->cur++;

        return token;
    }

    // Close square bracket
    if (lexer->source[lexer->cur] == ']') {
        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_SQBR_CLOSE;
        token->value = NULL;
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        lexer->cur++;

        return token;
    }

    // Exclamation for images
    if (lexer->source[lexer->cur] == '[' ||
        lexer->source[lexer->cur + 1] == '!') {
        
        // Skip character [
        lexer->char_num++;

        token_t* token = malloc(sizeof(token_t));
        token->type = TOKEN_EXCLAMATION;
        token->value = "!";
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        // Step two forward
        lexer->cur += 2;

        return token;
    }

    // Inline * and _ for potential emphasis
    if (lexer->source[lexer->cur] == '*' ||
        lexer->source[lexer->cur] == '_') {

        token_t* token = malloc(sizeof(token_t));
        token->type = lexer->source[lexer->cur] == '*' ? TOKEN_STAR : TOKEN_UNDERSCORE;
        token->value = lexer->source[lexer->cur] == '*' ? "*" : "_";
        token->line = lexer->line_num;
        token->character = lexer->char_num++;

        lexer->cur++;

        return token;
    }

    // Otherwise, read text until we find an inline token
    char* text_value = read_while(is_not_inline_token, lexer);

    token_t* token = malloc(sizeof(token_t));
    token->type = TOKEN_TEXT;
    token->value = text_value;
    token->line = lexer->line_num;
    token->character = lexer->char_num;
    // Characters are increased inside read_while...

    // Return text token
    return token;
}

token_array_t tokenize(lexer_t* lexer) {
    token_array_t token_array = { 
        malloc(sizeof(token_t) * INITIAL_CAPACITY),
        INITIAL_CAPACITY,
        0
    };

    while ((token_array.tokens[token_array.count] = *next_token(lexer)).type != TOKEN_EOF) {
        printf("[Token type]: %s, [Token value]: %s, [Token level]: %d\n", token_str[token_array.tokens[token_array.count].type], token_array.tokens[token_array.count].value, token_array.tokens[token_array.count].level);
        token_array.count++;
    }

    return token_array;
}

char* read_while(b8 (*predicate)(u8), lexer_t* lexer) {
    u64 start = lexer->cur;
    while (predicate(lexer->source[lexer->cur])) {
        lexer->char_num++;
        lexer->cur++;
    }
    u64 length = lexer->cur - start;
    char* result = malloc(length + 1);
    str_ncpy(result, &lexer->source[start], length);
    result[length] = '\0';

    return result;
}

// All the possible tokens that can appear inline
b8 is_not_inline_token(u8 c) {
    // Emphasis
    b8 em = c != '*' && c != '_';
    // Line break or EOF
    b8 neof = c != '\n' && c != '\0';
    // Links or images
    b8 li = c != '[' && c != ']' && c != ')';
    // Backticks and escape
    b8 bt = c != '`' && c != '\\';

    return em && neof && li && bt;
}

i64 load_file(const char* path, char* out) {
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
