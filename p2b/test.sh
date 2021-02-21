#!/bin/bash

#list testing
SYNCOPTS="m s"
echo "LIST-NONE-1000-SYNCS START"
date +"%T"
for s in $SYNCOPTS
do
  for i in 1 2 4 8 12 16 24
  do
    ./lab2_list --threads=$i --iterations=1000 --sync=$s >> lab2b_list.csv
  done
done
echo "LIST-NONE-1000-SYNCS END"
date +"%T"

echo "2 AND 3 TESTS START"
date +"%T"
SYNCOPTS="m s"
for t in 1 4 8 12 16
do
  for i in 1 2 4 8 16
  do
    ./lab2_list --threads=$t --iterations=$i --lists=4 --yield=id >> lab2b_list.csv
  done
done

for s in $SYNCOPTS
do
  for t in 1 4 8 12 16
  do
    for i in 10 20 40 80
    do
      ./lab2_list --threads=$t --iterations=$i --lists=4 --yield=id --sync=$s >> lab2b_list.csv
    done
  done
done
echo "2 AND 3 TESTS END"
date +"%T"

echo "4 TEST START"
date +"%T"
for s in $SYNCOPTS
do
  for t in 1 4 8 12
  do
    for l in 1 4 8 16
    do
      ./lab2_list --threads=$t --iterations=1000 --lists=$l --sync=$s >> lab2b_list.csv
    done
  done
done
echo "4 TESTS END"
date +"%T"
