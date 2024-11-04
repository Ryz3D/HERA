import sys

bus_symbol = '->'
f_debug = False
f_help = False
f_path = None

for f in sys.argv[1:]:
    if f in ['-d', '--debug']:
        f_debug = True
    elif f in ['-h', '--help']:
        f_help = True
    else:
        f_path = f

if f_help:
    print("""HERA assembler
""")
    exit()

def sim(program, debug=False, init_state=None):
    if init_state is None:
        sim_state = {
            'A': 0,
            'B': 0,
            'C': 0,
            'RAM': [0] * (2 ** 16),
            'RP': 0,
            'PC': 0,
            'STAT': 0,
            'program': program,
        }
    else:
        sim_state = init_state
        sim_state['program'].extend(program)

    for i in range(10000):
        pc = sim_state['PC']
        if pc >= len(sim_state['program']):
            return sim_state
        ins = sim_state['program'][pc]
        ins_w = ins >> 4
        ins_r = ins & 0xF
        sim_state['PC'] += 1
        if ins_w == 0x0:
            bus = (sim_state['program'][sim_state['PC']] << 8) | sim_state['program'][sim_state['PC'] + 1]
            sim_state['PC'] += 2
        elif ins_w == 0x1:
            bus = sim_state['A']
        elif ins_w == 0x2:
            bus = sim_state['B']
        elif ins_w == 0x3:
            bus = sim_state['C']
        elif ins_w == 0x4:
            bus = sim_state['RAM'][sim_state['RP']]
        elif ins_w == 0x5:
            bus = sim_state['RP']
        elif ins_w == 0x6:
            bus = sim_state['PC']
        elif ins_w == 0x7:
            bus = sim_state['STAT']
        elif ins_w >= 0xA and ins_w <= 0xC:
            sim_state['STAT'] &= ~0b111
            if ins_w == 0xA:
                bus = (sim_state['A'] + sim_state['B']) & 0xFFFF
                if (sim_state['A'] + sim_state['B']) & 0x10000 != 0:
                    sim_state['STAT'] |= 0x1
            elif ins_w == 0xB:
                bus = (~sim_state['A'] + 1) & 0xFFFF
                if (~sim_state['A'] + 1) & 0x10000 != 0:
                    sim_state['STAT'] |= 0x1
            elif ins_w == 0xC:
                bus = ~(sim_state['A'] | sim_state['B']) & 0xFFFF
                if bus & 0x1 != 0:
                    sim_state['STAT'] |= 0x1
            if bus == 0:
                sim_state['STAT'] |= 0x2
            if bus & 0x8000 != 0:
                sim_state['STAT'] |= 0x4
        else:
            print('unknown write nibble', ins_w, 'at', pc)
        if ins_r == 0x0:
            pass
        elif ins_r == 0x1:
            sim_state['A'] = bus
        elif ins_r == 0x2:
            sim_state['B'] = bus
        elif ins_r == 0x3:
            sim_state['C'] = bus
        elif ins_r == 0x4:
            sim_state['RAM'][sim_state['RP']] = bus
            if sim_state['RP'] == 0x200:
                if debug:
                    print('OUTPUT', hex(bus))
                else:
                    print(chr(bus), end='')
            elif debug:
                print('RAM', hex(bus), '->', hex(sim_state['RP']))
        elif ins_r == 0x5:
            sim_state['RP'] = bus
        elif ins_r == 0x6:
            sim_state['PC'] = bus
        elif ins_r == 0x7:
            sim_state['STAT'] = bus & 0xFF
        elif ins_r == 0xA:
            sim_state['A'] = sim_state['B'] = bus
        elif ins_r == 0xB:
            sim_state['B'] = sim_state['RP'] = bus
        elif ins_r == 0xC:
            sim_state['C'] = sim_state['PC'] = bus
        elif ins_r == 0xD:
            if sim_state['STAT'] & 0x1 != 0:
                sim_state['PC'] = bus
        elif ins_r == 0xE:
            if sim_state['STAT'] & 0x2 != 0:
                sim_state['PC'] = bus
        elif ins_r == 0xF:
            if sim_state['STAT'] & 0x4 != 0:
                sim_state['PC'] = bus
        else:
            print('unknown read nibble', ins_r, 'at', pc)

    return sim_state

def parse_logisim(raw):
    raw = '\n'.join(raw.replace('\r', '').split('\n')[1:]).replace('\n', ' ')
    bin_list = []
    for s in raw.split(' '):
        if s != '':
            if '*' in s:
                bin_list.extend(int(s.split('*')[0]) * [int(s.split('*')[1], 16)])
            else:
                bin_list.append(int(s, 16))
    return bin_list

def load_logisim(path):
    with open(path) as f:
        raw = f.read()
    return parse_logisim(raw)

asm_default_extras = {
    'NOP': 'A -> VOID',
    # register operations
    'STA': "0x11 -> RAM_P; A -> RAM",
    'RSA': "0x11 -> RAM_P; RAM -> A",
    'STB': "0x12 -> RAM_P; B -> RAM",
    'RSB': "0x12 -> RAM_P; RAM -> B",
    'STC': "0x13 -> RAM_P; C -> RAM",
    'RSC': "0x13 -> RAM_P; RAM -> C",
    'INCA': ['b', """1 -> B; ADD -> A"""],
    'DECA': ['b', """-1 -> B; ADD -> A"""],
    'INCB': ['a', """1 -> A; ADD -> B"""],
    'DECB': ['a', """-1 -> A; ADD -> B"""],
    'INCC': ['ab', """C -> A; 1 -> B; ADD -> C"""],
    'DECC': ['ab', """C -> A; -1 -> B; ADD -> C"""],
    # RAM operations
    'INC': ['ab', """RAM -> B; 1 -> A; ADD -> RAM"""],
    'DEC': ['ab', """RAM -> B; -1 -> A; ADD -> RAM"""],
    'IN': ['', """0x0100 -> RAM_P; RAM -> #"""],
    'OUT': ['', """0x0200 -> RAM_P; # -> RAM"""],
    'ARRINIT': ['', """1 -> A; # -> B RAM_P"""],
    'ARR': ['', """# -> RAM; ADD -> B RAM_P"""],
    # arithmetic operations
    'SUB': ['bc', """A -> C
B -> A
COM -> B
C -> A
ADD -> #"""],
    'MUL': "TODO",
    'DIV': "TODO",
    'MOD': "TODO",
    # TODO: 32bit arithmetic
    # binary operations
    'NOT': ['b', """A -> B
NOR -> #"""],
    'OR': ['ab', """NOR -> A B
NOR -> #"""],
    # TODO: any capital will have to be stored always, but never restored
    'AND': ['aBc', """A -> B
NOR -> C
*RSB
B -> A
NOR -> B
C -> A
NOR -> #"""],
    # TODO: don't restore read-from-bus-reg
    # TODO: check if first construct already contains neccessary keep instructions
    'XOR': ['abc', """*AND[keep=A B] -> C
NOR -> A
C -> B
NOR -> #
"""],
    'ASL': "TODO",
    'ROL': "TODO",
    'ASR': "TODO",
    'ROR': "TODO",
    # stack
    'PUSH': ['ab', """0xFF -> RAM_P
RAM -> RAM_P
# -> RAM
-1 -> A
RAM_P -> B
0xFF -> RAM_P
ADD -> RAM"""],
    'POP': ['', """0xFF -> RAM_P
*INC[keep=A B]
RAM -> RAM_P
RAM -> #"""],
    # jumps
    # TODO: adjust 10
    # TODO: insert RSA, RSB between two code strings
    # TODO: restore instructions for write-to-bus-reg if input flag # is in second code and write-to-bus-reg has been overwritten
    'JSR': ['AB', """25 -> A
PC -> B
ADD -> *PUSH""", """# -> PC"""],
    'RTS': ['', """*POP[keep=A B] -> PC"""],
}

print(asm_default_extras)

# JSR might look like
"""
01 00 00
62
05 00 ff
45
a4
01 ff ff
52
05 00 ff
a4
05 00 11
14
05 00 12
24
06 88 88
HERE
"""

# ; as \n
# label:
# "label" -> PC
# number parsing:
# decimal signed
# hexadecimal 0x octal 0o binary 0b -> count bits (may have zeropage addressing)

# bus writing stuff (A -> B)
# // comments
# literals (complicated int parsing)
# labels (pre-compile with placeholders, mark indices and corresponding label)
# constructs
#  check if input or output or none (warning logs)
#  store/restore based on rules (do we need to keep it? do we need to restore it? where?)

if f_path is None:
    state = None
    while True:
        try:
            prog_in = parse_logisim('v2.0 raw\n' + input('> '))
            if len(prog_in) > 0:
                state = sim(prog_in, f_debug, state)
                if f_debug:
                    debug_state = {**state}
                    debug_state.pop('RAM')
                    print(debug_state)
        except Exception as ex:
            print(ex)
else:
    sim(load_logisim(f_path), f_debug)
