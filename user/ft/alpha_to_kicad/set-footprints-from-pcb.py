#!/usr/bin/env python

"""
Set all schematic footprints from the PCB file, where the components footprints
are known. The footprints were exported once from the PCB using the menu option

  File -> Archive Footprints -> Create New Library and Archive Footprints

and the fp-lib-table was set up to map the name 'Cryptech_Alpha_Footprints' to
the archive directory (rev03-KiCad/footprints.pretty).
"""

import os
import sys
import pprint

import pcbnew

def add_footprints(fn_in, fn_out, comp):
    in_ = open(fn_in)
    out = open(fn_out, 'w')
    curr = None
    c = 0
    f0 = ''
    print('Adding footprints to {}'.format(fn_in))
    for line in in_.readlines():
        c += 1
        if line.startswith('L '):
            curr = line.split(' ')[-1]
            while curr[-1] == '\n':
                curr = curr[:-1]
            # Special case handling of chips divided into parts
            if '_' in curr and curr not in comp:
                curr = curr.split('_')[0]
        if line.startswith('F 0 '):
            f0 = line
        if line.startswith('F 2 ""'):
            if curr in comp:
                fp = comp[curr]
                line = 'F 2 "Cryptech_Alpha_Footprints:{}{}'.format(fp, line[5:])
                if line.endswith('0000 C CNN\n'):
                    # don't show the footprint in the schematics
                    line = line[:-8] + '1' + line[-7:]
                #print('{}: line {} {} fp {}'.format(fn_in, c, curr, fp))
            else:
                if not curr.startswith('#PWR'):
                    sys.stderr.write('{}: line {} footprint for {} not known\n'.format(fn_in, c, curr))
            curr = None
            f0 = ''
        if line.startswith('$EndComp') and curr is not None:
            #sys.stderr.write('{}: line {} footprint for {} not written\n'.format(fn_in, c, curr))
            if curr in comp:
                # Component without F 2 line. Construct one roughly from F 0.
                f = f0.split(' ')
                coord = f[4] + ' ' + f[5]
                f2 = 'F 2 "Cryptech_Alpha_Footprints:{}" H {} 60  0001 C CNN\n'.format(comp[curr], coord)
                out.write(f2)
            else:
                if not curr.startswith('#PWR'):
                    sys.stderr.write('{}: line {} footprint for {} not known\n'.format(fn_in, c, curr))
        out.write(line)
    return True

def main(pcb, schemas):
    print("Loading PCB '{}'".format(pcb))
    board = pcbnew.LoadBoard(pcb)
    comp = {}
    for mod in board.GetModules():
        ref = mod.GetReference()
        fp = mod.GetFPID().GetLibItemName()
        if ref in comp:
            sys.stderr.write('{} already in comp ({} -> {})!\n'.format(ref, comp[ref], fp))
        comp[ref] = str(fp)

    #print(sorted(comp.keys()))
    #print(pprint.pformat(comp))

    for this in schemas:
        if add_footprints(this, this + '.tmp', comp):
            os.rename(this + '.tmp', this)

    return True


if __name__ == '__main__':
    try:
        if len(sys.argv) == 0:
            sys.stderr.write('Syntax: set-footprints-from-pcb.py *.kicad_pcb *.sch\n')
            sys.exit(1)
        pcb = [x for x in sys.argv if x.endswith('.kicad_pcb')][0]
        schemas = [x for x in sys.argv if x.endswith('.sch')]
        res = main(pcb, schemas)
        if res:
            sys.exit(0)
        sys.exit(1)
    except KeyboardInterrupt:
        pass
