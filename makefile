CC = gcc
objects = main.o builtins.o input.o signal_proc.o

main: $(objects)
	$(CC) -o main $(objects)

main.o: builtins.h input.h signal_proc.h
builtins.o: builtins.h
input.o: input.h
signal_proc.o: signal_proc.h

.PHONY: clean
clean:
	rm -f *.o main
