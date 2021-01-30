#!/usr/bin/python
# CS 6250 Fall 2020 - SDN Firewall Project
# build anorak-v13

from pyretic.lib.corelib import *
from pyretic.lib.std import *
from pyretic.lib.query import packets
from pyretic.core import packet

def make_firewall_policy(config):

    # You may place any user-defined functions in this space.
    # You are not required to use this space - it is available if needed.

    # feel free to remove the following "print config" line once you no longer need it
    # it will not affect the performance of the autograder
    print config

    # The rules list contains all of the individual rule entries.
    disallowedrules = []

    tcp = match(protocol=packet.TCP_PROTO, ethtype=packet.IPV4)
    udp = match(protocol=packet.UDP_PROTO, ethtype=packet.IPV4)
    icmp = match(protocol=packet.ICMP_PROTO, ethtype=packet.IPV4)

    for entry in config:

        rule_items = []

        # TODO - This is where you build your firewall implementation using the Pyretic Language
        # "config" is a list that contains all of the entries that are parsed from your firewall-config.pol file.
        # Each entry represents one line from the firewall-config.pol file.  The entry is a python dictionary that
        # contains each item defined in a single line from the firewall-config.pol file.  For instance, to access 
        # the source IP address, you will access the dictionary by using entry[ipaddr_src].  The name of each
        # dictionary item is defined in firewall.py

        # You will need to remove the line below and create an implementation that builds a pyretic firewall rules 
        # using the data passed in entry.  Refer to Pyretic Documentation to learn how to write this implementation.
        # (Hint:  What are you doing to the traffic?  Think about matching items in the headers)
        for key, value in entry.iteritems():

            if key != 'rulenum' and value != '-':
                if key == 'macaddr_src':
                    rule_items.append(match(srcmac=MAC(value)))
                elif key == 'macaddr_dst':
                    rule_items.append(match(dstmac=MAC(value)))
                elif key == 'ipaddr_src':
                    rule_items.append(match(srcip=IPAddr(value.split('/')[0])))
                elif key == 'ipaddr_dst':
                    rule_items.append(match(dstip=IPAddr(value.split('/')[0])))
                elif key == 'port_src':
                    rule_items.append(match(srcport=int(value), ethtype=packet.IPV4))
                elif key == 'port_dst':
                    rule_items.append(match(dstport=int(value), ethtype=packet.IPV4))
                elif key == 'protocol':
                    if value == 'T':
                        rule_items.append(tcp)
                    elif value == 'U':
                        rule_items.append(udp)
                    elif value == 'I':
                        rule_items.append(icmp)
                elif key == 'ipproto':
                    rule_items.append(match(protocol=int(value), ethtype=packet.IPV4))
        if len(rule_items) > 0:
            rule = reduce(lambda x, y: x & y, rule_items)
            disallowedrules.append(rule)

    # DO NOT EDIT BELOW THIS LINE.  Think about the following line.  What is it doing?  What happens if you remove the ~

    allowed = ~(union(disallowedrules))

    return allowed
