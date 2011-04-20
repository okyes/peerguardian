#!/bin/sh
#
# Copyright 2005 Brian Bergstrand
#
# This program is free software; you can redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program;
# if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
# crontab entry for table updates every 6 hrs
# 
# 0 */6 * * * cd ~/Applications/PeerProtector; ./pploadtbls.sh >> LOGFILE 2>&1
#
# Replace LOGFILE with a path to a file of your choosing
#
# For the first load after a reboot, make sure to give the -f flag

FORCELOAD=0
if [ $# -eq 1 ] && [ $1 == "-f" ]; then
	FORCELOAD=1
fi

date

EPOCH="Sun Aug 29 00:00:01 GMT 2004"
LASTCHECK=$EPOCH
if [ -f .pplastcheck ]; then
	LASTCHECK=`cat .pplastcheck`
fi

#Compression Type, gz is not really supported
#CTYPE=p2b.gz
#DECOMPRESS="gzip -df"
CTYPE=7z
DECOMPRESS="./7za x -bd -y"

TTYPE=p2b

#if the table is not in this list, then ports 80, 443 and 8080 are allowed
HTTPBLOCK="ads spy bogon phishing"
i=0
# blocklist.org p2b format lists
for tbl in p2p ads spy gov bogon phishing
do
	blockhttp=""
	for j in $HTTPBLOCK
	do
		if [ $tbl == $j ]; then
			blockhttp="-b"
		fi
	done
			
	echo "Downloading $tbl.${CTYPE} table..."
	
	lastmod=$LASTCHECK
	if [ ! -f $tbl.${TTYPE} ]; then
		lastmod=$EPOCH
	fi
	curl -s -L -O -z "$lastmod" http://blocklist.org/$tbl.${CTYPE}
	err=$?
	if [ -f $tbl.${CTYPE} ]; then
		if [ -f $tbl.${TTYPE} ]; then
			mv $tbl.${TTYPE} $tbl.old
		fi
		${DECOMPRESS} $tbl.${CTYPE} > /dev/null
		# load new file
		if [ -f $tbl.${TTYPE} ]; then
			echo "Loading $tbl..."
			./ppload -i $i $blockhttp $tbl.${TTYPE}
		else
			echo "decompression of $tbl.${CTYPE} failed"
			# mv old file back
			if [ -f $tbl.old ]; then
				mv $tbl.old $tbl.${TTYPE}
			fi
		fi
		#remove compressed file
		if [ -f $tbl.${CTYPE} ]; then
			rm $tbl.${CTYPE}
		fi
	else
		if [ $err -gt 0 ]; then
			echo "curl failed with: $?"
		else
			echo "table is up to date"
		fi
		
		if [ $FORCELOAD -eq 1 ]; then
			echo "Force Loading $tbl..."
			./ppload -i $i $blockhttp $tbl.${TTYPE}
		fi
	fi
	i=$(($i + 1))
done

# bluetack.co.uk lists
# likely some overlapp with blocklist.org,
# but the 2 together give the most protection
HTTPBLOCK="70 19 20" # ads, spy, trojan
BLUE="bluetack.txt"
BLUEHTTP="bluetack-http.txt"

lastmod=$LASTCHECK
if [ ! -f $BLUE ] || [ ! -f $BLUEHTTP ]; then
	lastmod=$EPOCH
fi

#remove old aggregate tables
rm -f $BLUE $BLUEHTTP

for tbl in 70 8 19 20 # ads, p2p, spy, trojan
do
	bfile=$BLUE
	for j in $HTTPBLOCK
	do
		if [ $tbl == $j ]; then
			bfile=$BLUEHTTP
		fi
	done
	
	tmpfile=bluetack-$tbl.txt
	
	echo "Downloading bluetack table..."
	# bluetack can take a long time to download from and sometimes just drops the connection
	# and the file is incomplete -- don't know what their problem is
	# but if they set Apache to support gzip, things would work better
	curl -s -L -o $tmpfile --compressed -z "$lastmod" "http://www.bluetack.co.uk/modules.php?name=Downloads&d_op=getit&lid=$tbl"
	err=$?
	if [ -f $tmpfile ]; then
		cat $tmpfile >> $bfile
		gzip -f -9 $tmpfile #archive for loading when there is no update
	else
		if [ $err -gt 0 ]; then
			echo "curl failed with: $?"
		else
			echo "table is up to date"
		fi
		if [ -f $tmpfile.gz ]; then
			gzip -d -c $tmpfile.gz >> $bfile
		fi
	fi
done

#load the aggregate tables
if [ -f $BLUE ]; then
	echo "Loading Bluetrack allow HTTP table"
	./ppload -i $i $BLUE
else
	echo "Bluetrack allow HTTP table does not exist"
fi
i=$(($i + 1))
if [ -f $BLUEHTTP ]; then
	echo "Loading Bluetrack deny HTTP table"
	./ppload -i $i -b $BLUEHTTP
else
	echo "Bluetrack deny HTTP table does not exist"
fi
i=$(($i + 1))

#you can create custom allow/deny tables with pptbl
if [ $FORCELOAD -eq 1 ]; then
	#echo "Loading custom tables..."
	#./ppload -i $i -b myads.${TTYPE}
	i=$(($i + 1))
	
	#always allow example
	#./ppload -i $i -a myallow.${TTYPE}
	i=$(($i + 1))
fi

echo -n `date -u` > .pplastcheck
