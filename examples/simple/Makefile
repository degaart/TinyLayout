CC := x86_64-w64-mingw32-gcc
CFLAGS := -std=c99 -O2 -Wall -Wextra -Werror -pedantic -I../..
LDLIBS := -luser32 -lgdi32 -lm

.PHONY: all run clean

all: simple.exe

simple.exe: main.c ../../tinylayout.c ../../tinylayout.h
	$(CC) $(CFLAGS) -mwindows main.c ../../tinylayout.c -o $@ $(LDLIBS)

run: simple.exe
	WINEPREFIX=/var/tmp/win32 wine ./simple.exe

clean:
	rm -f simple.exe
