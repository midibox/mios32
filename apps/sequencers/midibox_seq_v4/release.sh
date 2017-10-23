# $Id$

if [[ "$1" == "" ]]; then
  echo "SYNTAX: release.sh <release-directory>"
  exit 1
fi

RELEASE_DIR=$1

if [[ -e $RELEASE_DIR ]]; then
  echo "ERROR: the release directory '$RELEASE_DIR' already exists!"
  exit 1
fi

###############################################################################
echo "Creating $RELEASE_DIR"

mkdir $RELEASE_DIR
cp README.txt $RELEASE_DIR
cp CHANGELOG.txt $RELEASE_DIR

mkdir -p $RELEASE_DIR/hwcfg/standard_v4
cp hwcfg/standard_v4/MBSEQ_HW.V4 $RELEASE_DIR/hwcfg/standard_v4
mkdir -p $RELEASE_DIR/hwcfg/tk
cp hwcfg/tk/MBSEQ_HW.V4 $RELEASE_DIR/hwcfg/tk
mkdir -p $RELEASE_DIR/hwcfg/wilba
cp hwcfg/wilba/MBSEQ_HW.V4 $RELEASE_DIR/hwcfg/wilba
mkdir -p $RELEASE_DIR/hwcfg/wilba_tpd
cp hwcfg/wilba_tpd/MBSEQ_HW.V4 $RELEASE_DIR/hwcfg/wilba_tpd
mkdir -p $RELEASE_DIR/hwcfg/antilog
cp hwcfg/wilba/MBSEQ_HW.V4 $RELEASE_DIR/hwcfg/antilog
cp hwcfg/README.txt $RELEASE_DIR/hwcfg

###############################################################################
configs=( stm32f1 lpc17 stm32f4 )
for i in "${configs[@]}"; do
  echo "Building for $i"
  source ../../../../source_me_${i}
  make cleanall
  mkdir -p $RELEASE_DIR/$MIOS32_BOARD
  make > $RELEASE_DIR/$MIOS32_BOARD/log.txt || exit 1
  cp project.hex $RELEASE_DIR/$MIOS32_BOARD
done

###############################################################################
make cleanall
echo "Done!"

