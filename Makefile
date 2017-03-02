CC = gcc
CFLAGS = -I. 
DEPS = linuxshell.h
OBJ = linuxshell.o

%.o: %c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) 
myexec: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) 
.PHONY: clean
clean:
	rm -f *.o myexec