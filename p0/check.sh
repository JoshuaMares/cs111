#!/bin/bash

#temp files
tmpfilein=$(mktemp tempinXXX)
tmpfileout=$(mktemp tempoutXXX)
echo "random stuff for the \n input file" > $tmpfilein

#should fail
#fail cases(exitcode!=0):Input file could not be opened, output file could not be outputted to, invalid option, segfault catch
#reminder $? gives us the last exit code
./lab0 --input thisfileshouldnotexist
if [ $? -eq 2 ]
then
  echo "Invalid --input arg test: PASSED"
else
  echo "Invalid --input arg test: FAILED"
fi

chmod 444 $tmpfileout
./lab0 --input $tmpfilein --output $tmpfileout
if [ $? -eq 3 ]
then
  echo "Invalid --output arg test: PASSED"
else
  echo "Invalid --output arg test: FAILED"
fi
chmod 666 $tmpfileout

./lab0 --badoption
if [ $? -eq 1 ]
then
  echo "Invalid option test: PASSED"
else
  echo "Invalid option test: FAILED"
fi

./lab0 --input $tmpfilein --output $tmpfileout --segfault --catch
if [ $? -eq 4 ]
then
  echo "--segfault --catch test: PASSED"
else
  echo "--segfault --catch test: FAILED"
fi
#should pass
#success cases(exits with 0):input to output,

./lab0 --input $tmpfilein --output $tmpfileout
if [ $? -eq 0 ]
then
  echo "Successfull copy test(p1): PASSED"
else
  echo "Successfull copy test(p1): FAILED"
fi

diff $tmpfilein $tmpfileout
if [ $? -eq 0 ]
then
  echo "Successfull copy test(p2): PASSED"
else
  echo "Successfull copy test(p2): FAILED"
fi

rm -f "$tmpfilein"
rm -f "$tmpfileout"
