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

RELEASE=Release
# useful when debugging this script or the installer
KEEPTMP=0
while : ; do
	case $# in 0) break ;; esac
	
	case "$1" in
		-d )
			RELEASE=Debug
			shift
		;;
		-k )
			KEEPTMP=1
			shift
		;;
		* )
			break
		;;
	esac
done

if [ ! -d pplib ]; then
	echo "Invalid working directory"
	exit 1
fi

CPUARCH=`uname -p`
HAVETEX=0
if [ -d /usr/local/teTeX/bin ]; then
	PATH="$PATH:/usr/local/teTeX/bin/${CPUARCH}-apple-darwin-current"
	HAVETEX=1
fi

echo "Version:"
read VERSION

MYROOT=`pwd`
PRIV_ROOT=/tmp/pgpriv.$$

#build installer for privileged components
echo "Creating installer..."
mkdir -p ${PRIV_ROOT}/Extensions
mkdir ${PRIV_ROOT}/LaunchDaemons
mkdir -p "${PRIV_ROOT}/Application Support/PeerGuardian"
mkdir -m 775 ${PRIV_ROOT}/Applications
mkdir ${PRIV_ROOT}/Widgets
mkdir ${PRIV_ROOT}/Packages
mkdir ${PRIV_ROOT}/InstallRes
mkdir ${PRIV_ROOT}/PeerGuardian

cp -R build/${RELEASE}/PeerGuardian.kext ${PRIV_ROOT}/Extensions/
if [ $RELEASE != "Debug" ]; then
	strip -S -x -arch i386 ${PRIV_ROOT}/Extensions/PeerGuardian.kext/Contents/MacOS/PeerGuardian
	# buiilding with XCode 3 gcc4 always adds an LC_UUID section, which is not comptabile with pre-10.5 PPC systems
	strip -S -x -no_uuid -arch ppc ${PRIV_ROOT}/Extensions/PeerGuardian.kext/Contents/MacOS/PeerGuardian
fi
/bin/echo -n "$VERSION" > ${PRIV_ROOT}/Extensions/PeerGuardian.kext/Contents/Resources/pgversion

cp utils/xxx.qnation.PeerProtector.kextload.plist ${PRIV_ROOT}/LaunchDaemons/xxx.qnation.PeerGuardian.kextload.plist

cp build/${RELEASE}/pgstart "${PRIV_ROOT}/Application Support/PeerGuardian"
strip "${PRIV_ROOT}/Application Support/PeerGuardian/pgstart"

# GUI
cp -R build/${RELEASE}/PeerGuardian.app ${PRIV_ROOT}/Applications
cp -R build/${RELEASE}/PeerGuardian.wdgt ${PRIV_ROOT}/Widgets/
strip ${PRIV_ROOT}/Applications/PeerGuardian.app/Contents/Resources/pploader.app/Contents/Resources/7za
if [ $RELEASE != "Debug" ]; then
	strip ${PRIV_ROOT}/Applications/PeerGuardian.app/Contents/Resources/pgmerge
	strip ${PRIV_ROOT}/Applications/PeerGuardian.app/Contents/Resources/pginfo
	strip ${PRIV_ROOT}/Applications/PeerGuardian.app/Contents/Resources/pploader.app/Contents/MacOS/pploader
	strip ${PRIV_ROOT}/Applications/PeerGuardian.app/Contents/Resources/pplogger.app/Contents/MacOS/pplogger
	strip ${PRIV_ROOT}/Applications/PeerGuardian.app/Contents/Resources/pgagent.app/Contents/MacOS/pgagent
	strip ${PRIV_ROOT}/Applications/PeerGuardian.app/Contents/MacOS/PeerGuardian
	strip -S -x ${PRIV_ROOT}/Widgets/PeerGuardian.wdgt/ppwidgethelper.widgetplugin/Contents/MacOS/ppwidgethelper 
fi

# Build user guide
echo "Creating documentation..."
if [ ${HAVETEX} -eq 1 ]; then
	for doc in `ls -d ./doc/*.lproj`
	do
		cd "${MYROOT}/${doc}"
		lang=`basename "${doc}"`
		
		#once to create refs
		pdflatex ./PeerGuardianUserGuide.tex > /dev/null 2>&1
		if [ -f ./PeerGuardianUserGuide.pdf ]; then
			#once more to use the refs
			pdflatex ./PeerGuardianUserGuide.tex > /dev/null 2>&1
			if [ -f ./PeerGuardianUserGuide.pdf ]; then
				mv ./PeerGuardianUserGuide.pdf ${PRIV_ROOT}/Applications/PeerGuardian.app/Contents/Resources/${lang}/
			else
				echo "`pwd` Doc compile failed."
			fi
		else
			echo "`pwd` Doc compile failed."
		fi
	done
	
	if [ -f "${PRIV_ROOT}/Applications/PeerGuardian.app/Contents/Resources/English.lproj/PeerGuardianUserGuide.pdf" ]; then
		cd ${PRIV_ROOT}
		cp "./Applications/PeerGuardian.app/Contents/Resources/English.lproj/PeerGuardianUserGuide.pdf" ./PeerGuardian/PeerGuardianUserGuide_en.pdf
		cd ..
	fi
	cd "${MYROOT}"
else
	echo "It seems LaTeX is not installed on your machine. LaTeX is required to make the documentation." 
	echo "See http://www.uoregon.edu/~koch/texshop/texshop.html for instructions on how to install LaTeX for Mac OS X."
fi

# build install packages
PKGMKR=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker
chmod -R ugo-w ${PRIV_ROOT}/*
chmod u+w ${PRIV_ROOT}/InstallRes ${PRIV_ROOT}/Packages ${PRIV_ROOT}/PeerGuardian

# the pre-install script runs before any new installtion and removes old components
# it's part of the Kext installer because that's the first installer to run
cp "${MYROOT}/installer/preinstall" ${PRIV_ROOT}/InstallRes/
chmod ugo+x ${PRIV_ROOT}/InstallRes/preinstall
cd ${PRIV_ROOT}/InstallRes
ln -sf ./preinstall preupgrade
cd "${MYROOT}"

# the various postinstall scripts setup perms
cat << EOF > ${PRIV_ROOT}/InstallRes/postinstall
#!/bin/sh
export PATH="/usr/bin:/bin:/usr/sbin:/sbin"
target=\${2}
chown -R root:wheel "\${target}"/PeerGuardian.kext
EOF
chmod ugo+x ${PRIV_ROOT}/InstallRes/postinstall
cd ${PRIV_ROOT}/InstallRes
ln -sf ./postinstall postupgrade
cd "${MYROOT}"

"${PKGMKR}" -build -p ${PRIV_ROOT}/Packages/PeerGuardianKext.pkg -f ${PRIV_ROOT}/Extensions \
-r ${PRIV_ROOT}/InstallRes -i "${MYROOT}"/installer/Kext-Info.plist \
-d "${MYROOT}"/installer/Kext-Description.plist

# don't need the pre* scripts anymore
rm ${PRIV_ROOT}/InstallRes/pre*

# postinstall
cat << EOF > ${PRIV_ROOT}/InstallRes/postinstall
#!/bin/sh
export PATH="/usr/bin:/bin:/usr/sbin:/sbin"
target=\${2}
chown root:wheel "\${target}"/xxx.qnation.PeerGuardian.kextload.plist
EOF
chmod ugo+x ${PRIV_ROOT}/InstallRes/postinstall

"${PKGMKR}" -build -p ${PRIV_ROOT}/Packages/PeerGuardianLaunchDaemon.pkg -f ${PRIV_ROOT}/LaunchDaemons \
-r ${PRIV_ROOT}/InstallRes -i "${MYROOT}"/installer/LaunchDaemon-Info.plist \
-d "${MYROOT}"/installer/LaunchDaemon-Description.plist

# postinstall
cat << EOF > ${PRIV_ROOT}/InstallRes/postinstall
#!/bin/sh
export PATH="/usr/bin:/bin:/usr/sbin:/sbin"
target=\${2}
chown -R root:wheel "\${target}"/PeerGuardian
EOF
chmod ugo+x ${PRIV_ROOT}/InstallRes/postinstall

# the support postflight reloads the kext
cat << EOF > ${PRIV_ROOT}/InstallRes/postflight
#!/bin/sh
export PATH="/usr/bin:/bin:/usr/sbin:/sbin"
target=\${2}
"\${target}"/PeerGuardian/pgstart -r
#ignore load errors from pgstart
exit 0
EOF
chmod ugo+x ${PRIV_ROOT}/InstallRes/postflight

"${PKGMKR}" -build -p ${PRIV_ROOT}/Packages/PeerGuardianSupport.pkg -f "${PRIV_ROOT}/Application Support" \
-r ${PRIV_ROOT}/InstallRes -i "${MYROOT}"/installer/Support-Info.plist \
-d "${MYROOT}"/installer/Support-Description.plist

rm ${PRIV_ROOT}/InstallRes/post*

"${PKGMKR}" -build -p ${PRIV_ROOT}/Packages/PeerGuardianWidget.pkg -f ${PRIV_ROOT}/Widgets \
-r ${PRIV_ROOT}/InstallRes -i "${MYROOT}"/installer/Widget-Info.plist \
-d "${MYROOT}"/installer/Widget-Description.plist

"${PKGMKR}" -build -p ${PRIV_ROOT}/Packages/PeerGuardianApp.pkg -f ${PRIV_ROOT}/Applications \
-r ${PRIV_ROOT}/InstallRes -i "${MYROOT}"/installer/App-Info.plist \
-d "${MYROOT}"/installer/App-Description.plist

# finally, create our master package

mkdir ${PRIV_ROOT}/InstallRes/en.lproj
cp "${MYROOT}/installer"/*.rtf ${PRIV_ROOT}/InstallRes/en.lproj/

# XXX: -mi ${PRIV_ROOT}/Packages is not working, so just force a dummy archive build
"${PKGMKR}" -build -p ${PRIV_ROOT}/PeerGuardian.mpkg  -f ${PRIV_ROOT}/InstallRes \
-r ${PRIV_ROOT}/InstallRes -i "${MYROOT}/installer/Meta-Info.plist" \
-d "${MYROOT}/installer/Meta-Description.plist"
# get rid of any archive files
rm -rf "${PRIV_ROOT}/PeerGuardian.mpkg/Contents"/*.bom "${PRIV_ROOT}/PeerGuardian.mpkg/Contents"/*.gz \
"${PRIV_ROOT}/PeerGuardian.mpkg/Contents/Resources"/*.bom "${PRIV_ROOT}/PeerGuardian.mpkg/Contents/Resources"/*.gz \
"${PRIV_ROOT}/PeerGuardian.mpkg/Contents/Resources"/*.sizes

cp -R "${PRIV_ROOT}/Packages" "${PRIV_ROOT}/PeerGuardian.mpkg/Contents/"
# Get the install sizes and then copy the distitribution script
APPSIZE=`du -ks ${PRIV_ROOT}/Applications | awk '{print $1}'`
WIDGETSIZE=`du -ks ${PRIV_ROOT}/Widgets | awk '{print $1}'`
SUPPORTSIZE=`du -ks "${PRIV_ROOT}/Application Support" | awk '{print $1}'`
KEXTSIZE=`du -ks ${PRIV_ROOT}/Extensions | awk '{print $1}'`
KEXTLOADSIZE=`du -ks ${PRIV_ROOT}/LaunchDaemons | awk '{print $1}'`

sed -E -e "s/^(.+xxx\.qnation\.PeerGuardian\.app.+installKBytes=\")[0-9]+(\".*)$/\1${APPSIZE}\2/" \
-e "s/^(.+xxx\.qnation\.PeerGuardian\.widget.+installKBytes=\")[0-9]+(\".*)$/\1${WIDGETSIZE}\2/" \
-e "s/^(.+xxx\.qnation\.PeerGuardian\.support.+installKBytes=\")[0-9]+(\".*)$/\1${SUPPORTSIZE}\2/" \
-e "s/^(.+xxx\.qnation\.PeerGuardian\.kext.+installKBytes=\")[0-9]+(\".*)$/\1${KEXTSIZE}\2/" \
-e "s/^(.+xxx\.qnation\.PeerGuardian\.kextload.+installKBytes=\")[0-9]+(\".*)$/\1${KEXTLOADSIZE}\2/" \
"${MYROOT}/installer/distribution.dist" > "${PRIV_ROOT}/PeerGuardian.mpkg/Contents/distribution.dist"

chmod -R u+w ${PRIV_ROOT}

# Now build the distribution folder
cd ${PRIV_ROOT}
mv ./PeerGuardian.mpkg ./PeerGuardian/

cp "${MYROOT}/doc/English.lproj/LICENSE.html" ./PeerGuardian/
cp -pR "${MYROOT}/build/${RELEASE}/PeerGuardian Uninstaller.app" ./PeerGuardian/
if [ $RELEASE != "Debug" ]; then
strip ./PeerGuardian/PeerGuardian\ Uninstaller.app/Contents/MacOS/PeerGuardian\ Uninstaller
fi

chmod -R go-w PeerGuardian

echo "Compressing..."
mv PeerGuardian PeerGuardian_$VERSION
zip -qr -9 ~/Desktop/PeerGuardian_$VERSION.zip PeerGuardian_$VERSION
#tar -czf ~/Desktop/PeerGuardian_$VERSION.tgz PeerGuardian_$VERSION

if [ ${KEEPTMP} -eq 0 ]; then
	rm -rf PeerGuardian_$VERSION
fi

cd ~/Desktop
if [ -x "${MYROOT}"/utils/bhash ]; then
	"${MYROOT}"/utils/bhash -t sha1 -t sha512 PeerGuardian_$VERSION.zip
else 
	openssl sha1 PeerGuardian_$VERSION.zip
fi

if [ ${KEEPTMP} -eq 0 ]; then
	rm -rf ${PRIV_ROOT}
fi

cd "${MYROOT}"
if [ -d .svn ] && [ $RELEASE != "Debug" ]; then
	#create src tarball (zipball in this case) of HEAD
	echo "Exporting source..."
	svn export . /tmp/PeerGuardianSrc
	cd /tmp
	zip -qr -9 ~/Desktop/PeerGuardianSrc_$VERSION.zip PeerGuardianSrc
	rm -rf PeerGuardianSrc
	cd ~/Desktop
	if [ -x "${MYROOT}"/utils/bhash ]; then
		"${MYROOT}"/utils/bhash -t sha1 -t sha512 PeerGuardianSrc_$VERSION.zip
	else 
		openssl sha1 PeerGuardianSrc_$VERSION.zip
	fi
fi

echo "Done."
