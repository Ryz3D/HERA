# 01 00 01  increment array by 1
# 0b 00 80  array base pointer 0x0080
# 04 00 3f  0x0080 = 0x3F
# ab
# 04 00 06
# ab
# 04 00 5b
# ab
# 04 00 4f
# ab
# 04 00 66
# ab
# 04 00 6d
# ab
# 04 00 7d
# ab
# 04 00 07
# ab
# 04 00 7f
# ab
# 04 00 6f
# ab
# 04 00 77
# ab
# 04 00 7c
# ab
# 04 00 39
# ab
# 04 00 5e
# ab
# 04 00 79
# ab
# 04 00 71
# 05 10 01  # &x = 0x1001
# 04 00 01  # x = 1
# 05 10 02  # &y = 0x1002
# 04 00 01  # y = 1
# loop (0x0051):
# 05 10 02  # &y = 0x1002
# 42        # B = y
# 01 00 80  # A = 0x0080
# a5        # ADD -> RAM_P
# 41        # RAM -> A
# 05 02 00  # A -> *GPOA
# 14
# 05 10 01  # &x = 0x1001
# 41        # A = x
# 05 10 02  # &y = 0x1002
# 42        # B = y
# a3        # ADD -> C
# 05 10 01  # &x = 0x1001
# 24        # x = B
# 05 10 02  # &y = 0x1002
# 34        # y = C
# 06 00 51  # "loop" -> PC

# easier (no lookup)
# 05 10 01  # &x = 0x1001
# 04 00 01  # x = 1
# 05 10 02  # &y = 0x1002
# 04 00 01  # y = 1
# loop (0x0051):
# 05 10 02  # &y = 0x1002
# 41        # A = y
# 05 02 00  # A -> *GPOA
# 14
# 05 10 01  # &x = 0x1001
# 41        # A = x
# 05 10 02  # &y = 0x1002
# 42        # B = y
# a3        # ADD -> C
# 05 10 01  # &x = 0x1001
# 24        # x = B
# 05 10 02  # &y = 0x1002
# 34        # y = C
# 06 00 51  # "loop" -> PC

# !keep_all # implicitly [keep=A B C] for each def call
# !no_opti # disable optimization by simulation

inc defaults.ha;

"main":
    0x1001 -> RAM_P;
    1 -> RAM;
    0x1002 -> RAM_P;
    1 -> RAM;
"loop":
    0x1002 -> RAM_P;
    RAM -> A;
    0x0200 -> RAM_P;
    A -> RAM;
    0x1001 -> RAM_P;
    RAM -> A;
    0x1002 -> RAM_P;
    RAM -> B;
    ADD -> C;
    "end" -> PC_C;
    0x1001 -> RAM_P;
    B -> RAM;
    0x1002 -> RAM_P;
    C -> RAM;
    "loop" -> PC;
"end":
    "end" -> PC;

# OPTIMIZATION
# main:
#     0x1001 -> RAM_P;
#     1 -> RAM;
#     0x1002 -> RAM_P;
#     1 -> RAM;
# loop:
#     RAM -> A;          # always at 0x1002
#     0x0200 -> RAM_P;
#     A -> RAM;
#     0x1002 -> RAM_P;
#     RAM -> B;
#     0x1001 -> RAM_P;
#     RAM -> A;
#     ADD -> C;
#     B -> RAM;          # always at 0x1001
#     0x1002 -> RAM_P;
#     C -> RAM;
#     "loop" -> PC;
