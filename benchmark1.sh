#! /bin/sh

( (sort < a1.txt | cat b - | tr A-Z a-z > c) 2>>d)
times