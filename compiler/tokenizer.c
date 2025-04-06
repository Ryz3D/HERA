#include "includes.h"

typedef enum token_type {
    TOKEN_UNKNOWN,
    TOKEN_EOF,                // EOF
    TOKEN_KEYWORD,            // anything
    TOKEN_DIRECTIVE,          // #include
    TOKEN_STRING,             // "test"
    TOKEN_ANGLE_STRING,       // <test>
    TOKEN_CHAR,               // 'c'
    TOKEN_INT,                // 0
    TOKEN_FLOAT,              // 0.0
    TOKEN_SEMICOLON,          // ;
    TOKEN_COMMA,              // ,
    TOKEN_COLON,              // :
    TOKEN_DOT,                // .
    TOKEN_QUESTION,           // ?
    TOKEN_PLUS,               // +
    TOKEN_MINUS,              // -
    TOKEN_DOUBLE_PLUS,        // ++
    TOKEN_DOUBLE_MINUS,       // --
    TOKEN_STAR,               // *
    TOKEN_SLASH,              // /
    TOKEN_PERCENT,            // %
    TOKEN_CIRCUMFLEX,         // ^
    TOKEN_BAR,                // |
    TOKEN_AMPERSAND,          // &
    TOKEN_DOUBLE_BAR,         // ||
    TOKEN_DOUBLE_AMPERSAND,   // &&
    TOKEN_EXCLAMATION,        // !
    TOKEN_TILDE,              // ~
    TOKEN_BACKSLASH,          // "\"
    TOKEN_ARROW,              // ->
    TOKEN_ASSIGN,             // =
    TOKEN_ASSIGN_PLUS,        // +=
    TOKEN_ASSIGN_MINUS,       // -=
    TOKEN_ASSIGN_STAR,        // *=
    TOKEN_ASSIGN_SLASH,       // /=
    TOKEN_ASSIGN_PERCENT,     // %=
    TOKEN_ASSIGN_CIRCUMFLEX,  // ^=
    TOKEN_ASSIGN_BAR,         // |=
    TOKEN_ASSIGN_AMPERSAND,   // &=
    TOKEN_EQUAL,              // ==
    TOKEN_UNEQUAL,            // !=
    TOKEN_LESS,               // <
    TOKEN_LESS_EQ,            // <=
    TOKEN_GREATER,            // >
    TOKEN_GREATER_EQ,         // >=
    TOKEN_BRACE_R_L,          // (
    TOKEN_BRACE_R_R,          // )
    TOKEN_BRACE_S_L,          // [
    TOKEN_BRACE_S_R,          // ]
    TOKEN_BRACE_C_L,          // {
    TOKEN_BRACE_C_R,          // }
} token_type_t;

typedef struct token {
    token_type_t type;
    const char *str;
} token_t;

typedef struct tokenizer_state {
    FILE *f_src;
    char buffer;
    token_t **tokens;
    uint32_t *tokens_count;
} tokenizer_state_t;

bool cmp_tokenizer_read_char(tokenizer_state_t state) {
    return fread(&state.buffer, sizeof(char), 1, state.f_src) > 0;
}

bool cmp_tokenizer_skip_whitespace(tokenizer_state_t state) {
    return false;
}

bool cmp_tokenizer_skip_linebreaks(tokenizer_state_t state) {
    return false;
}

bool cmp_tokenizer_skip_ws_and_lb(tokenizer_state_t state) {
    bool skipped = false;
    while (cmp_tokenizer_skip_whitespace(state) || cmp_tokenizer_skip_linebreaks(state)) {
        skipped = true;
    }
    return skipped;
}

bool cmp_tokenizer_read_token(tokenizer_state_t state) {
    return true;
}

bool cmp_tokenizer_run(FILE *f_src, token_t **tokens, uint32_t *tokens_count) {
    tokenizer_state_t state = {
        .f_src = f_src,
        .buffer = '\0',
        .tokens = tokens,
        .tokens_count = tokens_count,
    };
    if (!cmp_tokenizer_read_char(state)) {
        return true;
    }
    if (!cmp_tokenizer_read_token(state)) {
        return false;
    }
    return true;
}
