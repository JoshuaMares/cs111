#!/bin/bash

#list testing
SYNCOPTS="m s"
echo "LIST-i-1000-SYNCS START"
date +"%T"
for s in $SYNCOPTS
do
  for i in 1 2 4 8 12 16 24
  do
    ./lab2_list --threads=$i --iterations=1000 --sync=$s >> lab2b_list.csv
  done
done
echo "LIST-i-1000-SYNCS END"
date +"%T"
