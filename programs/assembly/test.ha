inc defaults.ha;
inc programs/assembly/malloc.ha;

"main":
    *STACK_INIT;
    *MALLOC_INIT;

    0x0010 -> A;
    *STA;
    "malloc" -> *JSR;
    A      -> *GPOA;
    0x1000 -> RAM_P; # store pointer at 0x1000
    A      -> RAM;

    1      -> A;
    0x1000 -> RAM_P;
    RAM    -> B RAM_P; 0x1234 -> RAM;
    ADD    -> B RAM_P; 0x5678 -> RAM;
    ADD    -> B RAM_P; 0x9abc -> RAM;
    ADD    -> B RAM_P; 0xdef0 -> RAM;

    1      -> A;
    0x1000 -> RAM_P;
    RAM    -> B RAM_P; RAM -> C; C -> *GPOA;
    ADD    -> B RAM_P; RAM -> C; C -> *GPOA;
    ADD    -> B RAM_P; RAM -> C; C -> *GPOA;
    ADD    -> B RAM_P; RAM -> C; C -> *GPOA;

    0x1000 -> RAM_P; # read back pointer from 0x1000
    RAM    -> A;
    *STA;
    "mfree" -> *JSR;

"end":
    "end"  -> PC;

"malloc": *MALLOC;
"mfree": *MFREE;
