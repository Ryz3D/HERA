def *MALLOC {
    "testfunc" -> *JSR;
    *R_PARAM1 -> A;
    ADD    -> A;
    *RTS[keep=A];
"testfunc":
    0xFF   -> B;
    *RTS[keep=B];
}

def *MFREE {
    *RTS;
}
