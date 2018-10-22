#!/bin/bash
#
# This script runs the conversion from Altium to KiCad. It expects the
# Altium project in ../../../hardware/cad/rev03/ and the altium2kicad
# source in ../altium2kicad
#

set -e

# this facilitates reproducible conversions, making all timestamps the same for consecutive runs
export A2K_STARTTIME="1476542686"

altiumdir="rev03-Altium"
kicaddir="rev03-KiCad"

#test -d altium2kicad || git clone https://github.com/thesourcerer8/altium2kicad
# We currently need some Cryptech hacks to this script, so get it from fredrikt's fork instead.
test -d altium2kicad || git clone -b ft-2017-09-cryptech_mods https://github.com/fredrikt/altium2kicad

rm -rf ${altiumdir}
test -d hardware || git clone https://git.cryptech.is/hardware.git
cp -rp hardware/cad/rev03 ${altiumdir}

cd ${altiumdir}

# make sheet numbers in filenames two digits to have them sort properly
rename 's/rev02_/rev02_0/' rev02_?.*

../altium2kicad/unpack.pl
# Create WRL files using FreeCAD, unless wrl-files.tar.gz already exists
# (and I've checked that file into the repository so that you do not have
# to install FreeCAD).
# Something is not 100% working in the step2wrl.FCMacro file, so you have
# to close all the FreeCAD windows that are opened after they finish executing
# the macro.
test -f ../wrl-files.tar.gz || ../make-wrl-files.sh
tar zxf ../wrl-files.tar.gz

time ../altium2kicad/convertschema.pl
time ../altium2kicad/convertpcb.pl

cd ..

rm -rf "${kicaddir}"
mkdir "${kicaddir}"
git checkout "${kicaddir}"/{GerberOutput,footprints.pretty,fp-lib-table}
cp ${altiumdir}/*.{sch,lib} "${kicaddir}"/
rm ${kicaddir}/rev02*-cache.lib
cp ${altiumdir}/CrypTech-PcbDoc.kicad_pcb "${kicaddir}/Cryptech Alpha.kicad_pcb"
cp -rp ${altiumdir}/wrlshp ${kicaddir}/wrlshp
# Install prepared KiCAD project file and top-level schematic sheet.
cp "Cryptech Alpha.pro.template" "${kicaddir}/Cryptech Alpha.pro"
cp "Cryptech Alpha.sch.template" "${kicaddir}/Cryptech Alpha.sch"

# Fix wrl paths
wrlpath=$(readlink -f ${altiumdir}/wrlshp)
echo "Changing WRL path ${wrlpath} to relative path wrlshp/"
sed -i -e "s!${wrlpath}!wrlshp!g" ${kicaddir}/rev02_* ${kicaddir}/*.kicad_pcb

# There are more WRL files in this directory
cp -rp ${altiumdir}/wrl/* ${kicaddir}/wrlshp/
wrlpath=$(readlink -f ${altiumdir}/wrl)
echo "Changing WRL path ${wrlpath} to relative path wrlshp/"
sed -i -e "s!${wrlpath}!wrlshp!g" ${kicaddir}/rev02_* ${kicaddir}/*.kicad_pcb

cd ${kicaddir}

# Change to more sensible filenames
rename 's/-SchDoc//' rev02_*
sed -i -e 's/-SchDoc//g' *.{sch,lib}

# Change some PCB parameters. Haven't figured out how to set global defauls in fix-pcb.py yet.
sed -i -e 's/trace_min 0.254/trace_min 0.15/g' "Cryptech Alpha.kicad_pcb"
sed -i -e 's/[(]via_min_drill 0.508/(via_min_drill 0.25/g' "Cryptech Alpha.kicad_pcb"
sed -i -e 's/[(]via_min_size 0.889/(via_min_size 0.5/g' "Cryptech Alpha.kicad_pcb"
# show ratsnest
sed -i -e 's/visible_elements 7FFFF77F/visible_elements 7FFFFF7F/g' "Cryptech Alpha.kicad_pcb"
# Power layers
for l in 1 3 4 6; do
    sed -i -e "s/${l} In${l}.Cu signal/${l} In${l}.Cu power/g" "Cryptech Alpha.kicad_pcb"
done
# Mixed layers
for l in 2 5; do
    sed -i -e "s/${l} In${l}.Cu signal/${l} In${l}.Cu mixed/g" "Cryptech Alpha.kicad_pcb"
done

# Sheet number fixups. This hides all the hierarchical sub-sheets from the project view.
num_sheets=$(ls Cryptech\ Alpha.sch rev02*sch | wc -l)
num=1
ls Cryptech\ Alpha.sch rev02*sch | while read file; do
    sed -i -e "s/^Sheet .*/Sheet ${num} ${num_sheets}/g" "${file}"
    num=$[$num + 1]
done

# Replace slashes in component names, seems to not work in KiCAD nightly
ls Cryptech*Alpha.lib rev02*sch | while read file; do
    sed -i -e "s#I/SN#I_SN#g" "${file}"
done

# KiCad nightly has changed how symbols are located
../remap-symbols.py rev02*sch
cp ../sym-lib-table.template sym-lib-table

# Turn some labels into global labels. All labels seem to be global in Altium?
../fix-labels.py rev02*sch
# Add NotConnected and some other symbols
../add-components.py rev02*sch
rm -f "Cryptech Alpha-cache.lib"

# Conversion seems to make all power pins power-input, change some to power-output
# LT3060ITS8-15
sed -i -e 's/^X OUT 6 600 300 200 L 70 70 0 1 W$/X OUT 6 600 300 200 L 70 70 0 1 w/' Cryptech_Alpha.lib
# Voltage regulator outputs
#sed -i -e 's/^X VOUT \(.*\) W$/X VOUT \1 w/g' Cryptech_Alpha.lib
# Power jack
sed -i \
    -e 's/^X PWR 1 100 300 100 L 1 1 0 1 P$/X PWR 1 100 300 100 L 1 1 0 1 W/' \
    -e 's/^X GND 2 100 100 100 L 1 1 0 1 P$/X GND 2 100 100 100 L 1 1 0 1 W/' \
    -e 's/^X GNDBREAK 3 100 200 100 L 1 1 0 1 P$/X GNDBREAK 3 100 200 100 L 1 1 0 1 W/' \
    Cryptech_Alpha.lib
# USB connector VBUS
sed -i \
    -e 's/^X VBUS VBUS 400 200 100 L 1 70 0 1 W$/X VBUS VBUS 400 200 100 L 1 70 0 1 w/' \
    Cryptech_Alpha.lib
# Mark _one_ of the seven VOUTs on the EN6347Q1 as power output instead of input, since net-ties haven't been used
sed -i \
    -e 's/^X VOUT 5 800 900 200 L 70 70 0 1 W$/X VOUT 5 800 900 200 L 70 70 0 1 w/' \
    Cryptech_Alpha.lib
# Mark _one_ of the nine VOUTs on the EN5364Q1 as power output instead of input, since net-ties haven't been used
sed -i \
    -e 's/^X VOUT 5 900 2100 200 L 70 70 0 1 W$/X VOUT 5 900 2100 200 L 70 70 0 1 w/' \
    Cryptech_Alpha.lib
# Mark _one_ of the two VOUTs on the LMZ13608 as power output instead of input, since net-ties haven't been used
sed -i \
    -e 's/^X VOUT 10 900 700 200 L 70 70 0 1 W$/X VOUT 10 900 700 200 L 70 70 0 1 w/' \
    Cryptech_Alpha.lib
# FT232H power inputs/outputs
#   last letter:
#     P = passive
#     W = Power input
#     w = power output
sed -i \
    -e 's/^X VPHY 3 -200 1500 200 D 70 70 0 1 P/X VPHY 3 -200 1500 200 D 70 70 0 1 W/' \
    -e 's/^X VPLL 8 -100 1500 200 D 70 70 0 1 P/X VPLL 8 -100 1500 200 D 70 70 0 1 W/' \
    -e 's/^X VCCA 37 -1100 800 200 R 70 70 0 1 W/X VCCA 37 -1100 800 200 R 70 70 0 1 w/' \
    -e 's/^X VCCORE 38 -1100 900 200 R 70 70 0 1 W/X VCCORE 38 -1100 900 200 R 70 70 0 1 w/' \
    -e 's/^X VCCD 39 -1100 1000 200 R 70 70 0 1 W/X VCCD 39 -1100 1000 200 R 70 70 0 1 w/' \
    Cryptech_Alpha.lib

# Fix off-grid capacitor
sed -i \
    -e 's/^X + 1 110 0 10 L 1 1 0 1 P$/X + 1 100 0 10 L 1 1 0 1 P/' \
    -e 's/^X - 2 -110 0 10 R 1 1 0 1 P$/X - 2 -100 0 10 R 1 1 0 1 P/' \
    Cryptech_Alpha.lib
# Fix off-grid oscillator
sed -i \
    -e 's/^X 1 1 -110 0 10 R 1 1 0 1 P$/X 1 1 -100 0 10 R 1 1 0 1 P/' \
    -e 's/^X 3 3 110 0 10 L 1 1 0 1 P$/X 3 3 100 0 10 L 1 1 0 1 P/' \
    Cryptech_Alpha.lib

# Component attributes seem to get added in a big pile on components
grep -vx \
     -e 'T 0 -80 120 50 0 1 1 10% Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 50V Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 6.3V Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 X5R Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 X7R Normal 1 C C' \
     -e 'T 0 -220 -50 50 0 1 1 5% Normal 1 C C' \
     -e 'T 0 -220 40 50 0 1 1 5% Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 16V Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 20% Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 10% Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 50V Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 6.3V Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 X5R Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 X7R Normal 1 C C' \
     -e 'T 0 -520 210 50 0 1 1 2058982 Normal 1 C C' \
     -e 'T 0 -520 210 50 0 1 1 RCLAMP0502A Normal 1 C C' \
     -e 'T 0 -520 210 50 0 1 1 SEMTECH Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 10% Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 16V Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 20% Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 50V Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 X5R Normal 1 C C' \
     -e 'T 0 -80 120 50 0 1 1 X7R Normal 1 C C' \
     -e 'T 0 -820 1510 50 0 1 1 2081142 Normal 1 C C' \
     -e 'T 0 -820 1510 50 0 1 1 EN5364QI Normal 1 C C' \
     -e 'T 0 -820 1510 50 0 1 1 ENPIRION Normal 1 C C' \
     -e 'T 0 -820 1510 50 0 1 1 QFN Normal 1 C C' \
     -e 'T 0 -220 -50 50 0 1 1 5% Normal 1 C C' \
     -e 'T 0 -220 40 50 0 1 1 5% Normal 1 C C' \
     -e 'T 0 -410 420 50 0 1 1 2425618 Normal 1 C C' \
     -e 'T 0 -410 330 50 0 1 1 CONN-08106 Normal 1 C C' \
     -e 'T 0 -410 330 50 0 1 1 PRT-12748 Normal 1 C C' \
     -e 'T 0 -1120 1520 50 0 1 1 1870924 Normal 1 C C' \
     -e 'T 0 -820 910 50 0 1 1 2081146 Normal 1 C C' \
     -e 'T 0 -820 910 50 0 1 1 EN6347QI Normal 1 C C' \
     -e 'T 0 -820 910 50 0 1 1 ENPIRION Normal 1 C C' \
     -e 'T 0 -820 910 50 0 1 1 QFN Normal 1 C C' \
     -e 'T 0 -1220 2320 50 0 1 1 IS45S32160F Normal 1 C C' \
     -e 'T 0 -1220 2320 50 0 1 1 ISSI Normal 1 C C' \
     Cryptech_Alpha.lib > Cryptech_Alpha.lib2
mv Cryptech_Alpha.lib2 Cryptech_Alpha.lib


# Segments on non-copper layer Eco2.User are not visible, and causes ERC warnings.
# Turn them into graphical lines instead.
sed -i -e 's/segment \(.*\)layer Eco2.User.*/gr_line \1layer Eco2.User\)\)/g' Cryptech\ Alpha.kicad_pcb

# Set all schematic footprints from the PCB
../set-footprints-from-pcb.py Cryptech?Alpha.kicad_pcb *.sch

# Make further modifications to the layout using KiCAD's Python bindings
test -d ../tmp || mkdir ../tmp
cp "Cryptech Alpha.kicad_pcb" "../tmp/Cryptech Alpha.kicad_pcb.a2k-out"
../fix-pcb.py "Cryptech Alpha.kicad_pcb" "Cryptech Alpha.kicad_pcb"
mv "Cryptech Alpha.kicad_pcb.before-fix-pcb" ../tmp
#diff -u "../tmp/Cryptech Alpha.kicad_pcb.before-fix-pcb" "Cryptech Alpha.kicad_pcb" || true

echo ""
echo "Done. The leftovers from conversion is in ${altiumdir}, and you can start KiCad like this:"
echo ""
echo "  kicad \"${kicaddir}/Cryptech Alpha.pro\""
echo ""
