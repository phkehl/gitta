####################################################################################################
# GITTA Tschenggins Lämpli

####################################################################################################

$(info PATH=$(PATH))
ifeq ($(findstring /xtensa-esp32-elf,$(PATH)),)
$(error Please add /path/to/xtensa-esp32-elf to your $$PATH)
endif
ifeq ($(IDF_PATH),)
$(error Please add IDF_PATH=/path/to/esp-idf to your environment)
endif

ifeq (,$(wildcard ./esp-idf))
$(warning You should create a symlink to /path/to/esp-idf here.)
endif
ifeq (,$(wildcard ./xtensa-esp32-elf))
$(warning You should create a symlink to /path/to/xtensa-esp32-elf here.)
endif

####################################################################################################

ifeq ($(RELEASE),1)
CFLAGS += -DNDEBUG -DCONFIG_FREERTOS_ASSERT_DISABLE -Os -DLOG_LOCAL_LEVEL=0
CPPFLAGS += -DNDEBUG -Os
endif


PROJECT_NAME := gitta
include $(IDF_PATH)/make/project.mk

####################################################################################################

.PHONY: distclean
distclean:
	@rm -rf build sdkconfig sdkconfig.old
