CC := gcc
CFLAGS := -Wall -O2 -g

PRELOADS := stackreport.so
PROGS := testprog

all: $(PRELOADS) $(PROGS)

stackreport.so: CPPFLAGS += -D_GNU_SOURCE
stackreport.so: LDFLAGS += -fPIC -shared -pthread
stackreport.so: LDLIBS += -ldl

testprog: CFLAGS += -O0
testprog: LDFLAGS += -pthread

clean:
	$(RM) *.o $(PRELOADS) $(PROGS)

check: all
	(ulimit -s 8192 ; ./testprog.py)

%.so: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

