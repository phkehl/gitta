python /path/to/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 --port /dev/ttyACM0 --baud 921600 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size detect --no-compress 0x0 docs/wififina.bin