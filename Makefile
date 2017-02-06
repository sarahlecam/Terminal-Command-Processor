#Makefile:
#	- default: make executable simpsh from object file simpsh.o
#		created from C source module simpsh.c
#	- check: runs test file (test.sh)
#	- clean: delete executable, object files, testing directory and
#		distribution tarball
#	- dist: creates the distribution tarball containing simpsh.c, Makefile,
#		README and test.sh

# Compiler: gcc
CC=gcc

# make
default: simpsh.o
	$(CC) -o simpsh simpsh.o

simpsh: simpsh.o
	$(CC) -o simpsh simpsh.o

simpsh.o: simpsh.c
	gcc -c -std=c11 -Wall simpsh.c


# make check
# TODO: remove grading script
check: simpsh
	# ./test.sh
	./gradingtesting.sh


# make clean
# TODO: fix testing directory clean up
clean:
	if [ -e simpsh ] ; \
		then \
			rm -rf simpsh ; \
	fi;
	if [ -e simpsh.o ] ; \
		then \
			rm -rf simpsh.o ; \
	fi;
	if [ -e lab1-sarahlecam.tar.gz ] ; \
		then \
			rm -rf lab1-sarahlecam.tar.gz; \
	fi;
	# TODO: change to chosen name
	if [ -e lab1creadergradingtempdir ] ; \
		then \
			rm -rf lab1creadergradingtempdir; \
	fi;
	if [ -e lab1-sarahlecam ] ; \
		then \
			rm -rf lab1-sarahlecam; \
	fi;

# make dist
dist:
	tar zcvf lab1-sarahlecam.tar.gz --transform='s,^,lab1-sarahlecam/,' simpsh.c Makefile README gradingtesting.sh test.sh