inc defaults.ha;
inc programs/assembly/malloc.ha;

"main":
    *RSS;
    *MALLOC_INIT;

    0x0010 -> A;
    *STA;
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
