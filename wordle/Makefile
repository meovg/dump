TARGET = wordle

CC = gcc
CFLAGS  = -g -Wall -std=c99 -Os 
RM = del

default: $(TARGET)

$(TARGET): game.o
	$(CC) $(CFLAGS) game.o -o $@

run:
	./wordle

clean:
	@echo cleaning up executables...
	$(RM) *.o *.exe *~