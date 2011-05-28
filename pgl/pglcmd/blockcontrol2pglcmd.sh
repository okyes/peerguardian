#!/bin/sh
# blockcontrol2pglcmd.sh - transition blockcontrol to pglcmd configuration
#
# Copyright (C) 2011 jre <jre-phoenix@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# Specify old configuration files
BLOCKCONTROL_CONF="/etc/blockcontrol/blockcontrol.conf"
BLOCKCONTROL_LISTS="/etc/blockcontrol/blocklists.list"

# If no old configuration exists, just exit.
[ ! -f "$BLOCKCONTROL_CONF" ] && [ ! -f "$BLOCKCONTROL_LISTS" ] && exit 0 || true

################################################################################
# The following code is common between pglcmd, pglcmd.wd,
# cron.daily, init, debian/pglcmd.postinst and blockcontrol2pglcmd.sh.

# if-up is similar, but exits successfully if CONTROL_MAIN is not there, yet.
# This can happen in early boot stages before local file systems are mounted.

# CONTROL_MAIN has to be set correctly in all just mentioned files.
# This is done by the pgl/pglcmd/Makefile based on the settings in the
# pgl/Makefile.
CONTROL_MAIN="/usr/lib/pgl/pglcmd.main"

# Configure pglcmd and load functions.
if [ -f "$CONTROL_MAIN" ] ; then
    . $CONTROL_MAIN || { echo "$0 Error: Failed to source $CONTROL_MAIN although this file exists."; exit 1; }
else
    echo "$0 Error 7: Missing file $CONTROL_MAIN."
    exit 7
fi

# End of the common code between pglcmd, pglcmd.wd,
# cron.daily, (if-up), init and debian/postinst.
################################################################################

test_root

# Remember new configuration files
PGLCMD_CONF="$CMD_CONF"
PGLCMD_LISTS="$BLOCKLISTS_LIST"

echo "This scripts transitions some of the old blockcontrol configuration settings"
echo "from blockcontrol.conf and blocklists.list. But not everything is handled, e.g."
echo "local blocklists, allow lists and custom iptables insertion and deletion"
echo "scripts."

if [ -f "$BLOCKCONTROL_CONF" ] ; then
    echo "Transitioning old blockcontrol configuration from"
    echo "${BLOCKCONTROL_CONF} to ${PGLCMD_CONF}:"

    # Get the old values
    . $BLOCKCONTROL_CONF

    # For the old blockcontrol variable names that make sense to be transitioned
    for VAR in ACCEPT ACCEPT_MARK CRON CRON_MAILTO INIT IP_REMOVE IPTABLES_ACTIVATION IPTABLES_SETTINGS IPTABLES_TARGET IPTABLES_TARGET_WHITELISTING LOG_IPTABLES LOG_SYSLOG LOG_TIMESTAMP LSB  NFQUEUE_NUMBER NICE_LEVEL PATH REJECT REJECT_FW REJECT_IN REJECT_MARK REJECT_OUT TESTHOST VERBOSITY WD WD_NICE WD_SLEEP WHITE_IP_FORWARD WHITE_IP_IN WHITE_IP_OUT WHITE_LOCAL WHITE_TCP_FORWARD WHITE_TCP_IN WHITE_TCP_OUT WHITE_UDP_FORWARD WHITE_UDP_IN WHITE_UDP_OUT ; do
        # If variable is set in BLOCKCONTROL_CONF
        if grep -v "^[[:space:]]*#" $BLOCKCONTROL_CONF | grep -q "${VAR}=" ; then
            eval VALUE=\$$VAR
            echo "# Automatically added from $BLOCKCONTROL_CONF:" >> $PGLCMD_CONF
            # Add the old VARIABLE=VALUE to the new CMD_CONF. Some variables where renamed:
            case $VAR in
            REJECT_FW)
                echo "REJECT_FWD=\"$VALUE\"" >> $PGLCMD_CONF
                echo "REJECT_FWD=\"$VALUE\""
                ;;
            WHITE_IP_FORWARD)
                echo "WHITE_IP_FWD=\"$VALUE\"" >> $PGLCMD_CONF
                echo "WHITE_IP_FWD=\"$VALUE\""
                ;;
            WHITE_TCP_FORWARD)
                echo "WHITE_TCP_FWD=\"$VALUE\"" >> $PGLCMD_CONF
                echo "WHITE_TCP_FWD=\"$VALUE\""
                ;;
            WHITE_UDP_FORWARD)
                echo "WHITE_UDP_FWD=\"$VALUE\"" >> $PGLCMD_CONF
                echo "WHITE_UDP_FWD=\"$VALUE\""
                ;;
            VERBOSITY)
                echo "VERBOSE=\"$VALUE\"" >> $PGLCMD_CONF
                echo "VERBOSE=\"$VALUE\""
                ;;
            *)
                echo ${VAR}=\"$VALUE\" >> $PGLCMD_CONF
                echo ${VAR}=\"$VALUE\"
                ;;
            esac
        fi
    done
    mv ${BLOCKCONTROL_CONF} ${BLOCKCONTROL_CONF}.transitioned
    echo "Done."
fi

if [ -f "$BLOCKCONTROL_LISTS" ] ; then
    # Get all configured URLs from blockcontrol blocklists.list
    BLOCKCONTROL_LISTS_URL="$(grep -Ev "^[[:space:]]*#|^[[:space:]]*$" $BLOCKCONTROL_LISTS | sed "s|#.*$||g")"
    # Needed if local lists are configured:
    LOCALLIST=0
    for LIST in $BLOCKCONTROL_LISTS_URL ; do

        # Local blocklists:
        # blockcontrol blocklists.list had local lists specied with these entries:
        # "locallist /path/to/local/blocklist"
        # Remember for the next run that a locallist is specified and continue ...
        if [ "$LIST" = locallist ] ; then
            LOCALLIST=1
            continue
        fi
        # ... in the next run, issue this and reset
        if [ "$LOCALLIST" = 1 ] ; then
            # I don't want to do this automatically, just inform user.
            echo "Please copy or link your local list"
            echo "$LIST to ${LOCAL_BLOCKLIST_DIR}/."
            echo "Then it will be used automatically."
            LOCALLIST=0
            continue
        fi

        # Normal (remote) blocklists:
        # If LIST is not configured in pglcmd blocklists.list
        # (this function does no uncommenting of commented lists,
        # nore does it check if URL aliases are already configured.
        if ! grep -Ev "^[[:space:]]*#" "$PGLCMD_LISTS" | grep -q "$LIST" ; then
            echo "# Automatically added from ${BLOCKCONTROL_LISTS}:" >> $PGLCMD_LISTS
            echo "$LIST" >> $PGLCMD_LISTS
            echo "$LIST added to $PGLCMD_LISTS"
        fi
    done
    mv $BLOCKCONTROL_LISTS ${BLOCKCONTROL_LISTS}.transitioned
fi
