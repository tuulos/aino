#!/bin/bash

MASTER="terae.hiit.fi:3333"
AINO="/mnt/masterfs/aino.fi/aino/wp/node/"
PP="$AINO:$AINO/../../ainopy/:/mnt/masterfs/aino.fi/erlay"

pkill -f noderun

ssh "tuulos@terae.hiit.fi" "pkill -f merger.py"
./noderun "tuulos@terae.hiit.fi" "cd /data/aino.fi; ulimit -c unlimited; ERLAY_DEBUG=1 NAME=calaino2 IBLOCK=0 PYTHONPATH=ainopy:erlay python merger.py $MASTER" merger &

ssh "tuulos@terae.hiit.fi" "pkill -f render.py"
./noderun "tuulos@terae.hiit.fi" "cd /data/aino.fi; ulimit -c unlimited; ERLAY_DEBUG=1 NAME=calaino2 IBLOCK=0 PYTHONPATH=ainopy:erlay python render.py $MASTER" render &

for ((i=0;i<15;i++))
do
        node="node`printf "%.2d" $((i+1))`"
	iblock1=$i
	iblock2=$((i+15))

	ssh $node "pkill -f dex.py"
	
	./noderun $node "cd /mnt/localfs/aino.fi; ulimit -c unlimited; ERLAY_DEBUG=1 NAME=calaino2 IBLOCK=$iblock1 PYTHONPATH=$PP python $AINO/dex.py $MASTER" "iblock$iblock1" &
	
	./noderun $node "cd /mnt/localfs/aino.fi; ulimit -c unlimited; ERLAY_DEBUG=1 NAME=calaino2 IBLOCK=$iblock2 PYTHONPATH=$PP python $AINO/dex.py $MASTER" "iblock$iblock2" &

done

