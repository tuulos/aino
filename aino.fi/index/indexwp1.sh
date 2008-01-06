
export AINO=/mnt/masterfs/ville/calaino/aino/preproc
export NAME=calaino2
export SEGMENT_SIZE=500
export IBLOCK_SIZE=500000
export LANG_MODELS="langrecg.models"
export PUMP="/home/nodeuser/antti/repo/hoowwwer-ng/docstore2docstream"
export NUM_DOX=1000000000
export BASE="fi_docstore"

echo "Running the 1st phase.."

$PUMP $BASE | MAKEDB=0 python suckdoc.py $NUM_DOX | $AINO/charconv |\
	$AINO/dehtml | $AINO/../lang/langrecg |\
        DUB_VERBOSE=1 NEW_IXICON=1 $AINO/tokenizer | nc -q 1 node14 5555
echo "Ok"

echo "Copying stats"
scp node14:/mnt/localfs/ville/calaino/$NAME.istats .

echo "Order the ixicon.."
$AINO/freq_ixicon
echo "Ok"

scp $NAME.ixi node14:/mnt/localfs/ville/calaino/

echo "Running the 2nd phase.."

$PUMP $BASE | MAKEDB=0 python suckdoc.py $NUM_DOX | $AINO/charconv |\
	$AINO/dehtml | python make_db.py | $AINO/../lang/langrecg |\
        DUB_VERBOSE=1 $AINO/tokenizer | nc -q 1  node14 5555
