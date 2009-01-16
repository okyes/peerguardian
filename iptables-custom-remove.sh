#!/bin/sh
# iptables-custom-remove.sh - custom iptables deletion script used by
# moblock-control

# This script will be executed on every "moblock-control stop" if
# IPTABLES_SETTINGS="2" is set in moblock.conf (/etc/moblock/moblock.conf) or
# moblock.default (/etc/default/moblock).
# This script will be executed on every "moblock-control stop" for 2 settings:
# If IPTABLES_SETTINGS="1" is set in moblock.conf (/etc/moblock/moblock.conf) or
# moblock.default (/etc/default/moblock) then first moblock-control will remove
# its iptables setup and afterwards this script gets executed. Note that you
# don't need to remove custom iptables rules inserted to the chains moblock_in,
# moblock_out and moblock_fw since these chains already get flushed by
# moblock-control.
# If IPTABLES_SETTINGS="2" is set then only this script will be executed.

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
