#!/bin/bash

export LIFESRCDUMB="${HOME}/Documents/GOL/lifesrc/lifesrcdumb -tr1 -ogr -vb20000 -d1000000"

find . -name "*.sh" -a \! -name "launch.sh" -print | sort -t / -n -k 2,3 > scriptlist
readarray -t array < scriptlist

echo "${#array[@]}"
cpus="$@"-1
echo "$cpus"

for i in "${array[@]}"
do
  :
  while [[ `jobs -r | wc -l | tr -d " "` -gt "$cpus" ]]; do
    sleep 1
  done
  jobs -r | wc -l | tr -d " "
  bash $i &
done
echo waiting for completion...
wait
