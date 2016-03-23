all:
	perl ../tools/bin2c.pl < eagle.flash.bin > 0x00000.inc
	sed -i.bak -e 's/my_array/esp8266_fw_img_0x00000/g' 0x00000.inc
	rm 0x00000.inc.bak
	perl ../tools/bin2c.pl < eagle.irom0text.bin > 0x40000.inc
	sed -i.bak -e 's/my_array/esp8266_fw_img_upper/g' 0x40000.inc
	rm 0x40000.inc.bak
