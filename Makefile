CFLAGS := -Werror -Wfatal-errors -g

all: cshell.o

cshell.o: 
	gcc $(CFLAGS) -o cshell cshell.c

clean:
	rm -f cshell *.o