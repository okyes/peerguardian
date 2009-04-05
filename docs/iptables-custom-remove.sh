#!/bin/sh
# iptables-custom-remove.sh - example custom iptables deletion script

# Every file in the IPTABLES_CUSTOM_DIR directory (/etc/blockcontrol), that ends
# in ...remove.sh will be executed on every "blockcontrol stop" for 2 settings:

# Default setup (IPTABLES_SETTINGS="1"):
# blockcontrol will first remove its iptables setup, afterwards this script gets
# executed. Note that you don't need to remove custom iptables rules from the
# chains blockcontrol_in, blockcontrol_out and blockcontrol_fw, since these
# chains get flushed by blockcontrol.

# IPTABLES_SETTINGS="2" is set in blockcontrol.conf
# (/etc/blockcontrol/blockcontrol.conf):
# Only this script will be executed.

# MoBlock (nfq) checks traffic that is sent to the iptables target NFQUEUE.
# (default queue number is 92).

# Remove the rules for complete blocking of IPv6:
ip6tables -D OUTPUT -j REJECT
ip6tables -D INPUT -j DROP
ip6tables -D FORWARD -j DROP
