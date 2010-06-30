#!/bin/sh

mntpnt=${1}
shift

f=/proc/$$/mounts
test ! -f $f && f=/proc/mounts
br=`
grep -w ${mntpnt}.\*xino $f |
cut -f4 -d' ' |
tr , '\n' |
egrep '^(dirs|br)=' |
cut -f2- -d'=' |
sed -e 's/=[^:]*:*/ /g'`

find ${br} $@
