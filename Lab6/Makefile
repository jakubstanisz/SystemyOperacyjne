CC = gcc
#zamien na clang na mac
CFLAGS = -Wall -Wextra -O2
LIBS = -pthread -lm

all: level1

level1: common.o program1.o
	$(CC) $(CFLAGS) -DLEVEL1 common.o program1.o -o program1 $(LIBS)

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

program1.o: program1.c common.h
	$(CC) $(CFLAGS) -DLEVEL1 -c program1.c

clean:
	rm -f *.o program1 left_*.txt right_*.txt robot_state.txt