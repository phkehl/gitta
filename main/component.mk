# https://dl.espressif.com/doc/esp-idf/latest/api-guides/build-system.html

CFLAGS += -Wall -Wextra
#CFLAGS += -Werror -Wpedantic
CFLAGS += -Wenum-compare -Wsign-compare -Wreturn-type -Wfloat-equal -Wshadow
CFLAGS += -Wunused-variable -Wunused -Wunused-parameter -Wunused-result -Wswitch
CFLAGS += -Wpointer-arith -Wchar-subscripts -Wcomment -Wimplicit-int -Werror-implicit-function-declaration
CFLAGS += -Wunreachable-code -Wbad-function-cast -Wsequence-point -Wparentheses -Wwrite-strings
CFLAGS += -Wnested-externs -Wmissing-prototypes -Waggregate-return -Wuninitialized -Wcast-align
CFLAGS += -Wtrigraphs  -Wunknown-pragmas -Wmain -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations
# -Winline


COMPONENT_EXTRA_CLEAN := gitta_version_gen.h

main.o: version_gen.h
wifi.o: version_gen.h

FF_VERSION := $(shell $(PROJECT_PATH)/tools/version.pl $(PROJECT_PATH))
FF_VERSION_OLD := $(shell sed -n '/FF_VERSION/s/.*"\(.*\)".*/\1/p' version_gen.h 2>/dev/null)
#$(info FF_VERSION=$(FF_VERSION) FF_VERSION_OLD=$(FF_VERSION_OLD))

ifneq ($(FF_VERSION),$(FF_VERSION_OLD))
$(shell touch .version_gen_h)
endif

version_gen.h: .version_gen_h
	@echo "Generating $@"
	@rm -f $@
	@echo -n > $@.tmp
	@echo "#ifndef __VERSION_GEN_H__" >> $@.tmp
	@echo "#define __VERSION_GEN_H__" >> $@.tmp
	@echo "#define FF_VERSION \"$(FF_VERSION)\"" >> $@.tmp
	@echo "#endif" >> $@.tmp
	@mv $@.tmp $@

# eof
