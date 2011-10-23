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

###############################################################################
echo "Building for MBHP_CORE_STM32"

make cleanall
export MIOS32_FAMILY=STM32F10x
export MIOS32_PROCESSOR=STM32F103RE
export MIOS32_BOARD=MBHP_CORE_STM32
export MIOS32_LCD=universal
mkdir -p $RELEASE_DIR/MBHP_CORE_STM32
make > $RELEASE_DIR/MBHP_CORE_STM32/log.txt
cp project.hex $RELEASE_DIR/MBHP_CORE_STM32

###############################################################################
make cleanall
echo "Done!"

