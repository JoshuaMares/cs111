#!/bin/bash

./lab4b --period=5 --scale=F --log=templogfile <<-EOF
SCALE=C
PERIOD=3
START
STOP
LOG THIS LINE
SCALE=F
OFF
EOF

if [ $? -eq 0 ]
then
  echo "No Errors"
else
  echo "Errors"
fi

for string in SCALE=C PERIOD=3 START STOP SCALE=F OFF SHUTDOWN
  do
    grep "$string" templogfile
    if [ $? -eq 0 ]
    then
      echo "$string logged"
    else
      echo "$string not logged"
    fi
  done

grep "LOG THIS LINE" templogfile
if [ $? -eq 0 ]
then
  echo "Log command logged"
else
  echo "Log command not logged"
fi

rm -f templogfile
