#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SRC_BUF_SIZE_INITIAL 5000
#define SRC_BUF_SIZE_INCREMENT 1000

typedef uint32_t index_t;

typedef enum token_type {
    TOKEN_UNKNOWN,
    TOKEN_END,           // EOF
    TOKEN_BUS,           // ->
    TOKEN_SEMICOLON,     // ;
    TOKEN_COLON,         // :
    TOKEN_PARAM,         // *
    TOKEN_EQUALS,        // =
    TOKEN_EXCLAM,        // !
    TOKEN_PLUS,          // +
    TOKEN_MINUS,         // -
    TOKEN_OPEN_C,        // {
    TOKEN_CLOSE_C,       // }
    TOKEN_OPEN_S,        // [
    TOKEN_CLOSE_S,       // ]
    TOKEN_LITERAL_b,     // 0b
    TOKEN_LITERAL_o,     // 0o
    TOKEN_LITERAL_x,     // 0x
    TOKEN_LITERAL_d,     // 0
    TOKEN_LABEL,         // "abc"
    TOKEN_STAR_KEYWORD,  // *abc
    TOKEN_KEYWORD,       // abc
    TOKEN_KEYWORD_INC,   // inc
    TOKEN_KEYWORD_DEF,   // def
} token_type_t;

typedef struct {
    token_type_t type;
    index_t start, end;  // incl. start, excl. end
} token_t;

typedef struct {
    index_t i;
} tokenizer_state_t;

char *read_src(char *path) {
    FILE *f_src = fopen(path, "r");

    index_t src_buf_size = SRC_BUF_SIZE_INITIAL;
    char *src_buf = malloc(src_buf_size * sizeof(char));
    if (src_buf == NULL) {
        fclose(f_src);
        printf("ERROR: inital source buffer allocation failed (%i chars * %i bytes/char)\r\n", src_buf_size, sizeof(char));
        return NULL;
    }
    index_t src_i = 0;
    while (fread((void *)(src_buf + src_i), sizeof(char), 1, f_src)) {
        if (src_i >= src_buf_size - 1) {
            // this is pretty much the behaviour of realloc. oh well.
            char *src_buf_next = malloc((src_buf_size + SRC_BUF_SIZE_INCREMENT) * sizeof(char));
            if (src_buf_next == NULL) {
                free(src_buf);
                fclose(f_src);
                printf("ERROR: source buffer allocation failed (%i chars * %i bytes/char)\r\n", src_buf_size + SRC_BUF_SIZE_INCREMENT, sizeof(char));
                return NULL;
            }
            memcpy(src_buf_next, src_buf, src_buf_size);
            free(src_buf);
            src_buf_size += SRC_BUF_SIZE_INCREMENT;
            src_buf = src_buf_next;
        }
        src_i++;
    }
    src_buf[src_i] = '\0';
    fclose(f_src);

    return src_buf;
}

void asm_tokenize_init(tokenizer_state_t *state) {
    state->i = 0;
}

#define RETURN_TOKEN(token_type) \
    return (token_t){            \
        .type = token_type,      \
        .start = token_start,    \
        .end = state->i,         \
    };

#define RETURN_TOKEN_WITH_LENGTH(token_len, token_type) \
    state->i += token_len;                              \
    RETURN_TOKEN(token_type);

token_t asm_tokenize(tokenizer_state_t *state, char *src) {
retry_tokenize:

    while (src[state->i] == ' ' || src[state->i] == '\t' || src[state->i] == '\r' || src[state->i] == '\n') {
        state->i++;
    }

    index_t token_start = state->i;

    if (src[state->i] == '\0') {
        // EOF
        RETURN_TOKEN_WITH_LENGTH(1, TOKEN_END);
    } else if (src[state->i] == '-' && src[state->i + 1] == '>') {
        // ->
        RETURN_TOKEN_WITH_LENGTH(2, TOKEN_BUS);
    } else if (src[state->i] == ';') {
        // ;
        RETURN_TOKEN_WITH_LENGTH(1, TOKEN_SEMICOLON);
    } else if (src[state->i] == ':') {
        // :
        RETURN_TOKEN_WITH_LENGTH(1, TOKEN_COLON);
    } else if (src[state->i] == '=') {
        // =
        RETURN_TOKEN_WITH_LENGTH(1, TOKEN_EQUALS);
    } else if (src[state->i] == '!') {
        // !
        RETURN_TOKEN_WITH_LENGTH(1, TOKEN_EXCLAM);
    } else if (src[state->i] == '+') {
        // +
        RETURN_TOKEN_WITH_LENGTH(1, TOKEN_PLUS);
    } else if (src[state->i] == '-') {
        // -
        RETURN_TOKEN_WITH_LENGTH(1, TOKEN_MINUS);
    } else if (src[state->i] == '{') {
        // {
        RETURN_TOKEN_WITH_LENGTH(1, TOKEN_OPEN_C);
    } else if (src[state->i] == '}') {
        // }
        RETURN_TOKEN_WITH_LENGTH(1, TOKEN_CLOSE_C);
    } else if (src[state->i] == '[') {
        // [
        RETURN_TOKEN_WITH_LENGTH(1, TOKEN_OPEN_S);
    } else if (src[state->i] == ']') {
        // ]
        RETURN_TOKEN_WITH_LENGTH(1, TOKEN_CLOSE_S);
    } else if (src[state->i] == '#') {
        // # comment (ended by [\n\;])
        while (src[state->i] != '\n' && src[state->i] != ';') {
            state->i++;
        }
        state->i++;
        goto retry_tokenize;
    } else if (src[state->i] == '0' && src[state->i + 1] == 'b') {
        // 0b (ended by [^01])
        state->i += 2;
        while (src[state->i] >= '0' && src[state->i] <= '1') {
            state->i++;
        }
        RETURN_TOKEN(TOKEN_LITERAL_b);
    } else if (src[state->i] == '0' && src[state->i + 1] == 'o') {
        // 0o (ended by [^01234567])
        state->i += 2;
        while (src[state->i] >= '0' && src[state->i] <= '7') {
            state->i++;
        }
        RETURN_TOKEN(TOKEN_LITERAL_o);
    } else if (src[state->i] == '0' && src[state->i + 1] == 'x') {
        // 0x (ended by [^0123456789abcdefABCDEF])
        state->i += 2;
        while ((src[state->i] >= '0' && src[state->i] <= '9') || (src[state->i] >= 'a' && src[state->i] <= 'f') || (src[state->i] >= 'A' && src[state->i] <= 'F')) {
            state->i++;
        }
        RETURN_TOKEN(TOKEN_LITERAL_x);
    } else if (src[state->i] >= '0' && src[state->i] <= '9') {
        // 0...9 (ended by [^0123456789])
        while (src[state->i] >= '0' && src[state->i] <= '9') {
            state->i++;
        }
        RETURN_TOKEN(TOKEN_LITERAL_d);
    } else if (src[state->i] == '"') {
        // "label" (ended by /[^\\]"/)
        state->i++;
        while (src[state->i - 1] == '\\' || src[state->i] != '"') {
            state->i++;
        }
        state->i++;
        RETURN_TOKEN(TOKEN_LABEL);
    } else if (src[state->i] == '*') {
        // *star_keyword (ended by [^\w\d\_])
        state->i++;
        while ((src[state->i] >= 'a' && src[state->i] <= 'z') || (src[state->i] >= 'A' && src[state->i] <= 'Z') || (src[state->i] >= '0' && src[state->i] <= '9') || src[state->i] == '_') {
            state->i++;
        }
        if (state->i - token_start == 1) {
            RETURN_TOKEN(TOKEN_PARAM);
        } else {
            RETURN_TOKEN(TOKEN_STAR_KEYWORD);
        }
    } else if ((src[state->i] >= 'a' && src[state->i] <= 'z') || (src[state->i] >= 'A' && src[state->i] <= 'Z')) {
        // keyword (try matching def & inc) (ended by [^\w\d\_\.\/])
        while ((src[state->i] >= 'a' && src[state->i] <= 'z') || (src[state->i] >= 'A' && src[state->i] <= 'Z') || (src[state->i] >= '0' && src[state->i] <= '9') || src[state->i] == '_' || src[state->i] == '.' || src[state->i] == '/') {
            state->i++;
        }
        if (state->i - token_start == 3) {
            if (src[token_start] == 'i' && src[token_start + 1] == 'n' && src[token_start + 2] == 'c') {
                RETURN_TOKEN(TOKEN_KEYWORD_INC);
            } else if (src[token_start] == 'd' && src[token_start + 1] == 'e' && src[token_start + 2] == 'f') {
                RETURN_TOKEN(TOKEN_KEYWORD_DEF);
            }
        }
        RETURN_TOKEN(TOKEN_KEYWORD);
    } else {
        printf("WARNING: tokenizer discarded \"%c\" (0x%02X) at %i\r\n", src[state->i], src[state->i], state->i);
        state->i++;
        goto retry_tokenize;
    }

    RETURN_TOKEN(TOKEN_UNKNOWN);
}

void asm_token_content_print(token_t *token, char *src) {
    for (index_t i = token->start; i < token->end; i++) {
        printf("%c", src[i]);
    }
}

#define PRINT_TOKEN(type_name)           \
    printf("TOKEN: " type_name " (");    \
    asm_token_content_print(token, src); \
    printf(")\r\n");

void asm_token_debug_print(token_t *token, char *src) {
    switch (token->type) {
        case TOKEN_END:
            printf("TOKEN: end\r\n");
            break;
        case TOKEN_BUS:
            PRINT_TOKEN("bus");
            break;
        case TOKEN_SEMICOLON:
            PRINT_TOKEN("semicolon");
            break;
        case TOKEN_COLON:
            PRINT_TOKEN("colon");
            break;
        case TOKEN_PARAM:
            PRINT_TOKEN("param");
            break;
        case TOKEN_EQUALS:
            PRINT_TOKEN("equals");
            break;
        case TOKEN_EXCLAM:
            PRINT_TOKEN("exclam");
            break;
        case TOKEN_PLUS:
            PRINT_TOKEN("plus");
            break;
        case TOKEN_MINUS:
            PRINT_TOKEN("minus");
            break;
        case TOKEN_OPEN_C:
            PRINT_TOKEN("open_c");
            break;
        case TOKEN_CLOSE_C:
            PRINT_TOKEN("close_c");
            break;
        case TOKEN_OPEN_S:
            PRINT_TOKEN("open_s");
            break;
        case TOKEN_CLOSE_S:
            PRINT_TOKEN("close_s");
            break;
        case TOKEN_LITERAL_b:
            PRINT_TOKEN("literal bin");
            break;
        case TOKEN_LITERAL_o:
            PRINT_TOKEN("literal oct");
            break;
        case TOKEN_LITERAL_x:
            PRINT_TOKEN("literal hex");
            break;
        case TOKEN_LITERAL_d:
            PRINT_TOKEN("literal dec");
            break;
        case TOKEN_LABEL:
            PRINT_TOKEN("label");
            break;
        case TOKEN_STAR_KEYWORD:
            PRINT_TOKEN("s_keyword");
            break;
        case TOKEN_KEYWORD:
            PRINT_TOKEN("keyword");
            break;
        case TOKEN_KEYWORD_INC:
            PRINT_TOKEN("assembler keyword inc");
            break;
        case TOKEN_KEYWORD_DEF:
            PRINT_TOKEN("assembler keyword def");
            break;
        case TOKEN_UNKNOWN:
        default:
            PRINT_TOKEN("unknown");
            break;
    }
}

int main(int argc, char *argv[]) {
    char *src_path;
    if (argc > 1) {
        src_path = argv[1];
    } else {
        char *default_path = "./programs/assembly/fib.ha";
        src_path = default_path;
    }

    char *src_buf = read_src(src_path);
    if (src_buf == NULL) {
        return 1;
    }
    printf("read source file \"%s\"\r\n", src_path);

    tokenizer_state_t tokenizer_state;

    asm_tokenize_init(&tokenizer_state);

    token_t current_token = {
        .type = TOKEN_UNKNOWN,
    };
    while (current_token.type != TOKEN_END) {
        current_token = asm_tokenize(&tokenizer_state, src_buf);
        asm_token_debug_print(&current_token, src_buf);
        printf(" ");
    }

    free(src_buf);

    return 0;
}
