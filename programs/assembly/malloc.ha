inc programs/assembly/defaults.ha;

# static ram 0x0300 - 0x7fff (size 0x7d00)
# heap 0x8004 (header starts at 0x8000) - 0xfbff (size 0x7bfc)
# stack 0xfc00 - 0xffff (size 0x0400)
# header:
#  -4: previous block (address after header)
#  -3: next block (address after header)
#  -2: capacity in address-space
#  -1: used size in address-space

# note: if you wanted to set up a block after a current one, add capacity (address right after current data) then add header size 4 (address after next header). this will need to be checked based on current address and capacity -> fits if 0xfbff-addr-capacity-4 >= newsize

def *MALLOC_INIT {
    1      -> A;
    0x8000 -> B RAM_P; 0x0000 -> RAM; # header: previous block (start of header)
    ADD    -> B RAM_P; 0x0000 -> RAM; # header: next block (start of header)
    ADD    -> B RAM_P; 0x7bfc -> RAM; # header: capacity
    ADD    -> B RAM_P; 0x0000 -> RAM; # header: size
}

def *MALLOC {
    *RSA;
    0x0040 -> RAM_P; # store requested size at 0x0040
    A      -> RAM;

    0x0041 -> RAM_P; # store current header pointer at 0x0041
    0x8000 -> RAM; # start at first heap block
"malloc_loop_find_block":
    # check if null pointer -> return 0
    0x0041 -> RAM_P;
    RAM    -> A;
    "malloc_return_null" -> PC_Z;

    # TODO: check if PARAM1<capacity -> allocate (shrink capacity to size or some other value, set "next" right after current block, set up header there based on previous "capacity" and "next"), return address at 0x0041 incremented by 4

    # load pointer to next block
    0x0041 -> RAM_P;
    RAM    -> A;
    1      -> B;
    ADD    -> RAM_P;
    RAM    -> A;
    0x0041 -> RAM_P;
    A      -> RAM;
    "malloc_loop_find_block" -> PC;

    *RSA;

"malloc_return_null":
    0      -> A;
    *RTS[keep=A];
}

def *MFREE {
    # TODO: merge previous and next free blocks
    *RSA;
    *RTS;
}
