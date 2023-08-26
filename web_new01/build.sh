#!/bin/bash

make clean
make
cd cgi/
g++ -o mspaint_cgi mspaint_cgi.cc -std=c++11 -I include -L lib -lmysqlclient
mv mspaint_cgi ../
cd ..
make o