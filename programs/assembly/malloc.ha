# heap 0x0040 (first header at 0x003c) - 0xfbff (size 0xfbc0)
# stack 0xfc00 - 0xffff (size 0x0400)
# header:
#  -4: previous block (address after header)
#  -3: next block (address after header)
#  -2: capacity in address-space
#  -1: used size in address-space

# note: if you wanted to set up a block after a current one, add capacity (address right after current data) then add header size 4 (address after next header). this will need to be checked based on current address and capacity -> fits if 0xfbff-addr-capacity-4 >= newsize

def *MALLOC_INIT {
    1      -> A;
    0x003c -> B RAM_P; 0x0000 -> RAM; # previous block
    ADD    -> B RAM_P; 0x0000 -> RAM; # next block
    ADD    -> B RAM_P; 0xfbbf -> RAM; # capacity
    ADD    -> B RAM_P; 0x0000 -> RAM; # size
}

def *MALLOC {
    # start at first malloc block
    0x0040 -> RAM_P;
"malloc_loop_find_empty":
    # TODO: iterate through next pointers until size == 0
    # TODO: check if next == 0 -> return 0
    # TODO: check if capacity < PARAM1 -> loop some more
    # TODO: if not -> set up size and next
    0x80   -> A;
    *RTS[keep=A];
}

def *MFREE {
    # TODO: merge previous and next free blocks
    *RTS;
}
