typedef enum ast_type {
    AST_UNKNOWN,
    AST_EXPRESSION,
} ast_type_t;

typedef struct ast_element {
    ast_type_t type;
    char *str;
} ast_element_t;

typedef struct parser_state {
    uint32_t i;
} parser_state_t;
