#!/bin/bash
for s in xxx fixme todo ; do
	S="${s^^}"
	echo "==== $S ===="
	git grep -nw $S
	echo
done
