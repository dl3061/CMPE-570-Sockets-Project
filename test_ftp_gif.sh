#!/usr/bin/bash

n=1
last=100
while [ $n -lt $last ]
do
	./TigerC.exe <<< "tconnect 127.0.0.1 user pass
tput sample.gif
tget sample.gif as oshawott_$n.gif
exit
" &
	n=$(($n+1))
done