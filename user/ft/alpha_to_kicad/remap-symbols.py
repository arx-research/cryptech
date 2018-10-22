#!/usr/bin/env python

"""
Remap symbols to suit KiCad nightly
"""

import os
import re
import sys
import pprint


def remap_symbols(fn_in, fn_out, refdes):
    in_ = open(fn_in)
    out = open(fn_out, 'w')
    fn = os.path.basename(fn_in)
    print('Remapping symbols in in {}'.format(fn))
    for line in in_.readlines():
        #print('R: {!r}'.format(line))
        if line.startswith('EESchema Schematic File Version 2'):
            line = 'EESchema Schematic File Version 4\n'
        elif line.startswith('EELAYER 27 0'):
            line = 'EELAYER 26 0\n'
        elif line.startswith('LIBS:'):
            continue
        elif line.startswith('L '):
            while line.endswith('\n'):
                line = line[:-1]
            _l, name, des = line.split(' ')
            if des.startswith('#'):
                # KiCad replaces designators like #PWR?58023E19 with VCC_5V0_6
                des = name
            if des in refdes:
                refdes[des] += 1
                des = '{}_{}'.format(des, refdes[des])
            else:
                refdes[des] = 1
            if name == 'GND':
                line = 'L power:GND {}\n'.format(des)
            else:
                line = 'L Cryptech_Alpha:{} {}\n'.format(name, des)
        out.write(line)
    return True


def main(schemas):
    refdes = {}
    for this in sorted(schemas):
        if remap_symbols(this, this + '.tmp', refdes):
            os.rename(this + '.tmp', this)

    return True


if __name__ == '__main__':
    try:
        if len(sys.argv) == 0:
            sys.stderr.write('Syntax: remap-symbols.py *.sch\n')
            sys.exit(1)
        schemas = [x for x in sys.argv if x.endswith('.sch')]
        res = main(schemas)
        if res:
            sys.exit(0)
        sys.exit(1)
    except KeyboardInterrupt:
        pass
