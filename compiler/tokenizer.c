#include "includes.h"

typedef enum token_type {
    TOKEN_DOUBLE_PLUS,        // ++
    TOKEN_DOUBLE_MINUS,       // --
    TOKEN_DOUBLE_BAR,         // ||
    TOKEN_DOUBLE_AMPERSAND,   // &&
    TOKEN_ARROW,              // ->
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
    TOKEN_LESS_EQ,            // <=
    TOKEN_GREATER_EQ,         // >=
    TOKEN_SEMICOLON,          // ;
    TOKEN_COMMA,              // ,
    TOKEN_COLON,              // :
    TOKEN_DOT,                // .
    TOKEN_QUESTION,           // ?
    TOKEN_PLUS,               // +
    TOKEN_MINUS,              // -
    TOKEN_STAR,               // *
    TOKEN_SLASH,              // /
    TOKEN_PERCENT,            // %
    TOKEN_CIRCUMFLEX,         // ^
    TOKEN_BAR,                // |
    TOKEN_AMPERSAND,          // &
    TOKEN_EXCLAMATION,        // !
    TOKEN_TILDE,              // ~
    TOKEN_BACKSLASH,          // "\"
    TOKEN_ASSIGN,             // =
    TOKEN_LESS,               // <
    TOKEN_GREATER,            // >
    TOKEN_BRACE_R_L,          // (
    TOKEN_BRACE_R_R,          // )
    TOKEN_BRACE_S_L,          // [
    TOKEN_BRACE_S_R,          // ]
    TOKEN_BRACE_C_L,          // {
    TOKEN_BRACE_C_R,          // }
    TOKEN_ENDL,               // \n
    TOKEN_KEYWORD,            // anything
    TOKEN_DIRECTIVE,          // #include
    TOKEN_STRING,             // "test"
    TOKEN_ANGLE_STRING,       // <test>
    TOKEN_CHAR,               // 'c'
    TOKEN_INT,                // 0
    TOKEN_FLOAT,              // 0.0
    TOKEN_EOF,
    TOKEN_UNKNOWN,
} token_type_t;

#define TOKEN_STR_COUNT 42
#define TOKEN_STR_MAX_LEN 2

const uint32_t token_str_len_transitions[] = { 17 };

const char *token_str[] = {
    "++",
    "--",
    "||",
    "&&",
    "->",
    "+=",
    "-=",
    "*=",
    "/=",
    "%%=",
    "^=",
    "|=",
    "&=",
    "==",
    "!=",
    "<=",
    ">=",
    ";",
    ",",
    ":",
    ".",
    "?",
    "+",
    "-",
    "*",
    "/",
    "%%",
    "^",
    "|",
    "&",
    "!",
    "~",
    "\\",
    "=",
    "<",
    ">",
    "(",
    ")",
    "[",
    "]",
    "{",
    "}",
    "\n",
};

typedef struct token {
    token_type_t type;
    const char *str;
} token_t;

typedef struct tokenizer_state {
    FILE *f_src;
    token_t **tokens;
    uint32_t *tokens_count;
    bool ended;
} tokenizer_state_t;

char cmp_tokenizer_read_char(tokenizer_state_t *state) {
    char c = '\0';
    if (!state->ended) {
        state->ended = fread(&c, sizeof(char), 1, state->f_src) == 0;
    }
    return c;
}

// returns true if character(s) skipped
bool cmp_tokenizer_skip_whitespace(tokenizer_state_t *state) {
    bool skipped = false;
    fpos_t pos;

    while (!state->ended) {
        fgetpos(state->f_src, &pos);
        char c = cmp_tokenizer_read_char(state);
        if (c != ' ' && c != '\t' && c != '\r') {
            fsetpos(state->f_src, &pos);
            break;
        }
        skipped = true;
    }
    return skipped;
}

// returns false on error
bool cmp_tokenizer_push_token(tokenizer_state_t *state, token_t token) {
    (*state->tokens_count)++;
    *state->tokens = realloc(*state->tokens, (*state->tokens_count) * sizeof(token_t));
    if (*state->tokens == NULL) {
        printf("(tokenizer) out of memory" ENDL);
        return false;
    }
    (*state->tokens)[(*state->tokens_count) - 1] = token;
    return true;
}

// returns false on error
bool cmp_tokenizer_read_token(tokenizer_state_t *state) {
    fpos_t start;
    fgetpos(state->f_src, &start);
    char f_buffer[TOKEN_STR_MAX_LEN];
    memset(f_buffer, 0, sizeof(f_buffer));
    for (uint32_t i = 0; i < TOKEN_STR_MAX_LEN && !state->ended; i++) {
        f_buffer[i] = cmp_tokenizer_read_char(state);
    }
    fsetpos(state->f_src, &start);

    // token_str_len = TOKEN_STR_MAX_LEN - token_str_len_counter
    uint32_t token_str_len_counter = 0;
    for (uint32_t i = 0; i < TOKEN_STR_COUNT; i++) {
        if (i == token_str_len_transitions[token_str_len_counter] &&
            token_str_len_counter < sizeof(token_str_len_transitions) / sizeof(uint32_t)) {
            token_str_len_counter++;
        }
        bool match = true;
        for (uint32_t j = 0; j < TOKEN_STR_MAX_LEN - token_str_len_counter; j++) {
            if (f_buffer[j] != token_str[i][j]) {
                match = false;
                break;
            }
        }
        if (match) {
            for (uint32_t j = 0; j < TOKEN_STR_MAX_LEN - token_str_len_counter; j++) {
                cmp_tokenizer_read_char(state);
            }
            return cmp_tokenizer_push_token(state, (token_t){
                .type = (token_type_t)i,
                .str = NULL,
            });
        }
    }

    // TODO: keywords/strings
    // TODO: comments

    // TODO: maybe this should be false? (error on no match)
    cmp_tokenizer_read_char(state);
    return true;
}

bool cmp_tokenizer_run(FILE *f_src, token_t **tokens, uint32_t *tokens_count) {
    tokenizer_state_t state = {
        .f_src = f_src,
        .tokens = tokens,
        .tokens_count = tokens_count,
    };
    while (!state.ended) {
        if (!cmp_tokenizer_read_token(&state)) {
            return false;
        }
        cmp_tokenizer_skip_whitespace(&state);
    }
    return true;
}
