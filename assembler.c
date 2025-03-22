#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ASM_DEBUG 0
// TODO: bad error message if insufficient def passes "ERROR: missing AST_INS_BUS_WRITE and/or AST_INS_BUS_READ"
// maximum depth of nested def calls
#define ASM_DEF_RESOLVE_PASSES 10
#define ASM_STA_ADDRESS 0x11
#define ASM_STB_ADDRESS 0x12
#define ASM_STC_ADDRESS 0x13

const char *asm_debug_src = "./programs/assembly/def_test.ha";

typedef uint32_t index_t;

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
    char *src;
} token_t;

typedef struct tokenizer_state {
    index_t i;
} tokenizer_state_t;

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

typedef uint32_t ast_index_t;

typedef enum ast_type {
    AST_UNKNOWN,
    AST_ROOT,
    AST_CONTENT,        // token as string content
    AST_INC,            // inc defaults.ha
    AST_DEF,            // def *AND {
    AST_DEF_CONFIG,     // def *AND[temp=A B] {
    AST_DEF_CALL,       // *AND ->
    AST_PARAM,          //   -> *
    AST_INSTRUCTION,    // A -> B
    AST_INS_BUS_WRITE,  // A ->
    AST_INS_BUS_READ,   //   -> B
    AST_LABEL,          // "main":
    AST_ASM_DIRECTIVE,  // !RS
    
    AST_STA,
    AST_STB,
    AST_STC,
    AST_RSA,
    AST_RSB,
    AST_RSC,
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
    token_t bus_w_def_token;
    token_t bus_r_def_token;
} instruction_t;

typedef struct inc_context {
    char *path;
    char *src;
    ast_element_t ast;
} inc_context_t;

char *asm_read_src(const char *path) {
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

void asm_write_bin(const char *path, uint8_t *bin, index_t bin_size) {
    printf("writing binary file \"%s\"... ", path);
    FILE *f_bin = fopen(path, "wb");

    if (f_bin == NULL) {
        printf("ERROR: could not access binary file \"%s\"\r\n", path);
        return;
    }

    printf("size %i... ", bin_size);

    index_t written = 0;
    if ((written = fwrite(bin, 1, bin_size, f_bin)) != bin_size) {
        fclose(f_bin);
        printf("ERROR: binary file \"%s\" write failed (%i bytes)\r\n", path, written);
        return;
    }
    fclose(f_bin);

    printf("done.\r\n");
}

void asm_write_img(const char *path, uint8_t *bin, index_t bin_size) {
    printf("writing image file \"%s\"... ", path);
    FILE *f_img = fopen(path, "wb");

    if (f_img == NULL) {
        printf("ERROR: could not access image file \"%s\"\r\n", path);
        return;
    }

    fprintf(f_img, "v2.0 raw\n");
    for (index_t i = 0; i < bin_size; i += 8) {
        for (index_t j = 0; j < 8; j++) {
            if (i + j >= bin_size) {
                break;
            }
            fprintf(f_img, "%02x", bin[i + j]);
            if (j < 7) {
                fprintf(f_img, " ");
            }
        }
        fprintf(f_img, "\n");
    }

    fclose(f_img);

    printf("done.\r\n");
}

void asm_token_print_position(token_t *token) {
    index_t line = 1;
    index_t col = 1;
    for (index_t i = 0; i < token->start; i++) {
        col++;
        if (token->src[i] == '\n') {
            line++;
            col = 1;
        }
    }
    printf("(line %u:%u)", line, col);
}

index_t asm_token_get_line(token_t *token) {
    index_t line = 1;
    for (index_t i = 0; i < token->start; i++) {
        if (token->src[i] == '\n') {
            line++;
        }
    }
    return line;
}

void asm_token_content_print(token_t *token) {
    for (index_t i = token->start; i < token->end; i++) {
        printf("%c", token->src[i]);
    }
}

char *asm_token_get_content(token_t *token) {
    char *content = malloc((token->end - token->start + 1) * sizeof(char));
    if (content == NULL) {
        printf("ERROR: out of memory in asm_token_get_content\r\n");
        return NULL;
    }
    memcpy(content, &token->src[token->start], token->end - token->start);
    content[token->end - token->start] = '\0';
    return content;
}

#define PRINT_TOKEN(type_name)           \
    printf("TOKEN: " type_name " (");    \
    asm_token_content_print(token); \
    printf(")\r\n");

void asm_token_debug_print(token_t *token) {
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

uint8_t asm_ins_to_binary(instruction_t *ins) {
    if (ins->bus_w == W_INVALID || ins->bus_r == R_INVALID) {
        return 0x99;
    }
    if (ins->bus_w == W_LABEL && ins->bus_w_label == NULL) {
        if (ins->bus_w_label == NULL) {
            // printf("ERROR: instruction missing w_label");
        } else {
            printf("ERROR: instruction has unresolved w_label");
        }
        return 0x99;
    }
    return (((uint8_t)ins->bus_w & 0xF) << 4) | ((uint8_t)ins->bus_r & 0xF);
}

void asm_write_txt(const char *path, inc_context_t *inc_contexts, ast_index_t inc_contexts_count, instruction_t *instructions, index_t instructions_count, uint8_t *binary, index_t binary_size) {
    printf("writing text file \"%s\"... ", path);
    FILE *f_txt = fopen(path, "wb");

    if (f_txt == NULL) {
        printf("ERROR: could not access text file \"%s\"\r\n", path);
        return;
    }

    uint16_t pc = 0;
    for (index_t i = 0; i < instructions_count; i++) {
        if (instructions[i].label != NULL) {
            fprintf(f_txt, "# %s:\n", instructions[i].label);
        }
        fprintf(f_txt, "(%04x) ", pc);
        uint8_t bin = binary[pc];
        fprintf(f_txt, "%02x", bin);
        ast_index_t inc_context_i = 0;
        bool inc_context_found = false;
        for (ast_index_t j = 0; j < inc_contexts_count; j++) {
            if (instructions[i].ref_token.src == inc_contexts[j].src) {
                inc_context_i = j;
                inc_context_found = true;
                break;
            }
        }
        if (bin == 0x99) {
            fprintf(f_txt, "       #  ");
            if (inc_context_found) {
                fprintf(f_txt, "%s  ", inc_contexts[inc_context_i].path);
            }
            fprintf(f_txt, "l. %u - INVALID INSTRUCTION\r\n", asm_token_get_line(&instructions[i].ref_token));
            pc++;
            continue;
        }
        if (instructions[i].bus_w == W_LIT) {
            fprintf(f_txt, " %04x", instructions[i].literal);
        }
        if (bin != 0x99) {
            if (instructions[i].bus_w != W_LIT)
                fprintf(f_txt, "     ");
            fprintf(f_txt, "  #  ");
            if (inc_context_found) {
                fprintf(f_txt, "%s  ", inc_contexts[inc_context_i].path);
            }
            fprintf(f_txt, "l. %u", asm_token_get_line(&instructions[i].ref_token));
        }
        fprintf(f_txt, "\r\n");
        pc += instructions[i].bus_w == W_LIT ? 3 : 1;
    }

    fclose(f_txt);

    printf("done.\r\n");
}

void asm_tokenize_init(tokenizer_state_t *state) {
    state->i = 0;
}

#define RETURN_TOKEN(token_type) \
    return (token_t){            \
        .type = token_type,      \
        .start = token_start,    \
        .end = state->i,         \
        .src = src,              \
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

void asm_parse_element_init(ast_element_t *element) {
    element->type = AST_UNKNOWN;
    element->content_token.type = TOKEN_UNKNOWN;
    element->content_token.start = 0;
    element->content_token.end = 0;
    element->content_token.src = NULL;
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
    if (state->current_token.type != TOKEN_END) {
        state->next_token = asm_tokenize(&state->tokenizer_state, state->src);
    }
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
    } else if (state->current_token.type == TOKEN_PARAM) {
        ast_element_t *ast_child = asm_parse_add_child(&ast_ins->children[0]);
        ast_child->type = AST_PARAM;
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
        ast_element_t *new_child = asm_parse_add_content_child(&ast_ins->children[1], state->next_token);
        if (state->next_token.type == TOKEN_PARAM) {
            new_child->type = AST_PARAM;
            asm_parse_step_tokenizer(state);
        } else if (state->next_token.type == TOKEN_STAR_KEYWORD) {
            new_child->type = AST_DEF_CALL;
            new_child->content_token = state->next_token;
            asm_parse_step_tokenizer(state);
    
            if (state->next_token.type == TOKEN_OPEN_S) {
                asm_parse_add_child(new_child);
                new_child->children[0].type = AST_DEF_CONFIG;
    
                asm_parse_step_tokenizer(state);
                while (state->next_token.type != TOKEN_CLOSE_S) {
                    if (state->next_token.type == TOKEN_END) {
                        return false;
                    }
                    asm_parse_add_content_child(&new_child->children[0], state->next_token);
                    asm_parse_step_tokenizer(state);
                }
                asm_parse_step_tokenizer(state);
            }
        } else {
            asm_parse_step_tokenizer(state);
        }
    }

    return true;
}

#define ASM_PARSE_ERR(msg)                                     \
    printf("ERROR: ");                                         \
    asm_token_print_position(&state.current_token); \
    printf(" \"");                                             \
    asm_token_content_print(&state.current_token);  \
    printf(" ");                                               \
    asm_token_content_print(&state.next_token);     \
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
                asm_token_debug_print(&state.current_token);
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
    if (ast->content_token.src != NULL) {
        free(ast->content_token.src);
    }
    if (ast->children != NULL) {
        free(ast->children);
    }
}

void asm_parse_debug_print(ast_element_t *ast, int level) {
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
        asm_token_debug_print(&ast->content_token);
    }

    for (int s = 0; s < level; s++)
        printf("  ");
    printf("children_count: %i\r\n", ast->children_count);

    for (ast_index_t i = 0; i < ast->children_count; i++) {
        for (int s = 0; s < level; s++)
            printf("  ");
        printf("child (%i):\r\n", i);
        asm_parse_debug_print(&ast->children[i], level + 1);
    }
}

void asm_ins_init(instruction_t *ins) {
    ins->bus_w = W_INVALID;
    ins->bus_r = R_INVALID;
    ins->label = NULL;
    ins->bus_w_label = NULL;
    ins->literal = 0;
}

void asm_ins_free(instruction_t *instructions, index_t instructions_count) {
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

// TODO: !! indexed !! labels
void asm_ins_resolve_labels(instruction_t *ins, index_t ins_count) {
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
                asm_token_print_position(&ins[i].ref_token);
                printf(" ERROR: can't resolve label %s\r\n", ins[i].bus_w_label);
            }
        }
    }
}

uint16_t asm_parse_literal(const char *content, uint8_t base) {
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

instruction_t asm_instruction_from_ast(ast_element_t *ast_element) {
    if (ast_element->type != AST_INSTRUCTION || ast_element->children_count != 2) {
        printf("ERROR: incorrect AST type %i for instruction\r\n", ast_element->type);
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
        printf("ERROR: missing AST_INS_BUS_WRITE and/or AST_INS_BUS_READ\r\n");
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
            char *content = asm_token_get_content(&ast_bus_w->children[0].content_token);
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
            } else if (ast_bus_w->children[0].content_token.type == TOKEN_MINUS) {
                if (ast_bus_w->children_count < 2) {
                    printf("ERROR: ");
                    asm_token_print_position(&ins.ref_token);
                    printf(" expected literal after sign\r\n");
                } else {
                    char *content2 = asm_token_get_content(&ast_bus_w->children[1].content_token);
                    if (ast_bus_w->children[1].content_token.type == TOKEN_LITERAL_b) {
                        ins.bus_w = W_LIT;
                        ins.literal = -asm_parse_literal(content2, 2);
                    } else if (ast_bus_w->children[1].content_token.type == TOKEN_LITERAL_o) {
                        ins.bus_w = W_LIT;
                        ins.literal = -asm_parse_literal(content2, 8);
                    } else if (ast_bus_w->children[1].content_token.type == TOKEN_LITERAL_d) {
                        ins.bus_w = W_LIT;
                        ins.literal = -asm_parse_literal(content2, 10);
                    } else if (ast_bus_w->children[1].content_token.type == TOKEN_LITERAL_x) {
                        ins.bus_w = W_LIT;
                        ins.literal = -asm_parse_literal(content2, 16);
                    } else {
                        printf("ERROR: ");
                        asm_token_print_position(&ins.ref_token);
                        printf(" expected literal after sign\r\n");
                    }
                    free(content2);
                }
            } else if (ast_bus_w->children[0].content_token.type == TOKEN_LITERAL_b) {
                ins.bus_w = W_LIT;
                ins.literal = asm_parse_literal(content, 2);
            } else if (ast_bus_w->children[0].content_token.type == TOKEN_LITERAL_o) {
                ins.bus_w = W_LIT;
                ins.literal = asm_parse_literal(content, 8);
            } else if (ast_bus_w->children[0].content_token.type == TOKEN_LITERAL_d) {
                ins.bus_w = W_LIT;
                ins.literal = asm_parse_literal(content, 10);
            } else if (ast_bus_w->children[0].content_token.type == TOKEN_LITERAL_x) {
                ins.bus_w = W_LIT;
                ins.literal = asm_parse_literal(content, 16);
            } else {
                printf("ERROR: ");
                asm_token_print_position(&ins.ref_token);
                printf(" unknown write-to-bus value\r\n");
            }
            free(content);
            break;
        }
        case AST_LABEL: {
            char *content = asm_token_get_content(&ast_bus_w->children[0].content_token);
            if (content == NULL) {
                break;
            }
            ins.bus_w = W_LABEL;
            ins.bus_w_label = content;
            break;
        }
        case AST_DEF_CALL:
            ins.bus_w = W_INVALID;
            ins.bus_w_def_token = ast_bus_w->children[0].content_token;
            break;
        default:
            printf("ERROR: ");
            asm_token_print_position(&ins.ref_token);
            printf(" unknown write-to-bus value\r\n");
            break;
    }
    switch (ast_bus_r->children[0].type) {
        case AST_CONTENT: {
            if (ast_bus_r->children_count == 1) {
                char *content = asm_token_get_content(&ast_bus_r->children[0].content_token);
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
                    printf("ERROR: ");
                    asm_token_print_position(&ins.ref_token);
                    printf(" unknown read-from-bus value\r\n");
                }
                free(content);
            } else if (ast_bus_r->children_count == 2) {
                char *content1 = asm_token_get_content(&ast_bus_r->children[0].content_token);
                if (content1 == NULL) {
                    break;
                }
                char *content2 = asm_token_get_content(&ast_bus_r->children[1].content_token);
                if (content2 == NULL) {
                    free(content1);
                    break;
                }
                if (strcmp(content1, "A") == 0 && strcmp(content2, "B") == 0) {
                    ins.bus_r = R_A_B;
                } else if (strcmp(content1, "B") == 0 && strcmp(content2, "RAM_P") == 0) {
                    ins.bus_r = R_B_RP;
                } else if (strcmp(content1, "C") == 0 && strcmp(content2, "PC") == 0) {
                    ins.bus_r = R_C_PC;
                } else {
                    printf("ERROR: ");
                    asm_token_print_position(&ins.ref_token);
                    printf(" unknown read-from-bus value\r\n");
                }
                free(content1);
                free(content2);
            } else {
                printf("ERROR: ");
                asm_token_print_position(&ins.ref_token);
                printf(" unknown read-from-bus value\r\n");
            }
            break;
        }
        case AST_DEF_CALL:
            ins.bus_r = R_INVALID;
            ins.bus_r_def_token = ast_bus_r->children[0].content_token;
            break;
        default:
            printf("ERROR: ");
            asm_token_print_position(&ins.ref_token);
            printf(" unknown read-from-bus value\r\n");
            break;
    }
    return ins;
}

void asm_ast_deep_copy(ast_element_t *dest, ast_element_t *src) {
    dest->type = src->type;
    dest->content_token = src->content_token;
    for (ast_index_t i = 0; i < src->children_count; i++) {
        ast_element_t *child = asm_parse_add_child(dest);
        asm_ast_deep_copy(child, &src->children[i]);
    }
}

bool asm_compare_token(token_t *token, const char *against) {
    const char *p_t = &token->src[token->start];
    const char *p_a = against;
    for (; *p_t != '\0' && *p_a != '\0'; p_t++, p_a++) {
        if (*p_t != *p_a) {
            return false;
        }
    }
    if (p_t == &token->src[token->end] && *p_a == '\0') {
        return true;
    }
    return false;
}

bool asm_compare_tokens(token_t *token1, token_t *token2) {
    index_t i1 = token1->start;
    index_t i2 = token2->start;
    for (; i1 < token1->end && i2 < token2->end; i1++, i2++) {
        if (token1->src[i1] != token2->src[i2]) {
            return false;
        }
    }
    if (i1 == token1->end && i2 == token2->end) {
        return true;
    }
    return false;
}

bool asm_create_def_if_matching(ast_element_t *root, const char *d1, ast_element_t def, ast_element_t def_call, ast_type_t bus_type, bool *keep_a, bool *keep_b, bool *keep_c, bool *rs_done, bool *param_set, ast_element_t *ast_bus_w, ast_element_t *ast_bus_r) {
    char *d2 = asm_token_get_content(&def.content_token);
    if (d2 == NULL) {
        printf("ERROR: out of memory for def content allocation\r\n");
        return false;
    }
    if (strcmp(d1, d2) == 0) {
        for (ast_index_t k = 0; k < def.children_count; k++) {
            switch (def.children[k].type) {
                case AST_DEF_CONFIG: {
                    ast_element_t *keep_elements = NULL;
                    ast_index_t keep_elements_count = 0;
                    for (ast_index_t i = 0; i < def_call.children_count; i++) {
                        if (def_call.children[i].type == AST_DEF_CONFIG) {
                            for (ast_index_t j = 0; j < def_call.children[i].children_count; j++) {
                                if (def_call.children[i].children[j].type == AST_CONTENT && asm_compare_token(&def_call.children[i].children[j].content_token, "keep")) {
                                    keep_elements = &def_call.children[i].children[j + 2];
                                    // TODO: this assumes all elements until end are part of keep, making following options impossible
                                    keep_elements_count = def_call.children[i].children_count - j - 2;
                                }
                            }
                        }
                    }
                    ast_element_t *temp_elements = NULL;
                    ast_index_t temp_elements_count;
                    for (ast_index_t l = 0; l < def.children[k].children_count; l++) {
                        if (def.children[k].children[l].type == AST_CONTENT && asm_compare_token(&def.children[k].children[l].content_token, "temp")) {
                            temp_elements = &def.children[k].children[l + 2];
                            // TODO: this assumes all elements until end are part of temp, making following options impossible
                            temp_elements_count = def.children[k].children_count - l - 2;
                        }
                    }
                    if (temp_elements != NULL && keep_elements != NULL) {
                        for (ast_index_t l = 0; l < keep_elements_count; l++) {
                            bool is_temp = false;
                            for (ast_index_t m = 0; m < temp_elements_count; m++) {
                                if (asm_compare_tokens(&keep_elements[l].content_token, &temp_elements[m].content_token)) {
                                    is_temp = true;
                                    if (asm_compare_token(&keep_elements[l].content_token, "A")) {
                                        *keep_a = true;
                                    } else if (asm_compare_token(&keep_elements[l].content_token, "B")) {
                                        *keep_b = true;
                                    } else if (asm_compare_token(&keep_elements[l].content_token, "C")) {
                                        *keep_c = true;
                                    } else {
                                        printf("ERROR: ");
                                        asm_token_print_position(&keep_elements[l].content_token);
                                        printf(" unknown keep/temp \"");
                                        asm_token_content_print(&keep_elements[l].content_token);
                                        printf("\"\r\n");
                                    }
                                }
                            }
                            if (!is_temp) {
                                printf("INFO: ");
                                asm_token_print_position(&keep_elements[l].content_token);
                                printf(" keep \"");
                                asm_token_content_print(&keep_elements[l].content_token);
                                printf("\" not required, as is not overwritten\r\n");
                            }
                        }
                    }
                    // TODO: !!! never restore read-from-bus param !!!
                    // TODO: restore instructions for write-to-bus-reg if input flag * is after !RS and write-to-bus-reg in temp (write-to-bus-reg for ADD/NOR is "A B" and COM "A")
                    // ^ this is important since the def doesn't know which parameter it receives, thus you would need STA, STB and STC everywhere if there are calculations before using param
                    if (*keep_a) {
                        asm_parse_add_child(root)->type = AST_STA;
                    }
                    if (*keep_b) {
                        asm_parse_add_child(root)->type = AST_STB;
                    }
                    if (*keep_c) {
                        asm_parse_add_child(root)->type = AST_STC;
                    }
                    break;
                }
                case AST_LABEL: {
                    ast_element_t *new_el = asm_parse_add_child(root);
                    asm_ast_deep_copy(new_el, &def.children[k]);
                    break;
                }
                case AST_INSTRUCTION: {
                    ast_element_t *new_el = asm_parse_add_child(root);
                    asm_ast_deep_copy(new_el, &def.children[k]);
                    
                    ast_element_t *new_bus_w = NULL;
                    ast_element_t *new_bus_r = NULL;
                    for (index_t i = 0; i < new_el->children_count; i++) {
                        if (new_el->children[i].type == AST_INS_BUS_WRITE) {
                            new_bus_w = &new_el->children[i];
                            if (new_bus_r != NULL) {
                                break;
                            }
                        } else if (new_el->children[i].type == AST_INS_BUS_READ) {
                            new_bus_r = &new_el->children[i];
                            if (new_bus_w != NULL) {
                                break;
                            }
                        }
                    }
                    if (new_bus_w != NULL) {
                        if (new_bus_w->children_count > 0) {
                            if (new_bus_w->children[0].type == AST_PARAM) {
                                if (bus_type != AST_INS_BUS_READ || ast_bus_w == NULL) {
                                    printf("ERROR: ");
                                    asm_token_print_position(&def_call.content_token);
                                    printf(" incorrect def usage (expected form A -> %s)\r\n", d2);
                                    break;
                                }
                                asm_ast_deep_copy(&new_bus_w->children[0], &ast_bus_w->children[0]);
                                *param_set = true;
                            }
                        }
                    }
                    if (new_bus_r != NULL) {
                        if (new_bus_r->children_count > 0) {
                            if (new_bus_r->children[0].type == AST_PARAM) {
                                if (bus_type != AST_INS_BUS_WRITE || ast_bus_r == NULL) {
                                    printf("ERROR: ");
                                    asm_token_print_position(&def_call.content_token);
                                    printf(" incorrect def usage (expected form %s -> A)\r\n", d2);
                                    break;
                                }
                                asm_ast_deep_copy(&new_bus_r->children[0], &ast_bus_r->children[0]);
                                *param_set = true;
                            }
                        }
                    }
                    break;
                }
                case AST_ASM_DIRECTIVE: {
                    if (def.children[k].children_count < 1) {
                        printf("ERROR: ");
                        asm_token_print_position(&def.content_token);
                        printf(" directive without content\r\n");
                    }

                    char *directive = asm_token_get_content(&def.children[k].children[0].content_token);
                    if (directive == NULL) {
                        printf("ERROR: ");
                        asm_token_print_position(&def.children[k].children[0].content_token);
                        printf(" failed directive content allocation\r\n");
                        break;
                    }
                    
                    if (directive[0] == 'R' && directive[1] == 'S' && directive[2] == '\0') {
                        *rs_done = true;
                        
                        if (*keep_a) {
                            asm_parse_add_child(root)->type = AST_RSA;
                        }
                        if (*keep_b) {
                            asm_parse_add_child(root)->type = AST_RSB;
                        }
                        if (*keep_c) {
                            asm_parse_add_child(root)->type = AST_RSC;
                        }
                    } else {
                        printf("ERROR: ");
                        asm_token_print_position(&def.children[k].children[0].content_token);
                        printf(" unknown directive \"%s\"\r\n", directive);
                    }
                    free(directive);
                    break;
                }
                default:
                    break;
            }
        }
        free(d2);
        return true;
    }
    free(d2);
    return false;
}

void asm_create_def(ast_element_t *root, ast_element_t instruction, ast_element_t def_call, inc_context_t *inc_contexts, ast_index_t inc_contexts_count, ast_type_t bus_type) {
    char *d1 = asm_token_get_content(&def_call.content_token);
    if (d1 == NULL) {
        printf("ERROR: out of memory for def call content allocation\r\n");
        return;
    }
    ast_element_t *ast_bus_w = NULL;
    ast_element_t *ast_bus_r = NULL;
    for (ast_index_t i = 0; i < instruction.children_count; i++) {
        if (instruction.children[i].type == AST_INS_BUS_WRITE) {
            ast_bus_w = &instruction.children[i];
            if (ast_bus_r != NULL) {
                break;
            }
        } else if (instruction.children[i].type == AST_INS_BUS_READ) {
            ast_bus_r = &instruction.children[i];
            if (ast_bus_w != NULL) {
                break;
            }
        }
    }
    bool keep_a = false, keep_b = false, keep_c = false;
    bool def_found = false;
    bool rs_done = false;
    bool param_set = false;
    for (ast_index_t j = 0; j < root->children_count; j++) {
        if (root->children[j].type == AST_DEF) {
            if (asm_create_def_if_matching(root, d1, root->children[j], def_call, bus_type, &keep_a, &keep_b, &keep_c, &rs_done, &param_set, ast_bus_w, ast_bus_r)) {
                def_found = true;
                break;
            }
        }
    }
    for (ast_index_t j = 0; j < inc_contexts_count; j++) {
        for (ast_index_t k = 0; k < inc_contexts[j].ast.children_count; k++) {
            if (inc_contexts[j].ast.children[k].type == AST_DEF) {
                if (asm_create_def_if_matching(root, d1, inc_contexts[j].ast.children[k], def_call, bus_type, &keep_a, &keep_b, &keep_c, &rs_done, &param_set, ast_bus_w, ast_bus_r)) {
                    def_found = true;
                    break;
                }
            }
        }
        if (def_found) {
            break;
        }
    }
    if (def_found) {
        if (!rs_done) {
            if (keep_a) {
                asm_parse_add_child(root)->type = AST_RSA;
            }
            if (keep_b) {
                asm_parse_add_child(root)->type = AST_RSB;
            }
            if (keep_c) {
                asm_parse_add_child(root)->type = AST_RSC;
            }
        }
        if (!param_set && bus_type != AST_INSTRUCTION) {
            printf("ERROR: ");
            asm_token_print_position(&def_call.content_token);
            printf(" def \"%s\" does not take or give a parameter\r\n", d1);
        }
    } else {
        printf("ERROR: ");
        asm_token_print_position(&def_call.content_token);
        printf(" def \"%s\" not found\r\n", d1);
    }
    free(d1);
}

ast_element_t asm_resolve_defs(ast_element_t ast, inc_context_t *inc_contexts, ast_index_t inc_contexts_count) {
    for (ast_index_t i = 0; i < ast.children_count; i++) {
        if (ast.children[i].type == AST_DEF) {
            char *d1 = asm_token_get_content(&ast.children[i].content_token);
            if (d1 == NULL) {
                continue;
            }
            for (ast_index_t j = i + 1; j < ast.children_count; j++) {
                if (ast.children[j].type == AST_DEF) {
                    char *d2 = asm_token_get_content(&ast.children[j].content_token);
                    if (d2 == NULL) {
                        continue;
                    }
                    if (strcmp(d1, d2) == 0) {
                        printf("WARNING: using ");
                        asm_token_print_position(&ast.children[i].content_token);
                        printf(", not ");
                        asm_token_print_position(&ast.children[j].content_token);
                        printf(" for duplicate def \"%s\"\r\n", d1);
                        break;
                    }
                    free(d2);
                }
            }
            free(d1);
        }
    }
    
    ast_element_t new_ast = {
        .children = NULL,
        .children_count = 0,
        .type = AST_ROOT,
    };

    for (ast_index_t i = 0; i < ast.children_count; i++) {
        bool was_def_call = false;
        if (ast.children[i].type == AST_INSTRUCTION) {
            ast_element_t *bus_w = NULL, *bus_r = NULL;
            for (ast_index_t j = 0; j < ast.children[i].children_count; j++) {
                if (ast.children[i].children[j].type == AST_INS_BUS_WRITE) {
                    bus_w = &ast.children[i].children[j];
                    if (bus_r != NULL) {
                        break;
                    }
                } else if (ast.children[i].children[j].type == AST_INS_BUS_READ) {
                    bus_r = &ast.children[i].children[j];
                    if (bus_w != NULL) {
                        break;
                    }
                }
            }
            bool bus_r_exists = false;
            if (bus_r != NULL) {
                if (bus_r->children_count > 0) {
                    bus_r_exists = true;
                    if (bus_r->children[0].type == AST_DEF_CALL) {
                        asm_create_def(&new_ast, ast.children[i], bus_r->children[0], inc_contexts, inc_contexts_count, AST_INS_BUS_READ);
                        was_def_call = true;
                    }
                }
            }
            if (bus_w != NULL) {
                if (bus_w->children_count > 0) {
                    if (bus_w->children[0].type == AST_DEF_CALL) {
                        asm_create_def(&new_ast, ast.children[i], bus_w->children[0], inc_contexts, inc_contexts_count, bus_r_exists ? AST_INS_BUS_WRITE : AST_INSTRUCTION);
                        was_def_call = true;
                    }
                }
            }
        }
        if (!was_def_call) {
            ast_element_t *new_el = asm_parse_add_child(&new_ast);
            asm_ast_deep_copy(new_el, &ast.children[i]);
        }
    }

    return new_ast;
}

instruction_t *asm_instructions_from_ast(ast_element_t ast, index_t *instructions_count) {
    (*instructions_count) = 0;
    instruction_t *instructions = NULL;
    char *current_label = NULL;

    for (ast_index_t i = 0; i < ast.children_count; i++) {
        switch (ast.children[i].type) {
            case AST_INSTRUCTION:
                (*instructions_count)++;
                instructions = realloc(instructions, (*instructions_count) * sizeof(instruction_t));
                if (instructions == NULL) {
                    printf("ERROR: out of memory during instruction allocation\r\n");
                    return NULL;
                }
                instructions[(*instructions_count) - 1] = asm_instruction_from_ast(&ast.children[i]);
                instructions[(*instructions_count) - 1].label = current_label;
                current_label = NULL;
                break;
            case AST_STA:
            case AST_STB:
            case AST_STC:
            case AST_RSA:
            case AST_RSB:
            case AST_RSC: {
                (*instructions_count) += 2;
                instructions = realloc(instructions, (*instructions_count) * sizeof(instruction_t));
                if (instructions == NULL) {
                    printf("ERROR: out of memory during instruction allocation\r\n");
                    return NULL;
                }
                instructions[(*instructions_count) - 2] = (instruction_t){
                    .bus_w = W_LIT,
                    .bus_r = R_RP,
                    .bus_w_label = NULL,
                    .label = NULL,
                    .literal = 0,
                    .ref_token = { .type = TOKEN_UNKNOWN, .start = 0, .end = 0 },
                };
                instructions[(*instructions_count) - 1] = (instruction_t){
                    .bus_w = W_INVALID,
                    .bus_r = R_INVALID,
                    .bus_w_label = NULL,
                    .label = NULL,
                    .literal = 0,
                    .ref_token = { .type = TOKEN_UNKNOWN, .start = 0, .end = 0 },
                };
                if (ast.children[i].type == AST_STA || ast.children[i].type == AST_RSA) {
                    // TODO: let this just add a def call to AST, expect def in assembly. should work with src per token
                    instructions[(*instructions_count) - 2].literal = ASM_STA_ADDRESS;
                    instructions[(*instructions_count) - 1].bus_w = ast.children[i].type == AST_STA ? W_A : W_RAM;
                    instructions[(*instructions_count) - 1].bus_r = ast.children[i].type == AST_STA ? R_RAM : W_A;
                }
                if (ast.children[i].type == AST_STB || ast.children[i].type == AST_RSB) {
                    instructions[(*instructions_count) - 2].literal = ASM_STB_ADDRESS;
                    instructions[(*instructions_count) - 1].bus_w = ast.children[i].type == AST_STB ? W_B : W_RAM;
                    instructions[(*instructions_count) - 1].bus_r = ast.children[i].type == AST_STB ? R_RAM : W_B;
                }
                if (ast.children[i].type == AST_STC || ast.children[i].type == AST_RSC) {
                    instructions[(*instructions_count) - 2].literal = ASM_STC_ADDRESS;
                    instructions[(*instructions_count) - 1].bus_w = ast.children[i].type == AST_STC ? W_C : W_RAM;
                    instructions[(*instructions_count) - 1].bus_r = ast.children[i].type == AST_STC ? R_RAM : W_C;
                }
                break;
            }
            case AST_LABEL:
                if (current_label != NULL) {
                    free(current_label);
                }
                current_label = asm_token_get_content(&ast.children[i].content_token);
                break;
            default:
                break;
        }
    }

    return instructions;
}

void asm_free_inc_contexts(inc_context_t *inc_contexts, ast_index_t inc_contexts_count) {
    for (ast_index_t i = 0; i < inc_contexts_count; i++) {
        asm_parse_free_ast(&inc_contexts[i].ast);
        if (inc_contexts[i].path != NULL) {
            free(inc_contexts[i].path);
        }
        if (inc_contexts[i].src != NULL) {
            free(inc_contexts[i].src);
        }
    }
    free(inc_contexts);
}

inc_context_t *asm_resolve_inc_contexts(ast_element_t ast, ast_index_t *inc_contexts_count) {
    inc_context_t *inc_contexts = NULL;
    for (ast_index_t i = 0; i < ast.children_count; i++) {
        if (ast.children[i].type == AST_INC) {
            (*inc_contexts_count)++;
            inc_contexts = realloc(inc_contexts, (*inc_contexts_count) * sizeof(inc_context_t));
            if (inc_contexts == NULL) {
                printf("ERROR: include context allocation failed\r\n");
                return NULL;
            }
            inc_context_t *new_context = &inc_contexts[(*inc_contexts_count) - 1];
            new_context->path = asm_token_get_content(&ast.children[i].content_token);
            new_context->src = asm_read_src(new_context->path);
            if (new_context->src == NULL) {
                printf("ERROR: failed to read included source file \"%s\"\r\n", new_context->path);
                continue;
            }
            new_context->ast = asm_parse(new_context->src);
        }
    }
    return inc_contexts;
}

int main(int argc, char *argv[]) {
    const char *src_path = asm_debug_src;

#if !ASM_DEBUG
    if (argc > 1) {
        src_path = argv[1];
    } else {
        printf("ERROR: please specify the path of one assembly file (.ha) as a command line parameter\r\n");
        return 1;
    }
#endif

    char *src_buf = asm_read_src(src_path);
    if (src_buf == NULL) {
        printf("ERROR: failed to read source file \"%s\"\r\n", src_path);
        return 1;
    }

    ast_element_t ast = asm_parse(src_buf);

    ast_index_t inc_contexts_count = 0;
    inc_context_t *inc_contexts = asm_resolve_inc_contexts(ast, &inc_contexts_count);
    for (uint8_t i = 0; i < ASM_DEF_RESOLVE_PASSES; i++) {
        ast = asm_resolve_defs(ast, inc_contexts, inc_contexts_count);
    }

    index_t instructions_count = 0;
    instruction_t *instructions = asm_instructions_from_ast(ast, &instructions_count);
    if (instructions_count == 0) {
        printf("ERROR: there are no instructions in the given source file\r\n");
        return 1;
    }
    if (instructions == NULL) {
        printf("ERROR: failed conversion from AST to instructions\r\n");
        return 1;
    }
    asm_ins_resolve_labels(instructions, instructions_count);

    uint8_t *binary = NULL;
    index_t binary_size = 0;
    for (index_t i = 0; i < instructions_count; i++) {
        binary_size += instructions[i].bus_w == W_LIT ? 3 : 1;
        binary = realloc(binary, binary_size);
        if (binary == NULL) {
            printf("ERROR: out of memory during binary allocation\r\n");
            return 1;
        }
        uint8_t bin = asm_ins_to_binary(&instructions[i]);
        binary[binary_size - (instructions[i].bus_w == W_LIT ? 3 : 1)] = bin;
        if (instructions[i].bus_w == W_LIT) {
            if (bin != 0x99) {
                binary[binary_size - 2] = instructions[i].literal >> 8;
                binary[binary_size - 1] = instructions[i].literal & 0xFF;
            } else {
                binary_size -= 2;
            }
        }
    }

    char *bin_path = malloc(strlen(src_path) + 4);
    char *img_path = malloc(strlen(src_path) + 4);
    char *txt_path = malloc(strlen(src_path) + 4);
    if (bin_path == NULL || img_path == NULL || txt_path == NULL) {
        printf("ERROR: out of memory during path allocation\r\n");
        return 1;
    }
    memcpy(bin_path, src_path, strlen(src_path) + 1);
    memcpy(img_path, src_path, strlen(src_path) + 1);
    memcpy(txt_path, src_path, strlen(src_path) + 1);
    char *bin_iterator = bin_path + strlen(src_path) - 1;
    char *img_iterator = img_path + strlen(src_path) - 1;
    char *txt_iterator = txt_path + strlen(src_path) - 1;
    while (*bin_iterator != '.' && bin_iterator != bin_path) {
        bin_iterator--;
        img_iterator--;
        txt_iterator--;
    }
    bin_iterator[1] = 'b';
    bin_iterator[2] = 'i';
    bin_iterator[3] = 'n';
    bin_iterator[4] = '\0';
    img_iterator[1] = 'i';
    img_iterator[2] = 'm';
    img_iterator[3] = 'g';
    img_iterator[4] = '\0';
    txt_iterator[1] = 't';
    txt_iterator[2] = 'x';
    txt_iterator[3] = 't';
    txt_iterator[4] = '\0';
    asm_write_bin(bin_path, binary, binary_size);
    asm_write_img(img_path, binary, binary_size);
    asm_write_txt(txt_path, inc_contexts, inc_contexts_count, instructions, instructions_count, binary, binary_size);

    if (binary != NULL) {
        free(binary);
    }
    if (instructions != NULL) {
        asm_ins_free(instructions, instructions_count);
    }
    if (inc_contexts != NULL) {
        asm_free_inc_contexts(inc_contexts, inc_contexts_count);
    }
    asm_parse_free_ast(&ast);
    if (src_buf != NULL) {
        free(src_buf);
    }

    return 0;
}
