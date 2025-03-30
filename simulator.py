import sys

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
                instruction['src_path'] = ''
                instruction['src_line'] = 0
            else:
                instruction['src'] = src_cache[src_path][line].strip()
                instruction['src_path'] = src_path
                instruction['src_line'] = line + 1
            program.append(instruction)
    return program

def hex_w(n: int, width=4, lower=False):
    s = hex(n)[2:].upper().zfill(width)
    return s.lower() if lower else s

asm_mapping = [
    ['',     'A', 'B', 'C', 'RAM', 'RAM_P', 'PC', 'STAT', '?', '?', 'ADD', 'COM', 'NOR', '?', '?', '?'],
    ['VOID', 'A', 'B', 'C', 'RAM', 'RAM_P', 'PC', 'STAT', '?', '?', 'A B', 'B RAM_P', 'C PC', 'PC_C', 'PC_Z', 'PC_N'],
]

def sim(program, debug=False, init_state=None, max_steps=10000):
    if init_state is None:
        sim_state = {
            'A': 0,
            'B': 0,
            'C': 0,
            'RAM': [0] * (2 ** 16),
            'RAM_P': 0,
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

        ins_w = (instruction['instruction'] >> 4) & 0xF
        ins_r = instruction['instruction'] & 0xF

        if debug:
            s_pc = f'({hex_w(instruction["PC"], lower=True)})'
            s_ins = hex_w(instruction['instruction'], 2, lower=True)
            if ins_w == 0x0:
                s_ins += f' {hex_w(instruction["literal"], lower=True)}'
            else:
                s_ins += ' ' * 5
            s_asm = instruction['src']
            if s_asm == '(generated code)':
                if ins_w == 0x0:
                    s_asm = f'0x{hex_w(instruction["literal"])}'
                else:
                    s_asm = asm_mapping[0][ins_w]
                s_asm += f' -> {asm_mapping[1][ins_r]};'
            # TODO: maybe show both generated and found assembly in case they differ
            s_src = f'{instruction["src_path"]}:{instruction["src_line"]}'
            s_spaces = ' ' * (35 - len(s_asm))

        debug_extras = [''] * 6

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
            bus = sim_state['RAM'][sim_state['RAM_P']]
        elif ins_w == 0x5:
            bus = sim_state['RAM_P']
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
            sim_state['RAM'][sim_state['RAM_P']] = bus
            if sim_state['RAM_P'] == 0x200:
                if debug:
                    debug_extras[1] = f'[GPOA]   {hex_w(bus)} ({chr(bus)})'
                else:
                    print(chr(bus), end='')
            elif debug:
                debug_extras[2] = f'[RAM]    d {hex_w(bus)} -> a {hex_w(sim_state["RAM_P"])}'
        elif ins_r == 0x5:
            sim_state['RAM_P'] = bus
        elif ins_r == 0x6:
            sim_state['PC'] = bus
        elif ins_r == 0x7:
            sim_state['STAT'] = bus & 0xFF
        elif ins_r == 0xA:
            sim_state['A'] = sim_state['B'] = bus
        elif ins_r == 0xB:
            sim_state['B'] = sim_state['RAM_P'] = bus
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

        if sim_state['RAM'][0x00FF] != 0 and sim_state['RAM'][0x00FF] < 0xFFFF:
            stack = sim_state['RAM'][sim_state['RAM'][0x00FF] + 1:]
            s_stack = ' '.join(reversed([hex_w(s) for s in stack]))
            s_overflow = ''
            if sim_state['RAM'][0x00FF] < 0xfc00:
                s_overflow = 'OVERFLOW '
            debug_extras[3] = f'[STACK]  {s_overflow}{s_stack}'

        heap_blocks = []
        heap_address = 0x0040
        while heap_address != 0:
            heap_blocks.append({
                'address': heap_address,
                'previous': sim_state['RAM'][heap_address - 4],
                'next': sim_state['RAM'][heap_address - 3],
                'capacity': sim_state['RAM'][heap_address - 2],
                'size': sim_state['RAM'][heap_address - 1],
            })
            heap_address = heap_blocks[-1]['next']
        if heap_blocks[0]['capacity'] > 0:
            s_blocks = ' '.join([f'{hex_w(s["address"])} ({s["size"]}/{s["capacity"]})' for s in heap_blocks])
            debug_extras[4] = f'[HEAP]   {s_blocks}'

        if debug:
            # TODO: check if instruction in def or label before, show def name or label
            print(f'{s_pc}  {s_ins}       {s_asm}{s_spaces}| {s_src}')
            print()
            print('[A___] [B___] [C___] [RAM_P] [RAM_] [STAT]')
            print(f' {hex_w(sim_state["A"])}   {hex_w(sim_state["B"])}   {hex_w(sim_state["C"])}   {hex_w(sim_state["RAM_P"])}    {hex_w(sim_state["RAM"][sim_state["RAM_P"]])}   {hex_w(sim_state["STAT"], 2)}')
            print('\n'.join(debug_extras))

        if '"end"  -> PC;' in instruction['src']:
            print('Program ended')
            break
        
        if debug:
            try:
                input()
            except KeyboardInterrupt:
                break

    return sim_state

if arg_path is None:
    print('Please specify path to assembler output file (.txt)')
else:
    program = parse_txt(arg_path)
    sim(program, True)
