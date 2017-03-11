CC := gcc
CFLAGS := -Wall -O2 -g

PRELOADS := stackreport.so

all: $(PRELOADS)

stackreport.so: CPPFLAGS += -D_GNU_SOURCE
stackreport.so: LDFLAGS += -fPIC -shared -pthread
stackreport.so: LDLIBS += -ldl

clean:
	$(RM) *.o $(PRELOADS)

%.so: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

