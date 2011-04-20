#!/bin/sh

tar -cf ~/Desktop/p2butils.tar pplib/pplib.[ch] pplib/ppmsg.h p2b/p2b.[ch]
tar -rf ~/Desktop/p2butils.tar -C cli pgmerge.c pptbl.c

cat << EOF >> /tmp/README
Attached is a p2b library that can read p2p (text), p2b v2, and p2b v3 and write p2p and p2b v2. Also included are 2 cli utilities: pgmerge and pgtbl.

The library provides a generic C API for reading and writing block list files. All of the format details are hidden behind the API, and consumers just deal with in memory arrays of ranges and names.

pgmerge is a tool that can read multiple inputs (any supported format) and output to either p2p or p2b v2.

pgtbl can list the contents of any file format and add/remove ranges to p2p or p2b v2 files.

The library and pgmerge are shipping code in PG OS X. pgtbl is an internal tool that I use in the development of PG OS X.

I've compiled and tested these on Suse Linux 9.0. They should also compile on pretty much any UNIX OS (*BSD for sure). Instructions for compiling the utils are in the respective files after the license header.
EOF

tar -rf ~/Desktop/p2butils.tar -C /tmp README

#gzip -9 ~/Desktop/p2butils.tar
# PG forum requires zip
zip -qr -9 ~/Desktop/p2butils.tar.zip ~/Desktop/p2butils.tar
