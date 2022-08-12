INCLUDEDIR = include
SRCDIR = src
TESTDIR = test
OBJDIR = obj

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)%.c=$(OBJDIR)%.o)
INCLUDES = $(SOURCES:$(SRCDIR)%.c=$(INCLUDEDIR)%.h)

TARGET = map dstring matrix treeset

CC = gcc
CFLAGS  = -g -Wall -std=c99 -I$(INCLUDEDIR) 
RM = del

default: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(TESTDIR)/$@.c -o $@

$(OBJDIR)/%.o: $(SRCDIR)/*.c
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) -c $(SRCDIR)/$*.c $(CFLAGS) -o $@

clean:
	@echo cleaning up executables...
	$(RM) $(OBJDIR)\*.o *.exe *~
	rmdir $(OBJDIR)