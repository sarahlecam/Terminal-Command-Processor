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
cp a1.txt ./$TEMPDIR/

cd $TEMPDIR

echo "==="

# benchmark 1 prompt command
echo ""
echo "--->test case 1:"
echo "./simpsh: "
touch test1out.txt
(./simpsh \
  --profile \
  --rdonly a1.txt \
  --pipe \
  --pipe \
  --creat --trunc --wronly c \
  --creat --append --wronly d \
  --command 3 5 6 tr A-Z a-z \
  --command 0 2 6 sort \
  --command 1 4 6 cat b - \
  --wait > test1out.txt)
echo "===="
cat test1out.txt
echo "===="
echo ""



# test case 2 --profile scope
echo ""
echo "--->test case 2:"
touch test2out.txt
echo "./simpsh:"
./simpsh \
  --profile \
  --rdonly a1.txt \
  --pipe \
  --pipe \
  --creat --trunc --wronly c \
  --creat --append --wronly d \
  --command 0 2 6 uniq -D \
  --command 1 4 6 sort \
  --command 3 5 6 cat -b \
  --wait > test2out.txt
echo "--profile output: "
echo "===="
cat test2out.txt
echo "===="
echo ""


# test case 3 --profile sort large file
echo ""
echo "--->test case 3:"
echo "./simpsh: "
touch test3out.txt
./simpsh
    --rdonly a0.txt --wronly test3out.txt \
    --wronly test3err.txt --profile --command 0 1 2 uniq --wait
echo "===="
cat test3out.txt
echo "===="
echo ""
