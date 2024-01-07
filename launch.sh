#!/bin/bash

export LIFESRCDUMB="${HOME}/Documents/GOL/lifesrc/lifesrcdumb -tr1 -ogc -vb20000 -d200000"

find . -name "*.sh" -a \! -name "launch.sh" -print | sort -t / -n -k 2,3 > scriptlist
readarray -t array < scriptlist

echo "${#array[@]}"
cpus="$0"
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
