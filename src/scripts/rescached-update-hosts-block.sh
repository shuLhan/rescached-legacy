#!/bin/sh

HOSTS_BLOCK=/etc/rescached/hosts.block

if [[ $${UID} != 0 ]]; then
	echo "[rescached] User root needed for overwriting ${HOSTS_BLOCK}!"
	exit 1
fi

wget -O - "http://pgl.yoyo.org/adservers/serverlist.php?hostformat=hosts&showintro=0&startdate[day]=&startdate[month]=&startdate[year]=&mimetype=plaintext" > $HOSTS_BLOCK
wget -O - "http://www.malwaredomainlist.com/hostslist/hosts.txt" >> $HOSTS_BLOCK
