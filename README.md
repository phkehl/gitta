# GITTA Tschenggins Lämpli

- Copyright (c) 2018 Philippe Kehl <flipflip at oinkzwurgl dot org>
- https://oinkzwurgl.org/projaeggd/tschenggins-laempli

This is a ESP32 version of https://github.com/phkehl/tschenggins-laempli.

## Introduction

This is like the original Tschenggins Lämpli, just for the ESP32 (e.g. a u-blox NINA-W102).

## TODO

- a lot

## Bugs

- plenty

## Notes

- To preset wifi and backend settings edit sdconfig.defaults and add `CONFIG_FF_ENV_SSID_DEFAULT="..."`,
 `CONFIG_FF_ENV_PASS_DEFAULT="..."`, and `CONFIG_FF_ENV_BACKEND_DEFAULT="..."`

- To use another defaults file, use `make SDKCONFIG_DEFAULTS=mydefaultsfile`.

- ESP32 esp-idf: https://docs.espressif.com/projects/esp-idf/en/latest/
- http://esp32.net/#Development
- make -j8 && make flash && make monitor
- https://github.com/npat-efault/picocom can control DTR and RTS
- use WiFiNINAFormwareUpdater on the Arduino MRK 1010 to use esp-idf directly on the ESP32 in the NINA
- https://github.com/rac2030/breakout-boards/tree/master/ublox_NINA-W102, http://rac.su/project/nina-w102-minimal-breakout/
- https://github.com/arduino/nina-fw, https://github.com/arduino-libraries/WiFiNINA

- binary size:
  - all debug, -g:   853
  - only error, -g:  832
  - all debug, -Os:  782
  - only error, -Os: 762
