#!/bin/sh
# iptables-custom-remove.sh - custom iptables deletion script used by
# blockcontrol

# This script will be executed on every "blockcontrol stop" for 2 settings:

# Default setup (IPTABLES_SETTINGS="1"):
# blockcontrol will first remove its iptables setup and afterwards this script
# gets executed. Note that you don't need to remove custom iptables rules
# inserted to the chains blockcontrol_in, blockcontrol_out and blockcontrol_fw
# since these chains already get flushed by blockcontrol.

# IPTABLES_SETTINGS="2" is set in blockcontrol.conf
# (/etc/blockcontrol/blockcontrol.conf):
# Only this script will be executed.

# MoBlock (nfq) checks traffic that is sent to the iptables target NFQUEUE.
# (default queue number is 92).

# Remove the rules for complete blocking of IPv6:
#ip6tables -D OUTPUT -j REJECT
#ip6tables -D INPUT -j DROP
#ip6tables -D FORWARD -j DROP

# Example for IPTABLES_SETTINGS="2".
# Check all traffic with MoBlock. This is the most basic way to use MoBlock.
# Remove these rules:
#iptables -D INPUT -j NFQUEUE --queue-num 92
#iptables -D OUTPUT -j NFQUEUE --queue-num 92
#iptables -D FORWARD -j NFQUEUE --queue-num 92
