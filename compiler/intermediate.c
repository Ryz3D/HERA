#include "includes.h"

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
bool cmp_inter_generate(ast_element_t *ast) {
    return true;
}
