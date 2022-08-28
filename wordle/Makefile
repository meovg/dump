TARGET = wordle

CC = gcc
CFLAGS  = -g -Wall -std=c99 -Os 
RM = del

default: $(TARGET)

$(TARGET): dict.o game.o
	$(CC) $(CFLAGS) dict.o game.o -o $@

run:
	./$(TARGET)

clean:
	@echo cleaning up executables...
	$(RM) *.o *.exe *~