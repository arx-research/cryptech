#!/usr/bin/python

import sys
import pcbnew


def remove_tracks(board, layer):
    """
    The tracks on Layer four are actually where there *should not* be copper.
    L4 is an inverted layer, but that information actually isn't in the files -
    it is specified in an excel sheet sent to the PCB house.

    By removing them and then setting some parameters carefully on the zones
    on that layer, we get a close-to-perfect (but no longer inverted) result
    in KiCAD.
    """
    for this in [x for x in board.GetTracks() if x.GetLayerName() == layer]:
        print('Removing track {} on {}'.format(this, layer))
        board.Delete(this)

#def set_tracks_width(board, layer, width):
#    for this in [x for x in board.GetTracks() if x.GetLayerName() == layer]:
#        this.SetWidth(width)
#
#def move_tracks(board, from_layer, to_layer):
#    for this in [x for x in board.GetTracks() if x.GetLayerName() == from_layer]:
#        this.SetLayer(board.GetLayerID(to_layer))


def hide_layers_except(board, layers):
    """
    As a convenience when working on getting a particular layer right, hide other layers.
    """
    lset = board.GetVisibleLayers()
    print('Layer set: {} / {}'.format(lset, lset.FmtHex()))
    #help(lset)
    mask = 0
    for x in lset.Seq():
        name = board.GetLayerName(x)
        print('  {} {}'.format(x, name))
        if name in layers:
            mask |= 1 << x
    hexset = '{0:013x}'.format(mask)
    visible_set = pcbnew.LSET()
    visible_set.ParseHex(hexset, len(hexset))
    board.SetVisibleLayers(visible_set)


def layer_zone_fixes(board, layer, clearance=0.125, min_width=0.05, thermal=0.5,
                     gnd_clearance=0.25, gnd_min_width=0.05, gnd_thermal=0.5, gnd_priority=50):
    for i in range(board.GetAreaCount()):
        area = board.GetArea(i)
        if area.GetLayerName() != layer:
            continue
        print('Area {} {}'.format(area, area.GetNetname()))
        if area.GetNetname() == 'GND':
            area.SetZoneClearance(pcbnew.FromMM(gnd_clearance))
            area.SetMinThickness(pcbnew.FromMM(gnd_min_width))
            # 0.25 clearance on the 'background' GND zone keeps the distance to the island
            # zones in the bottom half of layer 4, matching the clearance between the areas
            # but creating a bit more clearance around vias in KiCAD plot than in Altium
            area.SetThermalReliefCopperBridge(pcbnew.FromMM(gnd_thermal))
            area.SetPriority(gnd_priority)
        else:
            # 0.25 works better for the distance between zones in the bottom half of layer 4,
            # but does not allow copper between the vias under the FPGA
            #
            # Values below 0.15 or somewhere there does not seem to make a difference at all
            area.SetZoneClearance(pcbnew.FromMM(clearance))
            # This makes sharp edges matching Altium Designer
            # 0.0255 is the minimum KiCad wants in order to allow changes to the zone inside KiCad
            area.SetMinThickness(pcbnew.FromMM(min_width))
            area.SetThermalReliefCopperBridge(pcbnew.FromMM(thermal))


def change_via_drill_size(board, from_size, to_drill):
    """
    The Cryptech board has 296 vias that should be drilled 0.5 mm.

    Oddly enough, these have a 'width' of 1000000 but need to have the drill size set
    to not inherit the default for the net class which will be 0.25 mm.
    """
    from_size = pcbnew.FromMM(from_size)
    for this in board.GetTracks():
        if type(this) is pcbnew.VIA:
            size = this.GetWidth()
            if size == from_size:
                this.SetDrill(pcbnew.FromMM(to_drill))

def change_via_width(board, from_, to_):
    """
    """
    changed = 0
    from_ = pcbnew.FromMM(from_)
    for this in board.GetTracks():
        if type(this) is pcbnew.VIA:
            size = this.GetWidth()
            if size == from_:
                this.SetWidth(pcbnew.FromMM(to_))
                changed += 1
    print("Changed {} via's from width {} to {}".format(changed, from_, to_))


def show_via_widths(board):
    sizes = {}
    for this in board.GetTracks():
        if type(this) is pcbnew.VIA:
            size = this.GetWidth()
            sizes[size] = sizes.get(size, 0) + 1
    print("Via 'widths': {}".format(sizes))


def change_netclass_drill_size(board, from_, to_):
    names = board.GetAllNetClasses()
    for name, net in names.iterator():
        if net.GetViaDrill() == pcbnew.FromMM(from_):
            print("Netclass {} has drill size {}, changing to {}".format(name, net.GetViaDrill(), pcbnew.FromMM(to_)))
            net.SetViaDrill(pcbnew.FromMM(to_))


def change_netclass_clearance(board, to_):
    names = board.GetAllNetClasses()
    for name, net in names.iterator():
        old = net.GetClearance()
        print("Netclass {} has clearance {}, changing to {}".format(name, pcbnew.ToMM(old), pcbnew.FromMM(to_)))
        net.SetClearance(pcbnew.FromMM(to_))


#
# One function per layer
#
def fix_layer_F_aka_GTL(board):
    """
    There are four segments of Net1 to the far left, just below the four vias for
    VCCO_3V3. Altium covers this up in the GND polygon, much the same way as on the
    bottom layer. Wonder if Net1 is some kind of alias for GND in Altium?

    Anyway, remove the four segments.

    Compare with

      $ gerbv 'rev03-KiCad/GerberOutput/Cryptech Alpha-F.Cu.gbr' hardware/production_files/alpha/rev03/Gerbers/CrypTech.GTL
    """
    for this in [x for x in board.GetTracks() if x.GetLayerName() == 'F.Cu']:
        pos = tuple(this.GetStart())
        if this.GetNetname() == 'Net1' and pos[0] > 6000000 and pos[0] < 12000000:
            print('Removing Net1 segment on top layer: {}'.format(pos))
            board.Delete(this)
    layer_zone_fixes(board, 'F.Cu', gnd_clearance=0.15)


def fix_layer_In1_aka_GP1(board):
    """
    Layer 1 has a GND polygon that needs a little less clearance in order to fill in between
    the vias of the FPGA.

    Compare with

      $ gerbv 'rev03-KiCad/GerberOutput/Cryptech Alpha-In1.Cu.gbr' hardware/production_files/alpha/rev03/Gerbers/CrypTech.GP1
    """
    layer_zone_fixes(board, 'In1.Cu', gnd_clearance=0.125)


def fix_layer_In2_aka_G1(board):
    """
    Layer 2 has a GND fill with unusual fill properties. We could easilly change parameters
    on this zone to reach many more places on the board. There are even a few isolated islands
    that must be connected to GND on other layers, that would easilly get connected with the
    normal zone parameters.

    The current result is missing the connection of the three GND pins on SV1 (FPGA JTAG), but
    these three pins are connected on layer In5 aka G2.

    Compare with

      $ gerbv 'rev03-KiCad/GerberOutput/Cryptech Alpha-In2.Cu.gbr' hardware/production_files/alpha/rev03/Gerbers/CrypTech.G1
    """
    layer_zone_fixes(board, 'In2.Cu', gnd_clearance=0.5, gnd_min_width=0.7, gnd_thermal=0.71)


def fix_layer_In3_aka_GP2(board):
    """
    Compare with

      $ gerbv 'rev03-KiCad/GerberOutput/Cryptech Alpha-In3.Cu.gbr' hardware/production_files/alpha/rev03/Gerbers/CrypTech.GP2
    """
    layer_zone_fixes(board, 'In3.Cu')

def fix_layer_In4_aka_GP3(board):
    """
    Layer 4 is a layer with large polygons (GND and Power). In Altium, this was made
    like an inverted layer with drawn lines separating polygons. Issues were with
    line thickness in Kicad, clearance parameters preventing copper to flow in between
    vias under the FPGA and the fact that KiCAD doesn't do inverted layers so the
    lines had to be removed and zone clearances adjusted to create the same result.

    Compare with

      $ gerbv 'rev03-KiCad/GerberOutput/Cryptech Alpha-In4.Cu.gbr' hardware/production_files/alpha/rev03/Gerbers/CrypTech.GP3
    """
    remove_tracks(board, 'In4.Cu')
    # GND clearance should be 0.125 to minimise diff around thoughholes in the GND polygon
    # (best seen around the SDRAM vias on the right hand side), but that also makes the
    # line separating the polygons thinner, so I'll keep it this way. Maybe these polygons
    # should get a manual touch-up after conversion anyways - the isolated GND polygon in
    # the middle has a bit of an odd shape.
    layer_zone_fixes(board, 'In4.Cu', gnd_clearance=0.25)


def fix_layer_In5_aka_G2(board):
    """
    Compare with

      $ gerbv 'rev03-KiCad/GerberOutput/Cryptech Alpha-In5.Cu.gbr' hardware/production_files/alpha/rev03/Gerbers/CrypTech.G2
    """
    layer_zone_fixes(board, 'In5.Cu', gnd_clearance=0.5, gnd_min_width=0.7, gnd_thermal=0.71)


def fix_layer_In6_aka_GP4(board):
    """
    Layer 6 has a GND polygon that needs a little less clearance in order to fill in between
    the vias of the FPGA.

    Compare with

      $ gerbv 'rev03-KiCad/GerberOutput/Cryptech Alpha-In6.Cu.gbr' hardware/production_files/alpha/rev03/Gerbers/CrypTech.GP4
    """
    layer_zone_fixes(board, 'In6.Cu', gnd_clearance=0.15)


def fix_layer_B_aka_GBL(board):
    """
    There is one small segment that ends up on Net 1 instead of GND (Net 7) for some reason:

        #Tracks#3898: 200C000500FFFFFFFFFFFFFFFF41A799058A60450341A7990561AE4803E50106000000000000000000FFFF0001
        -  (segment (start 38.75 -74.875) (end 38.75 -75.425) (width 1) (layer B.Cu) (net 1))
        +  (segment (start 38.75 -74.875) (end 38.75 -75.425) (width 1) (layer B.Cu) (net 7))

    I'm guessing the surrounding GND polygon somehow has priority in Altium so this small
    bug is not visible there. This is a segment connected to one of the seven stiching vias
    for the GND polygon under and to the right of U14 (bottom left one of the four grouped together).

    The OSHW logo is lost, but I think we can live with that.

    The copper print saying PCB rev.03 is messed up, but I don't think we can expect the fonts
    to be similar in Altium and KiCAD anyways, so might as well just redo that.

    Compare with

      $ gerbv 'rev03-KiCad/GerberOutput/Cryptech Alpha-B.Cu.gbr' hardware/production_files/alpha/rev03/Gerbers/CrypTech.GBL
    """
    for this in [x for x in board.GetTracks() if x.GetLayerName() == 'B.Cu']:
        if tuple(this.GetStart()) == (38750000, -74875000):
            print('Moving track to net GND: {}'.format(this))
            this.SetNet(board.FindNet('GND'))

    layer_zone_fixes(board, 'B.Cu', gnd_clearance=0.15)


def fix_layer_Dwgs_User(board):
    """
    The Dwgs.User layer has three extra pin-1 marking for the ARM and the two SDRAM chips,
    as well as chip outlines for the same chips as 'segments'. KiCad gets confused by this
    and doesn't render the segments (probably since they are copper on a non-copper layer).

    They cause ERC warnings so we remove them.
    """
    for this in [x for x in board.GetDrawings() if x.GetLayerName() == 'Dwgs.User']:
        print("Removing drawing on Dwgs.User: {}".format(this))
        board.Delete(this)
    remove_tracks(board, 'Dwgs.User')


def main(in_fn='rev03-KiCad/convert.kicad_pcb', out_fn='rev03-KiCad/Cryptech Alpha.kicad_pcb'):
    board = pcbnew.LoadBoard(in_fn)
    # normalize contents to be able to use diff to show changes made
    pcbnew.SaveBoard(in_fn + '.before-fix-pcb', board)

    # Copper layers
    fix_layer_F_aka_GTL(board)
    fix_layer_In1_aka_GP1(board)
    fix_layer_In2_aka_G1(board)
    fix_layer_In3_aka_GP2(board)
    fix_layer_In4_aka_GP3(board)
    fix_layer_In5_aka_G2(board)
    fix_layer_In6_aka_GP4(board)
    fix_layer_B_aka_GBL(board)
    # Non-copper layers
    fix_layer_Dwgs_User(board)

    change_via_drill_size(board, 1.0, 0.5)
    # Changing these via widths minimizes diff on layer In1/GP1, but creates diff on
    # the top layer. Altium seems to have different internal layer clearings than KiCAD.
    #change_via_width(board, 0.5, 0.45)
    #change_via_width(board, 1.0, 0.7)
    #change_via_width(board, 1.5, 1.2)
    show_via_widths(board)
    change_netclass_drill_size(board, 0.635, 0.250)
    change_netclass_clearance(board, 0.125)

    # Only show a single layer while working on this
    #hide_layers_except(board, ['In5.Cu'])
    # Only show Through Via while working on this
    #board.SetVisibleElements(0x7FFC0009)

    # Make a better first impression =)
    hide_layers_except(board, ['F.Cu'])
    board.SetVisibleElements(0x7FFED33F)

    pcbnew.SaveBoard(out_fn, board)
    return True

if __name__ == '__main__':
    try:
        if len(sys.argv) != 3:
            sys.stderr.write('Syntax: fix-pcb.py infile.kicad_pcb outfile.kicad_pcb\n')
            sys.exit(1)
        res = main(sys.argv[1], sys.argv[2])
        if res:
            sys.exit(0)
        sys.exit(1)
    except KeyboardInterrupt:
        pass
