#!/bin/sh
#
# Copyright 2005-2008 Brian Bergstrand
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

PATH=/bin:/usr/bin

if [ ${EUID} -ne 0 ]; then
echo "Error: uninstall must be run as root"
exit 13
fi

cd /Library/LaunchDaemons
if [ -f xxx.qnation.PeerProtector.kextload.plist ]; then
	echo "Removing Launch Daemon plist..."
	rm ./xxx.qnation.PeerProtector.kextload.plist 
fi
if [ -f xxx.qnation.PeerGuardian.kextload.plist ]; then
	echo "Removing Launch Daemon plist..."
	rm ./xxx.qnation.PeerGuardian.kextload.plist 
fi

cd ../Extensions
if [ -d ./PeerProtector.kext ] || [ -d ./PeerGuardian.kext ] ; then
	echo "Unloading kext..."
	if [ -x /Library/Application\ Support/PeerGuardian/pgstart ]; then
	/Library/Application\ Support/PeerGuardian/pgstart -u
	elif [ -x /Library/Application\ Support/pgstart ]; then
	/Library/Application\ Support/pgstart -u
	else
	/Library/Application\ Support/ppktool -u
	fi
fi

if [ -d ./PeerProtector.kext ]; then
	echo "Removing PP kext..."
	rm -rf ./PeerProtector.kext
fi

if [ -d ./PeerGuardian.kext ]; then
	echo "Removing kext..."
	rm -rf ./PeerGuardian.kext
fi

cd ../Application\ Support
if [ -d ./PeerGuardian ]; then
	echo "Removing app support dir..."
	rm -rf ./PeerGuardian
fi
if [ -f ./ppktool ]; then
	echo "Removing pre-1.5 kext tool..."
	rm ./ppktool
fi
if [ -f ./pgstart ]; then
	echo "Removing kext tool..."
	rm ./pgstart
fi

rm -f /var/run/.pgkextinfo

cd ../Receipts
if [ -d ./PeerProtector.pkg ]; then
	echo "Removing PP Install receipt..."
	rm -rf ./PeerProtector.pkg
fi

echo "Removing PG Install receipts..."
rm -rf ./PeerGuard*.pkg

cd ../Widgets
if [ -d ./PeerProtector.wdgt ]; then
	echo "Removing PP widget..."
	rm -rf ./PeerProtector.wdgt
fi

if [ -d ./PeerGuardian.wdgt ]; then
	echo "Removing widget..."
	rm -rf ./PeerGuardian.wdgt
fi	

cd /Applications
if [ -d ./PeerProtector.app ]; then
	echo "Removing PP Application..."
	rm -rf ./PeerProtector.app
fi

if [ -d ./PeerGuardian.app ]; then
	echo "Removing Application..."
	rm -rf ./PeerGuardian.app
else
	echo "Warning: PeerGuardian application not found in /Applications."
fi

echo "Killing Applications"
killall PeerGuardian
killall pgagent

echo "Killing background helpers"
killall pploader
killall pplogger

cd ~/Library/Caches
if [ -d xxx.qnation.PeerProtector ]; then
	echo "Removing PP list cache"
	rm -rf ./xxx.qnation.PeerProtector
fi

if [ -d xxx.qnation.PeerGuardian ]; then
	echo "Removing list cache"
	rm -rf ./xxx.qnation.PeerGuardian
fi

if [ -d ../Widgets ]; then
	cd ../Widgets
	if [ -d ./PeerProtector.wdgt ]; then
		echo "Removing PP widget..."
		rm -rf ./PeerProtector.wdgt
	fi
	
	if [ -d ./PeerGuardian.wdgt ]; then
		echo "Removing widget..."
		rm -rf ./PeerGuardian.wdgt
	fi
fi

cd ../Preferences/
echo "Removing preferences..."
rm -f ./*xxx.qnation.*
