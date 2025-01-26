#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DEBUG 1
#define DEFAULT_SRC "./defaults.ha"

typedef uint32_t index_t;

char *read_src(char *path) {
    printf("reading source file \"%s\"... ", path);
    FILE *f_src = fopen(path, "rb");

    if (f_src == NULL) {
        printf("ERROR: could not access source file \"%s\"\r\n", path);
        return NULL;
    }

    fseek(f_src, 0, SEEK_END);
    index_t src_len = ftell(f_src);
    rewind(f_src);
    printf("size %i... ", src_len);

    char *src_buf = malloc(src_len + 1);

    if (src_buf == NULL) {
        fclose(f_src);
        printf("ERROR: source file \"%s\" buffer allocation failed (%i bytes)\r\n", path, src_len + 1);
        return NULL;
    }

    for (index_t src_i = 0; src_i < src_len; src_i++) {
        src_buf[src_i] = fgetc(f_src);
    }
    src_buf[src_len] = '\0';
    fclose(f_src);

    printf("done.\r\n");

    return src_buf;
}

typedef enum token_type {
    TOKEN_UNKNOWN,
    TOKEN_END,           // EOF
    TOKEN_BUS,           // ->
    TOKEN_SEMICOLON,     // ;
    TOKEN_COLON,         // :
    TOKEN_COMMA,         // ,
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

typedef struct token {
    token_type_t type;
    index_t start, end;  // incl. start, excl. end
} token_t;

typedef struct tokenizer_state {
    index_t i;
} tokenizer_state_t;

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
    } else if (src[state->i] == ',') {
        // ,
        RETURN_TOKEN_WITH_LENGTH(1, TOKEN_COMMA);
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

void asm_token_print_position(token_t *token, char *src) {
    index_t line = 1;
    index_t col = 1;
    for (index_t i = 0; i < token->start; i++) {
        col++;
        if (src[i] == '\n') {
            line++;
            col = 1;
        }
    }
    printf("(line %i:%i)", line, col);
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

// inc only at top of file (bool inc_done)
// normal context:
//   bus write: keyword/s_keyword, param, literals (b, o, d, x), label
//   bus
//   bus read: everything but literals and labels
//   ;

typedef uint16_t ast_index_t;

typedef enum ast_type {
    AST_UNKNOWN,
    AST_ROOT,
    AST_CONTENT,        // token as string content
    AST_INC,            // inc defaults.ha
    AST_DEF,            // def *AND {
    AST_DEF_CONFIG,     // def *AND[temp=A B] {
    AST_DEF_CALL,       // *AND ->
    AST_INSTRUCTION,    // A -> B
    AST_INS_BUS_WRITE,  // A ->
    AST_INS_BUS_READ,   //   -> B
    AST_LABEL,          // "main":
    AST_ASM_DIRECTIVE,  // !RS
} ast_type_t;

typedef struct ast_element {
    ast_type_t type;
    token_t content_token;
    struct ast_element *children;
    ast_index_t children_count;
} ast_element_t;

typedef struct parser_state {
    ast_element_t root;
    ast_element_t *current_context;
    token_t current_token, next_token;
    bool inc_done;
    char *src;
    tokenizer_state_t tokenizer_state;
} parser_state_t;

void asm_parse_element_init(ast_element_t *element) {
    element->type = AST_UNKNOWN;
    element->content_token.type = TOKEN_UNKNOWN;
    element->content_token.start = 0;
    element->content_token.end = 0;
    element->children = NULL;
    element->children_count = 0;
}

ast_element_t *asm_parse_add_child(ast_element_t *element) {
    element->children_count++;
    element->children = realloc(element->children, element->children_count * sizeof(ast_element_t));
    if (element->children == NULL) {
        printf("ERROR: AST reallocation failed\r\n");
        return NULL;
    }
    ast_element_t *child = &element->children[element->children_count - 1];
    asm_parse_element_init(child);
    if (child == NULL) {
        printf("ERROR: AST reallocation failed\r\n");
        return NULL;
    }
    return child;
}

ast_element_t *asm_parse_add_content_child(ast_element_t *element, token_t content) {
    ast_element_t *ast_child = asm_parse_add_child(element);
    ast_child->type = AST_CONTENT;
    ast_child->content_token = content;
    return ast_child;
}

void asm_parse_init(parser_state_t *state, char *src) {
    asm_parse_element_init(&state->root);
    state->root.type = AST_ROOT;
    state->current_context = &state->root;
    state->inc_done = false;
    state->src = src;

    asm_tokenize_init(&state->tokenizer_state);
}

void asm_parse_step_tokenizer(parser_state_t *state) {
    state->current_token = state->next_token;
    state->next_token = asm_tokenize(&state->tokenizer_state, state->src);
}

bool asm_parse_instruction(parser_state_t *state) {
    ast_element_t *ast_ins = asm_parse_add_child(state->current_context);
    ast_ins->type = AST_INSTRUCTION;

    // do not use returned pointer here, as adding the second child may reallocate the entire "children" array
    asm_parse_add_child(ast_ins);
    ast_ins->children[0].type = AST_INS_BUS_WRITE;
    asm_parse_add_child(ast_ins);
    ast_ins->children[1].type = AST_INS_BUS_READ;

    if (state->current_token.type == TOKEN_PLUS || state->current_token.type == TOKEN_MINUS) {
        if (state->next_token.type != TOKEN_LITERAL_b &&
            state->next_token.type != TOKEN_LITERAL_o &&
            state->next_token.type != TOKEN_LITERAL_d &&
            state->next_token.type != TOKEN_LITERAL_x) {
            return false;
        }

        // +/-
        asm_parse_add_content_child(&ast_ins->children[0], state->current_token);
        // literal
        asm_parse_add_content_child(&ast_ins->children[0], state->next_token);

        // next_token = bus
        asm_parse_step_tokenizer(state);
    } else if (state->current_token.type == TOKEN_STAR_KEYWORD) {
        // *ABC
        ast_element_t *ast_def = asm_parse_add_child(&ast_ins->children[0]);
        ast_def->type = AST_DEF_CALL;
        ast_def->content_token = state->current_token;

        if (state->next_token.type == TOKEN_OPEN_S) {
            asm_parse_add_child(ast_def);
            ast_def->children[0].type = AST_DEF_CONFIG;

            asm_parse_step_tokenizer(state);
            while (state->next_token.type != TOKEN_CLOSE_S) {
                if (state->next_token.type == TOKEN_END) {
                    return false;
                }
                asm_parse_add_content_child(&ast_def->children[0], state->next_token);
                asm_parse_step_tokenizer(state);
            }

            // next_token = bus
            asm_parse_step_tokenizer(state);
        }

        if (state->next_token.type == TOKEN_SEMICOLON) {
            return true;
        }
    } else {
        // probably a keyword
        asm_parse_add_content_child(&ast_ins->children[0], state->current_token);
    }

    if (state->next_token.type != TOKEN_BUS) {
        return false;
    }

    asm_parse_step_tokenizer(state);
    while (state->next_token.type != TOKEN_SEMICOLON) {
        if (state->next_token.type == TOKEN_END) {
            return false;
        }
        asm_parse_add_content_child(&ast_ins->children[1], state->next_token);
        asm_parse_step_tokenizer(state);
    }

    return true;
}

#define ASM_PARSE_ERR(msg)                                     \
    printf("ERROR: ");                                         \
    asm_token_print_position(&state.current_token, state.src); \
    printf(" \"");                                             \
    asm_token_content_print(&state.current_token, state.src);  \
    printf(" ");                                               \
    asm_token_content_print(&state.next_token, state.src);     \
    printf("\" " msg "\r\n");                                  \
    return state.root;

#define ASM_PARSE_ERR_IF_END()                                                         \
    if (state.current_token.type == TOKEN_END || state.next_token.type == TOKEN_END) { \
        ASM_PARSE_ERR("unexpected end of file");                                       \
    }

ast_element_t asm_parse(char *src) {
    parser_state_t state;
    asm_parse_init(&state, src);

    asm_parse_step_tokenizer(&state);
    asm_parse_step_tokenizer(&state);

    while (state.next_token.type != TOKEN_END) {
        if (state.current_token.type != TOKEN_KEYWORD_INC) {
            state.inc_done = true;
        }

        bool single_step = false;

        switch (state.current_token.type) {
            case TOKEN_KEYWORD:
            case TOKEN_STAR_KEYWORD:
            case TOKEN_PARAM:
            case TOKEN_PLUS:
            case TOKEN_MINUS:
            case TOKEN_LITERAL_b:
            case TOKEN_LITERAL_o:
            case TOKEN_LITERAL_d:
            case TOKEN_LITERAL_x:
                // instruction
                if (!asm_parse_instruction(&state)) {
                    ASM_PARSE_ERR("did not understand instruction format");
                }
                break;
            case TOKEN_LABEL: {
                if (state.next_token.type == TOKEN_COLON) {
                    // label definition (AST_LABEL)
                    ast_element_t *ast_label = asm_parse_add_child(state.current_context);
                    ast_label->type = AST_LABEL;
                    ast_label->content_token = state.current_token;
                } else if (state.next_token.type == TOKEN_BUS) {
                    // label jump (AST_INSTRUCTION)
                    if (!asm_parse_instruction(&state)) {
                        ASM_PARSE_ERR("did not understand instruction format");
                    }
                } else {
                    ASM_PARSE_ERR("\"label\" should be followed by colon (:) or bus (->)");
                }
                break;
            }
            case TOKEN_EXCLAM: {
                ast_element_t *ast_directive = asm_parse_add_child(state.current_context);
                ast_directive->type = AST_ASM_DIRECTIVE;

                while (state.next_token.type != TOKEN_SEMICOLON) {
                    ASM_PARSE_ERR_IF_END();
                    asm_parse_add_content_child(ast_directive, state.next_token);
                    asm_parse_step_tokenizer(&state);
                }
                break;
            }
            case TOKEN_KEYWORD_INC: {
                if (!state.inc_done) {
                    if (state.next_token.type != TOKEN_KEYWORD) {
                        ASM_PARSE_ERR("expected relative path after inc (path should not begin with any symbol)");
                    }

                    ast_element_t *ast_inc = asm_parse_add_child(state.current_context);
                    ast_inc->type = AST_INC;
                    ast_inc->content_token = state.next_token;

                    asm_parse_step_tokenizer(&state);
                    while (state.next_token.type != TOKEN_SEMICOLON) {
                        ASM_PARSE_ERR_IF_END();
                        asm_parse_add_content_child(ast_inc, state.next_token);
                        asm_parse_step_tokenizer(&state);
                    }
                } else {
                    ASM_PARSE_ERR("inc statements should be at top of file");
                }
                break;
            }
            case TOKEN_KEYWORD_DEF: {
                if (state.current_context == &state.root) {
                    if (state.next_token.type != TOKEN_STAR_KEYWORD) {
                        ASM_PARSE_ERR("expected name of def preceded by a star (*)");
                    }

                    ast_element_t *ast_def = asm_parse_add_child(state.current_context);
                    ast_def->type = AST_DEF;
                    ast_def->content_token = state.next_token;

                    asm_parse_step_tokenizer(&state);
                    if (state.next_token.type == TOKEN_OPEN_S) {
                        asm_parse_add_child(ast_def);
                        ast_def->children[0].type = AST_DEF_CONFIG;

                        asm_parse_step_tokenizer(&state);
                        while (state.next_token.type != TOKEN_CLOSE_S) {
                            ASM_PARSE_ERR_IF_END();
                            asm_parse_add_content_child(&ast_def->children[0], state.next_token);
                            asm_parse_step_tokenizer(&state);
                        }
                        asm_parse_step_tokenizer(&state);
                    }

                    if (state.next_token.type != TOKEN_OPEN_C) {
                        ASM_PARSE_ERR("opening brace ({) expected after def");
                    }

                    state.current_context = ast_def;
                } else {
                    ASM_PARSE_ERR("def inside def");
                }
                break;
            }
            case TOKEN_CLOSE_C:
                if (state.current_context != &state.root) {
                    state.current_context = &state.root;
                    single_step = true;
                } else {
                    ASM_PARSE_ERR("closing brace (}) outside of def");
                }
                break;
            default:
                asm_token_debug_print(&state.current_token, state.src);
                ASM_PARSE_ERR("unexpected token");
                break;
        }

        asm_parse_step_tokenizer(&state);
        if (!single_step) {
            asm_parse_step_tokenizer(&state);
        }
    }

    return state.root;
}

void asm_parse_free_ast(ast_element_t *ast) {
    for (ast_index_t i = 0; i < ast->children_count; i++) {
        asm_parse_free_ast(&ast->children[i]);
    }
    if (ast->children != NULL) {
        free(ast->children);
    }
}

void asm_parse_debug_print(ast_element_t *ast, char *src, int level) {
    for (int s = 0; s < level; s++)
        printf("  ");
    printf("type: ");
    switch (ast->type) {
        case AST_ROOT:
            printf("AST_ROOT");
            break;
        case AST_CONTENT:
            printf("AST_CONTENT");
            break;
        case AST_INC:
            printf("AST_INC");
            break;
        case AST_DEF:
            printf("AST_DEF");
            break;
        case AST_DEF_CONFIG:
            printf("AST_DEF_CONFIG");
            break;
        case AST_DEF_CALL:
            printf("AST_DEF_CALL");
            break;
        case AST_INSTRUCTION:
            printf("AST_INSTRUCTION");
            break;
        case AST_INS_BUS_WRITE:
            printf("AST_INS_BUS_WRITE");
            break;
        case AST_INS_BUS_READ:
            printf("AST_INS_BUS_READ");
            break;
        case AST_LABEL:
            printf("AST_LABEL");
            break;
        case AST_ASM_DIRECTIVE:
            printf("AST_ASM_DIRECTIVE");
            break;
        case AST_UNKNOWN:
        default:
            printf("?");
            break;
    }
    printf(" (%i)\r\n", ast->type);

    if (ast->content_token.type != TOKEN_UNKNOWN) {
        for (int s = 0; s < level; s++)
            printf("  ");
        printf("content_token: ");
        asm_token_debug_print(&ast->content_token, src);
    }

    for (int s = 0; s < level; s++)
        printf("  ");
    printf("children_count: %i\r\n", ast->children_count);

    for (ast_index_t i = 0; i < ast->children_count; i++) {
        for (int s = 0; s < level; s++)
            printf("  ");
        printf("child (%i):\r\n", i);
        asm_parse_debug_print(&ast->children[i], src, level + 1);
    }
}

int main(int argc, char *argv[]) {
    char *src_path = DEFAULT_SRC;

#if !DEBUG
    if (argc > 1) {
        src_path = argv[1];
    }
#endif

    char *src_buf = read_src(src_path);
    if (src_buf == NULL) {
        printf("failed to read source file \"%s\"\r\n", src_path);
        return 1;
    }

    ast_element_t ast = asm_parse(src_buf);
    // TODO: parse success message
    asm_parse_debug_print(&ast, src_buf, 0);
    asm_parse_free_ast(&ast);

    free(src_buf);

    return 0;
}
