
NOTE: This command needs to be run from the ESP-IDF command line environment.  Replace the PORT with the port you are using for your device.

esptool.py --chip=esp32 -p REPLACE_WITH_PORT -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size detect 0x8000 partition-table.bin 0x1000 bootloader.bin 0x10000 bt_source.bin