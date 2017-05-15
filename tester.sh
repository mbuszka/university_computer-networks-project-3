#!/bin/bash

PORT=40003

cd maciej_buszka
make

/usr/bin/time -f "\t%E real,\t%U user,\t%S sys,\t%M mem" ./transport $PORT out $1
mv out ../
cd ../
/usr/bin/time -f "\t%E real,\t%U user,\t%S sys,\t%M mem" ./transport-faster $PORT out_ref $1 > /dev/null

diff --brief out out_ref
rm out out_ref
