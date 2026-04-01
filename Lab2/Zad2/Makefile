all: main child

main: main.c
	clang -Wall main.c -o main

child: child.c
	clang -Wall child.c -o child

clean:
	rm -f main child