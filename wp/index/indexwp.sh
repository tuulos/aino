
export NOF_DOCS=5000000000

export AINO=/home/tuulos/src/calwpnu/aino/preproc
export NAME=calwpdemo
export SEGMENT_SIZE=500
export IBLOCK_SIZE=500000
export LANG_MODELS="langrecg.models"
export PYTHONPATH=mwlib-0.2.5

echo "Running the 1st phase.."
bunzip2 < enwiki-*-pages-articles.xml.bz2 | python makedb.py |\
        $AINO/charconv | $AINO/../lang/langrecg |\
        DUB_VERBOSE=1 NEW_IXICON=1 $AINO/tokenizer | $AINO/collect_stats
echo "Ok"

echo "Order the ixicon.."
$AINO/freq_ixicon
echo "Ok"

echo "Running the 2nd phase.."

bunzip2 < enwiki-*-pages-articles.xml.bz2 | WRITE_DB=1 python makedb.py |\
        $AINO/charconv | $AINO/../lang/langrecg |\
        DUB_VERBOSE=1 $AINO/tokenizer |\
        $AINO/encode_docs | $AINO/encode_info >/dev/null

for ((i=0;i<10;i++))
do
	./iblock.sh $i
done

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

