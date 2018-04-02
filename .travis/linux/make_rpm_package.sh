#!/bin/bash

# Enable debugging and logging of shell operations
# that are executed.
set -e
set -x

./bootstrap
./configure --prefix=`pwd`/usr/local

make -j2 install

#libstdc++ > 4.8 is not sufficient for all of C++11 but 2018/4/2 it does compile and all but provenance works.
#4.8 is all that's available on centos7 without workarounds
fpm -t rpm -n souffle -v `git describe --tags --always` -d gcc-c++ -d graphviz -d libgomp \
	-d libstdc++ > 4.8 -d mcpp -d ncurses-libs -d sqlite-libs -d zlib \
	--license UPL -s dir usr

mkdir deploy
# compute md5 for package &
# copy files to deploy directory
for f in *.rpm
do
  pkg=`basename $f .rpm`
  src="$pkg.rpm"
  dest="deploy/$pkg.rpm"
  cp $src $dest
  md5sum <$src >deploy/$pkg.md5
done

# show contents of deployment
ls deploy/*
