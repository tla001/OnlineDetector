#!/bin/bash  

set -e  

PROJ_ROOT=$PWD  
BUILD_ROOT=$PROJ_ROOT  
echo -e "\033[31m Current Build Root: $BUILD_ROOT \033[0m"  



BIN_PATH=$BUILD_ROOT/bin

if [ -d $BIN_PATH ];then
	cd $BIN_PATH
	rm -rf *
	cd ..
else
	mkdir -p $BIN_PATH  
fi

INC_PATH=$BUILD_ROOT/inc

if [ -d $INC_PATH ];then
	cd $INC_PATH
	rm -rf *
	cd ..
else
	mkdir -p $INC_PATH  
fi

#cp without path
#find ./src/ -name "*.h" |xargs -i cp {} ./inc/
DEST_PATH="./inc/"

headers=`find . -name *.h`
#echo $headers

for file in $headers
do
#echo $file
	temp=${file#*/}
	temp=${temp#*/}
	dstfile=$DEST_PATH$temp
	dstpath=${dstfile%/*}
	#echo $dstpath
	[ -d $dstpath ] || mkdir -p  $dstpath

	cp -R $file $dstfile

done

LIB_PATH=$BUILD_ROOT/lib

if [ -d $LIB_PATH ];then
	cd $LIB_PATH
	rm -rf *
	cd ..
else
	mkdir -p $LIB_PATH  
fi

BUILD_PATH=$BUILD_ROOT/build

if [ -d $BUILD_PATH ];then
	cd $BUILD_PATH
	rm -rf *
else
	mkdir -p $BUILD_PATH  
	cd $BUILD_PATH
fi

cmake  $PROJ_ROOT  
make -j4
