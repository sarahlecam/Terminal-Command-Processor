#! /bin/sh

# benchmark tests

if [ "${PATH:0:16}" == "/usr/local/cs/bin" ]
then
    true
else
    PATH=/usr/local/cs/bin:$PATH
fi

if [ -e "simpsh" ]
then
    rm -rf simpsh
fi

make || exit

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

echo "==="

# benchmark 1 prompt command
echo ""
echo "--->test case 1:"
echo "./simpsh: "
touch test1out.txt
(./simpsh \
  --profile \
  --verbose \
  --rdonly a0.txt \
  --pipe \
  --pipe \
  --creat --trunc --wronly c \
  --creat --append --wronly d \
  --command 3 5 6 tr A-Z a-z \
  --command 0 2 6 sort \
  --command 1 4 6 cat b - \
  --wait > test1out.txt)
echo "--profile output: "
echo "===="
cat test1out.txt
echo "===="
echo ""
echo "bash/dash: "
echo "===="
( (sort < a0.txt | cat b - | tr A-Z a-z > c) 2>>d)
times
echo "===="
echo ""



# test case 2 --profile scope
echo ""
echo "--->test case 2:"
touch test2out.txt
echo "./simpsh:"
./simpsh --profile --rdonly a0.txt --wronly b --wronly d \
     --verbose --command 0 1 2 sleep 0.01 --command 0 1 2 cat --wait > test2out.txt
echo "--profile output: "
echo "===="
cat test2out.txt
echo "===="
echo ""
echo "bash/dash: "
echo "===="
( (sleep 0.01 && cat a0.txt > c) 2>>d)
times
echo "===="
echo ""

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
