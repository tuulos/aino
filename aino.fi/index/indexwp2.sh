
export AINO=/mnt/masterfs/ville/calaino/aino/preproc
export NAME=calaino2
export SEGMENT_SIZE=500
export IBLOCK_SIZE=500000
export LANG_MODELS="langrecg.models"
export PUMP="/home/nodeuser/antti/repo/hoowwwer-ng/docstore2docstream"
export NUM_DOX=10000000000
export BASE="fi_docstore"

echo "Running the 1st phase.."

echo $INPUT
nc -q 1 -l -p 5555 | $AINO/collect_stats
echo "Ok"

#scp node15:/mnt/localfs/calaino/*.ixi .

#echo "Order the ixicon.."
#$AINO/freq_ixicon
#echo "Ok"

echo "Running the 2nd phase.."

nc -q 1 -l -p 5555 | $AINO/encode_docs | $AINO/encode_info >/dev/null


#echo "Encoding inva.."
#IBLOCK=0 $AINO/encode_inva
#echo "Ok"

#echo "Encoding fwlayers.."
#IBLOCK=0 $AINO/encode_fwlayers
#echo "Ok"

#echo "Normality prior.."
#IBLOCK=0 $AINO/normality_prior
#echo "Ok"

#echo "Building index.."
#IBLOCK=0 $AINO/build_index
#echo "Ok"

