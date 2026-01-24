# Ndless SDK path - MUST be set via NDLESS_SDK environment variable
ifndef NDLESS_SDK
$(error NDLESS_SDK is not set. Please export NDLESS_SDK=/path/to/ndless-sdk)
endif

export PATH := $(NDLESS_SDK)/bin:$(NDLESS_SDK)/toolchain/install/bin:$(PATH)

DEBUG = 0

GCC = nspire-gcc
LD = nspire-ld
GENZEHN = genzehn

GCCFLAGS = -Wall -W -Werror -Wno-format-truncation -marm -Os -I$(NDLESS_SDK)/thirdparty/nspire-io/include
LDFLAGS = -L$(NDLESS_SDK)/thirdparty/nspire-io/lib -lnspireio

OBJS = src/main.o src/ui.o src/input.o src/fs.o src/viewer.o src/editor.o src/image_viewer.o

all: nspire-fm.tns

nspire-fm.tns: nspire-fm.elf
	$(GENZEHN) --input $^ --output $@ --name "nspire-fm"

nspire-fm.elf: $(OBJS)
	$(LD) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

clean:
	rm -f *.o *.elf *.tns src/*.o
