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

    0x0010 -> *W_PARAM1;
    "malloc" -> *JSR;
    A      -> *GPOA;

"end":
    "end"  -> PC;

"malloc": *MALLOC;
"mfree": *MFREE;
