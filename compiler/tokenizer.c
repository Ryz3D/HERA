#ifndef INC_TOKENIZER_C
#define INC_TOKENIZER_C

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
    TOKEN_BRACKET_R_L,          // (
    TOKEN_BRACKET_R_R,          // )
    TOKEN_BRACKET_S_L,          // [
    TOKEN_BRACKET_S_R,          // ]
    TOKEN_BRACKET_C_L,          // {
    TOKEN_BRACKET_C_R,          // }
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

typedef struct tokenizer_pos {
    fpos_t f;
    uint32_t i;
} tokenizer_pos_t;

typedef struct token {
    token_type_t type;
    bool allocated;
    char *str;
    const char *f_path;
    uint32_t line, col;
} token_t;

typedef struct tokenizer_state {
    const char *f_path;
    FILE *f_src;
    token_t **tokens;
    uint32_t *tokens_count;
    bool ended;
    tokenizer_pos_t pos;
    tokenizer_pos_t last_linebreak_pos;
    uint32_t current_line;
} tokenizer_state_t;

#define TOKEN_POS_FORMAT "\e[1;36m%s:%u:%u:\e[m "
#define TOKEN_POS_FORMAT_VALUES(t) (t).f_path, (t).line, (t).col

void cmp_tokenizer_set_pos(tokenizer_state_t *state, tokenizer_pos_t pos) {
    fsetpos(state->f_src, &pos.f);
    state->pos = pos;
}

char cmp_tokenizer_read_char(tokenizer_state_t *state) {
    char c = '\0';
    if (!state->ended) {
        state->ended = fread(&c, sizeof(char), 1, state->f_src) == 0;
        fgetpos(state->f_src, &state->pos.f);
        state->pos.i++;
    }
    return c;
}

// returns true if character(s) skipped
bool cmp_tokenizer_skip_whitespace(tokenizer_state_t *state) {
    bool skipped = false;

    while (!state->ended) {
        tokenizer_pos_t pos = state->pos;
        char c = cmp_tokenizer_read_char(state);
        if (c != ' ' && c != '\t' && c != '\r') {
            cmp_tokenizer_set_pos(state, pos);
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
    (*state->tokens)[(*state->tokens_count) - 1].f_path = state->f_path;
    (*state->tokens)[(*state->tokens_count) - 1].line = state->current_line;
    (*state->tokens)[(*state->tokens_count) - 1].col = state->pos.i - state->last_linebreak_pos.i;
    return true;
}

// returns false on error
bool cmp_tokenizer_read_token(tokenizer_state_t *state) {
    while (!state->ended) {
        cmp_tokenizer_skip_whitespace(state);

        bool skip_linebreaks = true;
        if ((*state->tokens_count) > 0) {
            token_type_t t = (*state->tokens)[(*state->tokens_count) - 1].type;
            skip_linebreaks = t != TOKEN_DIRECTIVE;
        }

        char f_buffer[TOKEN_STR_MAX_LEN];
        memset(f_buffer, 0, sizeof(f_buffer));
        tokenizer_pos_t start;
        do {
            if (f_buffer[0] == '\n') {
                state->last_linebreak_pos = state->pos;
                state->current_line++;
            }
            start = state->pos;
            f_buffer[0] = cmp_tokenizer_read_char(state);
        } while ((f_buffer[0] == '\r' || (f_buffer[0] == '\n' && skip_linebreaks)) && !state->ended);
        cmp_tokenizer_set_pos(state, start);
        for (uint32_t i = 0; i < TOKEN_STR_MAX_LEN && !state->ended; i++) {
            f_buffer[i] = cmp_tokenizer_read_char(state);
        }
        cmp_tokenizer_set_pos(state, start);

        if (f_buffer[0] == '\'') {
            char *str = malloc(2 * sizeof(char));
            if (str == NULL) {
                printf(ERROR "out of memory" ENDL);
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
                        printf(ERROR "unknown escape sequence \"\\%c\"" ENDL, str[0]);
                        return false;
                }
            }
            char c = cmp_tokenizer_read_char(state);
            if (c != '\'') {
                free(str);
                printf(ERROR "missing closing '" ENDL);
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
                printf(ERROR "out of memory" ENDL);
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
                            state->last_linebreak_pos = state->pos;
                            state->current_line++;
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
                            printf(ERROR "unknown escape sequence \"\\%c\"" ENDL, c);
                            return false;
                    }
                } else if (c == '\n') {
                    state->last_linebreak_pos = state->pos;
                    state->current_line++;
                }
                str[str_length++] = c;
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf(ERROR "out of memory" ENDL);
                        return false;
                    }
                }
            } while (str[str_length - 1] != '"' && !state->ended);
            if (str[str_length - 1] != '"') {
                free(str);
                printf(ERROR "missing closing \"" ENDL);
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
                printf(ERROR "out of memory" ENDL);
                return false;
            }
            cmp_tokenizer_read_char(state);
            do {
                str[str_length++] = cmp_tokenizer_read_char(state);
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf(ERROR "out of memory" ENDL);
                        return false;
                    }
                }
                if (str[str_length - 1] == '\n') {
                    if (str[str_length - 2] == '\\') {
                        str_length -= 2;
                    } else {
                        free(str);
                        printf(ERROR "missing closing > before newline" ENDL);
                        return false;
                    }
                }
            } while (str[str_length - 1] != '>' && !state->ended);
            if (str[str_length - 1] != '>') {
                free(str);
                printf(ERROR "missing closing >" ENDL);
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
                printf(ERROR "out of memory" ENDL);
                return false;
            }
            cmp_tokenizer_read_char(state);
            do {
                start = state->pos;
                str[str_length++] = cmp_tokenizer_read_char(state);
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf(ERROR "out of memory" ENDL);
                        return false;
                    }
                }
            } while (((str[str_length - 1] >= 'a' && str[str_length - 1] <= 'z') ||
                      (str[str_length - 1] >= 'A' && str[str_length - 1] <= 'Z') ||
                      (str[str_length - 1] >= '0' && str[str_length - 1] <= '9') ||
                       str[str_length - 1] == '_') && !state->ended);
            str[str_length - 1] = '\0';
            cmp_tokenizer_set_pos(state, start);
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
                printf(ERROR "out of memory" ENDL);
                return false;
            }
            do {
                start = state->pos;
                str[str_length++] = cmp_tokenizer_read_char(state);
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf(ERROR "out of memory" ENDL);
                        return false;
                    }
                }
            } while (((str[str_length - 1] >= 'a' && str[str_length - 1] <= 'z') ||
                      (str[str_length - 1] >= 'A' && str[str_length - 1] <= 'Z') ||
                      (str[str_length - 1] >= '0' && str[str_length - 1] <= '9') ||
                       str[str_length - 1] == '_') && !state->ended);
            str[str_length - 1] = '\0';
            cmp_tokenizer_set_pos(state, start);
            return cmp_tokenizer_push_token(state, (token_t){
                .type = TOKEN_KEYWORD,
                .str = str,
                .allocated = true,
            });
        } else if (f_buffer[0] == '0' && (f_buffer[1] == 'b' || f_buffer[1] == 'B')) {
            uint32_t str_length = 0, str_capacity = 1;
            char *str = malloc(str_capacity * sizeof(char));
            if (str == NULL) {
                printf(ERROR "out of memory" ENDL);
                return false;
            }
            cmp_tokenizer_read_char(state);
            cmp_tokenizer_read_char(state);
            do {
                start = state->pos;
                str[str_length++] = cmp_tokenizer_read_char(state);
                if (str[str_length - 1] == '\'') {
                    str_length--;
                }
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf(ERROR "out of memory" ENDL);
                        return false;
                    }
                }
            } while ((str[str_length - 1] >= '0' && str[str_length - 1] <= '1') && !state->ended);
            str[str_length - 1] = '\0';
            cmp_tokenizer_set_pos(state, start);
            return cmp_tokenizer_push_token(state, (token_t){
                .type = TOKEN_INT_BIN,
                .str = str,
                .allocated = true,
            });
        } else if (f_buffer[0] == '0' && (f_buffer[1] == 'x' || f_buffer[1] == 'X')) {
            uint32_t str_length = 0, str_capacity = 1;
            char *str = malloc(str_capacity * sizeof(char));
            if (str == NULL) {
                printf(ERROR "out of memory" ENDL);
                return false;
            }
            cmp_tokenizer_read_char(state);
            cmp_tokenizer_read_char(state);
            do {
                start = state->pos;
                str[str_length++] = cmp_tokenizer_read_char(state);
                if (str[str_length - 1] == '\'') {
                    str_length--;
                }
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf(ERROR "out of memory" ENDL);
                        return false;
                    }
                }
            } while (((str[str_length - 1] >= '0' && str[str_length - 1] <= '9') ||
                      (str[str_length - 1] >= 'a' && str[str_length - 1] <= 'f') ||
                      (str[str_length - 1] >= 'A' && str[str_length - 1] <= 'F')) &&
                       !state->ended);
            str[str_length - 1] = '\0';
            cmp_tokenizer_set_pos(state, start);
            return cmp_tokenizer_push_token(state, (token_t){
                .type = TOKEN_INT_HEX,
                .str = str,
                .allocated = true,
            });
        } else if (f_buffer[0] >= '0' && f_buffer[0] <= '9') {
            uint32_t str_length = 0, str_capacity = 1;
            char *str = malloc(str_capacity * sizeof(char));
            if (str == NULL) {
                printf(ERROR "out of memory" ENDL);
                return false;
            }
            do {
                start = state->pos;
                str[str_length++] = cmp_tokenizer_read_char(state);
                if (str[str_length - 1] == '\'') {
                    str_length--;
                }
                if (str_length >= str_capacity) {
                    str_capacity *= 2;
                    str = realloc(str, str_capacity * sizeof(char));
                    if (str == NULL) {
                        printf(ERROR "out of memory" ENDL);
                        return false;
                    }
                }
            } while ((str[str_length - 1] >= '0' && str[str_length - 1] <= '9') &&
                      !state->ended);
            str[str_length - 1] = '\0';
            cmp_tokenizer_set_pos(state, start);
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
            state->last_linebreak_pos = state->pos;
            state->current_line++;
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

bool cmp_tokenizer_run(const char *f_path, token_t **tokens, uint32_t *tokens_count) {
    *tokens = NULL;
    *tokens_count = 0;

    if (f_path == NULL) {
        printf("source file path null" ENDL);
        return false;
    }

	FILE *f = fopen(f_path, "r");
	if (f == NULL) {
        printf(ERROR "could not open source file \"%s\"" ENDL, f_path);
        return false;
	}

    tokenizer_state_t state = {
        .f_path = f_path,
        .f_src = f,
        .tokens = tokens,
        .tokens_count = tokens_count,
        .pos = {
            .i = -1,
        },
        .last_linebreak_pos = {
            .i = -1,
        },
        .current_line = 1,
    };
    while (!state.ended) {
        if (!cmp_tokenizer_read_token(&state)) {
            return false;
        }
    }

    fclose(f);

    uint32_t deleted_tokens = 0;
    uint32_t *bracket_indices = NULL;
    uint32_t bracket_count = 0;
    for (uint32_t i = 0; i < *tokens_count; i++) {
        if (i < (*tokens_count) - 1) {
            if ((*tokens)[i].type == TOKEN_DIRECTIVE && (*tokens)[i + 1].type == TOKEN_ENDL) {
                printf(TOKEN_POS_FORMAT ERROR "line break after #directive" ENDL, TOKEN_POS_FORMAT_VALUES((*tokens)[i]));
                return false;
            }
        }
        switch ((*tokens)[i].type) {
            case TOKEN_BACKSLASH:
            case TOKEN_ENDL:
                memcpy(&(*tokens)[i], &(*tokens)[i + 1], ((*tokens_count) - i) * sizeof(token_t));
                (*tokens_count)--;
                deleted_tokens++;
                break;
            case TOKEN_BRACKET_R_L:
            case TOKEN_BRACKET_S_L:
            case TOKEN_BRACKET_C_L:
                bracket_count++;
                bracket_indices = realloc(bracket_indices, bracket_count * sizeof(uint32_t));
                if (bracket_indices == NULL) {
                    printf(ERROR "out of memory" ENDL);
                    return false;
                }
                bracket_indices[bracket_count - 1] = i;
                break;
            case TOKEN_BRACKET_R_R:
            case TOKEN_BRACKET_S_R:
            case TOKEN_BRACKET_C_R:
                if (bracket_count == 0) {
                    printf(TOKEN_POS_FORMAT ERROR "superfluous closing bracket" ENDL, TOKEN_POS_FORMAT_VALUES((*tokens)[i]));
                    free(bracket_indices);
                    return false;
                }
                bracket_count--;
                if ((*tokens)[bracket_indices[bracket_count]].type == TOKEN_BRACKET_R_L && (*tokens)[i].type == TOKEN_BRACKET_R_R) {
                    break;
                } else if ((*tokens)[bracket_indices[bracket_count]].type == TOKEN_BRACKET_S_L && (*tokens)[i].type == TOKEN_BRACKET_S_R) {
                    break;
                } else if ((*tokens)[bracket_indices[bracket_count]].type == TOKEN_BRACKET_C_L && (*tokens)[i].type == TOKEN_BRACKET_C_R) {
                    break;
                } else {
                    printf(TOKEN_POS_FORMAT ERROR "incorrect closing bracket: expected '", TOKEN_POS_FORMAT_VALUES((*tokens)[i]));
                    if ((*tokens)[bracket_indices[bracket_count]].type == TOKEN_BRACKET_R_L) {
                        printf(")");
                    } else if ((*tokens)[bracket_indices[bracket_count]].type == TOKEN_BRACKET_S_L) {
                        printf("]");
                    } else if ((*tokens)[bracket_indices[bracket_count]].type == TOKEN_BRACKET_C_L) {
                        printf("}");
                    } else {
                        printf("?");
                    }
                    printf("', got '");
                    if ((*tokens)[i].type == TOKEN_BRACKET_R_R) {
                        printf(")");
                    } else if ((*tokens)[i].type == TOKEN_BRACKET_S_R) {
                        printf("]");
                    } else if ((*tokens)[i].type == TOKEN_BRACKET_C_R) {
                        printf("}");
                    } else {
                        printf("?");
                    }
                    printf("'" ENDL);
                    free(bracket_indices);
                    return false;
                }
            default:
                break;
        }
    }
    for (uint32_t i = 0; i < bracket_count; i++) {
        printf(TOKEN_POS_FORMAT ERROR "missing closing bracket '", TOKEN_POS_FORMAT_VALUES((*tokens)[bracket_indices[bracket_count - i - 1]]));
        if ((*tokens)[bracket_indices[bracket_count - i - 1]].type == TOKEN_BRACKET_R_L) {
            printf(")");
        } else if ((*tokens)[bracket_indices[bracket_count - i - 1]].type == TOKEN_BRACKET_S_L) {
            printf("]");
        } else if ((*tokens)[bracket_indices[bracket_count - i - 1]].type == TOKEN_BRACKET_C_L) {
            printf("}");
        } else {
            printf("?");
        }
        printf("'" ENDL);
    }
    if (bracket_indices != NULL) {
        free(bracket_indices);
    }
    if (bracket_count > 0) {
        return false;
    }
    if (deleted_tokens > 0) {
        *tokens = realloc(*tokens, (*tokens_count) * sizeof(token_t));
        if (*tokens == NULL) {
            printf(ERROR "tokenizer out of memory" ENDL);
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

#endif
