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

fallback_contructs = {
    "*STA": "0x11 -> RAM_P; A -> RAM",
    "*RSA": "0x11 -> RAM_P; RAM -> A",
    "*STB": "0x12 -> RAM_P; B -> RAM",
    "*RSA": "0x12 -> RAM_P; RAM -> B",
    "*STC": "0x13 -> RAM_P; C -> RAM",
    "*RSA": "0x13 -> RAM_P; RAM -> C",
}

# for store/restore calls, try finding def of STA,RSA,... after all includes else search in fallback (above) else panic

# TODO: never restore read-from-bus-reg (param)
# TODO: restore instructions for write-to-bus-reg if input flag * is after !RS and write-to-bus-reg in temp (write-to-bus-reg for ADD/NOR is "A B" and COM "A")
# TODO: insert RSA, RSB at !RS

# i think this is irrelevant:
    # TODO: check if parent construct already contains neccessary keep instructions (could be done by optimization)
    # TODO: PUSH calls DEC (temp=A B), check for this

"""
def build_construct (code index or virtual (nested) raw construct call -> syntax (code/construct definition) to instances)
var reg_invalidated (can be set by self or nested construct [correction: i think only self])
var reg_restore (can be set by call, definition, nested definition?)
-> construct_context
-> sum length of all instances (instruction/construct calls) in construct definition (you could just append to assembly output and count instructions)
-> resolve construct calls during preprocessing (output assembly)
-> construct definition file (included like header file)
"""

# ; as \n
# "label":
# "label" -> PC
# "label_[x]":
# "label_[x+1]"
# number parsing:
# decimal signed
# hexadecimal 0x octal 0o binary 0b -> count bits (may have zeropage addressing)

# bus writing stuff (A -> B)
# # comments
# * params
# literals (complicated int parsing)
# labels (pre-compile with placeholders, mark indices and corresponding label)
# constructs
#  check if input or output or none (warning logs)
#  store/restore based on rules (do we need to keep it? do we need to restore it? where?)
# optimize .ha before export by simulating runtime register values (constant/register -> known, ADD/COM/NOR -> unknown) and removing second write, disable by option -> WATCH OUT FOR JUMPS (simply reset to unknown state at labels?)
# disassembler for analysing results

# .ha syntax highlighting (auto-complete would be very nice)

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
