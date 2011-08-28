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

mkdir -p $RELEASE_DIR/hwcfg/standard_v4l
cp hwcfg/standard_v4l/MBSEQ_HW.V4L $RELEASE_DIR/hwcfg/standard_v4l

################################################################################
#echo "Building for MBHP_CORE_STM32"
#
#make cleanall
#export MIOS32_FAMILY=STM32F10x
#export MIOS32_PROCESSOR=STM32F103RE
#export MIOS32_BOARD=MBHP_CORE_STM32
#export MIOS32_LCD=universal
#mkdir -p $RELEASE_DIR/MBHP_CORE_STM32
#make > $RELEASE_DIR/MBHP_CORE_STM32/log.txt
#cp project.hex $RELEASE_DIR/MBHP_CORE_STM32

###############################################################################
echo "Building for MBHP_CORE_LPC17"

make cleanall
export MIOS32_FAMILY=LPC17xx
export MIOS32_PROCESSOR=LPC1769
export MIOS32_BOARD=MBHP_CORE_LPC17
export MIOS32_LCD=universal
mkdir -p $RELEASE_DIR/MBHP_CORE_LPC17
make > $RELEASE_DIR/MBHP_CORE_LPC17/log.txt
cp project.hex $RELEASE_DIR/MBHP_CORE_LPC17

###############################################################################
make cleanall
echo "Done!"

