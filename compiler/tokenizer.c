#include "includes.h"

typedef enum token_type {
    TOKEN_ASSIGN_DOUBLE_LEFT,   // <<=
    TOKEN_ASSIGN_DOUBLE_RIGHT,  // >>=
    TOKEN_DOUBLE_PLUS,          // ++
    TOKEN_DOUBLE_MINUS,         // --
    TOKEN_DOUBLE_BAR,           // ||
    TOKEN_DOUBLE_AMPERSAND,     // &&
    TOKEN_ARROW,                // ->
    TOKEN_ASSIGN_PLUS,          // +=
    TOKEN_ASSIGN_MINUS,         // -=
    TOKEN_ASSIGN_STAR,          // *=
    TOKEN_ASSIGN_SLASH,         // /=
    TOKEN_ASSIGN_PERCENT,       // %=
    TOKEN_ASSIGN_CIRCUMFLEX,    // ^=
    TOKEN_ASSIGN_BAR,           // |=
    TOKEN_ASSIGN_AMPERSAND,     // &=
    TOKEN_EQUAL,                // ==
    TOKEN_UNEQUAL,              // !=
    TOKEN_LESS_EQ,              // <=
    TOKEN_GREATER_EQ,           // >=
    TOKEN_DOUBLE_LEFT,          // <<
    TOKEN_DOUBLE_RIGHT,         // >>
    TOKEN_SEMICOLON,            // ;
    TOKEN_COMMA,                // ,
    TOKEN_COLON,                // :
    TOKEN_DOT,                  // .
    TOKEN_QUESTION,             // ?
    TOKEN_PLUS,                 // +
    TOKEN_MINUS,                // -
    TOKEN_STAR,                 // *
    TOKEN_SLASH,                // /
    TOKEN_PERCENT,              // %
    TOKEN_CIRCUMFLEX,           // ^
    TOKEN_BAR,                  // |
    TOKEN_AMPERSAND,            // &
    TOKEN_EXCLAMATION,          // !
    TOKEN_TILDE,                // ~
    TOKEN_BACKSLASH,            // "\"
    TOKEN_ASSIGN,               // =
    TOKEN_LESS,                 // <
    TOKEN_GREATER,              // >
    TOKEN_BRACE_R_L,            // (
    TOKEN_BRACE_R_R,            // )
    TOKEN_BRACE_S_L,            // [
    TOKEN_BRACE_S_R,            // ]
    TOKEN_BRACE_C_L,            // {
    TOKEN_BRACE_C_R,            // }
    TOKEN_ENDL,                 // \n
    TOKEN_KEYWORD,              // anything
    TOKEN_DIRECTIVE,            // #include
    TOKEN_STRING,               // "test"
    TOKEN_ANGLE_STRING,         // <test>
    TOKEN_CHAR,                 // 'c'
    TOKEN_INT_DEC,              // 1
    TOKEN_INT_OCT,              // 01
    TOKEN_INT_BIN,              // 0b1
    TOKEN_INT_HEX,              // 0x1
    TOKEN_FLOAT,                // 0.0
    TOKEN_EOF,
    TOKEN_UNKNOWN,
} token_type_t;

bool cmp_tokenizer_token_prefix_un_operator(token_type_t t) {
    if (t == TOKEN_DOUBLE_PLUS ||
        t == TOKEN_DOUBLE_MINUS ||
        t == TOKEN_PLUS ||
        t == TOKEN_MINUS ||
        t == TOKEN_STAR ||
        t == TOKEN_AMPERSAND ||
        t == TOKEN_EXCLAMATION ||
        t == TOKEN_TILDE) {
        return true;
    }
    return false;
}

bool cmp_tokenizer_token_suffix_un_operator(token_type_t t) {
    if (t == TOKEN_DOUBLE_PLUS ||
        t == TOKEN_DOUBLE_MINUS) {
        return true;
    }
    return false;
}

int8_t cmp_tokenizer_precedence_un_operator(token_type_t t) {
    switch (t) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_DOUBLE_PLUS: // TODO: highest precedence if postfix
        case TOKEN_DOUBLE_MINUS:
        case TOKEN_STAR:
        case TOKEN_AMPERSAND:
        case TOKEN_EXCLAMATION:
        case TOKEN_TILDE:
            return 1;
        default:
            return 0;
    }
}

int8_t cmp_tokenizer_precedence_bi_operator(token_type_t t) {
    switch (t) {
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT:
            return 3;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return 4;
        case TOKEN_DOUBLE_LEFT:
        case TOKEN_DOUBLE_RIGHT:
            return 5;
        case TOKEN_LESS_EQ:
        case TOKEN_GREATER_EQ:
        case TOKEN_LESS:
        case TOKEN_GREATER:
            return 6;
        case TOKEN_EQUAL:
        case TOKEN_UNEQUAL:
            return 7;
        case TOKEN_AMPERSAND:
            return 8;
        case TOKEN_CIRCUMFLEX:
            return 9;
        case TOKEN_BAR:
            return 10;
        case TOKEN_DOUBLE_AMPERSAND:
            return 11;
        case TOKEN_DOUBLE_BAR:
            return 12;
        case TOKEN_ASSIGN:
        case TOKEN_ASSIGN_PLUS:
        case TOKEN_ASSIGN_MINUS:
        case TOKEN_ASSIGN_STAR:
        case TOKEN_ASSIGN_SLASH:
        case TOKEN_ASSIGN_PERCENT:
        case TOKEN_ASSIGN_CIRCUMFLEX:
        case TOKEN_ASSIGN_BAR:
        case TOKEN_ASSIGN_AMPERSAND:
        case TOKEN_ASSIGN_DOUBLE_LEFT:
        case TOKEN_ASSIGN_DOUBLE_RIGHT:
            return 14;
        default:
            return 0;
    }
}

#define TOKEN_STR_COUNT 47
#define TOKEN_STR_MAX_LEN 3

const uint32_t token_str_len_transitions[] = {2, 21};

const char *token_str[] = {
    "<<=",
    ">>=",
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
    "<<",
    ">>",
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
    bool allocated;
    char *str;
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
    while (!state->ended) {
        cmp_tokenizer_skip_whitespace(state);

        bool skip_linebreaks = true;
        if ((*state->tokens_count) > 0) {
            token_type_t t = (*state->tokens)[(*state->tokens_count) - 1].type;
            skip_linebreaks =
                t != TOKEN_BACKSLASH &&
                t != TOKEN_KEYWORD &&
                t != TOKEN_DIRECTIVE;
        }

        char f_buffer[TOKEN_STR_MAX_LEN];
        memset(f_buffer, 0, sizeof(f_buffer));
        fpos_t start;
        do {
            fgetpos(state->f_src, &start);
            f_buffer[0] = cmp_tokenizer_read_char(state);
        } while ((f_buffer[0] == '\r' || (f_buffer[0] == '\n' && skip_linebreaks)) && !state->ended);
        fsetpos(state->f_src, &start);
        for (uint32_t i = 0; i < TOKEN_STR_MAX_LEN && !state->ended; i++) {
            f_buffer[i] = cmp_tokenizer_read_char(state);
        }
        fsetpos(state->f_src, &start);

        if (f_buffer[0] == '\'') {
            char *str = malloc(2 * sizeof(char));
            if (str == NULL) {
                printf("ERROR: out of memory" ENDL);
                return false;
            }
            cmp_tokenizer_read_char(state);
            str[0] = cmp_tokenizer_read_char(state);
            if (str[0] == '\\') {
                str[0] = cmp_tokenizer_read_char(state);
                switch (str[0]) {
                    case '0':
                        str[0] = '\0'; // TODO: other values
                        break;
                    case 'a':
                        str[0] = '\a';
                        break;
                    case 'b':
                        str[0] = '\b';
                        break;
                    case 'e':
                        str[0] = '\e';
                        break;
                    case 'f':
                        str[0] = '\f';
                        break;
                    case 'n':
                        str[0] = '\n';
                        break;
                    case 'r':
                        str[0] = '\r';
                        break;
                    case 't':
                        str[0] = '\t';
                        break;
                    case 'u':
                        // TODO: unicode
                        str[0] = ' ';
                        break;
                    case 'v':
                        str[0] = '\v';
                        break;
                    case 'x':
                        // TODO: hex
                        str[0] = ' ';
                        break;
                    case '\\':
                        str[0] = '\\';
                        break;
                    case '"':
                        str[0] = '"';
                        break;
                    case '?':
                        str[0] = '?';
                        break;
                    case '\'':
                        str[0] = '\'';
                        break;
                    default:
                        printf("ERROR: unknown escape sequence \"\\%c\"" ENDL, str[0]);
                        return false;
                }
            }
            char c = cmp_tokenizer_read_char(state);
            if (c != '\'') {
                free(str);
                printf("ERROR: missing closing '" ENDL);
                return false;
            }
            str[1] = '\0';
            return cmp_tokenizer_push_token(state, (token_t){
                .type = TOKEN_CHAR,
                .str = str,
                .allocated = true,
            });
        } else if (f_buffer[0] == '"') {
            uint32_t str_length = 0, str_capacity = 1;
            char *str = malloc(str_capacity * sizeof(char));
            if (str == NULL) {
                printf("ERROR: out of memory" ENDL);
                return false;
            }
            cmp_tokenizer_read_char(state);
            do {
                char c = cmp_tokenizer_read_char(state);
                if (c == '\\') {
                    c = cmp_tokenizer_read_char(state);
                    if (c == '\r') {
                        c = cmp_tokenizer_read_char(state);
                    }
                    switch (c) {
                        case '\n':
                            break; // line continuation
                        case '0':
                            c = '\0'; // TODO: other values
                            break;
                        case 'a':
                            c = '\a';
                            break;
                        case 'b':
                            c = '\b';
                            break;
                        case 'e':
                            c = '\e';
                            break;
                        case 'f':
                            c = '\f';
                            break;
                        case 'n':
                            c = '\n';
                            break;
                        case 'r':
                            c = '\r';
                            break;
                        case 't':
                            c = '\t';
                            break;
                        case 'u':
                            // TODO: unicode
                            c = ' ';
                            break;
                        case 'v':
                            c = '\v';
                            break;
                        case 'x':
                            // TODO: hex
                            c = ' ';
                            break;
                        case '\\':
                            c = '\\';
                            break;
                        case '"':
                            c = '"';
                            break;
                        case '?':
                            c = '?';
                            break;
                        case '\'':
                            c = '\'';
                            break;
                        default:
                            printf("ERROR: unknown escape sequence \"\\%c\"" ENDL, c);
                            return false;
                    }
                }
                str[str_length++] = c;
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf("ERROR: out of memory" ENDL);
                        return false;
                    }
                }
            } while (str[str_length - 1] != '"' && !state->ended);
            if (str[str_length - 1] != '"') {
                free(str);
                printf("ERROR: missing closing \"" ENDL);
                return false;
            }
            str[str_length - 1] = '\0';
            return cmp_tokenizer_push_token(state, (token_t){
                .type = TOKEN_STRING,
                .str = str,
                .allocated = true,
            });
        } else if (f_buffer[0] == '<') {
            uint32_t str_length = 0, str_capacity = 1;
            char *str = malloc(str_capacity * sizeof(char));
            if (str == NULL) {
                printf("ERROR: out of memory" ENDL);
                return false;
            }
            cmp_tokenizer_read_char(state);
            do {
                str[str_length++] = cmp_tokenizer_read_char(state);
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf("ERROR: out of memory" ENDL);
                        return false;
                    }
                }
            } while (str[str_length - 1] != '>' && !state->ended);
            if (str[str_length - 1] != '>') {
                free(str);
                printf("ERROR: missing closing >" ENDL);
                return false;
            }
            str[str_length - 1] = '\0';
            return cmp_tokenizer_push_token(state, (token_t){
                .type = TOKEN_ANGLE_STRING,
                .str = str,
                .allocated = true,
            });
        } else if (f_buffer[0] == '#') {
            uint32_t str_length = 0, str_capacity = 1;
            char *str = malloc(str_capacity * sizeof(char));
            if (str == NULL) {
                printf("ERROR: out of memory" ENDL);
                return false;
            }
            cmp_tokenizer_read_char(state);
            do {
                fgetpos(state->f_src, &start);
                str[str_length++] = cmp_tokenizer_read_char(state);
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf("ERROR: out of memory" ENDL);
                        return false;
                    }
                }
            } while (((str[str_length - 1] >= 'a' && str[str_length - 1] <= 'z') ||
                      (str[str_length - 1] >= 'A' && str[str_length - 1] <= 'Z') ||
                      (str[str_length - 1] >= '0' && str[str_length - 1] <= '9') ||
                       str[str_length - 1] == '_') && !state->ended);
            str[str_length - 1] = '\0';
            fsetpos(state->f_src, &start);
            return cmp_tokenizer_push_token(state, (token_t){
                .type = TOKEN_DIRECTIVE,
                .str = str,
                .allocated = true,
            });
        } else if ((f_buffer[0] >= 'a' && f_buffer[0] <= 'z') ||
                   (f_buffer[0] >= 'A' && f_buffer[0] <= 'Z') ||
                    f_buffer[0] == '_') {
            uint32_t str_length = 0, str_capacity = 1;
            char *str = malloc(str_capacity * sizeof(char));
            if (str == NULL) {
                printf("ERROR: out of memory" ENDL);
                return false;
            }
            do {
                fgetpos(state->f_src, &start);
                str[str_length++] = cmp_tokenizer_read_char(state);
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf("ERROR: out of memory" ENDL);
                        return false;
                    }
                }
            } while (((str[str_length - 1] >= 'a' && str[str_length - 1] <= 'z') ||
                      (str[str_length - 1] >= 'A' && str[str_length - 1] <= 'Z') ||
                      (str[str_length - 1] >= '0' && str[str_length - 1] <= '9') ||
                       str[str_length - 1] == '_') && !state->ended);
            str[str_length - 1] = '\0';
            fsetpos(state->f_src, &start);
            return cmp_tokenizer_push_token(state, (token_t){
                .type = TOKEN_KEYWORD,
                .str = str,
                .allocated = true,
            });
        } else if (f_buffer[0] == '0' && (f_buffer[1] == 'b' || f_buffer[1] == 'B')) {
            uint32_t str_length = 0, str_capacity = 1;
            char *str = malloc(str_capacity * sizeof(char));
            if (str == NULL) {
                printf("ERROR: out of memory" ENDL);
                return false;
            }
            cmp_tokenizer_read_char(state);
            cmp_tokenizer_read_char(state);
            do {
                fgetpos(state->f_src, &start);
                str[str_length++] = cmp_tokenizer_read_char(state);
                if (str[str_length - 1] == '\'') {
                    str_length--;
                }
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf("ERROR: out of memory" ENDL);
                        return false;
                    }
                }
            } while ((str[str_length - 1] >= '0' && str[str_length - 1] <= '1') && !state->ended);
            str[str_length - 1] = '\0';
            fsetpos(state->f_src, &start);
            return cmp_tokenizer_push_token(state, (token_t){
                .type = TOKEN_INT_BIN,
                .str = str,
                .allocated = true,
            });
        } else if (f_buffer[0] == '0' && (f_buffer[1] == 'x' || f_buffer[1] == 'X')) {
            uint32_t str_length = 0, str_capacity = 1;
            char *str = malloc(str_capacity * sizeof(char));
            if (str == NULL) {
                printf("ERROR: out of memory" ENDL);
                return false;
            }
            cmp_tokenizer_read_char(state);
            cmp_tokenizer_read_char(state);
            do {
                fgetpos(state->f_src, &start);
                str[str_length++] = cmp_tokenizer_read_char(state);
                if (str[str_length - 1] == '\'') {
                    str_length--;
                }
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf("ERROR: out of memory" ENDL);
                        return false;
                    }
                }
            } while (((str[str_length - 1] >= '0' && str[str_length - 1] <= '9') ||
                      (str[str_length - 1] >= 'a' && str[str_length - 1] <= 'f') ||
                      (str[str_length - 1] >= 'A' && str[str_length - 1] <= 'F')) &&
                       !state->ended);
            str[str_length - 1] = '\0';
            fsetpos(state->f_src, &start);
            return cmp_tokenizer_push_token(state, (token_t){
                .type = TOKEN_INT_HEX,
                .str = str,
                .allocated = true,
            });
        } else if (f_buffer[0] >= '0' && f_buffer[0] <= '9') {
            uint32_t str_length = 0, str_capacity = 1;
            char *str = malloc(str_capacity * sizeof(char));
            if (str == NULL) {
                printf("ERROR: out of memory" ENDL);
                return false;
            }
            do {
                fgetpos(state->f_src, &start);
                str[str_length++] = cmp_tokenizer_read_char(state);
                if (str[str_length - 1] == '\'') {
                    str_length--;
                }
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf("ERROR: out of memory" ENDL);
                        return false;
                    }
                }
            } while ((str[str_length - 1] >= '0' && str[str_length - 1] <= '9') &&
                      !state->ended);
            str[str_length - 1] = '\0';
            fsetpos(state->f_src, &start);
            return cmp_tokenizer_push_token(state, (token_t){
                .type = str[0] == '0' ? TOKEN_INT_OCT : TOKEN_INT_DEC,
                .str = str,
                .allocated = true,
            });
        } else if (f_buffer[0] == '/' && f_buffer[1] == '/') {
            char c;
            do {
                c = cmp_tokenizer_read_char(state);
            } while (c != '\n' && !state->ended);
            continue;
        } else if (f_buffer[0] == '/' && f_buffer[1] == '*') {
            cmp_tokenizer_read_char(state);
            cmp_tokenizer_read_char(state);
            char c[2];
            c[0] = cmp_tokenizer_read_char(state);
            c[1] = cmp_tokenizer_read_char(state);
            do {
                c[0] = c[1];
                c[1] = cmp_tokenizer_read_char(state);
            } while ((c[0] != '*' || c[1] != '/') && !state->ended);
            continue;
        }

        // length of current token = TOKEN_STR_MAX_LEN - token_str_len_counter
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

        if (state->ended) {
            break;
        }
        if (cmp_tokenizer_skip_whitespace(state)) {
            continue;
        }

        char c = cmp_tokenizer_read_char(state);
        printf("unexpected character '%c'" ENDL, c);
        return false;
    }

    cmp_tokenizer_push_token(state, (token_t){
        .type = TOKEN_EOF,
    });
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
    }
    return true;
}

void cmp_tokenizer_free(token_t *tokens, uint32_t tokens_count) {
    for (uint32_t i = 0; i < tokens_count; i++) {
        if (tokens[i].allocated) {
            free(tokens[i].str);
        }
    }
    free(tokens);
}
