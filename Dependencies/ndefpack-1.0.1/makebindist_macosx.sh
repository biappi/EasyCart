#!/bin/sh
# based on make-bindist.sh from VICE,
# written by Marco van den Heuvel <blackystardust68@yahoo.com>
# based on make-bindist.sh for Mac OSX from VICE,
# written by Christian Vogelgsang <chris@vogelgsang.org>
#
# makebindist.sh <strip> <version> <top-srcdir>
#                $1      $2        $3

STRIP=$1
NDEFTVERSION=$2
TOPSRCDIR=$3

CLITOOLS="ndeft"
APPTOOLS="ndefgui"
DOCS="README NEWS COPYING COPYING_fw"
SUBDIRDOCS="README_tool README_gui"

# make sure Info.plist is available
if [ ! -e Info.plist ]; then
  echo "ERROR: missing: Info.plist"
  exit 1
fi

for i in $TOOLS
do
    if [ ! -e src/$i ]; then
        echo Error: executable file\(s\) not found, do a \"make\" first
        exit 1
    fi
done

DIRNAME=ndefpack-$NDEFTVERSION-macosx

echo Generating binary distribution.
rm -f -r $DIRNAME
mkdir $DIRNAME

# cli tools
for i in $CLITOOLS
do
    $STRIP src/$i
    cp src/$i $DIRNAME/$i
done

# gui/app tools
for i in $APPTOOLS
do
    # create app directory
    mkdir -p $DIRNAME/$i.app/Contents/MacOS

    # strip&copy exe
    $STRIP src/$i
    cp src/$i $DIRNAME/$i.app/Contents/MacOS

    # setup Info.plist
    sed -e "s/XVERSIONX/$NDEFTVERSION/g" \
        -e "s/XNAMEX/$APPTOOLS/g" \
        < "Info.plist" > "$DIRNAME/$i.app/Contents/Info.plist"
done

for i in $DOCS
do
    cp -a $TOPSRCDIR/$i $DIRNAME/$i.txt
done

for i in $SUBDIRDOCS
do
    cp -a $TOPSRCDIR/doc/$i $DIRNAME/$i.txt
done

zip -r -9 -q $DIRNAME.zip $DIRNAME
rm -f -r $DIRNAME

echo Binary distribution archive generated as $DIRNAME.zip
