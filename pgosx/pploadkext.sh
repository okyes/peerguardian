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
# !!! YOU MUST BE ROOT OR ADMIN !!!
#
# If you get a kernel panic /Library/Logs/panic.log and the xxx.qnation.PeerProtector.sym
# file are required to provide any help.
#

LOAD="/sbin/kextload -c -s ."
SUDO=""
if [ -z $UID ] || [ $UID -gt 0 ]; then
	SUDO="sudo"
fi

if [ ! -d PeerProtector.kext ]; then
	echo "Invalid working directory"
	exit 1
fi

loaded=`/usr/sbin/kextstat -l -b xxx.qnation.PeerProtector`
if [ "$loaded" != "" ]; then
	cpus=`sysctl -n hw.ncpu`
	if [ $cpus -ge 2 ]; then
		echo "There is a known bug with multi-cpu machines that will cause a system crash if the kext is unloaded. Please reboot your machine to re-load the kext."
		exit 1
	fi
	echo "Unloading current kext..."
	${SUDO} /sbin/kextunload -b xxx.qnation.PeerProtector
	err=$?
	if [ $err -ne 0 ]; then
		echo "Unload failed with: $err. One or more instances of pplogd may still be active."
		exit $err
	fi
fi

${SUDO} rm -f ./*.sym

${SUDO} chown -R root:wheel ./PeerProtector.kext
${SUDO} ${LOAD} ./PeerProtector.kext
