tests = [
    'x + 3 * (*y) / 6',
]

tests_tok = []
for test_str in tests:
    tests_tok.append([c for c in test_str])
    tests_tok[-1] = list(filter(lambda c: not str.isspace(c), tests_tok[-1]))

def prec(tok):
    if tok == '*' or tok == '/':
        return 1
    elif tok == '+' or tok == '-':
        return 2
    else:
        return 0

def parse(tokens):
    expect_operand = True
    ops = []
    tok_i = 0
    while tok_i < len(tokens):
        tok = tokens[tok_i]
        if expect_operand:
            if tok == '(':
                br_level = 0
                end_i = len(tokens)
                for tok2_i, tok2 in enumerate(tokens[tok_i + 1:]):
                    if tok2 == '(':
                        br_level += 1
                    elif tok2 == ')':
                        if br_level == 0:
                            end_i = tok_i + 1 + tok2_i
                            break
                        br_level -= 1
                ops.append(parse(tokens[tok_i + 1:end_i]))
                tok_i = end_i
            else:
                ops.append(tok)
        else:
            ops.append(tok)
        expect_operand = not expect_operand
        tok_i += 1
    # TODO IN C: custom type for expression operand? optional prefix operators, possibly nested expression

    for p in range(1, 255):
        op_i = 1
        while op_i < len(ops):
            if prec(ops[op_i]) == p:
                new_exp = [ops[op_i - 1], ops[op_i], ops[op_i + 1]]
                ops[op_i - 1] = new_exp
                if op_i < len(ops) - 1:
                    del ops[op_i + 1]
                del ops[op_i]
            else:
                op_i += 1

    return ops

for t_i in range(len(tests)):
    print(tests[t_i], '  -->  ', parse(tests_tok[t_i]))
