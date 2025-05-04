#ifndef INC_INTERMEDIATE_C
#define INC_INTERMEDIATE_C

#include "includes.h"

typedef struct inter_context {
    // TODO: context description struct (all defined functions/variables)
    // TODO: add all variables (+functions?) to list (maybe prefix: filename_function_variable), before adding: check if name already exists and append something to differenciate
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
    - access to pointer with offset (array, struct, multi-word)
    */
    int i;
} inter_ins_t;

// returns false on error
bool cmp_inter_generate_context_from_ast(ast_element_t *ast, inter_context_t *context) {
    return true;
}

// returns false on error
bool cmp_inter_generate_context_from_file(const char *f_path, inter_context_t *context) {
    // TODO: path transformer for relative (to file or to cwd?) and absolute paths and search in include paths
    // TODO: look in cache if file re-included
    ast_element_t ast;
    bool success = cmp_parser_run(f_path, &ast);
    if (!success) {
        return false;
    }
    cmp_inter_generate_context_from_ast(&ast, context);
    return true;
}

// returns false on error
bool cmp_inter_generate_from_context(inter_context_t *context, const char *func, inter_ins_t **inter_ins, uint32_t *inter_ins_count) {
    *inter_ins = NULL;
    *inter_ins_count = 0;

    /*
    TODO:
     - generate intermediate code for function "func" in context
     - resolve compile-time calls like sizeof -> check if expression is compile-time or runtime for switch-case
     - resolve constant expressions into context (enums, defines, const globals), calculate and store known value
     - resolve string literals to pointer to const
    */

    return true;
}

// returns false on error
bool cmp_inter_generate(const char *f_path, inter_ins_t **inter_ins, uint32_t *inter_ins_count) {
    inter_context_t ctx;
    cmp_inter_generate_context_from_file(f_path, &ctx);
    /*
    TODO: get all contexts that are included
    include magic:
    - compile all given .c files
    - in case of include, parse file in place (let parser build ast of external file, extend base ast by it -> recursive includes are recursive function calls)
      - care about #pragma once
      - all used functions should be *declared* in current context (not neccessarily defined -> search in all other compiled files)
    */
    // TODO: all functions at corresponding labels (temporary inter_ins_t[], then copy to big one including labels)
    cmp_inter_generate_from_context(&ctx, "main", inter_ins, inter_ins_count);
    // TODO: add code at 0x0000 to jump to label main
    // TODO: expression steps to ordered instructions (resolve pointers, do arithmetics)

    return true;
}

#endif
