#!/bin/sh
for s in XXX FIXME TODO ; do
	echo "==== $s ===="
	git grep -nw $s
	echo
done
