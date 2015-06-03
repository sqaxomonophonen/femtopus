#!/usr/bin/env bash
if [ -z "$1" ] ; then
	echo "usage: $0 <lump.blend>" > /dev/stderr
	exit 1
fi
dir="$( dirname "${BASH_SOURCE[0]}" )"
blender -noaudio -b $1 -P $dir/export_lump.py -- $2

