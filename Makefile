CC      = x86_64-w64-mingw32-gcc
WINDRES = x86_64-w64-mingw32-windres
CFLAGS  ?= -O2 -Wall -Wextra -municode -mwindows
LDFLAGS ?= -static -s -Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va
LIBS    ?= -luser32 -lshell32 -ladvapi32 -lgdi32 -lkernel32

all: build/tikun.exe

build/tikun_res.o: src/tikun.rc src/tikun.manifest assets/tikun.ico
	mkdir -p build
	$(WINDRES) src/tikun.rc -O coff -o $@

build/tikun.exe: src/tikun.c src/convert.h build/tikun_res.o
	$(CC) $(CFLAGS) -o $@ src/tikun.c build/tikun_res.o $(LDFLAGS) $(LIBS)

test: tests/test_convert.c src/convert.h
	mkdir -p build
	gcc -Wall -Wextra -o build/test_convert tests/test_convert.c
	./build/test_convert

clean:
	rm -rf build

.PHONY: all test clean
