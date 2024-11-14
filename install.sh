#!/bin/bash

gcc fdir.c -o fdir
mv ./fdir /usr/bin/
echo 'cd $(fdir $@)' > /usr/bin/fdirwrap
echo 'alias nav=". fdirwrap"' >> ~/.bashrc
source ~/.bashrc
