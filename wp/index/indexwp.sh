
export NOF_DOCS=5000000000

export AINO=$(pwd)/../../preproc/
export NAME=wpindex
export SEGMENT_SIZE=500
export IBLOCK_SIZE=$NOF_DOCS
export LANG_MODELS="langrecg.models"

echo "Running the 1st phase.."
bunzip2 < *-pages-articles.xml.bz2 | python makedb.py |\
        $AINO/charconv | DUB_VERBOSE=1 NEW_IXICON=1 $AINO/tokenizer | $AINO/collect_stats
echo "Ok"

echo "Order the ixicon.."
$AINO/freq_ixicon
echo "Ok"

echo "Running the 2nd phase.."

bunzip2 < *-pages-articles.xml.bz2 | WRITE_DB=1 python makedb.py |\
        $AINO/charconv | DUB_VERBOSE=1 $AINO/tokenizer |\
        $AINO/encode_docs | $AINO/encode_info >/dev/null

NUM_IBLOCK=$(ls -1 $NAME.fw.*.toc.sect | wc -l)

for ((i=0;i<$NUM_IBLOCK;i++))
do
	./iblock.sh $i
done
