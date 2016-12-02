#!/bin/bash
while true
do
#	/usr/local/sbin/rtpp_rtpc_test -l 101.201.76.54 -a 120.26.142.114 -p 100  -n 50 -r 2 -j 100 >> /data/rtpp/monitor/`date +'%Y-%m-%d'`.log
#	sleep 30
	/usr/local/sbin/rtpp_rtpc_test -l 101.201.76.54 -a 120.26.142.114 -p 100  -n 300 -m 3 -r 2 -j 200 >> /data/rtpp/monitor/`date +'%Y-%m-%d'`.log
    sleep 600
done
