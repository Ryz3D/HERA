inc defaults.ha;

"main":
    1 -> A;
    A -> *PUSH;
    2 -> A;
    A -> *PUSH;
    3 -> A;
    A -> *PUSH;

    *POP -> A;
    A -> *GPOA;
    *NOP;
    *NOP;
    *NOP;
    *NOP;
    *POP -> A;
    A -> *GPOA;
    *NOP;
    *NOP;
    *NOP;
    *NOP;
    *POP -> A;
    A -> *GPOA;
    *NOP;
    *NOP;
    *NOP;
    *NOP;
