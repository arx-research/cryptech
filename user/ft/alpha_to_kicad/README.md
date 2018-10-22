Work-in-progress repository with output of altium2kicad conversion
of the Cryptech Alpha rev03 board.

The conversion was done using 'convert.sh', with the Altium Designer
project files from

  https://wiki.cryptech.is/browser/hardware/cad/rev03

and the altium2kicad project from

  https://github.com/thesourcerer8/altium2kicad


Current status
--------------
NOTE: The latest stable KiCAD version as of this writing is 4.0.7 - it does
NOT include necessary support for stitching vias. Install KiCAD nightly build
to work with the Cryptech Alpha PCB.

The schematics are mostly converted. A few components do not connect with their
nets (e.g. C9 and C10 on sheet rev02_01), but maybe a manual overhaul will be
needed anyways at the end of conversion. A bigger issue is that no components
get footprints associated with them in the schema, so generating a new netlist
won't work at all. The footprints exists in some form in the PCB, so we only
need to figure out how to reference them properly in the schema.

All the copper layers convert reasonably well. The challenges are mostly
around filled polygons on the various layers. A python script (fix-pcb.py)
modifies parameters to get a fairly close result.

I'm currently looking into ensuring the drill hole sizes are right, and the
non-copper layers have been largely ignored this far.


Issues
------
Two layers (Altium Gerber files CrypTech.G1 and CrypTech.G2) have fills
that I have not been able to reproduce. I targeted not missing any copper,
accepting that the KiCAD gerber fills reach more places, so add some copper
on those layers.

Drill hole sizes have not been checked. KiCAD seems to add ~0.85 mil more
clearance around vias. This needs to be double checked but I'm hoping that
we can just tolerate that.

Importing WRL files (3D models) required some hacking of the altium2kicad
tool that I haven't been able to work on upstreaming yet. Something is still
not right here, but the board does have a fair amount of component (including
the more special ones) in KiCAD 3D view.

Another hack that has not been upstreamed is loading more of the source
files, IIRC to get all component footprints properly converted.

The KiCAD board does not pass DRC checks yet. I believe part of this is
because design rule settings aren't (fully?) imported from Altium. Need
to figure out the settings used for this project, and fix all drill sizes
I think.
