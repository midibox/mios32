# $Id$

################################################################################
# The make rule compiles the code for DUT and Tester
################################################################################
all:
	# build DUT project and rename bin/hex to bin_tmp/hex_tmp (so that "cleanall" won't remove them)
	make -f Makefile.DUT
	mv project.bin project_DUT.bin_tmp
	mv project.hex project_DUT.hex_tmp
	make -f Makefile.DUT cleanall

	# build Tester and rename bin/hex to bin_tmp/hex_tmp (so that "cleanall" won't remove them)
	make -f Makefile.Tester
	mv project.bin project_Tester.bin_tmp
	mv project.hex project_Tester.hex_tmp
	make -f Makefile.Tester cleanall

	# rename tmp -> final files
	mv project_DUT.bin_tmp project_DUT.bin
	mv project_DUT.hex_tmp project_DUT.hex
	mv project_Tester.bin_tmp project_Tester.bin
	mv project_Tester.hex_tmp project_Tester.hex

clean:
	make -f Makefile.DUT clean
	make -f Makefile.Tester clean

cleanall:
	make -f Makefile.DUT cleanall
	make -f Makefile.Tester cleanall
