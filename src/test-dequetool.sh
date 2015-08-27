#!/bin/bash

function check_dist_status() {
	binaryname=`which dequetool`
	if [ ! -z $binaryname ]; then 
		echo "*****WARNING!! The dequetool binary is installed on the path!"
		echo "This means you might not be testing what you think you are testing"
		echo "Recommend sudo make uninstall"
		echo $PATH
		which dequetool
		exit 1
	fi
}

check_dist_status
if [ $PWD != "$HOME/swdev/ulppk2/src" ]; then
	echo "Current working directory is incorrect!" 
	exit 1
fi

export PATH=$PWD:$PATH
datadir="/tmp/test-data"
dequename="testqueue"
itemsize=32
itemdatasize=20
testfile="../testdata/dequetest.ipsum.txt"
outfile="/tmp/dequetest.out.txt"

if [ -d $datadir ]; then
	rm -rf $datadir
fi
mkdir -p $datadir

if [ ! -f $testfile ]; then 
	echo "Can't acces test data file $testfile"
	env
	exit 1
fi

filesize=$(wc -c < "$testfile");
let npages=$filesize/4096
let remainder=$filesize%4096

if [ $remainder -ne 0 ]; then
	let npages+=1
fi

echo "================================================"
echo "test-dequetool"
echo ""
echo "Data directory: $datadir"
echo "Queue name: $dequename"
echo "Creating queue"
echo ""
echo "================================================"

let nitems=npages*4096/itemdatasize

echo ""
echo "Creating deque: $dequename for binary (block) transfer"
echo "Deque will have $nitems capacity."
echo "dequetool -c -d $datadir -q $dequename -n $nitems -s $itemsize"
dequetool -c -d $datadir -q $dequename -n $nitems -s $itemsize
if [ $? -ne 0 ]; then
	echo "Deque creation fails!"
	exit 2
fi
echo ""
echo "dequetool -r -d $datadir -q $dequename"
dequetool -r -d $datadir -q $dequename
if [ $? -ne 0 ]; then 
	echo "reporting function fails!"
	exit 3
fi

echo ""
echo "Injecting contents of $testfile into deque"


echo "dequetool -i -b -d $datadir -q $dequename -f $testfile"
dequetool -i -b -d $datadir -q $dequename -f $testfile
if [ $? -ne 0 ]; then 
	echo "data injection test fails!"
	exit 3
fi

echo ""
echo "dequetool -r -d $datadir -q $dequename"
dequetool -r -d $datadir -q $dequename
if [ $? -ne 0 ]; then 
	echo "reporting function fails!"
	exit 3
fi

echo ""

echo "dequetool -e -b -d $datadir -q $dequename -f $outfile"
dequetool -e -b -d $datadir -q $dequename -f $outfile
if [ $? -ne 0 ]; then 
	echo "data extraction test fails!"
	exit 3
fi

echo ""
echo "dequetool -r -d $datadir -q $dequename"
dequetool -r -d $datadir -q $dequename
if [ $? -ne 0 ]; then 
	echo "reporting function fails!"
	exit 3
fi

echo ""
diff $testfile $outfile 
if [ $? -ne 0 ]; then
	echo "Discrepency in diff between input $testfile and output $outfile!"
	exit 5
else
	echo "File transfer success: $testfile zero diffs with $outfile!"
fi

################## ASCII/Raw/Byte Stream Mode Transfer Test #####################################3

let nitems=npages*4096
itemsize=1
dequename=stream_$dequename

echo ""
echo "Creating deque: $dequename for byte stream (ASCII/raw) transfer"
echo "Deque will have $nitems capacity."
echo "dequetool -c -d $datadir -q $dequename -n $nitems -s $itemsize"
dequetool -c -d $datadir -q $dequename -n $nitems -s $itemsize
if [ $? -ne 0 ]; then
	echo "Deque creation fails!"
	exit 2
fi
echo ""
echo "dequetool -r -d $datadir -q $dequename"
dequetool -r -d $datadir -q $dequename
if [ $? -ne 0 ]; then 
	echo "reporting function fails!"
	exit 3
fi

echo ""
echo "Injecting contents of $testfile into deque"


echo "dequetool -i -d $datadir -q $dequename -f $testfile"
dequetool -i -d $datadir -q $dequename -f $testfile
if [ $? -ne 0 ]; then 
	echo "data injection test fails!"
	exit 3
fi

echo ""
echo "dequetool -r -d $datadir -q $dequename"
dequetool -r -d $datadir -q $dequename
if [ $? -ne 0 ]; then 
	echo "reporting function fails!"
	exit 3
fi

echo ""

outfile=/tmp/stream_dequetestout.txt
echo "dequetool -e -d $datadir -q $dequename -f $outfile"
dequetool -e -d $datadir -q $dequename -f $outfile
if [ $? -ne 0 ]; then 
	echo "data extraction test fails!"
	exit 3
fi

echo ""
echo "dequetool -r -d $datadir -q $dequename"
dequetool -r -d $datadir -q $dequename
if [ $? -ne 0 ]; then 
	echo "reporting function fails!"
	exit 3
fi

echo ""
diff $testfile $outfile 
if [ $? -ne 0 ]; then
	echo "Discrepency in diff between input $testfile and output $outfile!"
	exit 5
else
	echo "File transfer success: $testfile zero diffs with $outfile!"
fi

rm -rf $datadir

echo "All tests complete"
exit 0

