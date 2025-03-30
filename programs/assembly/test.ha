inc defaults.ha;
inc programs/assembly/malloc.ha;

def *W_PARAM1 {
    0x0020 -> RAM_P;
    *      -> RAM;
}
def *W_PARAM2 {
    0x0021 -> RAM_P;
    *      -> RAM;
}
def *W_PARAM3 {
    0x0022 -> RAM_P;
    *      -> RAM;
}
def *W_PARAM4 {
    0x0023 -> RAM_P;
    *      -> RAM;
}
def *R_PARAM1 {
    0x0020 -> RAM_P;
    RAM    -> *;
}
def *R_PARAM2 {
    0x0021 -> RAM_P;
    RAM    -> *;
}
def *R_PARAM3 {
    0x0022 -> RAM_P;
    RAM    -> *;
}
def *R_PARAM4 {
    0x0023 -> RAM_P;
    RAM    -> *;
}

"main":
    *RSS;
    *MALLOC_INIT;

    "testinit" -> *JSR;

    0x0010 -> *W_PARAM1;
    "malloc" -> *JSR;
    A      -> *GPOA;

"end":
    "end"  -> PC;

"testinit":
    1      -> A;
    0x003c -> B RAM_P; 0x0000 -> RAM; # previous block
    ADD    -> B RAM_P; 0x1040 -> RAM; # next block
    ADD    -> B RAM_P; 0x0ffc -> RAM; # capacity
    ADD    -> B RAM_P; 0x0010 -> RAM; # size

    1      -> A;
    0x103c -> B RAM_P; 0x0000 -> RAM; # previous block
    ADD    -> B RAM_P; 0x2040 -> RAM; # next block
    ADD    -> B RAM_P; 0x0ffc -> RAM; # capacity
    ADD    -> B RAM_P; 0x0020 -> RAM; # size

    1      -> A;
    0x203c -> B RAM_P; 0x0000 -> RAM; # previous block
    ADD    -> B RAM_P; 0x0000 -> RAM; # next block
    ADD    -> B RAM_P; 0xebbf -> RAM; # capacity
    ADD    -> B RAM_P; 0x2000 -> RAM; # size

    *RTS;

"malloc": *MALLOC;
"mfree": *MFREE;
