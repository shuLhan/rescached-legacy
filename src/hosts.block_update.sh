#!/bin/sh

HOSTS_BLOCK=hosts.block

wget -O - "http://pgl.yoyo.org/adservers/serverlist.php?hostformat=hosts&showintro=0&startdate[day]=&startdate[month]=&startdate[year]=&mimetype=plaintext" > $HOSTS_BLOCK
wget -O - "http://www.malwaredomainlist.com/hostslist/hosts.txt" >> $HOSTS_BLOCK
