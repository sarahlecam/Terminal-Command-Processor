#! /bin/sh

( (uniq -D < a1.txt | sort | cat -b > c) 2>>d)
times
