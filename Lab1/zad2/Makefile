CC=gcc
RM=rm
CFLAGS=-c -Wall -O2
LDFLAGS=-lm

SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)
EXECS=$(SOURCES:%.c=%)

.PHONY: all
all: $(OBJECTS) $(EXECS)
# kompilacja z c do o ( binarnych )
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

# linkowanie do plikow wykonalnych
%: %.o
	$(CC) $^ $(LDFLAGS) -o $@

.PHONY: clean
clean:
	-@ $(RM) *.o 
	-@ $(RM) $(EXECS)