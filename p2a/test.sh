#!/bin/bash

echo "ADD-NONE START"
date +"%T"
for i in 1 2 4 8 12
do
  for j in 10 20 40 80 100 1000 10000 100000
  do
    ./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
  done
done
echo "ADD-NONE END"
date +"%T"

echo "ADD-YIELD START"
date +"%T"
for i in 2 4 8 12
do
  for j in 10 20 40 80 100 1000 10000 100000
  do
    ./lab2_add --threads=$i --iterations=$j --yield >> lab2_add.csv
  done
done
echo "ADD-YIELD END"
date +"%T"

echo "ADD-MSC START"
date +"%T"
SYNCOPTS="m s c"
for c in $SYNCOPTS
do
  for i in 2 4 8 12
  do
    for j in 10 20 40 80 100 1000 10000 100000
    do
      ./lab2_add --threads=$i --iterations=$j --sync=$c >> lab2_add.csv
    done
  done
done
echo "ADD-MSC END"
date +"%T"

echo "ADD-YIELD-MC START"
date +"%T"
SYNCOPTS="m c"
for c in $SYNCOPTS
do
  for i in 2 4 8 12
  do
    for j in 10 20 40 80 100 1000 10000 100000
    do
      ./lab2_add --threads=$i --iterations=$j --sync=$c --yield >> lab2_add.csv
    done
  done
done
echo "ADD-YIELD-MC END"
date +"%T"

echo "ADD-YIELD-S START"
date +"%T"
for i in 2 4 8 12
do
  for j in 10 20 40 80 100 1000
  do
    ./lab2_add --threads=$i --iterations=$j --sync=s --yield >> lab2_add.csv
  done
done
echo "ADD-YIELD-S END"
date +"%T"



#list testing
for j in 10 100 1000 10000 20000
do
  ./lab2_list --threads=1 --iterations=$j >> lab2_list.csv
done

echo "LIST-NONE START"
date +"%T"
for i in 2 4 8 12
do
  for j in 1 10 100 1000
  do
    ./lab2_list --threads=$i --iterations=$j >> lab2_list.csv
  done
done
echo "LIST-NONE END"
date +"%T"

echo "LIST-YIELDS-NONE START"
date +"%T"
YIELDOPTS="i d l id il dl idl"
for y in $YIELDOPTS
do
  for i in 2 4 8 12
  do
    for j in 1 2 4 8 16 32
    do
      ./lab2_list --threads=$i --iterations=$j --yield=$y >> lab2_list.csv
    done
  done
done
echo "LIST-YIELDS-NONE END"
date +"%T"

echo "LIST-YIELDS-SYNCS START"
date +"%T"
YIELDOPTS="i d l id il dl idl"
SYNCOPTS="m s"
for c in $SYNCOPTS
do
  for y in $YIELDOPTS
  do
    for i in 2 4 8 12
    do
      for j in 1 2 4 8 16 32
      do
        ./lab2_list --threads=$i --iterations=$j --sync=$c --yield=$y >> lab2_list.csv
      done
    done
  done
done
echo "LIST-YIELDS-SYNCS END"
date +"%T"

echo "LIST-SYNCS START"
date +"%T"
SYNCOPTS="m s"
for c in $SYNCOPTS
do
  for i in 1 2 4 8 12 16 24
  do
    for j in 1 10 100 1000
    do
      ./lab2_list --threads=$i --iterations=$j --sync=$c >> lab2_list.csv
    done
  done
done
echo "LIST-SYNCS END"
date +"%T"
