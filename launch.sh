#!/bin/bash

export LIFESRCDUMB="${HOME}/Documents/GOL/lifesrc/lifesrcdumb -tr1 -ogr -vb5000"

# mapfile -d $'\0' array < <(find . -name "*.sh" -print0)

array=()
while IFS=  read -r -d $'\0'; do
    array+=("$REPLY")
done < <(find . -name "*.sh" -print0)

echo "${#array[@]}"
let cpus=$@-2
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
