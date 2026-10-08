#!/bin/bash

set -e

export DEVICE=a11
export VENDOR=htc

if [ $# -eq 0 ]; then
  SRC=adb
else
  if [ $# -eq 1 ]; then
    SRC=$1
  else
    echo "$0: bad number of arguments"
    echo ""
    echo "usage: $0 [PATH_TO_EXPANDED_ROM]"
    echo ""
    echo "If PATH_TO_EXPANDED_ROM is not specified, blobs will be extracted from"
    echo "the device using adb pull."
    exit 1
  fi
fi

BASE=../../../vendor/$VENDOR/$DEVICE/proprietary
rm -rf $BASE/*

if [ -f ../$DEVICE/proprietary-files.txt ]; then
  for FILE in `egrep -v '(^#|^$)' ../$DEVICE/proprietary-files.txt`; do
    FILE=`echo ${FILE[0]} | sed -e "s/^-//g"`
    # SRC:DST installs the device file SRC under the vendor name DST.
    SRCFILE=${FILE%%:*}
    DSTFILE=${FILE#*:}
    echo "Extracting /system/$SRCFILE ..."
    DIR=`dirname $DSTFILE`
    if [ ! -d $BASE/$DIR ]; then
      mkdir -p $BASE/$DIR
    fi
    if [ "$SRC" = "adb" ]; then
      adb pull /system/$SRCFILE $BASE/$DSTFILE
    else
      cp $SRC/system/$SRCFILE $BASE/$DSTFILE
    fi
  done
fi

../$DEVICE/setup-makefiles.sh
