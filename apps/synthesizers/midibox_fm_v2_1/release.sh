# $Id: release.sh 1944 2014-01-28 21:24:35Z tk $

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
cp -r cfg $RELEASE_DIR
cp -r touchosc $RELEASE_DIR
find $RELEASE_DIR/cfg -type d | grep .svn$ | xargs rm -rf

################################################################################
#echo "Building for MBHP_CORE_STM32"

make cleanall
export MIOS32_FAMILY=STM32F10x
export MIOS32_PROCESSOR=STM32F103RE
export MIOS32_BOARD=MBHP_CORE_STM32
export MIOS32_LCD=universal
mkdir -p $RELEASE_DIR/$MIOS32_BOARD
make > $RELEASE_DIR/$MIOS32_BOARD/log.txt
cp project.hex $RELEASE_DIR/$MIOS32_BOARD

###############################################################################
echo "Building for MBHP_CORE_LPC17"

make cleanall
export MIOS32_FAMILY=LPC17xx
export MIOS32_PROCESSOR=LPC1769
export MIOS32_BOARD=MBHP_CORE_LPC17
export MIOS32_LCD=universal
mkdir -p $RELEASE_DIR/$MIOS32_BOARD
make > $RELEASE_DIR/$MIOS32_BOARD/log.txt
cp project.hex $RELEASE_DIR/$MIOS32_BOARD

###############################################################################
echo "Building for MBHP_CORE_STM32F4"

make cleanall
export MIOS32_FAMILY=STM32F4xx
export MIOS32_PROCESSOR=STM32F407VG
export MIOS32_BOARD=MBHP_CORE_STM32F4
export MIOS32_LCD=universal
mkdir -p $RELEASE_DIR/$MIOS32_BOARD
make > $RELEASE_DIR/$MIOS32_BOARD/log.txt
cp project.hex $RELEASE_DIR/$MIOS32_BOARD

###############################################################################
make cleanall
echo "Done!"
