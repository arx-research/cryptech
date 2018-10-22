#!/usr/bin/env python

import os
import re
import sys
import pprint

import pcbnew


def extract_labels(fn_in, labels):
    in_ = open(fn_in)
    prev = None
    fn = os.path.basename(fn_in)
    for line in in_.readlines():
        #print('R: {!r}'.format(line))
        if prev is not None:
            text = line[:-1]
            if re.match('^[A-Z0-9_]+$', text):
                m = re.match('^Text Label (\d+) +(\d+) +(\d+) +(\d+) +~', prev)
                if m:
                    x, y, orient, dim = m.groups()
                    if text not in labels[fn]:
                        labels[fn][text] = []
                    labels[fn][text] += [{'t': 'GLabel',
                                          'dir': 'UnSpc',
                                          'x': x,
                                          'y': y,
                                          'ori': orient,
                                          'dim': dim}]
                    #print("Label: {!r}".format(text))
                else:
                    sys.stderr.write("Failed extracting data from previous line: {} - {}\n".format(text, prev))
                    return False
            prev = None
        elif line.startswith('Text Label '):
            prev = line

    return True


def main(schemas):
    labels = {}
    for this in schemas:
        labels[os.path.basename(this)] = {}
        if not extract_labels(this, labels):
            return False

    #print('{}'.format(pprint.pformat(labels)))

    print('labels = {')
    for this in sorted(schemas):
        fn = os.path.basename(this)
        if not len(labels[fn]):
            continue
        print('      {!r}: {{').format(fn)
        for text in sorted(labels[fn].keys()):
            sys.stdout.write('       {!r}: ['.format(text))
            first = True
            for v in labels[fn][text]:
                #sys.stdout.write(" x{}x ".format(first))
                if not first:
                    sys.stdout.write("\n{}".format(' ' * (len(text) + 16)))
                first = False
                if str(v['dim']) != '48':
                    sys.stdout.write("{{'t': {t!r}, 'dir': {dir!r}, 'x': {x!s}, 'y': {y!s}, 'ori': {ori!s}, 'dim': {dim!s}}},".format(**v))
                else:
                    sys.stdout.write("{{'t': {t!r}, 'dir': {dir!r}, 'x': {x!s}, 'y': {y!s}, 'ori': {ori!s}}},".format(**v))
            print('],')
        print('      },')
    print('    },')
    return True


if __name__ == '__main__':
    try:
        if len(sys.argv) == 0:
            sys.stderr.write('Syntax: fix-labels.py *.sch\n')
            sys.exit(1)
        schemas = [x for x in sys.argv if x.endswith('.sch') and not x.endswith('Cryptech Alpha.sch')]
        res = main(schemas)
        if res:
            sys.exit(0)
        sys.exit(1)
    except KeyboardInterrupt:
        pass
