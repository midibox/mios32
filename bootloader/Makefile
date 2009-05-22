# $Id: Makefile 194 2008-12-18 01:47:21Z tk $

################################################################################
# The make rule compiles the code and generates the mios32_bsl_*.inc file
################################################################################
all:
	make -f Makefile.bsl_MBHP_CORE_STM32
	mv project.bin project_MBHP_CORE_STM32.bin
	perl gen_inc_file.pl project_MBHP_CORE_STM32.bin ../mios32/STM32F10x/mios32_bsl_MBHP_CORE_STM32.inc mios32_bsl_image mios32_bsl
	perl gen_inc_file.pl project_MBHP_CORE_STM32.bin updater/bsl_image_MBHP_CORE_STM32.inc bsl_image 0
	make -f Makefile.bsl_MBHP_CORE_STM32 cleanall

	make -f Makefile.bsl_STM32_PRIMER
	mv project.bin project_STM32_PRIMER.bin
	perl gen_inc_file.pl project_STM32_PRIMER.bin ../mios32/STM32F10x/mios32_bsl_STM32_PRIMER.inc mios32_bsl_image mios32_bsl
	make -f Makefile.bsl_STM32_PRIMER cleanall

	make -f Makefile.bsl_dummy
	mv project.bin project_dummy.bin
	perl gen_inc_file.pl project_dummy.bin ../mios32/STM32F10x/mios32_bsl_dummy.inc mios32_bsl_image mios32_bsl
	make -f Makefile.bsl_dummy cleanall
