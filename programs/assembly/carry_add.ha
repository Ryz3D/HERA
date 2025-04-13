inc programs/assembly/defaults.ha;

"main":
    0x21 -> RAM_P; 0x001F -> RAM;
    0x20 -> RAM_P; 0xFFFF -> RAM;

    0x23 -> RAM_P; 0x002F -> RAM;
    0x22 -> RAM_P; 0xFFFF -> RAM;

    0x20 -> RAM_P; RAM -> A;
    0x22 -> RAM_P; RAM -> B;
    0x25 -> RAM_P;
    ADD -> RAM;
    STAT -> A; # alternatively use PC_C to set carry input to 0 or 1
    0x01 -> B;
    *AND -> A;
    0x21 -> RAM_P; RAM -> B;
    ADD -> A;
    0x23 -> RAM_P; RAM -> B;
    0x26 -> RAM_P;
    ADD -> RAM;

    0x26 -> RAM_P;
    RAM -> A;
    A -> *GPOA;
    0x25 -> RAM_P;
    RAM -> A;
    A -> *GPOA;

"end":
    "end"  -> PC;
