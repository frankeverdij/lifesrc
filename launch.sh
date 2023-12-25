#!/bin/bash

export LIFESRCDUMB="${HOME}/Documents/GOL/lifesrc/lifesrcdumb -tr1 -ogr -vb5000"

# mapfile -d $'\0' array < <(find . -name "*.sh" -print0)

array=()
while IFS=  read -r -d $'\0'; do
    array+=("$REPLY")
done < <(find . -name "*.sh" -print0)

for i in "${array[@]}"
do
  :
  while [[ `jobs -r | wc -l | tr -d " "` > "$@" ]]; do
    sleep 1
  done
  sh $i &
done
