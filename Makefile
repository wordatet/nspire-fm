export PATH := /home/mario/git/Ndless/ndless-sdk/bin:/home/mario/git/Ndless/ndless-sdk/toolchain/install/bin:$(PATH)

DEBUG = 0

GCC = nspire-gcc
LD = nspire-ld
GENZEHN = genzehn

GCCFLAGS = -Wall -W -Werror -Wno-format-truncation -marm -Os -I../Ndless/ndless-sdk/thirdparty/nspire-io/include
LDFLAGS = -L../Ndless/ndless-sdk/thirdparty/nspire-io/lib -lnspireio

OBJS = $(patsubst %.c, %.o, $(wildcard src/*.c))

all: nspire-fm.tns

nspire-fm.tns: nspire-fm.elf
	$(GENZEHN) --input $^ --output $@ --name "nspire-fm"

nspire-fm.elf: $(OBJS)
	$(LD) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

clean:
	rm -f *.o *.elf *.tns src/*.o
