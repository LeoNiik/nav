#!/bin/bash

gcc fdir.c -o fdir
mv ./fdir /usr/bin/
cp ./fdirwrap /usr/bin/fdirwrap
echo 'alias nav=". fdirwrap"' >> ~/.bashrc
source ~/.bashrc
echo Installed Succesfully!
