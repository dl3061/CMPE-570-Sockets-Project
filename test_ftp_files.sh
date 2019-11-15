#!/usr/bin/bash

n=0
modn=1
last=100
while [ $n -lt $last ]
do
	./TigerC.exe <<< "tconnect 127.0.0.1 user pass
tput sample$(modn).txt
tget sample$(modn).txt
exit
" &
	n=$(($n+1)) &
	modn=$((n % 10)) 
done
