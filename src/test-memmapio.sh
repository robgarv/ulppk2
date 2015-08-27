#!/bin/bash

datadir=/tmp/test-data
poolname=pool1
function runtest() {
	testswitch=$1
	poolname=$2
	datadir=$3
	label=$4
	program="./test-memmapio"
	echo ""
	echo "Running test: $label"
	eval "$program -d $datadir -p $poolname $testswitch"

	if [ $? -ne 0 ]; then
		echo "Test failure: $datadir has been preserved for examination"
		echo "$label test fails!"
		exit 1
	fi
}
	
if [ -d $datadir ]; then
	rm -rf $datadir
fi

mkdir -p $datadir


echo "Creating buffer pool for tests b and c"

./mmbuffpool -c -p $poolname -d $datadir -l 128 -C 16 -i 1 
./mmbuffpool -r -p $poolname -d $datadir

echo "Running tests"

runtest '-a' pool1 /tmp/test-data 'mmfor: memory mapped file of records'
runtest '-b' pool1 /tmp/test-data 'mmbuffpool: Allocate and write to memory mapped buffers'
runtest '-c' pool1 /tmp/test-data 'mmbuffpool: Read memory mapped buffers and deallocate'

echo "All tests successful!" 

rm -rf $datadir

exit 0



