#! /bin/sh

( (uniq d < a0.txt | sort | cat b - > c) 2>>d)
times
