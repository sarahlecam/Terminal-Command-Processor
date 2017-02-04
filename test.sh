
#!/bin/sh

# Uses UCLA CS 111 Lab 1a and Lab 1b testing script, written by Zhaoxing Bu (zbu@cs.ucla.edu)
# as base with additional test cases


if [ "${PATH:0:16}" == "/usr/local/cs/bin" ]
then
    true
else
    PATH=/usr/local/cs/bin:$PATH
fi


echo "please check if there is any error message below"
echo "==="

if [ -e "simpsh" ]
then
    rm -rf simpsh
fi

make || exit

# chmod 744 simpsh

TEMPDIR="lab1btestingtempdir"

rm -rf $TEMPDIR

mkdir $TEMPDIR

if [ "$(ls -A $TEMPDIR 2> /dev/null)" == "" ]
then
    true
else
    echo "fatal error! the testing directory is not empty"
    exit 1
fi

mv simpsh ./$TEMPDIR/

cd $TEMPDIR


# create testing files

cat > a0.txt <<'EOF'
Hello world! CS 111!
EOF

cat a0.txt > a1.txt
cat a0.txt > a2.txt
cat a0.txt > a3.txt
cat a0.txt > a4.txt
cat a0.txt > a5.txt
cat a0.txt > a10.txt
cat a0.txt > a11.txt
cat a0.txt > a12.txt
cat a0.txt > a13.txt
cat a0.txt > a14.txt
cat a0.txt > a15.txt

running_simpsh_check () {
    if ps | grep "simpsh"
    then
        echo "simpsh is running in background"
        echo "testing cannot continue"
        echo "kill it and then run the following test cases"
        exit 1
    fi
}

echo "==="

echo "please DO NOT run multiple testing scripts at the same time"
echo "make sure there is no simpsh running by you"
echo "infinite waiting of simpsh due to unclosed pipe is unacceptable"
echo "starting grading"

running_simpsh_check

NUM_PASSED=0
NUM_FAILED=0

# in Lab 1a, --rdonly, --wronly, --command, and --verbose.

# test case 1 --rdonly can be called with no error
echo ""
echo "--->test case 1:"
./simpsh --rdonly a1.txt > /dev/null 2>&1
if [ $? == 0 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 1 passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 1 failed"
fi

# test case 1b --wronly can be called with no error
echo ""
echo "--->test case 1b:"
./simpsh --wronly a1.txt > /dev/null 2>&1
if [ $? == 0 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 1b passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 1b failed"
fi

# test case 2 --rdonly does not have any default flag: test --create
echo ""
echo "--->test case 2:"
./simpsh --rdonly test2_none_exist.txt > /dev/null 2>&1
if [ -e "test2_none_exist.txt" ]
then
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 2 failed"
else
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 2 passed"
fi

# test case 3 --wronly does not have any default flag: test --trunc
echo ""
echo "--->test case 3:"
./simpsh --wronly a2.txt > /dev/null 2>&1
diff a2.txt a0.txt
if [ $? == 0 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 3 passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 3 failed"
fi

# test case 4 --command test rdonly and wronly
echo ""
echo "--->test case 4:"
touch test4out.txt
touch test4err.txt
./simpsh --rdonly a4.txt --wronly test4out.txt --wronly test4err.txt \
    --command 0 1 2 cat > /dev/null 2>&1
sleep 1
diff test4out.txt a0.txt
if [ $? == 0 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 4 passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 4 failed CHECK what's in test4out.txt"
    echo "may need to rerun this test case with more sleep time"
fi

# test case 5 --command test with wc, ls, ps, blah blah
echo ""
echo "--->test case 5:"
touch test5out.txt
touch test5err.txt
./simpsh --rdonly a5.txt --wronly test5out.txt --wronly test5err.txt \
    --command 0 1 2 wc > /dev/null 2>&1
sleep 1
wc < a0.txt > test5outstd.txt
diff test5out.txt test5outstd.txt
if [ $? == 0 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 5 passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 5 failed CHECK what's in test5out.txt"
    echo "may need to rerun this test case with more sleep time"
fi

# test case 6 test exit status of simpsh
echo ""
echo "--->test case 6:"
./simpsh --wronly test6_none_exist.txt > /dev/null 2>&1
if [ $? != 0 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 6 passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 6 failed"
fi

# test case 7 simpsh should continue to next option when encountered an error
echo ""
echo "-->test case 7:"
touch test7out.txt
touch test7err.txt
./simpsh --rdonly a5.txt --wronly test7out.txt --wronly test7err.txt \
    --coxxx --command 0 1 2 cat > /dev/null 2>&1
sleep 1
diff test7out.txt a0.txt
if [ $? == 0 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 7 passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 7 failed CHECK what's in test7out.txt"
    echo "may need to rerun this test case with more sleep time"
fi

# test case 8 --verbose
echo ""
echo "--->test case 8:"
echo "must show verbose info for --command"
echo "must NOT show verbose info for --rdonly and --wronly"
echo "showing --verbose itself or not is both OK"
touch test8in.txt
touch test8out.txt
touch test8err.txt
touch test8outmessage.txt
touch test8verbose_noneempty.txt
touch test8verbose_empty.txt
./simpsh --rdonly test8in.txt --wronly test8out.txt --wronly test8err.txt \
     --verbose --command 0 1 2 sleep 0.01 > test8outmessage.txt 2> /dev/null
cat test8outmessage.txt | grep "sleep" > test8verbose_noneempty.txt
cat test8outmessage.txt | grep "only" > test8verbose_empty.txt
TEST8_RESULT=0
if [ -s test8verbose_noneempty.txt ]
then
    TEST8_RESULT=0
else
    TEST8_RESULT=1
fi
if [ -s test8verbose_empty.txt ]
then
    TEST8_RESULT=1
fi
if [ $TEST8_RESULT == 0 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 8 passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 8 failed"
fi

# test case 9 test there is no wait
echo ""
echo "--->test case 9:"
echo "simpsh should immediately exits without waiting for 61 seconds"
echo "if you find simpsh waits for many seconds, then definitely failed"
#echo "please check the real time listed below"
#echo "if no time reported, manually run the test case without time command"
touch test9in.txt
touch test9out.txt
touch test9err.txt
touch test9time.txt
touch test9time_real.txt
touch test9time_final.txt
(time ./simpsh --rdonly test9in.txt --wronly test9out.txt --wronly test9err.txt \
    --command 0 1 2 sleep 61) 2> test9time.txt
TEST9_TIME_EXIST=1
if [ -s "test9time.txt" ]
then
    TEST9_TIME_EXIST=1
else
    echo "===no time output, pleaes manually check this test case"
    TEST9_TIME_EXIST=0
fi
cat test9time.txt | grep "real" > test9time_real.txt
if [ -s "test9time_real.txt" ]
then
    TEST9_TIME_EXIST=1
else
    echo "===no time output, pleaes manually check this test case"
    TEST9_TIME_EXIST=0
fi
if [ $TEST9_TIME_EXIST == 1 ]
then
    cat test9time_real.txt | grep "1m" > test9time_final.txt
    if [ -s "test9time_final.txt" ]
    then
        NUM_FAILED=`expr $NUM_FAILED + 1`
        echo "===>test case 9 failed"
    else
        NUM_PASSED=`expr $NUM_PASSED + 1`
        echo "===>test case 9 passed"
    fi
fi


# in Lab 1b, --rdwr, --pipe, --wait, --close, --abort, --catch, --ignore
# --default, --pause, and various file flags

# test case 10 file flags
echo ""
echo "--->test case 10:"
echo "It then waits for all three subprocesses to finish." > test10out.txt
./simpsh --rdonly a10.txt --trunc --wronly test10out.txt --creat --wronly \
    test10err.txt --command 0 1 2 cat > /dev/null 2>&1
sleep 1
diff test10out.txt a0.txt
TEST10_RESULT=$?
if [ -e test10err.txt ]
then
    TEST10_RESULT=`expr $TEST10_RESULT + 0`
else
    TEST10_RESULT=`expr $TEST10_RESULT + 1`
fi
if [ $TEST10_RESULT == 0 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 10 passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 10 failed CHECK what's in test1out.txt"
    echo "may need to rerun this test case with more sleep time"
fi

# test case 11 --pipe works fine
echo ""
echo "--->test case 11:"
echo "if simpsh hangs forever, then this test case is failed"
touch test11out.txt
touch test11err.txt
./simpsh --rdonly a11.txt --wronly test11err.txt --pipe --wronly test11out.txt \
    --command 0 3 1 cat --command 2 4 1 cat > /dev/null 2>&1
sleep 1
diff test11out.txt a0.txt
if [ $? == 0 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 11 passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 11 failed CHECK what's in test11out.txt"
    echo "may need to rerun this test case with more sleep time"
fi

# test case 12 --abort works fine
echo ""
echo "--->test case 12:"
touch test12out.txt
touch test12err.txt
./simpsh --rdonly a12.txt --wronly test12out.txt --wronly test12err.txt \
    --abort --command 0 1 2 cat > /dev/null 2>&1
sleep 1
if [ -s "test12out.txt" ]
then
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 12 failed"
else
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 12 passed"
fi

# test case 13 --pipe, --wait, --close all work fine together, no infinite wait
echo ""
echo "--->test case 13:"
echo "if simpsh hangs forever, then this test case is failed"
touch test13out.txt
touch test13err.txt
./simpsh --rdonly a13.txt --wronly test13out.txt --wronly test13err.txt --pipe \
    --command 0 4 2 cat --command 3 1 2 cat --close 3 --close 4 --wait \
    > /dev/null 2>&1
diff test13out.txt a0.txt
if [ $? == 0 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 13 passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 13 failed"
fi

# test case 14 --wait works fine
echo ""
echo "--->test case 14:"
#echo "simpsh should exits after waiting for 2 seconds"
#echo "please check the real time listed below (> 2 sceonds)"
#echo "if no time reported, manually run the test case without time command"
#echo "should also see a list of --command' exit status, command, and arguments"
touch test14in.txt
touch test14out.txt
touch test14err.txt
touch test14time.txt
touch test14time_real.txt
touch test14outmessage.txt
touch test14outmessage_grep.txt
(time ./simpsh --rdonly test14in.txt --wronly test14out.txt --wronly test14err.txt \
    --command 0 1 2 sleep 2 --wait) 2> test14time.txt > test14outmessage.txt
TEST14_RESULT=1
TEST14_TIME_EXIST=1
if grep -q "real" test14time.txt
then
    cat test14time.txt | grep "real" > test14time_real.txt
else
    echo "===no time output, pleaes manually check this test case"
    TEST14_TIME_EXIST=0
fi
if [ $TEST14_TIME_EXIST == 1 ]
then
    if grep -q "0m0\|0m1" test14time_real.txt
    then
        TEST14_RESULT=0
    fi
    if grep -q "sleep" test14outmessage.txt
    then
        true
    else
        TEST14_RESULT=0
    fi
    if [ $TEST14_RESULT == 1 ]
    then
        NUM_PASSED=`expr $NUM_PASSED + 1`
        echo "===>test case 14 passed"
    else
        NUM_FAILED=`expr $NUM_FAILED + 1`
        echo "===>test case 14 failed"
    fi
fi

# test case 15 --catch signal
echo ""
echo "--->test case 15:"
# running_simpsh_check

touch test15in.txt
touch test15out.txt
touch test15err.txt
touch test15errormessage.txt
touch test15ps.txt
./simpsh --rdonly test15in.txt --wronly test15out.txt --wronly test15err.txt \
    --catch 10 --command 0 1 2 sleep 20 --wait > /dev/null \
    2> test15errormessage.txt &
sleep 1
kill -10 $!
sleep 1
TEST15_RESULT=1
if grep -q "10" test15errormessage.txt
then
    true
else
    TEST15_RESULT=0
fi
if ps | grep -q "$!"
then
    TEST15_RESULT=0
fi
if [ $TEST15_RESULT == 1 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 15 passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 15 failed"
fi

# test case 16 --ignore signal
echo ""
echo "--->test case 16:"
# running_simpsh_check

touch test16in.txt
touch test16out.txt
touch test16err.txt
./simpsh --rdonly test16in.txt --wronly test16out.txt --wronly test16err.txt \
    --ignore 10 --command 0 1 2 sleep 20 --wait > /dev/null 2>&1 &
sleep 1
kill -10 $!
sleep 1
TEST16_RESULT=1
IGNORE_RESULT=0
if ps | grep -q "$!"
then
    true
else
    TEST16_RESULT=0
fi
kill -9 $!
sleep 1
if ps | grep -q "$!"
then
    TEST16_RESULT=0
fi
if [ $TEST16_RESULT == 1 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 16 passed"
    IGNORE_RESULT=1
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 16 failed"
fi

# test case 17 --default signal
# this test assumes the student's --ignore works
# can also depends on --catch instead of --ignore
# but I assume it is hardly to see a student's --catch works but --ignore not
echo ""
echo "--->test case 17:"
# running_simpsh_check

echo "this test case assumes the student's --ignore works"
if [ $IGNORE_RESULT == 1 ]
then
    touch test17in.txt
    touch test17out.txt
    touch test17err.txt
    ./simpsh --ignore 10 --rdonly test17in.txt --wronly test17out.txt \
        --wronly test17err.txt --default 10 --command 0 1 2 sleep 20 \
        --wait > /dev/null 2>&1 &
    sleep 1
    TEST17_RESULT=1
    if ps | grep -q "$!"
    then
        true
    else
        TEST17_RESULT=0
    fi
    kill -10 $!
    sleep 1
    if ps | grep -q "$!"
    then
        TEST17_RESULT=0
    fi
    if [ $TEST17_RESULT == 1 ]
    then
        NUM_PASSED=`expr $NUM_PASSED + 1`
        echo "===>test case 17 passed"
    else
        NUM_FAILED=`expr $NUM_FAILED + 1`
        echo "===>test case 17 failed"
    fi
else
    echo "===>--ignore test failed, this test case automatically faled"
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 17 failed"
fi

# test case 18 --pause
echo ""
echo "--->test case 18:"
# running_simpsh_check

./simpsh --pause &
TEST18_RESULT=1
sleep 1
if ps | grep -q "$!"
then
    true
else
    TEST18_RESULT=0
fi
kill -10 $!
sleep 1
if ps | grep -q "$!"
then
    TEST18_RESULT=0
fi
if [ $TEST18_RESULT == 1 ]
then
    NUM_PASSED=`expr $NUM_PASSED + 1`
    echo "===>test case 18 passed"
else
    NUM_FAILED=`expr $NUM_FAILED + 1`
    echo "===>test case 18 failed"
fi

# test case 19 test invalid file descriptor
echo ""
echo "READER MUST MANUALLY CHECK THE RESULTS FOR FOLLOWING TEST CASES"
echo ""
echo "--->test case 19:"
touch test19in.txt
touch test19out.txt
touch test19err.txt
./simpsh --rdonly test19in.txt --wronly test19out.txt --wronly test19err.txt \
    --command 0 1 3 cat > /dev/null 2>test19errormessage.txt
echo "should see invalid file descriptor error message below"
cat test19errormessage.txt

## test case 20: first argument is not an option
echo ""
echo "--->test case 20:"
echo "should see verbose output and not an option message below"
touch test20in.txt
touch test20out.txt
touch test20err.txt
./simpsh hello --verbose --rdonly test20in.txt --wronly test20out.txt --wronly \
    test20err.txt --command 0 1 2 cat 2>test20errormessage.txt
cat test20errormessage.txt

## test case 21: too many arguments for --rdonly
echo ""
echo "--->test case 21:"
echo "should see verbose output and too many arguments provided message bellow"
touch test21in.txt
touch test21out.txt
touch test21err.txt
./simpsh --verbose --rdonly test21in.txt test21err.txt --wronly test21out.txt --wronly \
    test21err.txt --command 0 1 2 cat 2>test21errormessage.txt
cat test21errormessage.txt

## test case 22: argument is not an option
echo ""
echo "--->test case 22:"
echo "should see cannot recognize option message bellow"
touch test22in.txt
touch test22out.txt
touch test22err.txt
./simpsh  --rdonly test22in.txt --wronly test22out.txt --wronly \
    test22err.txt --coxx --command 0 1 2 cat 2>test22errormessage.txt
cat test22errormessage.txt

## test case 23: fd are not integers
echo ""
echo "--->test case 23:"
echo "should see file descriptors are not integers message bellow"
touch test23in.txt
touch test23out.txt
touch test23err.txt
./simpsh  --rdonly test23in.txt --wronly test23out.txt --wronly \
    test23err.txt --command 0 1 f bl00pblop 2>test23errormessage.txt
cat test23errormessage.txt

# test case 24 --close works fine, there is an error message for using closed fd
echo ""
echo "--->test case 24:"
touch test24in.txt
touch test24out.txt
touch test24err.txt
./simpsh --rdonly test24in.txt --wronly test24out.txt --wronly test24err.txt \
    --close 2 --command 0 1 2 cat > /dev/null 2>test24errormessage.txt
echo "should see invalid file descriptor error message below"
echo "==="
cat test24errormessage.txt
echo "==="

# test case 25 --wait output
echo ""
echo "--->test case 25:"
touch a
cat > a <<'EOF'
Hello world! CS 111!
EOF
touch test25in.txt
touch test25out.txt
touch test25err.txt
./simpsh \
  --verbose \
  --rdonly a \
  --pipe \
  --pipe \
  --creat --trunc --wronly c \
  --creat --append --wronly d \
  --command 3 5 6 tr A-Z a-z \
  --command 0 2 6 sort \
  --command 1 4 6 cat b - \
  --wait > test25out.txt
echo "should see copy of commands and exit status bellow"
echo "==="
cat test25out.txt
echo "==="

# finished testing
echo ""
echo ""
echo "Testing finished"
NUM_COLLECTED=`expr $NUM_PASSED + $NUM_FAILED`
echo "among first $NUM_COLLECTED auto test cases: $NUM_PASSED passed, $NUM_FAILED failed"
if [ $NUM_COLLECTED != 19 ]
then
    echo "sum is not 19, check what happend"
fi;
echo ""
echo ""
