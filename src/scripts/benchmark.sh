#!/bin/sh

while read p; do
	host=`echo $p | cut -d' ' -f2-`
	resolver $host
done <$1
