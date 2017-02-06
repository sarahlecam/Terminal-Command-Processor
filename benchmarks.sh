# benchmark tests

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

TEMPDIR="lab1benchmarkstempdir"

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
cp a0.txt ./$TEMPDIR/

cd $TEMPDIR


# use the provided lab1c_generate_a0.sh and pg98.txt to generate a0.txt
# please use the provided a0.txt and make sure it is not modified by any code

echo "==="

echo "please DO NOT run multiple testing scripts at the same time"
echo "starting grading"

NUM_PASSED=0
NUM_FAILED=0

# test case 1 --profile almost no time
echo ""
echo "--->test case 1:"
echo "check if there is time info for --rdonly command"
echo "time should be almost 0"
touch test1in.txt
./simpsh --profile --rdonly test1in.txt

# test case 2 --profile scope
echo ""
echo "--->test case 2:"
echo "in terms of each command's time"
echo "you should only see time info for --wronly, but not for --rdonly"
touch test2in.txt
touch test2out.txt
./simpsh --profile --rdonly test2in.txt --wronly test2out.txt

# test case 3 --profile sort large file
echo ""
echo "--->test case 3:"
echo "check time info for --command sort, should not be 0 (you can test"
echo "the uniq command by yourself to see the running time in shell"
echo "here you may see multiple running time, but there should be one"
echo "corresponding to the running time of uniq the a0.txt"
touch test3out.txt
touch test3err.txt
./simpsh --verbose --rdonly a0.txt --wronly test3out.txt \
    --wronly test3err.txt --profile --command 0 1 2 uniq --wait

echo ""
