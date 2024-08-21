# markov algorithm interpreter
# this program takes in path of file containing rules (and other options, run with --help for details)
# and execute markov algorithm on input data from standard input
# details on markov algorithm: https://en.wikipedia.org/wiki/Markov_algorithm
# for syntax: https://mao.snuke.org/#about

import argparse
import itertools
import sys
import time

def parse(code):
    rules = []
    for line in code.splitlines():
        if ':' not in line:
            continue
        tokens = line.split(':')
        if len(tokens) == 2:
            a, b = tokens
            rules.append((a.strip(), b.strip(), False))
        elif len(tokens) == 3 and not tokens[1]:
            a, _, b = tokens
            rules.append((a.strip(), b.strip(), True))
        else:
            print('Failed to parse op:', line, file=sys.stderr)
            sys.exit(1)
    return rules


def apply(rules, data):
    for a, b, halt in rules:
        if a in data:
            data = data.replace(a, b, 1)
            return data, halt
    return data, True


def interpret(code, data, *, limit=9999, wait=0.0):
    rules = parse(code)
    print('Rules:')
    for i, rule in enumerate(rules):
        a, b, halt = rule
        print(f'[{i}] \'{a}\' => \'{b}\'' + (' (terminating)' if halt else ''))

    print('\nSteps:')
    print(f'[0] {data}', file=sys.stderr)
    for step in itertools.count(1):
        if limit is not None and step > limit:
            break
        data, terminate = apply(rules, data)
        print(f'[{step}] {data}', file=sys.stderr)
        if terminate:
            return data, step
        time.sleep(wait)
    return None


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Markov algorithm interpreter')
    parser.add_argument('file', help='Text file containing rules')
    parser.add_argument('-l', '--limit', type=int, default=None, help='Maximum number of steps')
    parser.add_argument('-w', '--wait', type=float, default=0.05, help='Waiting time (in seconds)')
    args = parser.parse_args()

    with open(args.file) as f:
        code = f.read()
    res = interpret(code, input(), limit=args.limit, wait=args.wait)
    if res is None:
        print('Limit reached')
    else:
        print(f'Output: {res[0]}, No. of steps: {res[1]}')