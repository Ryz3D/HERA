#include "includes.h"

typedef struct inter_context {
    // TODO: context description struct (all defined functions/variables)
    // TODO: add all variables to list (maybe prefix: filename_function_variable), before adding: check if name already exists and append something to differenciate
    int i;
} inter_context_t;

typedef struct inter_ins {
    /*
    TODO: abstract instructions:
    - get/set variables by name (even temporary values, maybe just number them in order), do not care where they are stored
    - declare variable type just before first usage (8, 16, 32, 64)
    - pointers/structs/arrays are arithmetic operations
        - return structs always as pointer -> if not a c-pointer, read values from returned pointer into new variable
    - functions are denoted by labels
    - type reprentations always as struct (multiple fields of individual size)? even for typedef uint64_t index_t;
    */
    int i;
} inter_ins_t;

// returns false on error
bool cmp_inter_generate_context_from_ast(ast_element_t *ast, inter_context_t *context) {
    return true;
}

bool cmp_inter_generate_context_from_file(const char *f_path, inter_context_t *context) {
    // TODO: look in cache if file re-included
    ast_element_t *ast;
    bool success = cmp_parser_run(f_path, &ast);
    if (ast == NULL || !success) {
        return false;
    }
    cmp_inter_generate_context_from_ast(ast, context);
    return true;
}

bool cmp_inter_generate_from_context(inter_context_t *context, const char *func, inter_ins_t **inter_ins, uint32_t *inter_ins_count) {
    *inter_ins = NULL;
    *inter_ins_count = 0;

    // TODO: generate intermediate code for function "func" in context

    return true;
}

bool cmp_inter_generate(const char *f_path, inter_ins_t **inter_ins, uint32_t *inter_ins_count) {
    inter_context_t ctx;
    cmp_inter_generate_context_from_file(f_path, &ctx);
    // TODO: get all contexts that are included
    // TODO: all functions at corresponding labels (temporary inter_ins_t[], then copy to big one including labels)
    cmp_inter_generate_from_context(&ctx, "main", inter_ins, inter_ins_count);
    // TODO: add code at 0x0000 to jump to label main

    return true;
}
