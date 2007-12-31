
export IBLOCK=$@
export AINO=aino/preproc
export SEGMENT_SIZE=500

echo "$IBLOCK) Encoding inva.."
$AINO/encode_inva
echo "Ok"

echo "$BLOCK) Encoding fwlayers.."
$AINO/encode_fwlayers
echo "Ok"

echo "$IBLOCK) Normality prior.."
$AINO/normality_prior
echo "Ok"

echo "$IBLOCK) Building index.."
$AINO/build_index
echo "Ok"


