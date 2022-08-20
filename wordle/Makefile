TARGET = wordle

CC = gcc
CFLAGS  = -g -Wall -std=c99 -Os 
RM = del

default: $(TARGET)

$(TARGET): map.o game.o
	$(CC) $(CFLAGS) map.o game.o -o $@

run:
	./$(TARGET)

clean:
	@echo cleaning up executables...
	$(RM) *.o *.exe *~