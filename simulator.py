import sys

# TODO:
#  - heap block info

arg_debug = False
arg_help = False
arg_path = None

i = 1
while i < len(sys.argv):
    if sys.argv[i] in ['-d', '--debug']:
        arg_debug = True
    elif sys.argv[i] in ['-h', '--help']:
        arg_help = True
    else:
        arg_path = sys.argv[i]
    i += 1

if arg_help:
    print("""HERA simulator

Arguments:
-v / --verbose [0...4 optional]     |  Specify amount of output (default: 1)
-h / --help                         |  This screen

Specify path to output file (.txt) of HERA assembler""")
    exit()

def parse_txt(path: str):
    if path.endswith('.ha') or path.endswith('.bin') or path.endswith('.img'):
        path = '.'.join(path.split('.')[:-1])
    if '.' not in path.split('/')[-1]:
        path += '.txt'
    with open(path) as f:
        raw = f.read()
    src_cache = {}
    program = []
    current_label = ''
    for l in raw.split('\n'):
        if l.startswith('# "'):
            current_label = list(filter(None, l.split('"')))[1]
        elif l.startswith('('):
            pc_ins_lit = list(filter(None, list(filter(None, l.split('#')))[0].split(' ')))
            instruction = {
                'PC': int(pc_ins_lit[0][1:-1], 16),
                'instruction': int(pc_ins_lit[1], 16),
            }
            if len(pc_ins_lit) > 2:
                instruction['literal'] = int(pc_ins_lit[2], 16)
            if current_label:
                instruction['label'] = current_label
                current_label = ''
            comment = list(filter(None, list(filter(None, l.split('#')))[1].split(' ')))
            src_path = '.'.join(path.split('.')[:-1]) + '.ha'
            if len(comment) > 2:
                src_path = comment[0]
            if src_path not in src_cache:
                with open(src_path) as f:
                    src_cache[src_path] = f.read()
                src_cache[src_path] = src_cache[src_path].split('\n')
            line = int(comment[-1]) - 1
            if line == 0:
                instruction['src'] = '(generated code)'
            else:
                instruction['src'] = src_cache[src_path][line].strip()
            program.append(instruction)
    return program

def hex_w(n, width=2):
    return hex(n)[2:].upper().zfill(width)

def sim(program, debug=False, init_state=None, max_steps=10000):
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

    for i in range(max_steps):
        instruction = list(filter(lambda ins: ins['PC'] == sim_state['PC'], program))
        if len(instruction) == 0:
            return sim_state
        instruction = instruction[0]

        ins_w = instruction['instruction'] >> 4
        ins_r = instruction['instruction'] & 0xF
        sim_state['PC'] += 1
        if ins_w == 0x0:
            bus = instruction['literal']
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
            print('unknown write nibble', ins_w, 'at', instruction['PC'])

        bus &= 0xFFFF

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
                    print('\t\t  ', '[OUTPUT] ', hex_w(bus, 2), '(' + chr(bus) + ')')
                else:
                    print(chr(bus), end='')
            elif debug:
                print('\t\t  ', '[RAM] ', 'd', hex_w(bus, 4), '->', 'a', hex_w(sim_state['RP'], 4))
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
            print('unknown read nibble', ins_r, 'at', instruction['PC'])
        
        if debug:
            print('(' + hex_w(instruction['PC'], 4) + ')', hex_w(instruction['instruction'], 2), '\t', instruction['src'])

        if '"end"  -> PC;' in instruction['src']:
            break

    return sim_state

if arg_path is None:
    print('Please specify path to assembler output file (.txt)')
else:
    program = parse_txt(arg_path)
    sim(program, True)
