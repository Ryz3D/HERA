def *NOP {
    A -> VOID;
}

# register operations
def *STA {
    0x11   -> RAM_P;
    A      -> RAM;
}
def *RSA {
    0x11   -> RAM_P;
    RAM    -> A;
}
def *STB {
    0x12   -> RAM_P;
    B      -> RAM;
}
def *RSB {
    0x12   -> RAM_P;
    RAM    -> B;
}
def *STC {
    0x13   -> RAM_P;
    C      -> RAM;
}
def *RSC {
    0x13   -> RAM_P;
    RAM    -> C;
}
def *INCA[temp=B] {
    1      -> B;
    ADD    -> A;
}
def *DECA[temp=B] {
    -1     -> B;
    ADD    -> A;
}
def *INCB[temp=A] {
    1      -> A;
    ADD    -> B;
}
def *DECB[temp=A] {
    -1     -> A;
    ADD    -> B;
}
def *INCC[temp=A B] {
    C      -> A;
    1      -> B;
    ADD    -> C;
}
def *DECC[temp=A B] {
    C      -> A;
    -1     -> B;
    ADD    -> C;
}

# RAM operations
def *INC[temp=A B] {
    RAM    -> B;
    1      -> A;
    ADD    -> RAM;
}
def *DEC[temp=A B] {
    RAM    -> B;
    -1     -> A;
    ADD    -> RAM;
}
def *GPIA { # write input data of GPIA into *
    0x0100 -> RAM_P;
    RAM    -> *;
}
def *GPOA { # write * to output data of GPOA
    0x0200 -> RAM_P;
    *      -> RAM;
}
def *ARRINIT { # init writing to an array at address *
    1      -> A;
    *      -> B RAM_P;
}
def *ARR { # append * to array (call *ARRINIT first)
    *      -> RAM;
    ADD    -> B RAM_P;
}

# arithmetic operations
def *SUB[temp=B C] { # set * to (A - B)
    A      -> C;
    B      -> A;
    COM    -> B;
    C      -> A;
    ADD    -> *;
}
def *MUL { #TODO; }
def *DIV { #TODO; }
def *MOD { #TODO; }
# TODO: 32bit arithmetic

# binary operations
def *NOT[temp=B] { # set * to (~A)
    A      -> B;
    NOR    -> *;
}
def *OR[temp=A B] { # set * to (A | B)
    NOR    -> A B;
    NOR    -> *;
}
def *AND[temp=A B C] { # set * to (A & B)
    *STB;
    A      -> B;
    NOR    -> C; # could we store b somewhere else and this result somewhere else entirely? no *RSB?
    *RSB;
    B      -> A;
    NOR    -> B;
    C      -> A;
    NOR    -> *;
}
def *XOR[temp=A B C] { # set * to (A ^ B)
    *AND[keep=A B] -> C;
    NOR    -> A;
    C      -> B;
    NOR    -> *;
}
def *ASL { #TODO; }
def *ROL { #TODO; }
def *ASR { #TODO; }
def *ROR { #TODO; }

# stack
# stack pointer 0xFF
# stack start   0xFFEF (descending)
def *RSS {
    0xFF   -> RAM_P;
    0xFEFF -> RAM;
}
def *PUSH[temp=A B] { # push * to stack
    0xFF   -> RAM_P;
    RAM    -> RAM_P;
    *      -> RAM;
    0xFF   -> RAM_P;
    *DEC;
}
def *POP[temp=A B] { # pop from stack to *
    0xFF   -> RAM_P;
    *INC;
    RAM    -> RAM_P;
    RAM    -> *;
}

# flow control
def *JSR[temp=A B] { # jump to subroutine (* -> PC)
#     "jsr_[x+1]" -> *PUSH[keep=A B];
#     *      -> PC;
# "jsr_[x]":
    PC     -> A;
    19     -> B;
    ADD    -> *PUSH;
    # !RS; # TODO: this varies in size... once indexed labels work, try implementing *JSR[keep=A] to pass A as parameter
    *      -> PC;
}
def *RTS[temp=A B] { # return from subroutine
    0xFF   -> RAM_P;
    *INC;
    !RS;
    0xFF   -> RAM_P;
    RAM    -> RAM_P;
    RAM    -> PC;
}
