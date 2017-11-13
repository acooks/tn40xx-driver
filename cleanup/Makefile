PROG = ccmtcnvt
CPPFLAGS := -D_POSIX_C_SOURCE=2 $(CPPFLAGS) 
CFLAGS := -Wall -std=c99 -pedantic -O2 $(CFLAGS)


.PHONY: all
all: $(PROG) Makefile

%.o: %.c %.h Makefile
	$(COMPILE.c) $< -o $@

$(PROG): $(PROG).o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm $(PROG) *.o || true
