all:
	perl $(MIOS32_PATH)/modules/esp8266/tools/bin2c.pl < 0x00000.bin > 0x00000.inc
	sed -i.bak -e 's/my_array/esp8266_fw_img_0x00000/g' 0x00000.inc
	rm 0x00000.inc.bak
	perl $(MIOS32_PATH)/modules/esp8266/tools/bin2c.pl < 0x20000.bin > 0x20000.inc
	sed -i.bak -e 's/my_array/esp8266_fw_img_upper/g' 0x20000.inc
	rm 0x20000.inc.bak
