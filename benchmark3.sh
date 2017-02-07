#! /bin/sh

( (uniq -u < a1.txt | sort | wc -w | cat > c) 2>>d)
times