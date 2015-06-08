#!/bin/sh
echo -n "C: "
cat `git ls-files \*.c \*.h` | wc -l
echo -n "Lua: "
cat `git ls-files \*.lua` | wc -l
echo -n "GLSL: "
cat `git ls-files \*.glsl` | wc -l
echo -n "Python: "
cat `git ls-files \*.py` | wc -l
echo -n "Perl: "
cat `git ls-files \*.pl` | wc -l
