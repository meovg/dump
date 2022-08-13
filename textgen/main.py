import argparse
import bisect
import random
from collections import defaultdict
from queue import Queue

def tokenize(filename):
    with open(filename, 'r') as f:
        word_seq = " ".join(f)
        return word_seq.split()

def get_markov_chain(words, state_size):
    word_count = len(words)

    if word_count < state_size:
        raise Exception('State size is larger than sample text size')

    chain = {}
    state_buf = Queue()

    for i, word in enumerate(words):
        if i < state_size:
            state_buf.put(word)
            if i == state_size - 1:
                current_state = ' '.join(list(state_buf.queue)) 
        else:
            state_buf.get()
            state_buf.put(word)
            next_state = ' '.join(list(state_buf.queue))

            if current_state not in chain:
                chain[current_state] = {}
            if next_state not in chain[current_state]:
                chain[current_state][next_state] = 0

            chain[current_state][next_state] += 1
            current_state = next_state
    return chain

def gen_next_state(chain, current):
    choices = list(chain[current].keys())
    choices_occ = list(chain[current].values())
    total_occ = sum(choices_occ)
    choices_cumdist = []

    current_cumdist = 0
    for occ in choices_occ: 
        dist = float(occ) / total_occ
        current_cumdist += dist
        choices_cumdist.append(current_cumdist)

    r = random.random() * current_cumdist
    index = bisect.bisect(choices_cumdist, r)
    return choices[index]

def gen_rand_state(chain):
    uppercase_states = [x for x in chain.keys() if x[0].isupper()]
    if len(uppercase_states):
        return random.choice(uppercase_states)
    else:
        return random.choice(list(markov_chain.keys()))

def gen_text(chain, wordcount):
    current = gen_rand_state(chain)
    res = current.split()[:wordcount]

    while len(res) < wordcount:
        current = gen_next_state(chain, current)
        res.append(current.split()[-1])
    return ' '.join(res)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Markov chain string generator')
    parser.add_argument('-f', '--filename',
                        required=True,
                        help='Input file to read the text')
    parser.add_argument('-s', '--state_size',
                        type=int, default=2,
                        help='The state size')
    parser.add_argument('-n', '--wordcount',
                        type=int, default=100,
                        help='Number of words in output')

    args = parser.parse_args()
    tokens = tokenize(args.filename)
    chain = get_markov_chain(tokens, args.state_size)
    print(gen_text(chain, args.wordcount))
