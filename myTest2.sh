#!/usr/bin/bash

n=1
modn=1
last=100
while [ $n -lt $last ]
do
	./TigerC.exe <<< "tconnect 127.0.0.1 user pass
exit
" &
	n=$(($n+1))
	modn=$(($n%10))
done
