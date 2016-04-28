#!/bin/sh
# based on make-bindist.sh from VICE,
# written by Marco van den Heuvel <blackystardust68@yahoo.com>
#
# makebindist.sh <strip> <version> <top-srcdir> <exeext>
#                $1      $2        $3           $4

STRIP=$1
NDEFTVERSION=$2
TOPSRCDIR=$3
EXEEXT=$4

TOOLS="ndeft ndefgui"
DOCS="README NEWS COPYING COPYING_fw"
SUBDIRDOCS="README_tool README_gui"

for i in $TOOLS
do
    if [ ! -e src/$i$EXEEXT ]; then
        echo Error: executable file\(s\) not found, do a \"make\" first
        exit 1
    fi
done

DIRNAME=ndefpack-$NDEFTVERSION-win32

echo Generating binary distribution.
rm -f -r $DIRNAME
mkdir $DIRNAME

for i in $TOOLS
do
    $STRIP src/$i$EXEEXT
    cp src/$i$EXEEXT $DIRNAME/$i$EXEEXT
done

for i in $DOCS
do
    cp -a $TOPSRCDIR/$i $DIRNAME/$i.txt
    unix2dos $DIRNAME/$i.txt
done

for i in $SUBDIRDOCS
do
    cp -a $TOPSRCDIR/doc/$i $DIRNAME/$i.txt
    unix2dos $DIRNAME/$i.txt
done

zip -r -9 -q $DIRNAME.zip $DIRNAME
rm -f -r $DIRNAME

echo Binary distribution archive generated as $DIRNAME.zip
