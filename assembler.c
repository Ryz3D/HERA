#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DEBUG 1

// const char *default_src = "./defaults.ha";
const char *default_src = "./programs/assembly/fib.ha";

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
    printf("(line %u:%u)", line, col);
}

void asm_token_content_print(token_t *token, char *src) {
    for (index_t i = token->start; i < token->end; i++) {
        printf("%c", src[i]);
    }
}

char *asm_token_get_content(token_t *token, char *src) {
    char *content = malloc((token->end - token->start + 1) * sizeof(char));
    if (content == NULL) {
        printf("asm_token_get_content: out of memory\r\n");
        return NULL;
    }
    memcpy(content, &src[token->start], token->end - token->start);
    content[token->end - token->start] = '\0';
    return content;
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
    } else if (state->current_token.type == TOKEN_LABEL) {
        ast_element_t *ast_child = asm_parse_add_child(&ast_ins->children[0]);
        ast_child->type = AST_LABEL;
        ast_child->content_token = state->current_token;
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

typedef enum ins_bus_w {
    W_INVALID = 0x10,
    W_LABEL = 0x11,
    W_LIT = 0x0,
    W_A = 0x1,
    W_B = 0x2,
    W_C = 0x3,
    W_RAM = 0x4,
    W_RP = 0x5,
    W_PC = 0x6,
    W_STAT = 0x7,
    W_ADD = 0xA,
    W_COM = 0xB,
    W_NOR = 0xC,
} ins_bus_w_t;

typedef enum ins_bus_r {
    R_INVALID = 0x10,
    R_VOID = 0x0,
    R_A = 0x1,
    R_B = 0x2,
    R_C = 0x3,
    R_RAM = 0x4,
    R_RP = 0x5,
    R_PC = 0x6,
    R_STAT = 0x7,
    R_A_B = 0xA,
    R_B_RP = 0xB,
    R_C_PC = 0xC,
    R_PC_C = 0xD,
    R_PC_Z = 0xE,
    R_PC_N = 0xF,
} ins_bus_r_t;

typedef struct instruction {
    ins_bus_w_t bus_w;
    ins_bus_r_t bus_r;
    char *label;
    char *bus_w_label;
    uint16_t literal;
    token_t ref_token;
} instruction_t;

void ins_init(instruction_t *ins) {
    ins->bus_w = W_INVALID;
    ins->bus_r = R_INVALID;
    ins->label = NULL;
    ins->bus_w_label = NULL;
    ins->literal = 0;
}

void ins_free(instruction_t *instructions, index_t instructions_count) {
    for (index_t i = 0; i < instructions_count; i++) {
        if (instructions[i].label != NULL) {
            free(instructions[i].label);
        }
        if (instructions[i].bus_w_label != NULL) {
            free(instructions[i].bus_w_label);
        }
    }
    free(instructions);
}

uint8_t ins_to_binary(instruction_t *ins) {
    if (ins->bus_w == W_INVALID || ins->bus_r == R_INVALID) {
        printf("invalid instruction");
        goto ins_errinfo;
    }
    if (ins->bus_w == W_LABEL && ins->bus_w_label == NULL) {
        if (ins->bus_w_label == NULL) {
            printf("instruction missing w_label");
        } else {
            printf("instruction has unresolved w_label");
        }
        goto ins_errinfo;
    }
    return (((uint8_t)ins->bus_w & 0xF) << 4) | ((uint8_t)ins->bus_r & 0xF);

ins_errinfo:
    printf(" (0x%02X -> 0x%02X)", ins->bus_w, ins->bus_r);
    if (ins->label != NULL) {
        printf(" (label %s)", ins->label);
    }
    printf("\r\n");
    return 0x99;
}

void ins_resolve_labels(instruction_t *ins, index_t ins_count, char *src) {
    for (index_t i = 0; i < ins_count; i++) {
        if (ins[i].bus_w == W_LABEL) {
            index_t program_index = 0;
            for (index_t search_i = 0; search_i < ins_count; search_i++) {
                if (ins[search_i].label != NULL) {
                    if (strcmp(ins[i].bus_w_label, ins[search_i].label) == 0) {
                        ins[i].bus_w = W_LIT;
                        ins[i].literal = program_index;
                        break;
                    }
                }
                if (ins[search_i].bus_w == W_LIT || ins[search_i].bus_w == W_LABEL) {
                    program_index += 3;
                } else {
                    program_index++;
                }
            }
            if (ins[i].bus_w != W_LIT) {
                ins[i].bus_w = W_INVALID;
                asm_token_print_position(&ins[i].ref_token, src);
                printf(" can't resolve label \"%s\"\r\n", ins[i].bus_w_label);
            }
        }
    }
}

uint16_t parse_literal(char *content, uint8_t base) {
    if (base == 2 || base == 8 || base == 16) {
        content += 2;
    }
    uint16_t res = 0;
    while (*content != '\0') {
        switch (base) {
            case 2:
                if (*content < '0' || *content > '1') {
                    return res;
                }
                break;
            case 8:
                if (*content < '0' || *content > '8') {
                    return res;
                }
                break;
            case 10:
                if (*content < '0' || *content > '9') {
                    return res;
                }
                break;
            case 16:
                if ((*content < '0' || *content > '9') && (*content < 'a' || *content > 'f') && (*content < 'A' || *content > 'F')) {
                    return res;
                }
                break;
            break;
            default:
                break;
        }
        res *= base;
        if (*content >= '0' && *content <= '9') {
            res += *content - '0';
        } else if (*content >= 'a' && *content <= 'f') {
            res += *content - 'a' + 10;
        } else if (*content >= 'A' && *content <= 'F') {
            res += *content - 'A' + 10;
        }
        content++;
    }
    return res;
}

instruction_t ins_from_ast(ast_element_t *ast_element, char *src) {
    if (ast_element->type != AST_INSTRUCTION || ast_element->children_count != 2) {
        printf("ins_from_ast: incorrect AST type %i\r\n", ast_element->type);
        return (instruction_t){
            .bus_w = W_INVALID,
            .bus_r = R_INVALID,
            .bus_w_label = NULL,
            .label = NULL,
            .literal = 0,
            .ref_token = { .type = TOKEN_UNKNOWN, .start = 0, .end = 0 },
        };
    }

    ast_element_t *ast_bus_w = NULL;
    ast_element_t *ast_bus_r = NULL;
    for (index_t i = 0; i < ast_element->children_count; i++) {
        if (ast_element->children[i].type == AST_INS_BUS_WRITE) {
            ast_bus_w = &ast_element->children[i];
            if (ast_bus_r != NULL) {
                break;
            }
        } else if (ast_element->children[i].type == AST_INS_BUS_READ) {
            ast_bus_r = &ast_element->children[i];
            if (ast_bus_w != NULL) {
                break;
            }
        }
    }
    if (ast_bus_r == NULL || ast_bus_r->children_count == 0 ||
        ast_bus_w == NULL || ast_bus_w->children_count == 0) {
        printf("ins_from_ast: missing AST_INS_BUS_WRITE and/or AST_INS_BUS_READ\r\n");
        return (instruction_t){
            .bus_w = W_INVALID,
            .bus_r = R_INVALID,
            .bus_w_label = NULL,
            .label = NULL,
            .literal = 0,
            .ref_token = { .type = TOKEN_UNKNOWN, .start = 0, .end = 0 },
        };
    }

    instruction_t ins = {
        .bus_w = W_INVALID,
        .bus_r = R_INVALID,
        .bus_w_label = NULL,
        .label = NULL,
        .literal = 0,
        .ref_token = ast_bus_w->children[0].content_token,
    };

    switch (ast_bus_w->children[0].type) {
        case AST_CONTENT: {
            char *content = asm_token_get_content(&ast_bus_w->children[0].content_token, src);
            if (content == NULL) {
                break;
            }
            if (strcmp(content, "A") == 0) {
                ins.bus_w = W_A;
            } else if (strcmp(content, "B") == 0) {
                ins.bus_w = W_B;
            } else if (strcmp(content, "C") == 0) {
                ins.bus_w = W_C;
            } else if (strcmp(content, "RAM") == 0) {
                ins.bus_w = W_RAM;
            } else if (strcmp(content, "RAM_P") == 0) {
                ins.bus_w = W_RP;
            } else if (strcmp(content, "PC") == 0) {
                ins.bus_w = W_PC;
            } else if (strcmp(content, "STAT") == 0) {
                ins.bus_w = W_STAT;
            } else if (strcmp(content, "ADD") == 0) {
                ins.bus_w = W_ADD;
            } else if (strcmp(content, "COM") == 0) {
                ins.bus_w = W_COM;
            } else if (strcmp(content, "NOR") == 0) {
                ins.bus_w = W_NOR;
            } else if (ast_bus_w->children[0].content_token.type == TOKEN_LITERAL_b) {
                ins.bus_w = W_LIT;
                ins.literal = parse_literal(content, 2);
            } else if (ast_bus_w->children[0].content_token.type == TOKEN_LITERAL_o) {
                ins.bus_w = W_LIT;
                ins.literal = parse_literal(content, 8);
            } else if (ast_bus_w->children[0].content_token.type == TOKEN_LITERAL_d) {
                ins.bus_w = W_LIT;
                ins.literal = content[0] == '-' ? -parse_literal(content + 1, 10) : parse_literal(content, 10);
            } else if (ast_bus_w->children[0].content_token.type == TOKEN_LITERAL_x) {
                ins.bus_w = W_LIT;
                ins.literal = parse_literal(content, 16);
            } else {
                printf("ERROR: unknown write-to-bus value\r\n");
            }
            free(content);
            break;
        }
        case AST_LABEL: {
            char *content = asm_token_get_content(&ast_bus_w->children[0].content_token, src);
            if (content == NULL) {
                break;
            }
            ins.bus_w = W_LABEL;
            ins.bus_w_label = content;
            break;
        }
        case AST_DEF_CALL: {
            // TODO
            char *content = asm_token_get_content(&ast_bus_w->children[0].content_token, src);
            if (content == NULL) {
                break;
            }
            printf("call (WHICH IS NOT SUPPORTED): '%s' \r\n", content);
            free(content);
            break;
        }
        default:
            printf("ERROR: unknown write-to-bus value\r\n");
            break;
    }
    switch (ast_bus_r->children[0].type) {
        case AST_CONTENT: {
            if (ast_bus_r->children_count == 1) {
                char *content = asm_token_get_content(&ast_bus_r->children[0].content_token, src);
                if (content == NULL) {
                    break;
                }
                if (strcmp(content, "VOID") == 0) {
                    ins.bus_r = R_VOID;
                } else if (strcmp(content, "A") == 0) {
                    ins.bus_r = R_A;
                } else if (strcmp(content, "B") == 0) {
                    ins.bus_r = R_B;
                } else if (strcmp(content, "C") == 0) {
                    ins.bus_r = R_C;
                } else if (strcmp(content, "RAM") == 0) {
                    ins.bus_r = R_RAM;
                } else if (strcmp(content, "RAM_P") == 0) {
                    ins.bus_r = R_RP;
                } else if (strcmp(content, "PC") == 0) {
                    ins.bus_r = R_PC;
                } else if (strcmp(content, "STAT") == 0) {
                    ins.bus_r = R_STAT;
                } else if (strcmp(content, "PC_C") == 0) {
                    ins.bus_r = R_PC_C;
                } else if (strcmp(content, "PC_Z") == 0) {
                    ins.bus_r = R_PC_Z;
                } else if (strcmp(content, "PC_N") == 0) {
                    ins.bus_r = R_PC_N;
                } else {
                    printf("ERROR: unknown read-from-bus value\r\n");
                }
                free(content);
            } else if (ast_bus_r->children_count == 2) {
                char *content1 = asm_token_get_content(&ast_bus_r->children[0].content_token, src);
                if (content1 == NULL) {
                    break;
                }
                char *content2 = asm_token_get_content(&ast_bus_r->children[1].content_token, src);
                if (content2 == NULL) {
                    break;
                }
                if (strcmp(content1, "A") == 0 && strcmp(content2, "B") == 0) {
                    ins.bus_r = R_A_B;
                } else if (strcmp(content1, "B") == 0 && strcmp(content2, "RAM_P") == 0) {
                    ins.bus_r = R_B_RP;
                } else if (strcmp(content1, "C") == 0 && strcmp(content2, "PC") == 0) {
                    ins.bus_r = R_C_PC;
                } else {
                    printf("ERROR: unknown read-from-bus value\r\n");
                }
                free(content1);
                free(content2);
            } else {
                printf("ERROR: unknown read-from-bus value\r\n");
            }
            break;
        }
        case AST_DEF_CALL:
            // TODO
            break;
        default:
            printf("ERROR: unknown read-from-bus value\r\n");
            break;
    }
    return ins;
}

int main(int argc, char *argv[]) {
    char *src_path = (char *)default_src;

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
    // asm_parse_debug_print(&ast, src_buf, 0);

    // TODO: parse def's, replace def calls (could the children pointer be bent (reused) from def children to call children?)
    // now the AST should be flat and include only labels and instructions

    instruction_t *instructions = NULL;
    index_t instructions_count = 0;
    char *current_label = NULL;
    for (ast_index_t i = 0; i < ast.children_count; i++) {
        if (ast.children[i].type == AST_INSTRUCTION) {
            instructions_count++;
            instructions = realloc(instructions, instructions_count * sizeof(instruction_t));
            if (instructions == NULL) {
                printf("out of memory during instruction allocation\r\n");
                return 1;
            }
            instructions[instructions_count - 1] = ins_from_ast(&ast.children[i], src_buf);
            instructions[instructions_count - 1].label = current_label;
            current_label = NULL;
        } else if (ast.children[i].type == AST_LABEL) {
            if (current_label != NULL) {
                free(current_label);
            }
            current_label = asm_token_get_content(&ast.children[i].content_token, src_buf);
        }
    }
    ins_resolve_labels(instructions, instructions_count, src_buf);
    uint8_t *binary = NULL;
    index_t binary_size = 0;
    for (index_t i = 0; i < instructions_count; i++) {
        binary_size += instructions[i].bus_w == W_LIT ? 3 : 1;
        binary = realloc(binary, binary_size);
        if (binary == NULL) {
            printf("out of memory during binary allocation\r\n");
            return 1;
        }
        uint8_t bin = ins_to_binary(&instructions[i]);
        binary[binary_size - (instructions[i].bus_w == W_LIT ? 3 : 1)] = bin;
        if (bin != 0x99) {
            printf("0x%02X", bin);
        }
        if (instructions[i].bus_w == W_LIT) {
            if (bin != 0x99) {
                binary[binary_size - 2] = instructions[i].literal >> 8;
                binary[binary_size - 1] = instructions[i].literal & 0xFF;
                printf(" 0x%04X", instructions[i].literal);
            } else {
                binary[binary_size - 2] = 0x00;
                binary[binary_size - 1] = 0x00;
            }
        }
        if (bin != 0x99) {
            printf("\r\n");
        }
    }

    // TODO: output format

    asm_parse_free_ast(&ast);
    ins_free(instructions, instructions_count);
    free(binary);
    free(src_buf);

    return 0;
}
