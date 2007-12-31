#!/bin/bash

ssh cnode01 "pkill -f render.py"
ssh cnode01 "pkill -f merger.py"
./noderun cnode01 "cd src/calwp; NAME=calwpdemo IBLOCK=0 python render.py cfront:3333" render &
./noderun cnode01 "cd src/calwp; NAME=calwpdemo IBLOCK=0 python merger.py cfront:3333" merger &

for ((i=2;i<7;i++))
do
ssh cnode0$i "pkill -f dex.py"
./noderun cnode0$i "cd src/calwp; ulimit -c unlimited; NAME=calwpdemo IBLOCK=$((i - 2)) python dex.py cfront:3333" dex &
done


