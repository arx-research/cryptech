#!/bin/bash

set -e
set -x

basepath=$(dirname $(readlink -e $0))
start=${PWD}
for d in *-PcbLib *-PcbDoc; do
    (
	test -d "${d}/Root Entry/Models" && cd "${d}/Root Entry/Models"
	test -d "${d}/Root Entry/Library/Models" && cd "${d}/Root Entry/Library/Models"
	echo "Converting STEP files in ${PWD}..."
	freecad ${basepath}/altium2kicad/step2wrl.FCMacro || true
    )
done

# converting all the step files takes time and is interactive, so cache the result
find . -name '*.wrl' -print0 | xargs -0 tar zcvf ${basepath}/wrl-files.tar.gz
